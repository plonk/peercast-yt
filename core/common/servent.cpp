// ------------------------------------------------
// File : servent.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Servents are the actual connections between clients. They do the handshaking,
//      transfering of data and processing of GnuPackets. Each servent has one socket allocated
//      to it on connect, it uses this to transfer all of its data.
//
// (c) 2002 peercast.org
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
// todo: make lan->yp not check firewall

#include "servent.h"
#include "sys.h"
#include "xml.h"
#include "html.h"
#include "http.h"
#include "stats.h"
#include "servmgr.h"
#include "peercast.h"
#include "atom.h"
#include "pcp.h"
#include "version2.h"
#include "defer.h"
#include "gnutella.h"
#include "chanmgr.h"
#include "regexp.h"

const int DIRECT_WRITE_TIMEOUT = 60;

// -----------------------------------
const char *Servent::statusMsgs[] =
{
    "NONE",
    "CONNECTING",
    "PROTOCOL",
    "HANDSHAKE",
    "CONNECTED",
    "CLOSING",
    "LISTENING",
    "TIMEOUT",
    "REFUSED",
    "VERIFIED",
    "ERROR",
    "WAIT",
    "FREE"
};

// -----------------------------------
const char *Servent::typeMsgs[] =
{
    "NONE",
    "INCOMING",
    "SERVER",
    "RELAY",
    "DIRECT",
    "COUT",
    "CIN",
    "COMMAND",
};

// -----------------------------------
bool    Servent::isPrivate()
{
    Host h = getHost();
    return servMgr->isFiltered(ServFilter::F_PRIVATE, h) || h.isLocalhost();
}

// -----------------------------------
bool    Servent::isAllowed(int a)
{
    Host h = getHost();

    if (servMgr->isFiltered(ServFilter::F_BAN, h))
        return false;

    return (allow&a)!=0;
}

// -----------------------------------
bool    Servent::isFiltered(int f)
{
    Host h = getHost();
    return servMgr->isFiltered(f, h);
}

// -----------------------------------
// リレーあるいはダイレクト接続でストリーミングできるかを真偽値で返す。
// できない場合は reason に理由が設定される。できる場合は reason は変
// 更されない。
bool Servent::canStream(std::shared_ptr<Channel> ch, Servent::StreamRequestDenialReason *reason)
{
    if (ch==nullptr)
    {
        *reason = StreamRequestDenialReason::Other;
        return false;
    }

    if (servMgr->isDisabled)
    {
        *reason = StreamRequestDenialReason::Other;
        return false;
    }

    if (!isPrivate())
    {
        if  (servMgr->bitrateFull(ch->getBitrate()))
        {
            LOG_DEBUG("Unable to stream because there is not enough bandwidth left");
            *reason = StreamRequestDenialReason::InsufficientBandwidth;
            return false;
        }

        if ((type == T_RELAY) && servMgr->relaysFull())
        {
            LOG_DEBUG("Unable to stream because server already has max. number of relays");
            *reason = StreamRequestDenialReason::RelayLimit;
            return false;
        }

        if ((type == T_DIRECT) && servMgr->directFull())
        {
            LOG_DEBUG("Unable to stream because server already has max. number of directs");
            *reason = StreamRequestDenialReason::DirectLimit;
            return false;
        }

        if (!ch->isPlaying())
        {
            LOG_DEBUG("Unable to stream because channel is not playing");
            *reason = StreamRequestDenialReason::NotPlaying;
            return false;
        }

        if ((type == T_RELAY) && ch->isFull())
        {
            LOG_DEBUG("Unable to stream because channel already has max. number of relays");
            *reason = StreamRequestDenialReason::PerChannelRelayLimit;
            return false;
        }
    }

    return true;
}

// -----------------------------------
Servent::Servent(int index)
    : serventIndex(index)
    , sock(nullptr)
    , next(nullptr)
{
    reset();
}

// -----------------------------------
Servent::~Servent()
{
}

// -----------------------------------
void    Servent::kill()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    thread.shutdown();

    setStatus(S_CLOSING);

    if (pcpStream)
    {
        PCPStream *pcp = pcpStream;
        pcpStream = nullptr;
        pcp->kill();
        delete pcp;
    }

    if (sock)
    {
        sock->close();
        sock = nullptr;
    }

    if (pushSock)
    {
        pushSock->close();
        pushSock = nullptr;
    }

    if (type != T_SERVER)
    {
        reset();
        setStatus(S_FREE);
    }
}

// -----------------------------------
void    Servent::abort()
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    thread.shutdown();
    if (sock)
    {
        sock->close();
    }
}

// -----------------------------------
void Servent::reset()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    remoteID.clear();

    servPort = 0;

    pcpStream = nullptr;

    networkID.clear();

    chanID.clear();

    outputProtocol = ChanInfo::SP_UNKNOWN;

    agent.clear();
    sock = nullptr;
    allow = ALLOW_ALL;
    syncPos = 0;
    addMetadata = false;
    nsSwitchNum = 0;
    lastConnect = lastPing = lastPacket = 0;

    loginPassword.clear();
    loginMount.clear();

    priorityConnect = false;
    pushSock = nullptr;
    sendHeader = true;

    status = S_NONE;
    type = T_NONE;

    streamPos = 0;

    cookie.clear();
}

// -----------------------------------
bool Servent::sendPacket(ChanPacket &pack, const GnuID &cid, const GnuID &sid, const GnuID &did, Servent::TYPE t)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    if  (      (type == t)
            && (isConnected())
            && (!cid.isSet() || chanID.isSame(cid))
            && (!sid.isSet() || !sid.isSame(remoteID))
            && (pcpStream != nullptr)
        )
    {
        return pcpStream->sendPacket(pack, did);
    }
    return false;
}

// -----------------------------------
bool Servent::acceptGIV(std::shared_ptr<ClientSocket> givSock)
{
    if (!pushSock)
    {
        pushSock = givSock;
        return true;
    }else
        return false;
}

// -----------------------------------
Host Servent::getHost()
{
    Host h(0, 0);

    if (sock)
        h = sock->host;

    return h;
}

// ------------------------------------------------------------------
// ソケットでの待ち受けを行う SERVER サーバントを開始する。成功すれば
// true を返す。
bool Servent::initServer(Host &h)
{
    try
    {
        checkFree();

        status = S_WAIT;

        createSocket();

        sock->bind(h);

        thread.data = this;
        thread.func = serverProc;

        type = T_SERVER;

        if (!sys->startThread(&thread))
            throw StreamException("Can`t start thread");
    }catch (StreamException &e)
    {
        LOG_ERROR("Bad server: %s", e.msg);
        kill();
        return false;
    }

    return true;
}

// -----------------------------------
void Servent::checkFree()
{
    if (sock)
        throw StreamException("Socket already set");
    if (thread.active())
        throw StreamException("Thread already active");
}

// -----------------------------------
// クライアントとの対話を開始する。
void Servent::initIncoming(std::shared_ptr<ClientSocket> s, unsigned int a)
{
    try{
        checkFree();

        type = T_INCOMING;
        sock = s;
        allow = a;
        thread.data = this;
        thread.func = incomingProc;

        setStatus(S_PROTOCOL);

        LOG_DEBUG("Incoming from %s", sock->host.str().c_str());

        if (!sys->startThread(&thread))
            throw StreamException("Can`t start thread");
    }catch (StreamException &e)
    {
        kill();

        LOG_ERROR("INCOMING FAILED: %s", e.msg);
    }
}

// -----------------------------------
void Servent::initOutgoing(TYPE ty)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    try
    {
        checkFree();

        type = ty;

        thread.data = this;
        thread.func = outgoingProc;

        if (!sys->startThread(&thread))
            throw StreamException("Can`t start thread");
    }catch (StreamException &e)
    {
        LOG_ERROR("Unable to start outgoing: %s", e.msg);
        kill();
    }
}

// -----------------------------------
void Servent::initPCP(const Host &rh)
{
    try
    {
        checkFree();

        createSocket();

        type = T_COUT;

        sock->open(rh);

        if (!isAllowed(ALLOW_NETWORK))
            throw StreamException("Servent not allowed");

        thread.data = this;
        thread.func = outgoingProc;

        LOG_DEBUG("Outgoing to %s", rh.str().c_str());

        if (!sys->startThread(&thread))
            throw StreamException("Can`t start thread");
    }catch (StreamException &e)
    {
        LOG_ERROR("Unable to open connection to %s - %s", rh.str().c_str(), e.msg);
        kill();
    }
}

// -----------------------------------
void Servent::initGIV(const Host &h, const GnuID &id)
{
    try
    {
        checkFree();

        givID = id;

        createSocket();

        sock->open(h);

        if (!isAllowed(ALLOW_NETWORK))
            throw StreamException("Servent not allowed");

        sock->connect();

        thread.data = this;
        thread.func = givProc;

        type = T_RELAY;

        if (!sys->startThread(&thread))
            throw StreamException("Can`t start thread");
    }catch (StreamException &e)
    {
        LOG_ERROR("GIV error to %s: %s", h.str().c_str(), e.msg);
        kill();
    }
}

// -----------------------------------
void Servent::createSocket()
{
    if (sock)
        LOG_ERROR("Servent::createSocket attempt made while active");

    sock = sys->createSocket();
}

// -----------------------------------
void Servent::setStatus(STATUS s)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    if (s != status)
    {
        status = s;

        if ((s == S_HANDSHAKE) || (s == S_CONNECTED) || (s == S_LISTENING))
            lastConnect = sys->getTime();
    }
}

// -----------------------------------
bool    Servent::pingHost(Host &rhost, const GnuID &rsid)
{
    char ipstr[64];
    strcpy(ipstr, rhost.str().c_str());
    LOG_DEBUG("Ping host %s: trying..", ipstr);
    std::shared_ptr<ClientSocket> s;
    bool hostOK=false;
    try
    {
        s = sys->createSocket();
        if (!s)
            return false;
        else
        {
            s->setReadTimeout(15000);
            s->setWriteTimeout(15000);
            s->open(rhost);
            s->connect();

            AtomStream atom(*s);

            if (rhost.ip.isIPv4Mapped()) {
                atom.writeInt(PCP_CONNECT, 1);
            } else {
                atom.writeInt(PCP_CONNECT, 100);
            }
            atom.writeParent(PCP_HELO, 1);
                atom.writeBytes(PCP_HELO_SESSIONID, servMgr->sessionID.id, 16);

            GnuID sid;

            int numc, numd;
            ID4 id = atom.read(numc, numd);
            if (id == PCP_OLEH)
            {
                for (int i=0; i<numc; i++)
                {
                    int c, d;
                    ID4 pid = atom.read(c, d);
                    if (pid == PCP_SESSIONID)
                        atom.readBytes(sid.id, 16, d);
                    else
                        atom.skip(c, d);
                }
            }else
            {
                LOG_DEBUG("Ping response: %s", id.getString().str());
                throw StreamException("Bad ping response");
            }

            if (!sid.isSame(rsid))
                throw StreamException("SIDs don`t match");

            hostOK = true;
            LOG_DEBUG("Ping host %s: OK", ipstr);
            atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT);
        }
    }catch (StreamException &e)
    {
        LOG_DEBUG("Ping host %s: %s", ipstr, e.msg);
    }
    if (s)
    {
        s->close();
    }

    if (!hostOK)
        rhost.port = 0;

    return true;
}

// -----------------------------------
// HTTP ヘッダーを読み込み、gotPCP, reqPos, nsSwitchNum, this->addMetaData,
// this->agent を設定する。
void Servent::handshakeStream_readHeaders(bool& gotPCP, unsigned int& reqPos, int& nsSwitchNum)
{
    HTTP http(*sock);

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        if (http.isHeader(PCX_HS_PCP))
            gotPCP = atoi(arg)!=0;
        else if (http.isHeader(PCX_HS_POS))
            reqPos = atoi(arg);
        else if (http.isHeader("icy-metadata"))
            addMetadata = atoi(arg) > 0;
        else if (http.isHeader(HTTP_HS_AGENT))
            agent = arg;
        else if (http.isHeader("Pragma"))
        {
            char *ssc = stristr(arg, "stream-switch-count=");

            if (ssc)
            {
                nsSwitchNum = 1;
                //nsSwitchNum = atoi(ssc+20);
            }
        }

        LOG_DEBUG("Stream: %s", http.cmdLine);
    }
}


// -----------------------------------
// 状況に応じて this->outputProtocol を設定する。
void Servent::handshakeStream_changeOutputProtocol(bool gotPCP, const ChanInfo& chanInfo)
{
    // WMV ならば MMS(MMSH)
    if (outputProtocol == ChanInfo::SP_HTTP)
    {
        if  ( (chanInfo.srcProtocol == ChanInfo::SP_MMS)
              || (chanInfo.contentType == ChanInfo::T_WMA)
              || (chanInfo.contentType == ChanInfo::T_WMV)
              || (chanInfo.contentType == ChanInfo::T_ASX)
            )
        outputProtocol = ChanInfo::SP_MMS;
    }
}

// -----------------------------------
// ストリームできる時の応答のヘッダー部分を送信する。
void Servent::handshakeStream_returnStreamHeaders(AtomStream& atom,
                                                  std::shared_ptr<Channel> ch,
                                                  const ChanInfo& chanInfo)
{
    if (chanInfo.contentType != ChanInfo::T_MP3)
        addMetadata = false;

    if (addMetadata && (outputProtocol == ChanInfo::SP_HTTP))       // winamp mp3 metadata check
    {
        sock->writeLine(ICY_OK);

        sock->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        sock->writeLineF("icy-name:%s", chanInfo.name.c_str());
        sock->writeLineF("icy-br:%d", chanInfo.bitrate);
        sock->writeLineF("icy-genre:%s", chanInfo.genre.c_str());
        sock->writeLineF("icy-url:%s", chanInfo.url.c_str());
        sock->writeLineF("icy-metaint:%d", chanMgr->icyMetaInterval);
        sock->writeLineF("%s %s", PCX_HS_CHANNELID, chanInfo.id.str().c_str());

        sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_MP3);
    }else
    {
        sock->writeLine(HTTP_SC_OK);

        if ((chanInfo.contentType != ChanInfo::T_ASX) &&
            (chanInfo.contentType != ChanInfo::T_WMV) &&
            (chanInfo.contentType != ChanInfo::T_WMA))
        {
            sock->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);

            sock->writeLine("Accept-Ranges: none");

            sock->writeLineF("x-audiocast-name: %s", chanInfo.name.c_str());
            sock->writeLineF("x-audiocast-bitrate: %d", chanInfo.bitrate);
            sock->writeLineF("x-audiocast-genre: %s", chanInfo.genre.c_str());
            sock->writeLineF("x-audiocast-description: %s", chanInfo.desc.c_str());
            sock->writeLineF("x-audiocast-url: %s", chanInfo.url.c_str());
            sock->writeLineF("%s %s", PCX_HS_CHANNELID, chanInfo.id.str().c_str());
        }

        if (outputProtocol == ChanInfo::SP_HTTP)
        {
            if (chanInfo.contentType == ChanInfo::T_MOV)
            {
                sock->writeLine("Connection: close");
                sock->writeLine("Content-Length: 10000000");
            }
            sock->writeLine("Access-Control-Allow-Origin: *");
            sock->writeLineF("%s %s", HTTP_HS_CONTENT, chanInfo.getMIMEType());
        }else if (outputProtocol == ChanInfo::SP_MMS)
        {
            sock->writeLine("Server: Rex/9.0.0.2980");
            sock->writeLine("Cache-Control: no-cache");
            sock->writeLine("Pragma: no-cache");
            sock->writeLine("Pragma: client-id=3587303426");
            sock->writeLine("Pragma: features=\"broadcast, playlist\"");

            if (nsSwitchNum)
            {
                sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_MMS);
            }else
            {
                sock->writeLine("Content-Type: application/vnd.ms.wms-hdr.asfv1");
                if (ch)
                    sock->writeLineF("Content-Length: %d", ch->headPack.len);
                sock->writeLine("Connection: Keep-Alive");
            }
        }else if (outputProtocol == ChanInfo::SP_PCP)
        {
            sock->writeLineF("%s %d", PCX_HS_POS, streamPos);
            sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_XPCP);
        }
    }
    sock->writeLine("");
}

// -----------------------------------
// リレー要求に対してストリームできない時、ノード情報を送信する。
void Servent::handshakeStream_returnHits(AtomStream& atom,
                                         const GnuID& channelID,
                                         ChanHitList* chl,
                                         Host& rhost)
{
    ChanHitSearch chs;

    if (chl)
    {
        ChanHit best;

        // search for up to 8 other hits
        int cnt = 0;
        for (int i = 0; i < 8; i++)
        {
            best.init();

            // find best hit this network if local IP
            if (!rhost.globalIP())
            {
                chs.init();
                chs.matchHost = servMgr->serverHost;
                chs.waitDelay = 2;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            // find best hit on same network
            if (!best.host.ip)
            {
                chs.init();
                chs.matchHost = rhost;
                chs.waitDelay = 2;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            // find best hit on other networks
            if (!best.host.ip)
            {
                chs.init();
                chs.waitDelay = 2;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            if (!best.host.ip)
                break;

            best.writeAtoms(atom, channelID);
            cnt++;
        }

        if (cnt)
        {
            LOG_DEBUG("Sent %d channel hit(s) to %s", cnt, rhost.str().c_str());
        }else if (rhost.port)
        {
            // find firewalled host
            chs.init();
            chs.waitDelay = 30;
            chs.useFirewalled = true;
            chs.excludeID = remoteID;
            if (chl->pickHits(chs))
            {
                best = chs.best[0];
                int cnt = servMgr->broadcastPushRequest(best, rhost, chl->info.id, Servent::T_RELAY);
                LOG_DEBUG("Broadcasted channel push request to %d clients for %s", cnt, rhost.str().c_str());
            }
        }

        // if all else fails, use tracker
        if (!best.host.ip)
        {
            // find best tracker on this network if local IP
            if (!rhost.globalIP())
            {
                chs.init();
                chs.matchHost = servMgr->serverHost;
                chs.trackersOnly = true;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            // find local tracker
            if (!best.host.ip)
            {
                chs.init();
                chs.matchHost = rhost;
                chs.trackersOnly = true;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            // find global tracker
            if (!best.host.ip)
            {
                chs.init();
                chs.trackersOnly = true;
                chs.excludeID = remoteID;
                if (chl->pickHits(chs))
                    best = chs.best[0];
            }

            if (best.host.ip)
            {
                best.writeAtoms(atom, channelID);
                LOG_DEBUG("Sent 1 tracker hit to %s", rhost.str().c_str());
            }else if (rhost.port)
            {
                // find firewalled tracker
                chs.init();
                chs.useFirewalled = true;
                chs.trackersOnly = true;
                chs.excludeID = remoteID;
                chs.waitDelay = 30;
                if (chl->pickHits(chs))
                {
                    best = chs.best[0];
                    int cnt = servMgr->broadcastPushRequest(best, rhost, chl->info.id, Servent::T_CIN);
                    LOG_DEBUG("Broadcasted tracker push request to %d clients for %s", cnt, rhost.str().c_str());
                }
            }
        }
    }
    // return not available yet code
    const int error = PCP_ERROR_QUIT + PCP_ERROR_UNAVAILABLE;
    atom.writeInt(PCP_QUIT, error);
}

// -----------------------------------
bool Servent::handshakeStream_returnResponse(bool gotPCP,
                                             bool chanReady, // ストリーム可能である。
                                             std::shared_ptr<Channel> ch,
                                             ChanHitList* chl,
                                             const ChanInfo& chanInfo)
{
    Host rhost = sock->host;
    AtomStream atom(*sock);

    if (!chl)
    {
        sock->writeLine(HTTP_SC_NOTFOUND);
        sock->writeLine("");
        LOG_DEBUG("Sending channel not found");
        return false;
    }else if (!chanReady)       // cannot stream
    {
        if (outputProtocol == ChanInfo::SP_PCP)    // relay stream
        {
            sock->writeLine(HTTP_SC_UNAVAILABLE);
            sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_XPCP);
            sock->writeLine("");

            handshakeIncomingPCP(atom, rhost, remoteID, agent);

            LOG_DEBUG("Sending channel unavailable");

            handshakeStream_returnHits(atom, chanInfo.id, chl, rhost);
            return false;
        }else                                      // direct stream
        {
            LOG_DEBUG("Sending channel unavailable");
            sock->writeLine(HTTP_SC_UNAVAILABLE);
            sock->writeLine("");
            return false;
        }
    }else
    {
        handshakeStream_returnStreamHeaders(atom, ch, chanInfo);

        if (gotPCP)
        {
            handshakeIncomingPCP(atom, rhost, remoteID, agent);
            atom.writeInt(PCP_OK, 0);
        }
        return true;
    }
}

// -----------------------------------
// リレー自動管理で切断の対象になるか。
bool Servent::isTerminationCandidate(ChanHit* hit)
{
    // リレー追加不可で現在リレーしていない。
    // あるいは、ポート0でGIVメソッドに対応してないクライアントである。
    return (!hit->relay && hit->numRelays == 0) ||
        (hit->host.port == 0 && !hit->canGiv());
}

// -----------------------------------
// "/stream/<channel ID>" あるいは "/channel/<channel ID>" エンドポイ
// ントへの要求に対する反応。
bool Servent::handshakeStream(ChanInfo &chanInfo)
{
    bool gotPCP = false;
    unsigned int reqPos = 0;
    nsSwitchNum = 0;

    handshakeStream_readHeaders(gotPCP, reqPos, nsSwitchNum);
    handshakeStream_changeOutputProtocol(gotPCP, chanInfo);

    bool chanReady = false;

    auto ch = chanMgr->findChannelByID(chanInfo.id);
    if (ch)
    {
        sendHeader = true;
        if (reqPos)
        {
            streamPos = ch->rawData.findOldestPos(reqPos);
        }else
        {
            streamPos = ch->rawData.getLatestPos();
        }

        // 自動リレー管理。
        bool autoManageTried = false;
        do
        {
            StreamRequestDenialReason reason = StreamRequestDenialReason::None;
            {
                static std::mutex streamRequestMutex;
                std::lock_guard<std::mutex> lock(streamRequestMutex);

                chanReady = canStream(ch, &reason);
                if (chanReady)
                {
                    // リソース制約の計算のために、この時点で該当チャ
                    // ンネルをリレー・視聴している servent であると設
                    // 定する。
                    chanID = chanInfo.id;
                    setStatus(S_CONNECTED);
                }
            }
            LOG_DEBUG("chanReady = %d; reason = %s", chanReady, denialReasonToName(reason));

            if (chanReady)
            {
                break;
            }else if (!autoManageTried &&
                      type == T_RELAY &&
                      (reason == StreamRequestDenialReason::InsufficientBandwidth ||
                       reason == StreamRequestDenialReason::RelayLimit ||
                       reason == StreamRequestDenialReason::PerChannelRelayLimit))
            {
                autoManageTried = true;
                LOG_DEBUG("Auto-manage relays");
                auto chl = chanMgr->findHitList(chanInfo);
                if (!chl)
                    break;

                auto chanHitFor =
                    [=](Servent* t)
                    -> std::shared_ptr<ChanHit>
                    {
                        for (auto h = chl->hit; h != nullptr; h = h->next)
                        {
                            if (h->sessionID.isSame(t->remoteID))
                                return h;
                        }
                        return nullptr;
                    };

                // リレー自動管理。
                std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
                for (Servent* s = servMgr->servents; s != nullptr; s = s->next)
                {
                    if (s == this) continue;

                    auto hit = chanHitFor(s);

                    if (s->type == Servent::T_RELAY &&
                        s->chanID.isSame(chanInfo.id) &&
                        !s->isPrivate() &&
                        hit &&
                        Servent::isTerminationCandidate(hit.get()))
                    {
                        LOG_INFO("Terminating relay connection to %s (color=%s)",
                                 s->getHost().str().c_str(),
                                 ChanHit::colorToName(hit->getColor()));
                        s->abort();
                        // 切断されるのを待つ。
                        sys->sleep(200);
                        break;
                    }
                }
            }else {
                break;
            }
        }while (thread.active() && sock->active());
    }

    auto chl = chanMgr->findHitList(chanInfo);

    return handshakeStream_returnResponse(gotPCP, chanReady, ch, chl.get(), chanInfo);
}

// -----------------------------------
// GIV しにいく
void Servent::handshakeGiv(const GnuID &id)
{
    if (id.isSet())
    {
        char idstr[64];
        id.toStr(idstr);
        sock->writeLineF("GIV /%s", idstr);
    }else
        sock->writeLine("GIV");

    sock->writeLine("");
}

// ------------------------------------------------------------------
// Pushリレーサーバントのメインプロシージャ。こちらからリモートホスト
// へ接続に行くが、その後は着信したかのように振る舞い、要求を受け付け
// る。
int Servent::givProc(ThreadInfo *thread)
{
    sys->setThreadName("Servent GIV");

    Servent *sv = (Servent*)thread->data;
    Defer cb([sv]() { sv->kill(); });

    try
    {
        sv->handshakeGiv(sv->givID);
        sv->handshakeIncoming();
    }catch (StreamException &e)
    {
        LOG_ERROR("GIV: %s", e.msg);
    }

    return 0;
}

// -----------------------------------
void Servent::writeHeloAtom(AtomStream &atom, bool sendPort, bool sendPing, bool sendBCID, const GnuID& sessionID, uint16_t port, const GnuID& broadcastID)
{
     atom.writeParent(PCP_HELO, 3 + (sendPort?1:0) + (sendPing?1:0) + (sendBCID?1:0));
         atom.writeString(PCP_HELO_AGENT, PCX_AGENT);
         atom.writeInt(PCP_HELO_VERSION, PCP_CLIENT_VERSION);
         atom.writeBytes(PCP_HELO_SESSIONID, sessionID.id, 16);
         if (sendPort)
              atom.writeShort(PCP_HELO_PORT, port);
         if (sendPing)
              atom.writeShort(PCP_HELO_PING, port);
         if (sendBCID)
              atom.writeBytes(PCP_HELO_BCID, broadcastID.id, 16);
}

// -----------------------------------
void Servent::handshakeOutgoingPCP(AtomStream &atom, const Host &rhost, /*out*/ GnuID &rid, /*out*/ String &agent, bool isTrusted)
{
    int ipv = rhost.ip.isIPv4Mapped() ? 4 : 6;
    if (servMgr->flags.get("sendPortAtomWhenFirewallUnknown"))
    {
        bool sendPort = (servMgr->getFirewall(ipv) != ServMgr::FW_ON);
        bool testFW   = (servMgr->getFirewall(ipv) == ServMgr::FW_UNKNOWN);
        bool sendBCID = isTrusted && chanMgr->isBroadcasting();

        writeHeloAtom(atom, sendPort, testFW, sendBCID, servMgr->sessionID, servMgr->serverHost.port, chanMgr->broadcastID);
    }else
    {
        bool sendPort = (servMgr->getFirewall(ipv) == ServMgr::FW_OFF);
        bool testFW   = (servMgr->getFirewall(ipv) == ServMgr::FW_UNKNOWN);
        bool sendBCID = isTrusted && chanMgr->isBroadcasting();

        writeHeloAtom(atom, sendPort, testFW, sendBCID, servMgr->sessionID, servMgr->serverHost.port, chanMgr->broadcastID);
    }

    LOG_DEBUG("PCP outgoing waiting for OLEH..");

    int numc, numd;
    ID4 id = atom.read(numc, numd);
    if (id != PCP_OLEH)
    {
        LOG_DEBUG("PCP outgoing reply: %s", id.getString().str());
        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT + PCP_ERROR_BADRESPONSE);
        throw StreamException("Got unexpected PCP response");
    }

    char arg[64];

    GnuID clientID;
    rid.clear();
    int version = 0;
    int disable = 0;

    Host thisHost;

    // read OLEH response
    for (int i = 0; i < numc; i++)
    {
        int c, dlen;
        ID4 id = atom.read(c, dlen);

        if (id == PCP_HELO_AGENT)
        {
            atom.readString(arg, sizeof(arg), dlen);
            agent.set(arg);
        }else if (id == PCP_HELO_REMOTEIP)
        {
            thisHost.ip = atom.readAddress();
        }else if (id == PCP_HELO_PORT)
        {
            thisHost.port = atom.readShort();
        }else if (id == PCP_HELO_VERSION)
        {
            version = atom.readInt();
        }else if (id == PCP_HELO_DISABLE)
        {
            disable = atom.readInt();
        }else if (id == PCP_HELO_SESSIONID)
        {
            atom.readBytes(rid.id, 16);
            if (rid.isSame(servMgr->sessionID))
                throw StreamException("Servent loopback");
        }else
        {
            LOG_DEBUG("PCP handshake skip: %s", id.getString().str());
            atom.skip(c, dlen);
        }
    }

    // update server ip/firewall status
    if (isTrusted)
    {
        if (thisHost.isValid())
        {
            if ((servMgr->serverHost.ip != thisHost.ip) && (servMgr->forceIP.isEmpty()))
            {
                LOG_DEBUG("Got new ip: %s", thisHost.str().c_str());
                servMgr->updateIPAddress(thisHost.ip);
            }

            if (servMgr->getFirewall(ipv) == ServMgr::FW_UNKNOWN)
            {
                if (thisHost.port && thisHost.globalIP())
                    servMgr->setFirewall(ipv, ServMgr::FW_OFF);
                else
                    servMgr->setFirewall(ipv, ServMgr::FW_ON);
            }
        }

        if (disable == 1)
        {
            LOG_ERROR("client disabled: %d", disable);
            servMgr->isDisabled = true;
        }else
        {
            servMgr->isDisabled = false;
        }
    }

    if (!rid.isSet())
    {
        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT + PCP_ERROR_NOTIDENTIFIED);
        throw StreamException("Remote host not identified");
    }

    LOG_DEBUG("PCP Outgoing handshake complete.");
}

// -------------------------------------------------------------------
Servent::SupportStatus Servent::continuationPacketSupportStatus(const std::string& agent)
{
    // PeerCastStation
    {
        static std::vector<int> minimumVersion({ 2, 8, 0 });
        static Regexp reg("^PeerCastStation/([0-9.]+)$");
        auto m = reg.exec(agent);

        if (m.size() > 0)
        {
            auto strvec = str::split(m.at(1), ".");
            std::vector<int> ver;
            for (auto& str : strvec)
                ver.push_back(std::atoi(str.c_str()));

            if (ver >= minimumVersion)
                return SupportStatus::Supported;
            else
                return SupportStatus::Unsupported;
        }
    }

    // YT
    {
        static Regexp reg("^PeerCast/0.1218 \\(YT(\\d+)\\)$");
        auto m = reg.exec(agent);
        if (m.size() > 0)
        {
            if (std::atoi(m.at(1).c_str()) < 15)
                return SupportStatus::Unsupported;
            else
                return SupportStatus::Supported;
        }
    }

    // IM
    {
        static Regexp reg("^PeerCast/0.1218\\(IM(\\d+)\\)$"); // "PeerCast/0.1218(IM0051)" etc.

        auto m = reg.exec(agent);
        if (m.size() > 0)
        {
            if (std::atoi(m.at(1).c_str()) <= 51)
                return SupportStatus::Unsupported;
            else
                return SupportStatus::Unknown;
        }
    }

    // VP
    {
        static Regexp reg("^PeerCast/0.1218\\(VP(\\d+)\\)$"); // "PeerCast/0.1218(VP0027)" etc.

        auto m = reg.exec(agent);
        if (m.size() > 0)
        {
            if (std::atoi(m.at(1).c_str()) <= 27)
                return SupportStatus::Unsupported;
            else
                return SupportStatus::Unknown;
        }
    }

    // PP?
    {
        if (agent == "PeerCast/0.1218-J")
            return SupportStatus::Unsupported;
    }

    // 無印(!)
    if (agent == "PeerCast/0.1218")
        return SupportStatus::Unsupported;

    return SupportStatus::Unknown;
}

// -------------------------------------------------------------------
// PCPハンドシェイク。HELO, OLEH。グローバルIP、ファイアウォールチェッ
// ク。
void Servent::handshakeIncomingPCP(AtomStream &atom, Host &rhost, GnuID &rid, String &agent)
{
    int numc, numd;
    ID4 id = atom.read(numc, numd);

    if (id != PCP_HELO)
    {
        LOG_DEBUG("PCP incoming reply: %s", id.getString().str());
        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_BADRESPONSE);
        throw StreamException("Got unexpected PCP response");
    }

    char arg[64];

    ID4 osType;

    int version=0;

    int pingPort=0;

    GnuID bcID;
    GnuID clientID;

    rhost.port = 0;

    for (int i=0; i<numc; i++)
    {
        int c, dlen;
        ID4 id = atom.read(c, dlen);

        if (id == PCP_HELO_AGENT)
        {
            atom.readString(arg, sizeof(arg), dlen);
            agent.set(arg);
        }else if (id == PCP_HELO_VERSION)
        {
            version = atom.readInt();
        }else if (id == PCP_HELO_SESSIONID)
        {
            atom.readBytes(rid.id, 16);
            if (rid.isSame(servMgr->sessionID))
                throw StreamException("Servent loopback");
        }else if (id == PCP_HELO_BCID)
        {
            atom.readBytes(bcID.id, 16);
        }else if (id == PCP_HELO_OSTYPE)
        {
            osType = atom.readInt();
        }else if (id == PCP_HELO_PORT)
        {
            rhost.port = atom.readShort();
        }else if (id == PCP_HELO_PING)
        {
            pingPort = atom.readShort();
        }else
        {
            LOG_DEBUG("PCP handshake skip: %s", id.getString().str());
            atom.skip(c, dlen);
        }
    }

    if (version)
        LOG_DEBUG("Incoming PCP is %s : v%d", agent.cstr(), version);

    if (!rhost.globalIP() && servMgr->serverHost.globalIP())
        rhost.ip = servMgr->serverHost.ip;

    if (pingPort)
    {
        LOG_DEBUG("Incoming firewalled test request: %s ", rhost.str().c_str());
        rhost.port = pingPort;
        if (!rhost.globalIP() || !pingHost(rhost, rid))
            rhost.port = 0;
    }

    atom.writeParent(PCP_OLEH, 5);
        atom.writeString(PCP_HELO_AGENT, PCX_AGENT);
        atom.writeBytes(PCP_HELO_SESSIONID, servMgr->sessionID.id, 16);
        atom.writeInt(PCP_HELO_VERSION, PCP_CLIENT_VERSION);
        atom.writeAddress(PCP_HELO_REMOTEIP, rhost.ip);
        atom.writeShort(PCP_HELO_PORT, rhost.port);

    if (version)
    {
        if (version < PCP_CLIENT_MINVERSION)
        {
            atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_BADAGENT);
            throw StreamException("Agent is not valid");
        }
    }

    if (servMgr->flags.get("requireContinuationPacketSupportFromPeer"))
    {
        SupportStatus status = continuationPacketSupportStatus(agent);
        if (status == SupportStatus::Unsupported)
        {
            LOG_DEBUG("requireContinuationPacketSupportFromPeer: Denied %s %s", agent.cstr(), rhost.str().c_str());
            atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_BADAGENT);
            throw StreamException("Agent is not valid (continuation packets unsupported)");
        }else if (status == SupportStatus::Unknown)
        {
            LOG_WARN("requireContinuationPacketSupportFromPeer: Allowing agent of unknown status %s", str::inspect(agent).c_str());
        }else if (status == SupportStatus::Supported)
        {
            LOG_DEBUG("requireContinuationPacketSupportFromPeer: Allowed %s %s", agent.cstr(), rhost.str().c_str());
        }
    }

    if (!rid.isSet())
    {
        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT+PCP_ERROR_NOTIDENTIFIED);
        throw StreamException("Remote host not identified");
    }

    if (servMgr->isRoot)
    {
        servMgr->writeRootAtoms(atom, false);
    }

    LOG_DEBUG("PCP Incoming handshake complete.");
}

// ------------------------------------------------------------------
// コントロール・インの処理。通信の状態は、"pcp\n" を読み込んだ後。
// suggestOthers は常に true が渡される。
void Servent::processIncomingPCP(bool suggestOthers)
{
    PCPStream::readVersion(*sock);

    AtomStream atom(*sock);
    Host rhost = sock->host;

    handshakeIncomingPCP(atom, rhost, remoteID, agent);

    bool alreadyConnected = (servMgr->findConnection(Servent::T_COUT, remoteID)!=nullptr) ||
                            (servMgr->findConnection(Servent::T_CIN,  remoteID)!=nullptr);
    bool unavailable      = servMgr->controlInFull();
    bool offair           = !servMgr->isRoot && !chanMgr->isBroadcasting();

    char rstr[64];
    strcpy(rstr, rhost.str().c_str());

    // 接続を断わる場合の処理。コントロール接続数が上限に達しているか、
    // リモートホストとのコントロール接続が既にあるか、自分は放送中の
    // トラッカーではない。
    if (unavailable || alreadyConnected || offair)
    {
        int error;

        if (alreadyConnected)
            error = PCP_ERROR_QUIT+PCP_ERROR_ALREADYCONNECTED;
        else if (unavailable)
            error = PCP_ERROR_QUIT+PCP_ERROR_UNAVAILABLE;
        else if (offair)
            error = PCP_ERROR_QUIT+PCP_ERROR_OFFAIR;
        else
            error = PCP_ERROR_QUIT;

        if (suggestOthers)
        {
            ChanHit best;
            ChanHitSearch chs;

            int cnt=0;
            for (int i=0; i<8; i++)
            {
                best.init();

                // find best hit on this network
                if (!rhost.globalIP())
                {
                    chs.init();
                    chs.matchHost = servMgr->serverHost;
                    chs.waitDelay = 2;
                    chs.excludeID = remoteID;
                    chs.trackersOnly = true;
                    chs.useBusyControls = false;
                    if (chanMgr->pickHits(chs))
                        best = chs.best[0];
                }

                // find best hit on same network
                if (!best.host.ip)
                {
                    chs.init();
                    chs.matchHost = rhost;
                    chs.waitDelay = 2;
                    chs.excludeID = remoteID;
                    chs.trackersOnly = true;
                    chs.useBusyControls = false;
                    if (chanMgr->pickHits(chs))
                        best = chs.best[0];
                }

                // else find best hit on other networks
                if (!best.host.ip)
                {
                    chs.init();
                    chs.waitDelay = 2;
                    chs.excludeID = remoteID;
                    chs.trackersOnly = true;
                    chs.useBusyControls = false;
                    if (chanMgr->pickHits(chs))
                        best = chs.best[0];
                }

                if (!best.host.ip)
                    break;

                best.writeAtoms(atom, GnuID());
                cnt++;
            }

            if (cnt)
            {
                LOG_DEBUG("Sent %d tracker(s) to %s", cnt, rstr);
            }else if (rhost.port)
            {
                // send push request to best firewalled tracker on other network
                chs.init();
                chs.waitDelay = 30;
                chs.excludeID = remoteID;
                chs.trackersOnly = true;
                chs.useFirewalled = true;
                chs.useBusyControls = false;
                if (chanMgr->pickHits(chs))
                {
                    best = chs.best[0];
                    int cnt = servMgr->broadcastPushRequest(best, rhost, GnuID(), Servent::T_CIN);
                    LOG_DEBUG("Broadcasted tracker push request to %d clients for %s", cnt, rstr);
                }
            }else
            {
                LOG_DEBUG("No available trackers");
            }
        }

        LOG_ERROR("Sending QUIT to incoming: %d", error);

        atom.writeInt(PCP_QUIT, error);
        return;
    }

    type = T_CIN;
    setStatus(S_CONNECTED);

    atom.writeInt(PCP_OK, 0);

    // ask for update
    atom.writeParent(PCP_ROOT, 1);
        atom.writeParent(PCP_ROOT_UPDATE, 0);

    pcpStream = new PCPStream(remoteID);

    int error = 0;
    BroadcastState bcs;
    while (!error && thread.active() && !sock->eof())
    {
        error = pcpStream->readPacket(*sock, bcs);
        sys->sleepIdle();

        if (!servMgr->isRoot && !chanMgr->isBroadcasting())
            error = PCP_ERROR_OFFAIR;
        if (peercastInst->isQuitting)
            error = PCP_ERROR_SHUTDOWN;
    }

    pcpStream->flush(*sock);

    error += PCP_ERROR_QUIT;
    atom.writeInt(PCP_QUIT, error);

    LOG_DEBUG("PCP Incoming to %s closed: %d", rstr, error);
}

// -----------------------------------
int Servent::outgoingProc(ThreadInfo *thread)
{
    sys->setThreadName("COUT");

    LOG_DEBUG("COUT started");

    Servent *sv = (Servent*)thread->data;
    Defer cb([sv]() { sv->kill(); });

    sv->pcpStream = new PCPStream(GnuID());

    while (sv->thread.active())
    {
        sv->setStatus(S_WAIT);

        if (chanMgr->isBroadcasting() && servMgr->autoServe)
        {
            ChanHit bestHit;
            ChanHitSearch chs;

            do
            {
                bestHit.init();

                if (servMgr->rootHost.isEmpty())
                    break;

                if (sv->pushSock)
                {
                    sv->sock = sv->pushSock;
                    sv->pushSock = nullptr;
                    bestHit.host = sv->sock->host;
                    break;
                }

                auto chl = chanMgr->findHitListByID(GnuID());
                if (chl)
                {
                    // find local tracker
                    chs.init();
                    chs.matchHost = servMgr->serverHost;
                    chs.waitDelay = MIN_TRACKER_RETRY;
                    chs.excludeID = servMgr->sessionID;
                    chs.trackersOnly = true;
                    if (!chl->pickHits(chs))
                    {
                        // else find global tracker
                        chs.init();
                        chs.waitDelay = MIN_TRACKER_RETRY;
                        chs.excludeID = servMgr->sessionID;
                        chs.trackersOnly = true;
                        chl->pickHits(chs);
                    }

                    if (chs.numResults)
                    {
                        bestHit = chs.best[0];
                    }
                }

                unsigned int ctime = sys->getTime();

                if ((!bestHit.host.ip) && ((ctime-chanMgr->lastYPConnect) > MIN_YP_RETRY))
                {
                    bestHit.host.fromStrName(servMgr->rootHost.cstr(), DEFAULT_PORT);
                    bestHit.yp = true;
                    chanMgr->lastYPConnect = ctime;
                }
                sys->sleepIdle();
            }while (!bestHit.host.ip && (sv->thread.active()));

            if (!bestHit.host.ip)       // give up
            {
                LOG_ERROR("COUT giving up");
                break;
            }

            const std::string ipStr = bestHit.host.str();

            int error=0;
            try
            {
                LOG_DEBUG("COUT to %s: Connecting..", ipStr.c_str());

                if (!sv->sock)
                {
                    sv->setStatus(S_CONNECTING);
                    sv->sock = sys->createSocket();
                    if (!sv->sock)
                        throw StreamException("Unable to create socket");
                    sv->sock->open(bestHit.host);
                    sv->sock->connect();
                }

                sv->sock->setReadTimeout(30000);
                AtomStream atom(*sv->sock);

                sv->setStatus(S_HANDSHAKE);

                Host rhost = sv->sock->host;
                atom.writeInt(PCP_CONNECT, 1);
                handshakeOutgoingPCP(atom, rhost, sv->remoteID, sv->agent, bestHit.yp);

                sv->setStatus(S_CONNECTED);

                LOG_DEBUG("COUT to %s: OK", ipStr.c_str());

                sv->pcpStream->init(sv->remoteID);

                BroadcastState bcs;
                error = 0;
                while (!error && sv->thread.active() && !sv->sock->eof() && servMgr->autoServe)
                {
                    error = sv->pcpStream->readPacket(*sv->sock, bcs);

                    sys->sleepIdle();

                    if (!chanMgr->isBroadcasting())
                        error = PCP_ERROR_OFFAIR;
                    if (peercastInst->isQuitting)
                        error = PCP_ERROR_SHUTDOWN;

                    if (sv->pcpStream->nextRootPacket)
                        if (sys->getTime() > (sv->pcpStream->nextRootPacket+30))
                            error = PCP_ERROR_NOROOT;
                }
                sv->setStatus(S_CLOSING);

                sv->pcpStream->flush(*sv->sock);

                error += PCP_ERROR_QUIT;
                atom.writeInt(PCP_QUIT, error);

                LOG_ERROR("COUT to %s closed: %d", ipStr.c_str(), error);
            }catch (TimeoutException &e)
            {
                LOG_ERROR("COUT to %s: timeout (%s)", ipStr.c_str(), e.msg);
                sv->setStatus(S_TIMEOUT);
            }catch (StreamException &e)
            {
                LOG_ERROR("COUT to %s: %s", ipStr.c_str(), e.msg);
                sv->setStatus(S_ERROR);
            }

            try
            {
                if (sv->sock)
                {
                    sv->sock->close();
                    sv->sock = nullptr;
                }
            }catch (StreamException &) {}

            // don`t discard this hit if we caused the disconnect (stopped broadcasting)
            if (error != (PCP_ERROR_QUIT+PCP_ERROR_OFFAIR))
                chanMgr->deadHit(bestHit);
        }

        sys->sleepIdle();
    }

    LOG_DEBUG("COUT ended");

    return 0;
}

// -------------------------------------------------------------
// SERVER サーバントから起動されたサーバントのメインプロシージャ
int Servent::incomingProc(ThreadInfo *thread)
{
    Servent *sv = (Servent*)thread->data;
    Defer cb([sv]() { sv->kill(); });

    std::string ipStr = sv->sock->host.str(true);
    sys->setThreadName(String::format("INCOMING %s", ipStr.c_str()));

    try
    {
        sv->handshakeIncoming();
    }catch (HTTPException &e)
    {
        try
        {
            sv->sock->writeLine(e.msg);
            if (e.code == 401)
                sv->sock->writeLine("WWW-Authenticate: Basic realm=\"PeerCast\"");

            auto content = str::STR(e.msg, "\n\n", e.additionalMessage);
            sv->sock->writeLineF("Content-Type: text/plain; charset=utf-8");
            sv->sock->writeLineF("Content-Length: %zu", content.size());
            sv->sock->writeLine("");
            sv->sock->writeString(content.c_str());
        }catch (StreamException &) {}

        LOG_ERROR("Incoming from %s: %s", ipStr.c_str(), e.msg);
    }catch (StreamException &e)
    {
        LOG_ERROR("Incoming from %s: %s", ipStr.c_str(), e.msg);
    }

    return 0;
}

// -----------------------------------
void Servent::processStream(ChanInfo &chanInfo)
{
    setStatus(S_HANDSHAKE);

    if (!handshakeStream(chanInfo))
        return;

    ASSERT(chanID.isSet());
    ASSERT(this->status == S_CONNECTED);

    if (chanInfo.id.isSet())
    {
        chanID = chanInfo.id;

        LOG_INFO("Sending channel: %s ", ChanInfo::getProtocolStr(outputProtocol));

        if (!waitForChannelHeader(chanInfo))
            throw StreamException("Channel not ready");

        servMgr->totalStreams++;

        auto ch = chanMgr->findChannelByID(chanID);
        if (!ch)
            throw StreamException("Channel not found");

        if (outputProtocol == ChanInfo::SP_HTTP)
        {
            if ((addMetadata) && (chanMgr->icyMetaInterval))
                sendRawMetaChannel(chanMgr->icyMetaInterval);
            else
                sendRawChannel(true, true);
        }else if (outputProtocol == ChanInfo::SP_MMS)
        {
            if (nsSwitchNum)
            {
                sendRawChannel(true, true);
            }else
            {
                sendRawChannel(true, false);
            }
        }else if (outputProtocol  == ChanInfo::SP_PCP)
        {
            sendPCPChannel();
        }
    }

    setStatus(S_CLOSING);
}

// -----------------------------------
bool Servent::waitForChannelHeader(ChanInfo &info)
{
    for (int i=0; i<30*10; i++)
    {
        auto ch = chanMgr->findChannelByID(info.id);
        if (!ch)
            return false;

        if (ch->isPlaying() && (ch->rawData.writePos>0))
            return true;

        if (!thread.active() || !sock->active())
            break;
        sys->sleep(100);
    }
    return false;
}

// -----------------------------------
void Servent::sendRawChannel(bool sendHead, bool sendData)
{
    WriteBufferedStream bsock(sock.get());

    try
    {
        sock->setWriteTimeout(DIRECT_WRITE_TIMEOUT*1000);

        auto ch = chanMgr->findChannelByID(chanID);
        if (!ch)
            throw StreamException("Channel not found");

        LOG_DEBUG("Starting Raw stream of %s at %d", ch->info.name.cstr(), streamPos);

        if (sendHead)
        {
            ch->headPack.writeRaw(bsock);
            streamPos = ch->headPack.pos + ch->headPack.len;
            auto ncpos = ch->rawData.getLatestNonContinuationPos();
            if (ncpos && streamPos < ncpos)
                streamPos = ncpos;
            LOG_DEBUG("Sent %d bytes header ", ch->headPack.len);
        }

        if (sendData)
        {
            unsigned int streamIndex = ch->streamIndex;
            unsigned int connectTime = sys->getTime();
            unsigned int lastWriteTime = connectTime;
            bool         skipContinuation = servMgr->flags.get("startPlayingFromKeyFrame");

            while ((thread.active()) && sock->active())
            {
                ch = chanMgr->findChannelByID(chanID);
                if (!ch)
                {
                    throw StreamException("Channel not found");
                }

                if (streamIndex != ch->streamIndex)
                {
                    streamIndex = ch->streamIndex;
                    streamPos = ch->headPack.pos;
                    LOG_DEBUG("sendRaw got new stream index %u", streamIndex);
                }

                ChanPacket rawPack;
                while (ch->rawData.findPacket(streamPos, rawPack))
                {
                    if (syncPos != rawPack.sync)
                        LOG_ERROR("Send skip: %d", rawPack.sync-syncPos);
                    syncPos = rawPack.sync + 1;

                    if ((rawPack.type == ChanPacket::T_DATA) || (rawPack.type == ChanPacket::T_HEAD))
                    {
                        if (!skipContinuation || !rawPack.cont)
                        {
                            skipContinuation = false;
                            rawPack.writeRaw(bsock);
                            lastWriteTime = sys->getTime();
                        }else
                        {
                            LOG_DEBUG("raw: skip continuation %s packet pos=%d",
                                      (rawPack.type == ChanPacket::T_DATA) ? "DATA" : "HEAD",
                                      rawPack.pos);
                        }
                    }

                    if (rawPack.pos < streamPos)
                        LOG_DEBUG("raw: skip back %d", rawPack.pos - streamPos);
                    streamPos = rawPack.pos + rawPack.len;
                }

                if ((sys->getTime() - lastWriteTime) > DIRECT_WRITE_TIMEOUT)
                    throw TimeoutException();

                bsock.flush();
                sys->sleep(200);
                //sys->sleepIdle();
            }
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("Stream channel: %s", e.msg);
    }
}

// -----------------------------------
void Servent::sendRawMetaChannel(int interval)
{
    try
    {
        auto ch = chanMgr->findChannelByID(chanID);
        if (!ch)
            throw StreamException("Channel not found");

        sock->setWriteTimeout(DIRECT_WRITE_TIMEOUT*1000);

        LOG_DEBUG("Starting Raw Meta stream of %s (metaint: %d) at %d", ch->info.name.cstr(), interval, streamPos);

        String lastTitle, lastURL;

        int     lastMsgTime=sys->getTime();
        bool    showMsg=true;

        char buf[16384];
        int bufPos=0;

        if ((interval > (int) sizeof(buf)) || (interval < 1))
            throw StreamException("Bad ICY Meta Interval value");

        unsigned int connectTime = sys->getTime();
        unsigned int lastWriteTime = connectTime;

        streamPos = 0;      // raw meta channel has no header (its MP3)

        while ((thread.active()) && sock->active())
        {
            ch = chanMgr->findChannelByID(chanID);
            if (!ch)
            {
                throw StreamException("Channel not found");
            }

            ChanPacket rawPack;
            if (ch->rawData.findPacket(streamPos, rawPack))
            {
                if (syncPos != rawPack.sync)
                    LOG_ERROR("Send skip: %d", rawPack.sync-syncPos);
                syncPos = rawPack.sync+1;

                MemoryStream mem(rawPack.data, rawPack.len);

                if (rawPack.type == ChanPacket::T_DATA)
                {
                    int len = rawPack.len;
                    char *p = rawPack.data;
                    while (len)
                    {
                        int rl = len;
                        if ((bufPos+rl) > interval)
                            rl = interval-bufPos;
                        memcpy(&buf[bufPos], p, rl);
                        bufPos+=rl;
                        p+=rl;
                        len-=rl;

                        if (bufPos >= interval)
                        {
                            bufPos = 0;
                            sock->write(buf, interval);
                            lastWriteTime = sys->getTime();

                            if (chanMgr->broadcastMsgInterval)
                                if ((sys->getTime()-lastMsgTime) >= chanMgr->broadcastMsgInterval)
                                {
                                    showMsg ^= true;
                                    lastMsgTime = sys->getTime();
                                }

                            String *metaTitle = &ch->info.track.title;
                            if (!ch->info.comment.isEmpty() && (showMsg))
                                metaTitle = &ch->info.comment;

                            if (!metaTitle->isSame(lastTitle) || !ch->info.url.isSame(lastURL))
                            {
                                char tmp[1024];
                                String title, url;

                                title = *metaTitle;
                                url = ch->info.url;

                                title.convertTo(String::T_META);
                                url.convertTo(String::T_META);

                                sprintf(tmp, "StreamTitle='%s';StreamUrl='%s';", title.cstr(), url.cstr());
                                int len = ((strlen(tmp) + 15+1) / 16);
                                sock->writeChar(len);
                                sock->write(tmp, len*16);

                                lastTitle = *metaTitle;
                                lastURL = ch->info.url;

                                LOG_DEBUG("StreamTitle: %s, StreamURL: %s", lastTitle.cstr(), lastURL.cstr());
                            }else
                            {
                                sock->writeChar(0);
                            }
                        }
                    }
                }
                streamPos = rawPack.pos + rawPack.len;
            }

            if ((sys->getTime()-lastWriteTime) > DIRECT_WRITE_TIMEOUT)
                throw TimeoutException();

            sys->sleepIdle();
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("Stream channel: %s", e.msg);
    }
}

// -----------------------------------
void Servent::sendPCPChannel()
{
    auto ch = chanMgr->findChannelByID(chanID);
    if (!ch)
        throw StreamException("Channel not found");

    WriteBufferedStream bsock(sock.get());
    AtomStream atom(bsock);

    pcpStream = new PCPStream(remoteID);
    int error=0;

    try
    {
        LOG_DEBUG("Starting PCP stream of channel at %d", streamPos);

        atom.writeParent(PCP_CHAN, 3 + ((sendHeader)?1:0));
            atom.writeBytes(PCP_CHAN_ID, chanID.id, 16);
            ch->info.writeInfoAtoms(atom);
            ch->info.writeTrackAtoms(atom);
            if (sendHeader)
            {
                atom.writeParent(PCP_CHAN_PKT, 3);
                    atom.writeID4(PCP_CHAN_PKT_TYPE, PCP_CHAN_PKT_HEAD);
                    atom.writeInt(PCP_CHAN_PKT_POS, ch->headPack.pos);
                    atom.writeBytes(PCP_CHAN_PKT_DATA, ch->headPack.data, ch->headPack.len);

                streamPos = ch->headPack.pos+ch->headPack.len;
                LOG_DEBUG("Sent %d bytes header", ch->headPack.len);
            }

        unsigned int streamIndex = ch->streamIndex;

        while (thread.active())
        {
            auto ch = chanMgr->findChannelByID(chanID);

            if (!ch)
            {
                error = PCP_ERROR_QUIT + PCP_ERROR_OFFAIR;
                break;
            }

            if (streamIndex != ch->streamIndex)
            {
                streamIndex = ch->streamIndex;
                streamPos = ch->headPack.pos;
                LOG_DEBUG("sendPCPStream got new stream index %u", streamIndex);
            }

            ChanPacket rawPack;

            // FIXME: ストリームインデックスの変更を確かめずにどんどん読み出して大丈夫？
            while (ch->rawData.findPacket(streamPos, rawPack))
            {
                if (rawPack.type == ChanPacket::T_HEAD)
                {
                    atom.writeParent(PCP_CHAN, 2);
                        atom.writeBytes(PCP_CHAN_ID, chanID.id, 16);
                        atom.writeParent(PCP_CHAN_PKT, 3);
                            atom.writeID4(PCP_CHAN_PKT_TYPE, PCP_CHAN_PKT_HEAD);
                            atom.writeInt(PCP_CHAN_PKT_POS, rawPack.pos);
                            atom.writeBytes(PCP_CHAN_PKT_DATA, rawPack.data, rawPack.len);
                }else if (rawPack.type == ChanPacket::T_DATA)
                {
                    if (rawPack.cont)
                    {
                        atom.writeParent(PCP_CHAN, 2);
                            atom.writeBytes(PCP_CHAN_ID, chanID.id, 16);
                            atom.writeParent(PCP_CHAN_PKT, 4);
                                atom.writeID4(PCP_CHAN_PKT_TYPE, PCP_CHAN_PKT_DATA);
                                atom.writeInt(PCP_CHAN_PKT_POS, rawPack.pos);
                                atom.writeChar(PCP_CHAN_PKT_CONTINUATION, true);
                                atom.writeBytes(PCP_CHAN_PKT_DATA, rawPack.data, rawPack.len);
                    }else
                    {
                        atom.writeParent(PCP_CHAN, 2);
                            atom.writeBytes(PCP_CHAN_ID, chanID.id, 16);
                            atom.writeParent(PCP_CHAN_PKT, 3);
                                atom.writeID4(PCP_CHAN_PKT_TYPE, PCP_CHAN_PKT_DATA);
                                atom.writeInt(PCP_CHAN_PKT_POS, rawPack.pos);
                                atom.writeBytes(PCP_CHAN_PKT_DATA, rawPack.data, rawPack.len);
                    }
                }

                if (rawPack.pos < streamPos)
                    LOG_DEBUG("pcp: skip back %d", rawPack.pos-streamPos);

                //LOG_DEBUG("Sending %d-%d (%d, %d, %d)", rawPack.pos, rawPack.pos+rawPack.len, ch->streamPos, ch->rawData.getLatestPos(), ch->rawData.getOldestPos());

                streamPos = rawPack.pos + rawPack.len;
            }
            bsock.flush();

            BroadcastState bcs;
            // どうしてここで bsock を使ったら動かないのか理解していない。
            error = pcpStream->readPacket(*sock, bcs);
            if (error)
                throw StreamException("PCP exception");

            sys->sleep(200);
            //sys->sleepIdle();
        }

        LOG_DEBUG("PCP channel stream closed normally.");
    }catch (StreamException &e)
    {
        LOG_ERROR("Stream channel: %s", e.msg);
    }

    try
    {
        atom.writeInt(PCP_QUIT, error);
    }catch (StreamException &) {}
}

// -----------------------------------------------------------------
// ソケットでの待ち受けを行う SERVER サーバントのメインプロシージャ。
int Servent::serverProc(ThreadInfo *thread)
{
    Servent *sv = (Servent*)thread->data;
    Defer cb([sv]() { sv->kill(); });

    try
    {
        if (!sv->sock)
            throw StreamException("Server has no socket");

        sys->setThreadName(String::format("LISTEN %hu", sv->sock->host.port));

        sv->setStatus(S_LISTENING);

        if (servMgr->isRoot)
            LOG_INFO("Root Server started on port %d", (int) sv->sock->host.port);
        else
            LOG_INFO("Server started on port %d", (int) sv->sock->host.port);

        while (thread->active() && sv->sock->active())
        {
            if (!sv->sock->readReady(100))
                continue;

            if (servMgr->numActiveOnPort(sv->sock->host.port) >= servMgr->maxServIn)
            {
                sys->sleep(100);
                continue;
            }

            auto cs = sv->sock->accept();
            if (!cs)
            {
                LOG_ERROR("accept failed");
                continue;
            }

            LOG_TRACE("accepted incoming");
            Servent *ns = servMgr->allocServent();
            if (!ns)
            {
                LOG_ERROR("Out of servents");
                cs->close();
                continue;
            }

            servMgr->lastIncoming = sys->getTime();
            ns->servPort = sv->sock->host.port;
            ns->networkID = servMgr->networkID;
            ns->initIncoming(cs, sv->allow);
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("Server Error: %s:%d", e.msg, e.err);
    }

    LOG_INFO("Server stopped: %s", sv->sock->host.str().c_str());

    return 0;
}

#include "sslclientsocket.h"
// -----------------------------------
amf0::Value    Servent::getState()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    bool ssl = static_cast<bool>( std::dynamic_pointer_cast<SslClientSocket>(sock) );

    return amf0::Value::object(
        {
            {"id", std::to_string(serventIndex)},
            {"type", getTypeStr()},
            {"status", getStatusStr()},
            {"address", getHost().ip.str()},
            {"endpoint", getHost().str()},
            {"agent", agent.c_str()},
            {"bitrate", str::format("%.1f", BYTES_TO_KBPS(sock ? sock->bytesInPerSec() + sock->bytesOutPerSec() : 0))},
            {"bitrateAvg", str::format("%.1f", BYTES_TO_KBPS(sock ? sock->stat.bytesInPerSecAvg() + sock->stat.bytesOutPerSecAvg() : 0))},
            {"uptime", (lastConnect) ? String().setFromStopwatch(sys->getTime() - lastConnect).c_str() : "-"},
            {"chanID", chanID.str()},
            {"remoteID", remoteID.str()},
            {"isPrivate", std::to_string(isPrivate())},
            {"ssl", ssl},
        });
}

