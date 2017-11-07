// ------------------------------------------------
// File : channel.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Channel streaming classes. These do the actual
//      streaming of media between clients.
//
// (c) 2002 peercast.org
//
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include <algorithm>
#include <numeric> // accumulate

#include "common.h"
#include "socket.h"
#include "channel.h"
#include "gnutella.h"
#include "servent.h"
#include "servmgr.h"
#include "sys.h"
#include "xml.h"
#include "http.h"
#include "peercast.h"
#include "atom.h"
#include "pcp.h"
#include "chandir.h"

#include "mp3.h"
#include "ogg.h"
#include "mms.h"
#include "nsv.h"
#include "flv.h"
#include "mkv.h"
#include "wmhttp.h"

#include "icy.h"
#include "url.h"
#include "httppush.h"

#include "str.h"

#include "version2.h"

// -----------------------------------
const char *Channel::srcTypes[] =
{
    "NONE",
    "PEERCAST",
    "SHOUTCAST",
    "ICECAST",
    "URL",
    "HTTPPUSH",
};

// -----------------------------------
const char *Channel::statusMsgs[] =
{
    "NONE",
    "WAIT",
    "CONNECT",
    "REQUEST",
    "CLOSE",
    "RECEIVE",
    "BROADCAST",
    "ABORT",
    "SEARCH",
    "NOHOSTS",
    "IDLE",
    "ERROR",
    "NOTFOUND"
};

// -----------------------------------------------------------------------------
// Initialise the channel to its default settings of unallocated and reset.
// -----------------------------------------------------------------------------
Channel::Channel()
{
    next = NULL;
    reset();
}

// -----------------------------------------------------------------------------
void Channel::endThread()
{
    if (pushSock)
    {
        pushSock->close();
        delete pushSock;
        pushSock = NULL;
    }

    if (sock)
    {
        sock->close();
        sock = NULL;
    }

    if (sourceData)
    {
        delete sourceData;
        sourceData = NULL;
    }

    reset();

    chanMgr->deleteChannel(shared_from_this());
}

// -----------------------------------------------------------------------------
void Channel::resetPlayTime()
{
    info.lastPlayStart = sys->getTime();
}

// -----------------------------------------------------------------------------
void Channel::setStatus(STATUS s)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    if (s != status)
    {
        bool wasPlaying = isPlaying();

        status = s;

        if (isPlaying())
        {
            info.status = ChanInfo::S_PLAY;
            resetPlayTime();
        }else
        {
            if (wasPlaying)
                info.lastPlayEnd = sys->getTime();
            info.status = ChanInfo::S_UNKNOWN;
        }

        if (isBroadcasting())
        {
            ChanHitList *chl = chanMgr->findHitListByID(info.id);
            if (!chl)
                chanMgr->addHitList(info);
        }

        peercastApp->channelUpdate(&info);
    }
}

// -----------------------------------------------------------------------------
// Reset channel and make it available
// -----------------------------------------------------------------------------
void Channel::reset()
{
    sourceHost.init();
    remoteID.clear();

    streamIndex = 0;

    lastIdleTime = 0;

    info.init();

    mount.clear();
    bump = false;
    stayConnected = false;

    icyMetaInterval = 0;
    streamPos = 0;

    insertMeta.init();

    headPack.init();

    sourceStream = NULL;

    rawData.init();
    rawData.accept = ChanPacket::T_HEAD | ChanPacket::T_DATA;

    status = S_NONE;
    type = T_NONE;

    readDelay = false;
    sock = NULL;
    pushSock = NULL;

    sourceURL.clear();
    sourceData = NULL;

    lastTrackerUpdate = 0;
    lastMetaUpdate = 0;

    srcType = SRC_NONE;

    startTime = 0;
    syncTime = 0;
}

// -----------------------------------
void    Channel::newPacket(ChanPacket &pack)
{
    if (pack.type == ChanPacket::T_PCP)
        return;

    rawData.writePacket(pack, true);
}

// -----------------------------------
bool    Channel::checkIdle()
{
    return
        (info.getUptime() > chanMgr->prefetchTime) &&
        (localListeners() == 0) &&
        (!stayConnected) &&
        (status != S_BROADCASTING);
}

// -----------------------------------
// チャンネルごとのリレー数制限に達しているか。
bool    Channel::isFull()
{
    return chanMgr->maxRelaysPerChannel ? localRelays() >= chanMgr->maxRelaysPerChannel : false;
}

// -----------------------------------
int Channel::localRelays()
{
    return servMgr->numStreams(info.id, Servent::T_RELAY, true);
}

// -----------------------------------
int Channel::localListeners()
{
    return servMgr->numStreams(info.id, Servent::T_DIRECT, true);
}

// -----------------------------------
int Channel::totalRelays()
{
    int tot = 0;
    ChanHitList *chl = chanMgr->findHitListByID(info.id);
    if (chl)
        tot += chl->numHits();
    return tot;
}

// -----------------------------------
int Channel::totalListeners()
{
    int tot = localListeners();
    ChanHitList *chl = chanMgr->findHitListByID(info.id);
    if (chl)
        tot += chl->numListeners();
    return tot;
}

// -----------------------------------
void    Channel::startGet()
{
    srcType = SRC_PEERCAST;
    type = T_RELAY;
    info.srcProtocol = ChanInfo::SP_PCP;

    sourceData = new PeercastSource();

    startStream();
}

// -----------------------------------
void    Channel::startURL(const char *u)
{
    sourceURL.set(u);

    srcType = SRC_URL;
    type = T_BROADCAST;
    stayConnected = true;

    resetPlayTime();

    sourceData = new URLSource(u);

    startStream();
}

// -----------------------------------
void Channel::startStream()
{
    thread.channel = shared_from_this();
    thread.func = stream;
    if (!sys->startThread(&thread))
        reset();
}

// -----------------------------------
void Channel::sleepUntil(double time)
{
    double sleepTime = time - (sys->getDTime()-startTime);

//  LOG("sleep %g", sleepTime);
    if (sleepTime > 0)
    {
        if (sleepTime > 60) sleepTime = 60;

        double sleepMS = sleepTime*1000;

        sys->sleep((int)sleepMS);
    }
}

// -----------------------------------
void Channel::checkReadDelay(unsigned int len)
{
    if (readDelay && info.bitrate > 0)
    {
        unsigned int time = (len*1000)/((info.bitrate*1024)/8);
        sys->sleep(time);
    }
}

// -----------------------------------
THREAD_PROC Channel::stream(ThreadInfo *thread)
{
    auto ch = thread->channel;

    assert(thread->channel != nullptr);
    thread->channel = nullptr; // make sure to not leave the reference behind

    sys->setThreadName("CHANNEL");

    while (thread->active() && !peercastInst->isQuitting)
    {
        LOG_INFO("Channel started");

        ChanHitList *chl = chanMgr->findHitList(ch->info);
        if (!chl)
            chanMgr->addHitList(ch->info);

        ch->sourceData->stream(ch);

        LOG_INFO("Channel stopped");

        if (!ch->stayConnected)
        {
            break;
        }else
        {
            if (!ch->info.lastPlayEnd)
                ch->info.lastPlayEnd = sys->getTime();

            unsigned int diff = (sys->getTime() - ch->info.lastPlayEnd) + 5;

            for (unsigned int i=0; i<diff; i++)
            {
                if (!thread->active() || peercastInst->isQuitting)
                    break;

                if (i == 0)
                    LOG_DEBUG("Channel sleeping for %d seconds", diff);
                sys->sleep(1000);
            }
        }
    }

    ch->endThread();

    return 0;
}

// -----------------------------------
bool Channel::acceptGIV(ClientSocket *givSock)
{
    if (!pushSock)
    {
        pushSock = givSock;
        return true;
    }else
        return false;
}

// -----------------------------------
void Channel::connectFetch()
{
    sock = sys->createSocket();

    if (!sock)
        throw StreamException("Can`t create socket");

    if (sourceHost.tracker || sourceHost.yp)
    {
        sock->setReadTimeout(30000);
        sock->setWriteTimeout(30000);
        LOG_INFO("Channel using longer timeouts");
    }

    sock->open(sourceHost.host);

    sock->connect();
}

// -----------------------------------
int Channel::handshakeFetch()
{
    sock->writeLineF("GET /channel/%s HTTP/1.0", info.id.str().c_str());
    sock->writeLineF("%s %d", PCX_HS_POS, streamPos);
    sock->writeLineF("%s %d", PCX_HS_PCP, 1);

    sock->writeLine("");

    HTTP http(*sock);

    int r = http.readResponse();

    LOG_INFO("Got response: %d", r);

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        if (http.isHeader(PCX_HS_POS))
            streamPos = atoi(arg);
        else
        {
            // info の為。ロックする範囲が狭すぎるか。
            std::lock_guard<std::recursive_mutex> cs(lock);
            Servent::readICYHeader(http, info, NULL, 0);
        }

        LOG_INFO("Channel fetch: %s", http.cmdLine);
    }

    if ((r != 200) && (r != 503))
        return r;

    if (rawData.getLatestPos() > streamPos)
        rawData.init();

    AtomStream atom(*sock);

    String agent;

    Host rhost = sock->host;

    if (info.srcProtocol == ChanInfo::SP_PCP)
    {
        // don`t need PCP_CONNECT here
        Servent::handshakeOutgoingPCP(atom, rhost, remoteID, agent, sourceHost.yp|sourceHost.tracker);
    }

    return 0;
}

// -----------------------------------
int PeercastSource::getSourceRate()
{
    if (m_channel && m_channel->sock)
        return m_channel->sock->bytesInPerSec();
    else
        return 0;
}

// -----------------------------------
int PeercastSource::getSourceRateAvg()
{
    if (m_channel && m_channel->sock)
        return m_channel->sock->stat.bytesInPerSecAvg();
    else
        return 0;
}

// -----------------------------------
ChanHit PeercastSource::pickFromHitList(std::shared_ptr<Channel> ch, ChanHit &oldHit)
{
    ChanHit res = oldHit;
    ChanHitList *chl = NULL;

    chl = chanMgr->findHitList(ch->info);
    if (chl)
    {
        ChanHitSearch chs;

        // find local hit
        chs.init();
        chs.matchHost = servMgr->serverHost;
        chs.waitDelay = MIN_RELAY_RETRY;
        chs.excludeID = servMgr->sessionID;
        if (chl->pickHits(chs))
            res = chs.best[0];

        // else find global hit
        if (!res.host.ip)
        {
            chs.init();
            chs.waitDelay = MIN_RELAY_RETRY;
            chs.excludeID = servMgr->sessionID;
            if (chl->pickHits(chs))
                res = chs.best[0];
        }

        // else find local tracker
        if (!res.host.ip)
        {
            chs.init();
            chs.matchHost = servMgr->serverHost;
            chs.waitDelay = MIN_TRACKER_RETRY;
            chs.excludeID = servMgr->sessionID;
            chs.trackersOnly = true;
            if (chl->pickHits(chs))
                res = chs.best[0];
        }

        // else find global tracker
        if (!res.host.ip)
        {
            chs.init();
            chs.waitDelay = MIN_TRACKER_RETRY;
            chs.excludeID = servMgr->sessionID;
            chs.trackersOnly = true;
            if (chl->pickHits(chs))
                res = chs.best[0];
        }
    }
    return res;
}

// -----------------------------------
static std::string chName(ChanInfo& info)
{
    if (info.name.str().empty())
        return info.id.str().substr(0,7) + "...";
    else
        return info.name.str();
}

// -----------------------------------
void PeercastSource::stream(std::shared_ptr<Channel> ch)
{
    m_channel = ch;

    int numYPTries=0;
    int numCDTries = 0;
    while (ch->thread.active())
    {
        ch->sourceHost.init();

        ch->setStatus(Channel::S_SEARCHING);
        LOG_INFO("Channel searching for hit..");
        do
        {
            if (ch->pushSock)
            {
                ch->sock = ch->pushSock;
                ch->pushSock = NULL;
                ch->sourceHost.host = ch->sock->host;
                break;
            }

            if (ch->designatedHost.host.ip != 0)
            {
                ch->sourceHost = ch->designatedHost;
                ch->designatedHost.init();
                break;
            }

            ch->sourceHost = pickFromHitList(ch, ch->sourceHost);

            // consult channel directory
            if (!ch->sourceHost.host.ip)
            {
                std::string trackerIP = servMgr->channelDirectory->findTracker(ch->info.id);
                if (!trackerIP.empty())
                {
                    if (numCDTries >= 1)
                        break;
                    numCDTries++;
                    peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネルフィードで "+chName(ch->info)+" のトラッカーが見付かりました。");

                    ch->sourceHost.host.fromStrIP(trackerIP.c_str(), DEFAULT_PORT);
                    ch->sourceHost.rhost[0].fromStrIP(trackerIP.c_str(), DEFAULT_PORT);
                    ch->sourceHost.tracker = true;

                    auto chl = chanMgr->findHitList(ch->info);
                    if (chl)
                        chl->addHit(ch->sourceHost);
                    break;
                }
            }

            // no trackers found so contact YP
            if (!ch->sourceHost.host.ip)
            {
                if (servMgr->rootHost.isEmpty())
                    break;

                if (numYPTries >= 3)
                    break;

                unsigned int ctime=sys->getTime();
                if ((ctime-chanMgr->lastYPConnect) > MIN_YP_RETRY)
                {
                    ch->sourceHost.host.fromStrName(servMgr->rootHost.cstr(), DEFAULT_PORT);
                    ch->sourceHost.yp = true;
                    chanMgr->lastYPConnect=ctime;
                }
            }

            sys->sleepIdle();
        }while ((ch->sourceHost.host.ip==0) && (ch->thread.active()));

        if (!ch->sourceHost.host.ip)
        {
            LOG_ERROR("Channel giving up");
            break;
        }

        if (ch->sourceHost.yp)
        {
            numYPTries++;
            LOG_INFO("Channel contacting YP, try %d", numYPTries);
            peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネル "+chName(ch->info)+" をYPに問い合わせています...");
        }else
        {
            LOG_INFO("Channel found hit");
            numYPTries=0;
        }

        if (ch->sourceHost.host.ip)
        {
            //bool isTrusted = ch->sourceHost.tracker | ch->sourceHost.yp;

            if (ch->sourceHost.tracker)
                peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネル "+chName(ch->info)+" をトラッカーに問い合わせています...");

            char ipstr[64];
            ch->sourceHost.host.toStr(ipstr);

            const char *type = "";
            if (ch->sourceHost.tracker)
                type = "(tracker)";
            else if (ch->sourceHost.yp)
                type = "(YP)";

            int error=-1;
            try
            {
                ch->setStatus(Channel::S_CONNECTING);

                if (!ch->sock)
                {
                    LOG_INFO("Channel connecting to %s %s", ipstr, type);
                    ch->connectFetch();
                }

                error = ch->handshakeFetch();
                if (error)
                    throw StreamException("Handshake error");

                ch->sourceStream = ch->createSource();

                error = ch->readStream(*ch->sock, ch->sourceStream);
                if (error)
                    throw StreamException("Stream error");

                error = 0;      // no errors, closing normally.
                ch->setStatus(Channel::S_CLOSING);

                LOG_INFO("Channel closed normally");
            }catch (StreamException &e)
            {
                ch->setStatus(Channel::S_ERROR);
                LOG_ERROR("Channel to %s %s : %s", ipstr, type, e.msg);
                if (!ch->sourceHost.tracker || ((error != 503) && ch->sourceHost.tracker))
                    chanMgr->deadHit(ch->sourceHost);
            }

            // broadcast quit to any connected downstream servents
            {
                ChanPacket pack;
                MemoryStream mem(pack.data, sizeof(pack.data));
                AtomStream atom(mem);
                atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_OFFAIR);
                pack.len = mem.pos;
                pack.type = ChanPacket::T_PCP;
                GnuID noID;
                servMgr->broadcastPacket(pack, ch->info.id, ch->remoteID, noID, Servent::T_RELAY);
            }

            if (ch->sourceStream)
            {
                try
                {
                    if (!error)
                    {
                        ch->sourceStream->updateStatus(ch);
                        ch->sourceStream->flush(*ch->sock);
                    }
                }catch (StreamException &)
                {}
                ChannelStream *cs = ch->sourceStream;
                ch->sourceStream = NULL;
                cs->kill();
                delete cs;
            }

            if (ch->sock)
            {
                ch->sock->close();
                delete ch->sock;
                ch->sock = NULL;
            }

            if (error == 404)
            {
                LOG_ERROR("Channel not found");
                return;
            }
        }

        ch->lastIdleTime = sys->getTime();
        ch->setStatus(Channel::S_IDLE);
        while ((ch->checkIdle()) && (ch->thread.active()))
        {
            sys->sleep(200);
        }

        sys->sleepIdle();
    }
}

// -----------------------------------
void    Channel::startHTTPPush(ClientSocket *cs, bool isChunked)
{
    srcType = SRC_HTTPPUSH;
    type    = T_BROADCAST;

    sock = cs;
    info.srcProtocol = ChanInfo::SP_HTTP;

    sourceData = new HTTPPushSource(isChunked);
    startStream();
}

// -----------------------------------
void    Channel::startWMHTTPPush(ClientSocket *cs)
{
    srcType = SRC_HTTPPUSH;
    type    = T_BROADCAST;

    sock = cs;
    info.srcProtocol = ChanInfo::SP_WMHTTP;

    sourceData = new HTTPPushSource(false);
    startStream();
}

// -----------------------------------
void    Channel::startICY(ClientSocket *cs, SRC_TYPE st)
{
    srcType = st;
    type = T_BROADCAST;
    cs->setReadTimeout(0);  // stay connected even when theres no data coming through
    sock = cs;
    info.srcProtocol = ChanInfo::SP_HTTP;

    streamIndex = ++chanMgr->icyIndex;

    sourceData = new ICYSource();
    startStream();
}

// -----------------------------------
static char *nextMetaPart(char *str, char delim)
{
    while (*str)
    {
        if (*str == delim)
        {
            *str++ = 0;
            return str;
        }
        str++;
    }
    return NULL;
}

// -----------------------------------
void Channel::processMp3Metadata(char *str)
{
    ChanInfo newInfo = info;

    char *cmd=str;
    while (cmd)
    {
        char *arg = nextMetaPart(cmd, '=');
        if (!arg)
            break;

        char *next = nextMetaPart(arg, ';');

        if (strcmp(cmd, "StreamTitle")==0)
        {
            newInfo.track.title.setUnquote(arg, String::T_ASCII);
            newInfo.track.title.convertTo(String::T_UNICODE);
        }else if (strcmp(cmd, "StreamUrl")==0)
        {
            newInfo.track.contact.setUnquote(arg, String::T_ASCII);
            newInfo.track.contact.convertTo(String::T_UNICODE);
        }

        cmd = next;
    }

    updateInfo(newInfo);
}

// -----------------------------------
XML::Node *ChanHit::createXML()
{
    return new XML::Node("host ip=\"%s\" hops=\"%d\" listeners=\"%d\" relays=\"%d\" uptime=\"%d\" push=\"%d\" relay=\"%d\" direct=\"%d\" cin=\"%d\" stable=\"%d\" version=\"%d\" update=\"%d\" tracker=\"%d\"",
        host.str().c_str(),
        numHops,
        numListeners,
        numRelays,
        upTime,
        firewalled?1:0,
        relay?1:0,
        direct?1:0,
        cin?1:0,
        stable?1:0,
        version,
        sys->getTime()-time,
        tracker
        );
}

// -----------------------------------
XML::Node *ChanHitList::createXML(bool addHits)
{
    XML::Node *hn = new XML::Node("hits hosts=\"%d\" listeners=\"%d\" relays=\"%d\" firewalled=\"%d\" closest=\"%d\" furthest=\"%d\" newest=\"%d\"",
        numHits(),
        numListeners(),
        numRelays(),
        numFirewalled(),
        closestHit(),
        furthestHit(),
        sys->getTime()-newestHit()
        );

    if (addHits)
    {
        ChanHit *h = hit;
        while (h)
        {
            if (h->host.ip)
                hn->add(h->createXML());
            h = h->next;
        }
    }

    return hn;
}

// -----------------------------------
XML::Node *Channel::createRelayXML(bool showStat)
{
    const char *ststr;
    ststr = getStatusStr();
    if (!showStat)
        if ((status == S_RECEIVING) || (status == S_BROADCASTING))
            ststr = "OK";

    ChanHitList *chl = chanMgr->findHitList(info);

    return new XML::Node("relay listeners=\"%d\" relays=\"%d\" hosts=\"%d\" status=\"%s\"",
        localListeners(),
        localRelays(),
        (chl!=NULL)?chl->numHits():0,
        ststr
        );
}

// -----------------------------------
void ChanMeta::fromXML(XML &xml)
{
    MemoryStream tout(data, MAX_DATALEN);
    xml.write(tout);

    len = tout.pos;
}

// -----------------------------------
void ChanMeta::fromMem(void *p, int l)
{
    len = l;
    memcpy(data, p, len);
}

// -----------------------------------
void ChanMeta::addMem(void *p, int l)
{
    if ((len+l) <= MAX_DATALEN)
    {
        memcpy(data+len, p, l);
        len += l;
    }
}

// -----------------------------------
void Channel::broadcastTrackerUpdate(GnuID &svID, bool force /* = false */)
{
    unsigned int ctime = sys->getTime();

    if (((ctime-lastTrackerUpdate) > 30) || (force))
    {
        ChanPacket pack;

        MemoryStream mem(pack.data, sizeof(pack.data));

        AtomStream atom(mem);

        ChanHit hit;

        ChanHitList *chl = chanMgr->findHitListByID(info.id);
        if (!chl)
            throw StreamException("Broadcast channel has no hitlist");

        int numListeners = totalListeners();
        int numRelays = totalRelays();

        unsigned int oldp = rawData.getOldestPos();
        unsigned int newp = rawData.getLatestPos();

        hit.initLocal(numListeners, numRelays, info.numSkips, info.getUptime(), isPlaying(), oldp, newp, this->sourceHost.host);
        hit.tracker = true;

        atom.writeParent(PCP_BCST, 10);
            atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_ROOT);
            atom.writeChar(PCP_BCST_HOPS, 0);
            atom.writeChar(PCP_BCST_TTL, 7);
            atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
            atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
            atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
            atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
            atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
            atom.writeParent(PCP_CHAN, 4);
                atom.writeBytes(PCP_CHAN_ID, info.id.id, 16);
                atom.writeBytes(PCP_CHAN_BCID, chanMgr->broadcastID.id, 16);
                info.writeInfoAtoms(atom);
                info.writeTrackAtoms(atom);
            hit.writeAtoms(atom, info.id);

        pack.len = mem.pos;
        pack.type = ChanPacket::T_PCP;

        GnuID noID;
        int cnt = servMgr->broadcastPacket(pack, noID, servMgr->sessionID, svID, Servent::T_COUT);

        if (cnt)
        {
            LOG_DEBUG("Sent tracker update for %s to %d client(s)", info.name.cstr(), cnt);
            lastTrackerUpdate = ctime;
        }
    }
}

// -----------------------------------
bool    Channel::sendPacketUp(ChanPacket &pack, GnuID &cid, GnuID &sid, GnuID &did)
{
    if ( isActive()
        && (!cid.isSet() || info.id.isSame(cid))
        && (!sid.isSet() || !remoteID.isSame(sid))
        && sourceStream
       )
        return sourceStream->sendPacket(pack, did);

    return false;
}

// -----------------------------------
void Channel::updateInfo(const ChanInfo &newInfo)
{
    String oldComment = info.comment;

    if (!info.update(newInfo))
        return; // チャンネル情報は更新されなかった。

    // コメント更新の通知。
    if (!oldComment.isSame(info.comment))
    {
        // Shift_JIS かも知れない文字列を UTF8 に変換したい。
        String newComment = info.comment;
        newComment.convertTo(String::T_UNICODE);

        peercast::notifyMessage(ServMgr::NT_PEERCAST, info.name.str() + "「" + newComment.str() + "」");
    }

    if (isBroadcasting())
    {
        unsigned int ctime = sys->getTime();
        if ((ctime - lastMetaUpdate) > 30)
        {
            lastMetaUpdate = ctime;

            ChanPacket pack;
            MemoryStream mem(pack.data, sizeof(pack.data));
            AtomStream atom(mem);

            atom.writeParent(PCP_BCST, 10);
                atom.writeChar(PCP_BCST_HOPS, 0);
                atom.writeChar(PCP_BCST_TTL, 7);
                atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_RELAYS);
                atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
                atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
                atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
                atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
                atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
                atom.writeBytes(PCP_BCST_CHANID, info.id.id, 16);
                atom.writeParent(PCP_CHAN, 3);
                    atom.writeBytes(PCP_CHAN_ID, info.id.id, 16);
                    info.writeInfoAtoms(atom);
                    info.writeTrackAtoms(atom);

            pack.len = mem.pos;
            pack.type = ChanPacket::T_PCP;
            GnuID noID;
            servMgr->broadcastPacket(pack, info.id, servMgr->sessionID, noID, Servent::T_RELAY);

            broadcastTrackerUpdate(noID);
        }
    }

    ChanHitList *chl = chanMgr->findHitList(info);
    if (chl)
        chl->info = info;

    peercastApp->channelUpdate(&info);
}

// -----------------------------------
ChannelStream *Channel::createSource()
{
//  if (servMgr->relayBroadcast)
//      chanMgr->broadcastRelays(NULL, chanMgr->minBroadcastTTL, chanMgr->maxBroadcastTTL);

    ChannelStream *source=NULL;

    if (info.srcProtocol == ChanInfo::SP_PEERCAST)
    {
        LOG_INFO("Channel is Peercast");
        source = new PeercastStream();
    }
    else if (info.srcProtocol == ChanInfo::SP_PCP)
    {
        LOG_INFO("Channel is PCP");
        PCPStream *pcp = new PCPStream(remoteID);
        source = pcp;
    }
    else if (info.srcProtocol == ChanInfo::SP_MMS)
    {
        LOG_INFO("Channel is MMS");
        source = new MMSStream();
    }else if (info.srcProtocol == ChanInfo::SP_WMHTTP)
    {
        switch (info.contentType)
        {
            case ChanInfo::T_WMA:
            case ChanInfo::T_WMV:
                LOG_INFO("Channel is WMHTTP");
                source = new WMHTTPStream();
                break;
            default:
                throw StreamException("Channel is WMHTTP - but not WMA/WMV");
                break;
        }
    }else{
        switch (info.contentType)
        {
            case ChanInfo::T_MP3:
                LOG_INFO("Channel is MP3 - meta: %d", icyMetaInterval);
                source = new MP3Stream();
                break;
            case ChanInfo::T_NSV:
                LOG_INFO("Channel is NSV");
                source = new NSVStream();
                break;
            case ChanInfo::T_WMA:
            case ChanInfo::T_WMV:
                LOG_INFO("Channel is MMS");
                source = new MMSStream();
                break;
            case ChanInfo::T_FLV:
                LOG_INFO("Channel is FLV");
                source = new FLVStream();
                break;
            case ChanInfo::T_OGG:
            case ChanInfo::T_OGM:
                LOG_INFO("Channel is OGG");
                source = new OGGStream();
                break;
            case ChanInfo::T_MKV:
                LOG_INFO("Channel is MKV");
                source = new MKVStream();
                break;
            case ChanInfo::T_WEBM:
                LOG_INFO("Channel is WebM");
                source = new MKVStream();
                break;
            default:
                LOG_INFO("Channel is Raw");
                source = new RawStream();
                break;
        }
    }

    return source;
}

// -----------------------------------
bool    Channel::checkBump()
{
    if (!isBroadcasting() && (!sourceHost.tracker))
        if (rawData.lastWriteTime && ((sys->getTime() - rawData.lastWriteTime) > 30))
        {
            LOG_ERROR("Channel Auto bumped");
            bump = true;
        }

    if (bump)
    {
        bump = false;
        return true;
    }else
        return false;
}

// -----------------------------------
int Channel::readStream(Stream &in, ChannelStream *source)
{
    int error = 0;

    info.numSkips = 0;

    source->readHeader(in, shared_from_this());

    peercastApp->channelStart(&info);

    rawData.lastWriteTime = 0;

    bool wasBroadcasting=false;

    try
    {
        while (thread.active() && !peercastInst->isQuitting)
        {
            if (checkIdle())
            {
                LOG_DEBUG("Channel idle");
                peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネル "+chName(info)+" がアイドル状態になりました。");
                break;
            }

            if (checkBump())
            {
                LOG_DEBUG("Channel bumped");
                peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネル "+chName(info)+" をバンプしました。");
                error = -1;
                break;
            }

            if (in.eof())
            {
                LOG_DEBUG("Channel eof");
                break;
            }

            if (in.readReady(sys->idleSleepTime))
            {
                error = source->readPacket(in, shared_from_this());

                if (error)
                    break;

                if (rawData.writePos > 0)
                {
                    if (isBroadcasting())
                    {
                        if ((sys->getTime() - lastTrackerUpdate) >= 120)
                        {
                            GnuID noID;
                            broadcastTrackerUpdate(noID);
                        }
                        wasBroadcasting = true;
                    }else
                    {
                        if (!isReceiving())
                            peercast::notifyMessage(ServMgr::NT_PEERCAST, info.name.str() + "を受信中です。");
                        setStatus(Channel::S_RECEIVING);
                    }
                    source->updateStatus(shared_from_this());
                }
            }
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("readStream: %s", e.msg);
        error = -1;
    }

    setStatus(S_CLOSING);

    if (wasBroadcasting)
    {
        GnuID noID;
        broadcastTrackerUpdate(noID, true);
    }

    peercastApp->channelStop(&info);

    source->readEnd(in, shared_from_this());

    return error;
}

// -----------------------------------
void PeercastStream::readHeader(Stream &in, std::shared_ptr<Channel> ch)
{
    if (in.readTag() != 'PCST')
        throw StreamException("Not PeerCast stream");
}

// -----------------------------------
void PeercastStream::readEnd(Stream &, std::shared_ptr<Channel>)
{
}

// -----------------------------------
int PeercastStream::readPacket(Stream &in, std::shared_ptr<Channel> ch)
{
    ChanPacket pack;

    pack.readPeercast(in);

    MemoryStream mem(pack.data, pack.len);

    switch(pack.type)
    {
        case ChanPacket::T_HEAD:
            // update sync pos
            ch->headPack = pack;
            pack.pos = ch->streamPos;
            ch->newPacket(pack);
            ch->streamPos += pack.len;
            break;
        case ChanPacket::T_DATA:
            pack.pos = ch->streamPos;
            ch->newPacket(pack);
            ch->streamPos += pack.len;
            break;
        case ChanPacket::T_META:
            ch->insertMeta.fromMem(pack.data, pack.len);
            if (pack.len)
            {
                XML xml;
                xml.read(mem);
                XML::Node *n = xml.findNode("channel");
                if (n)
                {
                    ChanInfo newInfo = ch->info;
                    newInfo.updateFromXML(n);
                    ChanHitList *chl = chanMgr->findHitList(ch->info);
                    if (chl)
                        newInfo.updateFromXML(n);
                    ch->updateInfo(newInfo);
                }
            }
            break;
#if 0
        case ChanPacket::T_SYNC:
            {
                unsigned int s = mem.readLong();
                if ((s-ch->syncPos) != 1)
                {
                    LOG_INFO("Ch.%d SKIP: %d to %d (%d)", ch->index, ch->syncPos, s, ch->info.numSkips);
                    if (ch->syncPos)
                    {
                        ch->info.numSkips++;
                        if (ch->info.numSkips>50)
                            throw StreamException("Bumped - Too many skips");
                    }
                }

                ch->syncPos = s;
            }
            break;
#endif
        default:
            throw StreamException("unknown packet type");
    }

    return 0;
}

// ------------------------------------------
void RawStream::readHeader(Stream &, std::shared_ptr<Channel>)
{
}

// ------------------------------------------
int RawStream::readPacket(Stream &in, std::shared_ptr<Channel> ch)
{
    readRaw(in, ch);
    return 0;
}

// ------------------------------------------
void RawStream::readEnd(Stream &, std::shared_ptr<Channel>)
{
}

// -----------------------------------
std::string Channel::renderHexDump(const std::string& in)
{
    std::string res;
    size_t i;
    for (i = 0; i < in.size()/16; i++)
    {
        auto line = in.substr(i*16, 16);
        res += str::hexdump(line) + "  " + str::ascii_dump(line) + "\n";
    }
    auto rem = in.size() - 16*(in.size()/16);
    if (rem)
    {
        auto line = in.substr(16*(in.size()/16), rem);
        res += str::format("%-47s  %s\n", str::hexdump(line).c_str(), str::ascii_dump(line).c_str());
    }
    return res;
}

// -----------------------------------
std::string Channel::getSourceString()
{
    std::string buf;

    if (sourceURL.isEmpty())
    {
        if (srcType == SRC_HTTPPUSH)
            buf = sock->host.str();
        else
        {
            buf = sourceHost.str(true);
            if (sourceHost.uphost.ip)
            {
                buf += " recv. from ";
                buf += sourceHost.uphost.str();
            }
        }
    }
    else
        buf = sourceURL.c_str();

    return buf;
}

// -----------------------------------
std::string Channel::getBufferString()
{
    std::string buf;
    String time;
    auto lastWritten = (double)sys->getTime() - rawData.lastWriteTime;

    if (lastWritten < 5)
        time = "< 5 sec";
    else
        time.setFromStopwatch(lastWritten);

    auto stat = rawData.getStatistics();
    auto& lens = stat.packetLengths;
    double byterate = (sourceData) ? sourceData->getSourceRateAvg() : 0.0;
    auto sum = std::accumulate(lens.begin(), lens.end(), 0);

    buf = str::format("Length: %s bytes (%.2f sec)\n",
                      str::group_digits(std::to_string(sum)).c_str(),
                      sum / byterate);
    buf += str::format("Packets: %lu (c %d / nc %d)\n",
                       lens.size(),
                       stat.continuations,
                       stat.nonContinuations);

    if (lens.size() > 0)
    {
        auto pmax = std::max_element(lens.begin(), lens.end());
        auto pmin = std::min_element(lens.begin(), lens.end());
        buf += str::format("Packet length min/avg/max: %u/%lu/%u\n",
                           *pmin, sum/lens.size(), *pmax);
    }
    buf += str::format("Last written: %s", time.str().c_str());

    return buf;
}

// -----------------------------------
bool Channel::writeVariable(Stream &out, const String &var)
{
    using namespace std;

    string buf;
    String utf8;

    if (var == "name")
    {
        utf8 = info.name;
        utf8.convertTo(String::T_UNICODESAFE);
        buf = utf8.c_str();
    }else if (var == "bitrate")
    {
        buf = to_string(info.bitrate);
    }else if (var == "srcrate")
    {
        if (sourceData)
        {
            unsigned int tot = sourceData->getSourceRate();
            buf = str::format("%.0f", BYTES_TO_KBPS(tot));
        }else
            buf = "0";
    }else if (var == "genre")
    {
        utf8 = info.genre;
        utf8.convertTo(String::T_UNICODE);
        buf = utf8.c_str();
    }else if (var == "desc")
    {
        utf8 = info.desc;
        utf8.convertTo(String::T_UNICODE);
        buf = utf8.c_str();
    }else if (var == "comment")
    {
        utf8 = info.comment;
        utf8.convertTo(String::T_UNICODE);
        buf = utf8.c_str();
    }else if (var == "uptime")
    {
        String uptime;
        if (info.lastPlayStart)
            uptime.setFromStopwatch(sys->getTime()-info.lastPlayStart);
        else
            uptime.set("-");
        buf = uptime.c_str();
    }
    else if (var == "type")
        buf = info.getTypeStr();
    else if (var == "typeLong")
        buf = info.getTypeStringLong();
    else if (var == "ext")
        buf = info.getTypeExt();
    else if (var == "localRelays")
        buf = to_string(localRelays());
    else if (var == "localListeners")
        buf = to_string(localListeners());
    else if (var == "totalRelays")
        buf = to_string(totalRelays());
    else if (var == "totalListeners")
        buf = to_string(totalListeners());
    else if (var == "status")
        buf = getStatusStr();
    else if (var == "keep")
        buf = stayConnected?"Yes":"No";
    else if (var == "id")
        buf = info.id.str();
    else if (var.startsWith("track."))
    {
        if (var == "track.title")
            utf8 = info.track.title;
        else if (var == "track.artist")
            utf8 = info.track.artist;
        else if (var == "track.album")
            utf8 = info.track.album;
        else if (var == "track.genre")
            utf8 = info.track.genre;
        else if (var == "track.contactURL")
            utf8 = info.track.contact;

        utf8.convertTo(String::T_UNICODE);
        buf = utf8.c_str();
    }else if (var == "contactURL")
        buf = info.url.cstr();
    else if (var == "streamPos")
        buf = str::group_digits(std::to_string(streamPos), ",");
    else if (var == "sourceType")
        buf = getSrcTypeStr();
    else if (var == "sourceProtocol")
        buf = ChanInfo::getProtocolStr(info.srcProtocol);
    else if (var == "sourceURL")
        buf = getSourceString();
    else if (var == "headPos")
        buf = str::group_digits(std::to_string(headPack.pos), ",");
    else if (var == "headLen")
        buf = str::group_digits(std::to_string(headPack.len), ",");
    else if (var == "buffer")
        buf = getBufferString();
    else if (var == "headDump")
    {
        out.writeString(renderHexDump(std::string(headPack.data, headPack.data + headPack.len)));
        return true;
    }else if (var == "numHits")
    {
        ChanHitList *chl = chanMgr->findHitListByID(info.id);
        buf = to_string((chl) ? chl->numHits() : 0);
    }else if (var == "authToken")
        buf = chanMgr->authToken(info.id).c_str();
    else if (var == "plsExt")
        buf = info.getPlayListExt();
    else
        return false;

    out.writeString(buf);
    return true;
}

// -----------------------------------
// message check
#if 0
                ChanPacket pack;
                MemoryStream mem(pack.data, sizeof(pack.data));
                AtomStream atom(mem);
                atom.writeParent(PCP_BCST, 3);
                    atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_ALL);
                    atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
                    atom.writeParent(PCP_MESG, 1);
                        atom.writeString(PCP_MESG_DATA, msg.cstr());

                mem.len = mem.pos;
                mem.rewind();
                pack.len = mem.len;

                GnuID noID;
                noID.clear();

                BroadcastState bcs;
                PCPStream::readAtom(atom, bcs);
                //int cnt = servMgr->broadcastPacketUp(pack, noID, servMgr->sessionID);
                //int cnt = servMgr->broadcastPacketDown(pack, noID, servMgr->sessionID);
                //int cnt = chanMgr->broadcastPacketUp(pack, noID, servMgr->sessionID);
                //LOG_DEBUG("Sent message to %d clients", cnt);
#endif
