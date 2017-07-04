// ------------------------------------------------
// File : servmgr.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Management class for handling multiple servent connections.
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

#include <stdlib.h>
#include "servent.h"
#include "servmgr.h"
#include "inifile.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "atom.h"
#include "version2.h"

ThreadInfo ServMgr::serverThread, ServMgr::idleThread;

// -----------------------------------
ServMgr::ServMgr()
    : publicDirectoryEnabled(false)
{
    validBCID = NULL;

    authType = AUTH_COOKIE;
    cookieList.init();

    serventNum = 0;

    startTime = sys->getTime();

    allowServer1 = Servent::ALLOW_ALL;
    allowServer2 = Servent::ALLOW_BROADCAST;

    clearHostCache(ServHost::T_NONE);
    password[0]=0;

    allowGnutella = false;
    useFlowControl = true;

    maxServIn = 50;
    minGnuIncoming = 10;
    maxGnuIncoming = 20;

    lastIncoming = 0;

    maxBitrateOut = 0;
    maxRelays = MIN_RELAYS;
    maxDirect = 0;
    refreshHTML = 5;

    networkID.clear();

    notifyMask = 0xffff;

    tryoutDelay = 10;

    sessionID.generate();

    isDisabled = false;
    isRoot = false;

    forceIP.clear();

    strcpy(connectHost, "connect1.peercast.org");
    strcpy(htmlPath, "html/en");

    rootHost = "bayonet.ddo.jp:7146";

    serverHost.fromStrIP("127.0.0.1", DEFAULT_PORT);

    firewalled = FW_UNKNOWN;
    allowDirect = true;
    autoConnect = true;
    forceLookup = true;
    autoServe = true;
    forceNormal = false;

    maxControl = 3;

    queryTTL = 7;

    totalStreams = 0;
    firewallTimeout = 30;
    pauseLog = false;
    showLog = (1<<LogBuffer::T_ERROR);

    shutdownTimer = 0;

    downloadURL[0] = 0;
    rootMsg.clear();

    restartServer=false;

    setFilterDefaults();

    servents = NULL;

    chanLog="";

    serverName = "";
}

// -----------------------------------
ServMgr::~ServMgr()
{
    while (servents)
    {
        auto next = servents->next;
        delete servents;
        servents = next;
    }
}

// -----------------------------------
BCID *ServMgr::findValidBCID(int index)
{
    int cnt = 0;
    BCID *bcid = validBCID;
    while (bcid)
    {
        if (cnt == index)
            return bcid;
        cnt++;
        bcid=bcid->next;
    }
    return 0;
}

// -----------------------------------
BCID *ServMgr::findValidBCID(GnuID &id)
{
    BCID *bcid = validBCID;
    while (bcid)
    {
        if (bcid->id.isSame(id))
            return bcid;
        bcid=bcid->next;
    }
    return 0;
}

// -----------------------------------
void ServMgr::removeValidBCID(GnuID &id)
{
    BCID *bcid = validBCID, *prev=0;
    while (bcid)
    {
        if (bcid->id.isSame(id))
        {
            if (prev)
                prev->next = bcid->next;
            else
                validBCID = bcid->next;
            return;
        }
        prev = bcid;
        bcid=bcid->next;
    }
}

// -----------------------------------
void ServMgr::addValidBCID(BCID *bcid)
{
    removeValidBCID(bcid->id);

    bcid->next = validBCID;
    validBCID = bcid;
}

// -----------------------------------
void    ServMgr::connectBroadcaster()
{
    if (!rootHost.isEmpty())
    {
        if (!numUsed(Servent::T_COUT))
        {
            Servent *sv = allocServent();
            if (sv)
            {
                sv->initOutgoing(Servent::T_COUT);
                sys->sleep(3000);
            }
        }
    }
}

// -----------------------------------
void ServMgr::setFilterDefaults()
{
    numFilters = 0;

    filters[numFilters].host.fromStrIP("255.255.255.255", 0);
    filters[numFilters].flags = ServFilter::F_NETWORK|ServFilter::F_DIRECT;
    numFilters++;
}

// -----------------------------------
void    ServMgr::setPassiveSearch(unsigned int t)
{
//  if ((t > 0) && (t < 60))
//      t = 60;
//  passiveSearch = t;
}

// -----------------------------------
bool ServMgr::seenHost(Host &h, ServHost::TYPE type, unsigned int time)
{
    time = sys->getTime()-time;

    for (int i=0; i<MAX_HOSTCACHE; i++)
        if (hostCache[i].type == type)
            if (hostCache[i].host.ip == h.ip)
                if (hostCache[i].time >= time)
                    return true;
    return false;
}

// -----------------------------------
void ServMgr::addHost(Host &h, ServHost::TYPE type, unsigned int time)
{
    if (!h.isValid())
        return;

    ServHost *sh=NULL;

    for (int i=0; i<MAX_HOSTCACHE; i++)
        if (hostCache[i].type == type)
            if (hostCache[i].host.isSame(h))
            {
                sh = &hostCache[i];
                break;
            }

    char str[64];
    h.toStr(str);

    if (!sh)
        LOG_DEBUG("New host: %s - %s", str, ServHost::getTypeStr(type));
    else
        LOG_DEBUG("Old host: %s - %s", str, ServHost::getTypeStr(type));

    if (!sh)
    {
        // find empty slot
        for (int i=0; i<MAX_HOSTCACHE; i++)
            if (hostCache[i].type == ServHost::T_NONE)
            {
                sh = &hostCache[i];
                break;
            }

        // otherwise, find oldest host and replace
        if (!sh)
            for (int i=0; i<MAX_HOSTCACHE; i++)
                if (hostCache[i].type != ServHost::T_NONE)
                {
                    if (sh)
                    {
                        if (hostCache[i].time < sh->time)
                            sh = &hostCache[i];
                    }else{
                        sh = &hostCache[i];
                    }
                }
    }

    if (sh)
        sh->init(h, type, time);
}

// -----------------------------------
void ServMgr::deadHost(Host &h, ServHost::TYPE t)
{
    for (int i=0; i<MAX_HOSTCACHE; i++)
        if (hostCache[i].type == t)
            if (hostCache[i].host.ip == h.ip)
                if (hostCache[i].host.port == h.port)
                    hostCache[i].init();
}

// -----------------------------------
void ServMgr::clearHostCache(ServHost::TYPE type)
{
    for (int i=0; i<MAX_HOSTCACHE; i++)
        if ((hostCache[i].type == type) || (type == ServHost::T_NONE))
            hostCache[i].init();
}

// -----------------------------------
unsigned int ServMgr::numHosts(ServHost::TYPE type)
{
    unsigned int cnt = 0;
    for (int i=0; i<MAX_HOSTCACHE; i++)
        if ((hostCache[i].type == type) || (type == ServHost::T_NONE))
            cnt++;
    return cnt;
}

// -----------------------------------
int ServMgr::getNewestServents(Host *hl, int max, Host &rh)
{
    int cnt=0;
    for (int i=0; i<max; i++)
    {
        // find newest host not in list
        ServHost *sh=NULL;
        for (int j=0; j<MAX_HOSTCACHE; j++)
        {
            // find newest servent
            if (hostCache[j].type == ServHost::T_SERVENT)
                if (!(rh.globalIP() && !hostCache[j].host.globalIP()))
                {
                    // and not in list already
                    bool found=false;
                    for (int k=0; k<cnt; k++)
                        if (hl[k].isSame(hostCache[j].host))
                        {
                            found=true;
                            break;
                        }

                    if (!found)
                    {
                        if (!sh)
                        {
                            sh = &hostCache[j];
                        }else{
                            if (hostCache[j].time > sh->time)
                                sh = &hostCache[j];
                        }
                    }
                }
        }

        // add to list
        if (sh)
            hl[cnt++]=sh->host;
    }

    return cnt;
}

// -----------------------------------
ServHost ServMgr::getOutgoingServent(GnuID &netid)
{
    ServHost host;

    Host lh(ClientSocket::getIP(NULL), 0);

    // find newest host not in list
    ServHost *sh=NULL;
    for (int j=0; j<MAX_HOSTCACHE; j++)
    {
        ServHost *hc=&hostCache[j];
        // find newest servent not already connected.
        if (hc->type == ServHost::T_SERVENT)
        {
            if (!((lh.globalIP() && !hc->host.globalIP()) || lh.isSame(hc->host)))
            {
#if 0
                if (!findServent(Servent::T_OUTGOING, hc->host, netid))
                {
                    if (!sh)
                    {
                        sh = hc;
                    }else{
                        if (hc->time > sh->time)
                            sh = hc;
                    }
                }
#endif
            }
        }
    }

    if (sh)
        host = *sh;

    return host;
}

// -----------------------------------
Servent *ServMgr::findOldestServent(Servent::TYPE type, bool priv)
{
    Servent *oldest=NULL;

    Servent *s = servents;
    while (s)
    {
        if (s->type == type)
            if (s->thread.active)
                if (s->isOlderThan(oldest))
                    if (s->isPrivate() == priv)
                        oldest = s;
        s=s->next;
    }
    return oldest;
}

// -----------------------------------
Servent *ServMgr::findServent(Servent::TYPE type, Host &host, GnuID &netid)
{
    lock.on();
    Servent *s = servents;
    while (s)
    {
        if (s->type == type)
        {
            Host h = s->getHost();
            if (h.isSame(host) && s->networkID.isSame(netid))
            {
                lock.off();
                return s;
            }
        }
        s=s->next;
    }
    lock.off();
    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServent(unsigned int ip, unsigned short port, GnuID &netid)
{
    lock.on();
    Servent *s = servents;
    while (s)
    {
        if (s->type != Servent::T_NONE)
        {
            Host h = s->getHost();
            if ((h.ip == ip) && (h.port == port) && (s->networkID.isSame(netid)))
            {
                lock.off();
                return s;
            }
        }
        s=s->next;
    }
    lock.off();
    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServent(Servent::TYPE t)
{
    Servent *s = servents;
    while (s)
    {
        if (s->type == t)
            return s;
        s=s->next;
    }
    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServentByIndex(int id)
{
    Servent *s = servents;
    int cnt=0;
    while (s)
    {
        if (cnt == id)
            return s;
        cnt++;
        s=s->next;
    }
    return NULL;
}

// -----------------------------------
Servent *ServMgr::allocServent()
{
    lock.on();

    Servent *s = servents;
    while (s)
    {
        if (s->status == Servent::S_FREE)
            break;
        s=s->next;
    }

    if (!s)
    {
        s = new Servent(++serventNum);
        s->next = servents;
        servents = s;

        LOG_DEBUG("allocated servent %d", serventNum);
    }else
        LOG_DEBUG("reused servent %d", s->serventIndex);

    s->reset();

    lock.off();

    return s;
}

// --------------------------------------------------
void    ServMgr::closeConnections(Servent::TYPE type)
{
    Servent *sv = servents;
    while (sv)
    {
        if (sv->isConnected())
            if (sv->type == type)
                sv->thread.active = false;
        sv=sv->next;
    }
}

// -----------------------------------
unsigned int ServMgr::numConnected(int type, bool priv, unsigned int uptime)
{
    unsigned int cnt=0;

    unsigned int ctime=sys->getTime();
    Servent *s = servents;
    while (s)
    {
        if (s->thread.active)
            if (s->isConnected())
                if (s->type == type)
                    if (s->isPrivate()==priv)
                        if ((ctime-s->lastConnect) >= uptime)
                            cnt++;

        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numConnected()
{
    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        if (s->thread.active)
            if (s->isConnected())
                cnt++;

        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numServents()
{
    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        cnt++;
        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numUsed(int type)
{
    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        if (s->type == type)
            cnt++;
        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numActiveOnPort(int port)
{
    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        if (s->thread.active && s->sock && (s->servPort == port))
            cnt++;
        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numActive(Servent::TYPE tp)
{
    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        if (s->thread.active && s->sock && (s->type == tp))
            cnt++;
        s=s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::totalOutput(bool all)
{
    unsigned int tot = 0;
    Servent *s = servents;
    while (s)
    {
        if (s->isConnected())
            if (all || !s->isPrivate())
                if (s->sock)
                    tot += s->sock->bytesOutPerSec();
        s=s->next;
    }

    return tot;
}

// -----------------------------------
unsigned int ServMgr::numOutgoing()
{
    int cnt=0;

    Servent *s = servents;
    while (s)
    {
//      if ((s->type == Servent::T_INCOMING) ||
//          (s->type == Servent::T_OUTGOING))
//          cnt++;
        s=s->next;
    }
    return cnt;
}

// -----------------------------------
bool ServMgr::seenPacket(GnuPacket &p)
{
    Servent *s = servents;
    while (s)
    {
        if (s->isConnected())
            if (s->seenIDs.contains(p.id))
                return true;
        s=s->next;
    }
    return false;
}

// -----------------------------------
void ServMgr::quit()
{
    LOG_DEBUG("ServMgr is quitting..");

    serverThread.shutdown();

    idleThread.shutdown();

    Servent *s = servents;
    while (s)
    {
        try
        {
            if (s->thread.active)
                s->thread.shutdown();
        }catch (StreamException &)
        {
        }
        s=s->next;
    }
}

// -----------------------------------
int ServMgr::broadcast(GnuPacket &pack, Servent *src)
{
    int cnt=0;
    if (pack.ttl)
    {
        Servent *s = servents;
        while (s)
        {
            if (s != src)
                if (s->isConnected())
                    if (s->type == Servent::T_PGNU)
                        if (!s->seenIDs.contains(pack.id))
                        {
                            if (src)
                                if (!src->networkID.isSame(s->networkID))
                                    continue;

                            if (s->outputPacket(pack, false))
                                cnt++;
                        }
            s=s->next;
        }
    }

    LOG_NETWORK("broadcast: %s (%d) to %d servents", GNU_FUNC_STR(pack.func), pack.ttl, cnt);

    return cnt;
}

// -----------------------------------
int ServMgr::route(GnuPacket &pack, GnuID &routeID, Servent *src)
{
    int cnt=0;
    if (pack.ttl)
    {
        Servent *s = servents;
        while (s)
        {
            if (s != src)
                if (s->isConnected())
                    if (s->type == Servent::T_PGNU)
                        if (!s->seenIDs.contains(pack.id))
                            if (s->seenIDs.contains(routeID))
                            {
                                if (src)
                                    if (!src->networkID.isSame(s->networkID))
                                        continue;

                                if (s->outputPacket(pack, true))
                                    cnt++;
                            }
            s=s->next;
        }
    }

    LOG_NETWORK("route: %s (%d) to %d servents", GNU_FUNC_STR(pack.func), pack.ttl, cnt);
    return cnt;
}

// -----------------------------------
bool ServMgr::checkForceIP()
{
    if (!forceIP.isEmpty())
    {
        unsigned int newIP = ClientSocket::getIP(forceIP.cstr());
        if (serverHost.ip != newIP)
        {
            serverHost.ip = newIP;
            char ipstr[64];
            serverHost.IPtoStr(ipstr);
            LOG_DEBUG("Server IP changed to %s", ipstr);
            return true;
        }
    }
    return false;
}

// -----------------------------------
void ServMgr::checkFirewall()
{
    if ((getFirewall() == FW_UNKNOWN) && !servMgr->rootHost.isEmpty())
    {
        LOG_DEBUG("Checking firewall..");
        Host host;
        host.fromStrName(servMgr->rootHost.cstr(), DEFAULT_PORT);

        ClientSocket *sock = sys->createSocket();
        if (!sock)
            throw StreamException("Unable to create socket");
        sock->setReadTimeout(30000);
        sock->open(host);
        sock->connect();

        AtomStream atom(*sock);

        atom.writeInt(PCP_CONNECT, 1);

        GnuID remoteID;
        String agent;
        Servent::handshakeOutgoingPCP(atom, sock->host, remoteID, agent, true);

        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT);

        sock->close();
        delete sock;
    }
}

// -----------------------------------
void ServMgr::setFirewall(FW_STATE state)
{
    if (firewalled != state)
    {
        const char *str;
        switch (state)
        {
            case FW_ON:
                str = "ON";
                break;
            case FW_OFF:
                str = "OFF";
                break;
            case FW_UNKNOWN:
            default:
                str = "UNKNOWN";
                break;
        }

        LOG_DEBUG("Firewall is set to %s", str);
        firewalled = state;
    }
}

// -----------------------------------
bool ServMgr::isFiltered(int fl, Host &h)
{
    for (int i=0; i<numFilters; i++)
        if (filters[i].flags & fl)
            if (h.isMemberOf(filters[i].host))
                return true;

    return false;
}

#if 0
// -----------------------------------
bool ServMgr::canServeHost(Host &h)
{
    if (server)
    {
        Host sh = server->getHost();

        if (sh.globalIP() || (sh.localIP() && h.localIP()))
            return true;
    }
    return false;
}
#endif

// --------------------------------------------------
void writeServerSettings(IniFile &iniFile, unsigned int a)
{
    iniFile.writeBoolValue("allowHTML", a & Servent::ALLOW_HTML);
    iniFile.writeBoolValue("allowBroadcast", a & Servent::ALLOW_BROADCAST);
    iniFile.writeBoolValue("allowNetwork", a & Servent::ALLOW_NETWORK);
    iniFile.writeBoolValue("allowDirect", a & Servent::ALLOW_DIRECT);
}

// --------------------------------------------------
void writeFilterSettings(IniFile &iniFile, ServFilter &f)
{
    char ipstr[64];
    f.host.IPtoStr(ipstr);
    iniFile.writeStrValue("ip", ipstr);
    iniFile.writeBoolValue("private", f.flags & ServFilter::F_PRIVATE);
    iniFile.writeBoolValue("ban", f.flags & ServFilter::F_BAN);
    iniFile.writeBoolValue("network", f.flags & ServFilter::F_NETWORK);
    iniFile.writeBoolValue("direct", f.flags & ServFilter::F_DIRECT);
}

// --------------------------------------------------
static void  writeServHost(IniFile &iniFile, ServHost &sh)
{
    iniFile.writeSection("Host");

    char ipStr[64];
    sh.host.toStr(ipStr);
    iniFile.writeStrValue("type", ServHost::getTypeStr(sh.type));
    iniFile.writeStrValue("address", ipStr);
    iniFile.writeIntValue("time", sh.time);

    iniFile.writeLine("[End]");
}

// --------------------------------------------------
void ServMgr::saveSettings(const char *fn)
{
    IniFile iniFile;
    if (!iniFile.openWriteReplace(fn))
    {
        LOG_ERROR("Unable to open ini file");
    }else{
        LOG_DEBUG("Saving settings to: %s", fn);

        char idStr[64];

        iniFile.writeSection("Server");
        iniFile.writeStrValue("serverName", servMgr->serverName);
        iniFile.writeIntValue("serverPort", servMgr->serverHost.port);
        iniFile.writeBoolValue("autoServe", servMgr->autoServe);
        iniFile.writeStrValue("forceIP", servMgr->forceIP);
        iniFile.writeBoolValue("isRoot", servMgr->isRoot);
        iniFile.writeIntValue("maxBitrateOut", servMgr->maxBitrateOut);
        iniFile.writeIntValue("maxRelays", servMgr->maxRelays);
        iniFile.writeIntValue("maxDirect", servMgr->maxDirect);
        iniFile.writeIntValue("maxRelaysPerChannel", chanMgr->maxRelaysPerChannel);
        iniFile.writeIntValue("firewallTimeout", firewallTimeout);
        iniFile.writeBoolValue("forceNormal", forceNormal);
        iniFile.writeStrValue("rootMsg", rootMsg.cstr());
        iniFile.writeStrValue("authType", servMgr->authType==ServMgr::AUTH_COOKIE?"cookie":"http-basic");
        iniFile.writeStrValue("cookiesExpire", servMgr->cookieList.neverExpire==true?"never":"session");
        iniFile.writeStrValue("htmlPath", servMgr->htmlPath);
        iniFile.writeIntValue("minPGNUIncoming", servMgr->minGnuIncoming);
        iniFile.writeIntValue("maxPGNUIncoming", servMgr->maxGnuIncoming);
        iniFile.writeIntValue("maxServIn", servMgr->maxServIn);
        iniFile.writeStrValue("chanLog", servMgr->chanLog.cstr());
        iniFile.writeBoolValue("publicDirectory", servMgr->publicDirectoryEnabled);
        iniFile.writeStrValue("genrePrefix", servMgr->genrePrefix.c_str());

        networkID.toStr(idStr);
        iniFile.writeStrValue("networkID", idStr);

        iniFile.writeSection("Broadcast");
        iniFile.writeIntValue("broadcastMsgInterval", chanMgr->broadcastMsgInterval);
        iniFile.writeStrValue("broadcastMsg", chanMgr->broadcastMsg.cstr());
        iniFile.writeIntValue("icyMetaInterval", chanMgr->icyMetaInterval);
        chanMgr->broadcastID.toStr(idStr);
        iniFile.writeStrValue("broadcastID", idStr);
        iniFile.writeIntValue("hostUpdateInterval", chanMgr->hostUpdateInterval);
        iniFile.writeIntValue("maxControlConnections", servMgr->maxControl);
        iniFile.writeStrValue("rootHost", servMgr->rootHost.cstr());

        iniFile.writeSection("Client");
        iniFile.writeIntValue("refreshHTML", refreshHTML);
        iniFile.writeIntValue("relayBroadcast", servMgr->relayBroadcast);
        iniFile.writeIntValue("minBroadcastTTL", chanMgr->minBroadcastTTL);
        iniFile.writeIntValue("maxBroadcastTTL", chanMgr->maxBroadcastTTL);
        iniFile.writeIntValue("pushTries", chanMgr->pushTries);
        iniFile.writeIntValue("pushTimeout", chanMgr->pushTimeout);
        iniFile.writeIntValue("maxPushHops", chanMgr->maxPushHops);
        iniFile.writeIntValue("autoQuery", chanMgr->autoQuery);
        iniFile.writeIntValue("queryTTL", servMgr->queryTTL);

        iniFile.writeSection("Privacy");
        iniFile.writeStrValue("password", servMgr->password);
        iniFile.writeIntValue("maxUptime", chanMgr->maxUptime);

        for (int i=0; i<servMgr->numFilters; i++)
        {
            iniFile.writeSection("Filter");
                writeFilterSettings(iniFile, servMgr->filters[i]);
            iniFile.writeLine("[End]");
        }

        for (auto feed : servMgr->channelDirectory.feeds())
        {
            iniFile.writeSection("Feed");
            iniFile.writeStrValue("url", feed.url.c_str());
            iniFile.writeBoolValue("isPublic", feed.isPublic);
            iniFile.writeLine("[End]");
        }

        iniFile.writeSection("Notify");
            iniFile.writeBoolValue("PeerCast", notifyMask&NT_PEERCAST);
            iniFile.writeBoolValue("Broadcasters", notifyMask&NT_BROADCASTERS);
            iniFile.writeBoolValue("TrackInfo", notifyMask&NT_TRACKINFO);
        iniFile.writeLine("[End]");

        iniFile.writeSection("Server1");
            writeServerSettings(iniFile, allowServer1);
        iniFile.writeLine("[End]");

        iniFile.writeSection("Server2");
            writeServerSettings(iniFile, allowServer2);
        iniFile.writeLine("[End]");

        iniFile.writeSection("Debug");
        iniFile.writeBoolValue("logDebug", (showLog&(1<<LogBuffer::T_DEBUG))!=0);
        iniFile.writeBoolValue("logErrors", (showLog&(1<<LogBuffer::T_ERROR))!=0);
        iniFile.writeBoolValue("logNetwork", (showLog&(1<<LogBuffer::T_NETWORK))!=0);
        iniFile.writeBoolValue("logChannel", (showLog&(1<<LogBuffer::T_CHANNEL))!=0);
        iniFile.writeBoolValue("pauseLog", pauseLog);
        iniFile.writeIntValue("idleSleepTime", sys->idleSleepTime);

        if (servMgr->validBCID)
        {
            BCID *bcid = servMgr->validBCID;
            while (bcid)
            {
                iniFile.writeSection("ValidBCID");
                char idstr[128];
                bcid->id.toStr(idstr);
                iniFile.writeStrValue("id", idstr);
                iniFile.writeStrValue("name", bcid->name.cstr());
                iniFile.writeStrValue("email", bcid->email.cstr());
                iniFile.writeStrValue("url", bcid->url.cstr());
                iniFile.writeBoolValue("valid", bcid->valid);
                iniFile.writeLine("[End]");

                bcid=bcid->next;
            }
        }

        Channel *c = chanMgr->channel;
        while (c)
        {
            char idstr[64];
            if (c->isActive() && c->stayConnected)
            {
                c->getIDStr(idstr);

                iniFile.writeSection("RelayChannel");
                iniFile.writeStrValue("name", c->getName());
                iniFile.writeStrValue("genre", c->info.genre.cstr());
                if (!c->sourceURL.isEmpty())
                    iniFile.writeStrValue("sourceURL", c->sourceURL.cstr());
                iniFile.writeStrValue("sourceProtocol", ChanInfo::getProtocolStr(c->info.srcProtocol));
                iniFile.writeStrValue("contentType", c->info.getTypeStr());
                iniFile.writeStrValue("MIMEType", c->info.MIMEType);
                iniFile.writeStrValue("streamExt", c->info.streamExt);
                iniFile.writeIntValue("bitrate", c->info.bitrate);
                iniFile.writeStrValue("contactURL", c->info.url.cstr());
                iniFile.writeStrValue("id", idstr);
                iniFile.writeBoolValue("stayConnected", c->stayConnected);

                ChanHitList *chl = chanMgr->findHitListByID(c->info.id);
                if (chl)
                {
                    ChanHitSearch chs;
                    chs.trackersOnly = true;
                    if (chl->pickHits(chs))
                    {
                        char ipStr[64];
                        chs.best[0].host.toStr(ipStr);
                        iniFile.writeStrValue("tracker", ipStr);
                    }
                }
                iniFile.writeLine("[End]");
            }
            c=c->next;
        }

#if 0
        Servent *s = servents;
        while (s)
        {
            if (s->type == Servent::T_OUTGOING)
                if (s->isConnected())
                {
                    ServHost sh;
                    Host h = s->getHost();
                    sh.init(h, ServHost::T_SERVENT, 0, s->networkID);
                    writeServHost(iniFile, sh);
                }
            s=s->next;
        }
#endif

        for (int i=0; i<ServMgr::MAX_HOSTCACHE; i++)
        {
            ServHost *sh = &servMgr->hostCache[i];
            if (sh->type != ServHost::T_NONE)
                writeServHost(iniFile, *sh);
        }

        iniFile.close();
    }
}

// --------------------------------------------------
unsigned int readServerSettings(IniFile &iniFile, unsigned int a)
{
    while (iniFile.readNext())
    {
        if (iniFile.isName("[End]"))
            break;
        else if (iniFile.isName("allowHTML"))
            a = iniFile.getBoolValue()?a|Servent::ALLOW_HTML:a&~Servent::ALLOW_HTML;
        else if (iniFile.isName("allowDirect"))
            a = iniFile.getBoolValue()?a|Servent::ALLOW_DIRECT:a&~Servent::ALLOW_DIRECT;
        else if (iniFile.isName("allowNetwork"))
            a = iniFile.getBoolValue()?a|Servent::ALLOW_NETWORK:a&~Servent::ALLOW_NETWORK;
        else if (iniFile.isName("allowBroadcast"))
            a = iniFile.getBoolValue()?a|Servent::ALLOW_BROADCAST:a&~Servent::ALLOW_BROADCAST;
    }
    return a;
}

// --------------------------------------------------
void readFilterSettings(IniFile &iniFile, ServFilter &sv)
{
    sv.host.init();

    while (iniFile.readNext())
    {
        if (iniFile.isName("[End]"))
            break;
        else if (iniFile.isName("ip"))
            sv.host.fromStrIP(iniFile.getStrValue(), 0);
        else if (iniFile.isName("private"))
            sv.flags = (sv.flags & ~ServFilter::F_PRIVATE) | (iniFile.getBoolValue()?ServFilter::F_PRIVATE:0);
        else if (iniFile.isName("ban"))
            sv.flags = (sv.flags & ~ServFilter::F_BAN) | (iniFile.getBoolValue()?ServFilter::F_BAN:0);
        else if (iniFile.isName("allow") || iniFile.isName("network"))
            sv.flags = (sv.flags & ~ServFilter::F_NETWORK) | (iniFile.getBoolValue()?ServFilter::F_NETWORK:0);
        else if (iniFile.isName("direct"))
            sv.flags = (sv.flags & ~ServFilter::F_DIRECT) | (iniFile.getBoolValue()?ServFilter::F_DIRECT:0);
    }
}

// --------------------------------------------------
void ServMgr::loadSettings(const char *fn)
{
    int feedIndex = 0;
    IniFile iniFile;

    if (!iniFile.openReadOnly(fn))
        saveSettings(fn);

    servMgr->numFilters = 0;
    showLog = 0;

    if (iniFile.openReadOnly(fn))
    {
        while (iniFile.readNext())
        {
            // server settings
            if (iniFile.isName("serverName"))
                servMgr->serverName = iniFile.getStrValue();
            else if (iniFile.isName("serverPort"))
                servMgr->serverHost.port = iniFile.getIntValue();
            else if (iniFile.isName("autoServe"))
                servMgr->autoServe = iniFile.getBoolValue();
            else if (iniFile.isName("autoConnect"))
                servMgr->autoConnect = iniFile.getBoolValue();
            else if (iniFile.isName("icyPassword"))     // depreciated
                strcpy(servMgr->password, iniFile.getStrValue());
            else if (iniFile.isName("forceIP"))
                servMgr->forceIP = iniFile.getStrValue();
            else if (iniFile.isName("isRoot"))
                servMgr->isRoot = iniFile.getBoolValue();
            else if (iniFile.isName("broadcastID"))
            {
                chanMgr->broadcastID.fromStr(iniFile.getStrValue());
                chanMgr->broadcastID.id[0] = PCP_BROADCAST_FLAGS;           // hacky, but we need to fix old clients
            }else if (iniFile.isName("htmlPath"))
                strcpy(servMgr->htmlPath, iniFile.getStrValue());
            else if (iniFile.isName("maxPGNUIncoming"))
                servMgr->maxGnuIncoming = iniFile.getIntValue();
            else if (iniFile.isName("minPGNUIncoming"))
                servMgr->minGnuIncoming = iniFile.getIntValue();

            else if (iniFile.isName("maxControlConnections"))
            {
                servMgr->maxControl = iniFile.getIntValue();
            }
            else if (iniFile.isName("maxBitrateOut"))
                servMgr->maxBitrateOut = iniFile.getIntValue();

            else if (iniFile.isName("maxStreamsOut"))       // depreciated
                servMgr->setMaxRelays(iniFile.getIntValue());
            else if (iniFile.isName("maxRelays"))
                servMgr->setMaxRelays(iniFile.getIntValue());
            else if (iniFile.isName("maxDirect"))
                servMgr->maxDirect = iniFile.getIntValue();

            else if (iniFile.isName("maxStreamsPerChannel"))        // depreciated
                chanMgr->maxRelaysPerChannel = iniFile.getIntValue();
            else if (iniFile.isName("maxRelaysPerChannel"))
                chanMgr->maxRelaysPerChannel = iniFile.getIntValue();

            else if (iniFile.isName("firewallTimeout"))
                firewallTimeout = iniFile.getIntValue();
            else if (iniFile.isName("forceNormal"))
                forceNormal = iniFile.getBoolValue();
            else if (iniFile.isName("broadcastMsgInterval"))
                chanMgr->broadcastMsgInterval = iniFile.getIntValue();
            else if (iniFile.isName("broadcastMsg"))
                chanMgr->broadcastMsg.set(iniFile.getStrValue(), String::T_ASCII);
            else if (iniFile.isName("hostUpdateInterval"))
                chanMgr->hostUpdateInterval = iniFile.getIntValue();
            else if (iniFile.isName("icyMetaInterval"))
                chanMgr->icyMetaInterval = iniFile.getIntValue();
            else if (iniFile.isName("maxServIn"))
                servMgr->maxServIn = iniFile.getIntValue();
            else if (iniFile.isName("chanLog"))
                servMgr->chanLog.set(iniFile.getStrValue(), String::T_ASCII);
            else if (iniFile.isName("publicDirectory"))
                servMgr->publicDirectoryEnabled = iniFile.getBoolValue();

            else if (iniFile.isName("rootMsg"))
                rootMsg.set(iniFile.getStrValue());
            else if (iniFile.isName("networkID"))
                networkID.fromStr(iniFile.getStrValue());
            else if (iniFile.isName("authType"))
            {
                const char *t = iniFile.getStrValue();
                if (Sys::stricmp(t, "cookie")==0)
                    servMgr->authType = ServMgr::AUTH_COOKIE;
                else if (Sys::stricmp(t, "http-basic")==0)
                    servMgr->authType = ServMgr::AUTH_HTTPBASIC;
            }else if (iniFile.isName("cookiesExpire"))
            {
                const char *t = iniFile.getStrValue();
                if (Sys::stricmp(t, "never")==0)
                    servMgr->cookieList.neverExpire = true;
                else if (Sys::stricmp(t, "session")==0)
                    servMgr->cookieList.neverExpire = false;
            }else if (iniFile.isName("genrePrefix"))
            {
                servMgr->genrePrefix = iniFile.getStrValue();
            }

            // privacy settings
            else if (iniFile.isName("password"))
                strcpy(servMgr->password, iniFile.getStrValue());
            else if (iniFile.isName("maxUptime"))
                chanMgr->maxUptime = iniFile.getIntValue();

            // client settings

            else if (iniFile.isName("rootHost"))
            {
                if (!PCP_FORCE_YP)
                    servMgr->rootHost = iniFile.getStrValue();
            }else if (iniFile.isName("deadHitAge"))
                chanMgr->deadHitAge = iniFile.getIntValue();
            else if (iniFile.isName("tryoutDelay"))
                servMgr->tryoutDelay = iniFile.getIntValue();
            else if (iniFile.isName("refreshHTML"))
                refreshHTML = iniFile.getIntValue();
            else if (iniFile.isName("relayBroadcast"))
            {
                servMgr->relayBroadcast = iniFile.getIntValue();
                if (servMgr->relayBroadcast < 30)
                    servMgr->relayBroadcast = 30;
            }
            else if (iniFile.isName("minBroadcastTTL"))
                chanMgr->minBroadcastTTL = iniFile.getIntValue();
            else if (iniFile.isName("maxBroadcastTTL"))
                chanMgr->maxBroadcastTTL = iniFile.getIntValue();
            else if (iniFile.isName("pushTimeout"))
                chanMgr->pushTimeout = iniFile.getIntValue();
            else if (iniFile.isName("pushTries"))
                chanMgr->pushTries = iniFile.getIntValue();
            else if (iniFile.isName("maxPushHops"))
                chanMgr->maxPushHops = iniFile.getIntValue();
            else if (iniFile.isName("autoQuery"))
            {
                chanMgr->autoQuery = iniFile.getIntValue();
                if ((chanMgr->autoQuery < 300) && (chanMgr->autoQuery > 0))
                    chanMgr->autoQuery = 300;
            }
            else if (iniFile.isName("queryTTL"))
            {
                servMgr->queryTTL = iniFile.getIntValue();
            }

            // debug
            else if (iniFile.isName("logDebug"))
                showLog |= iniFile.getBoolValue() ? 1<<LogBuffer::T_DEBUG:0;
            else if (iniFile.isName("logErrors"))
                showLog |= iniFile.getBoolValue() ? 1<<LogBuffer::T_ERROR:0;
            else if (iniFile.isName("logNetwork"))
                showLog |= iniFile.getBoolValue() ? 1<<LogBuffer::T_NETWORK:0;
            else if (iniFile.isName("logChannel"))
                showLog |= iniFile.getBoolValue() ? 1<<LogBuffer::T_CHANNEL:0;
            else if (iniFile.isName("pauseLog"))
                pauseLog = iniFile.getBoolValue();
            else if (iniFile.isName("idleSleepTime"))
                sys->idleSleepTime = iniFile.getIntValue();
            else if (iniFile.isName("[Server1]"))
                allowServer1 = readServerSettings(iniFile, allowServer1);
            else if (iniFile.isName("[Server2]"))
                allowServer2 = readServerSettings(iniFile, allowServer2);
            else if (iniFile.isName("[Filter]"))
            {
                readFilterSettings(iniFile, filters[numFilters]);

                if (numFilters < (MAX_FILTERS-1))
                    numFilters++;
            }
            else if (iniFile.isName("[Feed]"))
            {
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("url"))
                        servMgr->channelDirectory.addFeed(iniFile.getStrValue());
                    else if (iniFile.isName("isPublic"))
                    {
                        servMgr->channelDirectory.setFeedPublic(feedIndex, iniFile.getBoolValue());
                    }
                }
                feedIndex++;
            }
            else if (iniFile.isName("[Notify]"))
            {
                notifyMask = NT_UPGRADE;
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("PeerCast"))
                        notifyMask |= iniFile.getBoolValue()?NT_PEERCAST:0;
                    else if (iniFile.isName("Broadcasters"))
                        notifyMask |= iniFile.getBoolValue()?NT_BROADCASTERS:0;
                    else if (iniFile.isName("TrackInfo"))
                        notifyMask |= iniFile.getBoolValue()?NT_TRACKINFO:0;
                }
            }
            else if (iniFile.isName("[RelayChannel]"))
            {
                ChanInfo info;
                bool stayConnected=false;
                String sourceURL;
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("name"))
                        info.name.set(iniFile.getStrValue());
                    else if (iniFile.isName("id"))
                        info.id.fromStr(iniFile.getStrValue());
                    else if (iniFile.isName("sourceType"))
                        info.srcProtocol = ChanInfo::getProtocolFromStr(iniFile.getStrValue());
                    else if (iniFile.isName("contentType"))
                    {
                        info.contentType    = ChanInfo::getTypeFromStr(iniFile.getStrValue());
                        info.contentTypeStr = iniFile.getStrValue();
                    }
                    else if (iniFile.isName("MIMEType"))
                        info.MIMEType = iniFile.getStrValue();
                    else if (iniFile.isName("streamExt"))
                        info.streamExt = iniFile.getStrValue();
                    else if (iniFile.isName("stayConnected"))
                        stayConnected = iniFile.getBoolValue();
                    else if (iniFile.isName("sourceURL"))
                        sourceURL.set(iniFile.getStrValue());
                    else if (iniFile.isName("genre"))
                        info.genre.set(iniFile.getStrValue());
                    else if (iniFile.isName("contactURL"))
                        info.url.set(iniFile.getStrValue());
                    else if (iniFile.isName("bitrate"))
                        info.bitrate = atoi(iniFile.getStrValue());
                    else if (iniFile.isName("tracker"))
                    {
                        ChanHit hit;
                        hit.init();
                        hit.tracker = true;
                        hit.host.fromStrName(iniFile.getStrValue(), DEFAULT_PORT);
                        hit.rhost[0] = hit.host;
                        hit.rhost[1] = hit.host;
                        hit.chanID = info.id;
                        hit.recv = true;
                        chanMgr->addHit(hit);
                    }
                }
                if (sourceURL.isEmpty())
                {
                    chanMgr->createRelay(info, stayConnected);
                }else
                {
                    info.bcID = chanMgr->broadcastID;
                    Channel *c = chanMgr->createChannel(info, NULL);
                    if (c)
                        c->startURL(sourceURL.cstr());
                }
            } else if (iniFile.isName("[Host]"))
            {
                Host h;
                ServHost::TYPE type = ServHost::T_NONE;
                unsigned int time = 0;

                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("address"))
                        h.fromStrIP(iniFile.getStrValue(), DEFAULT_PORT);
                    else if (iniFile.isName("type"))
                        type = ServHost::getTypeFromStr(iniFile.getStrValue());
                    else if (iniFile.isName("time"))
                        time = iniFile.getIntValue();
                }
                servMgr->addHost(h, type, time);
            } else if (iniFile.isName("[ValidBCID]"))
            {
                BCID *bcid = new BCID();
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("id"))
                        bcid->id.fromStr(iniFile.getStrValue());
                    else if (iniFile.isName("name"))
                        bcid->name.set(iniFile.getStrValue());
                    else if (iniFile.isName("email"))
                        bcid->email.set(iniFile.getStrValue());
                    else if (iniFile.isName("url"))
                        bcid->url.set(iniFile.getStrValue());
                    else if (iniFile.isName("valid"))
                        bcid->valid = iniFile.getBoolValue();
                }
                servMgr->addValidBCID(bcid);
            }
        }
    }

    if (!numFilters)
        setFilterDefaults();
}

// --------------------------------------------------
unsigned int ServMgr::numStreams(GnuID &cid, Servent::TYPE tp, bool all)
{
    int cnt = 0;
    Servent *sv = servents;
    while (sv)
    {
        if (sv->isConnected())
            if (sv->type == tp)
                if (sv->chanID.isSame(cid))
                    if (all || !sv->isPrivate())
                        cnt++;
        sv=sv->next;
    }
    return cnt;
}

// --------------------------------------------------
unsigned int ServMgr::numStreams(Servent::TYPE tp, bool all)
{
    int cnt = 0;
    Servent *sv = servents;
    while (sv)
    {
        if (sv->isConnected())
            if (sv->type == tp)
                if (all || !sv->isPrivate())
                    cnt++;
        sv=sv->next;
    }
    return cnt;
}

// --------------------------------------------------
bool ServMgr::getChannel(char *str, ChanInfo &info, bool relay)
{
    procConnectArgs(str, info);

    Channel *ch;

    ch = chanMgr->findChannelByNameID(info);
    if (ch)
    {
        if (!ch->isPlaying())
        {
            if (relay)
            {
                ch->info.lastPlayStart = 0; // force reconnect
                ch->info.lastPlayEnd = 0;
                for (int i = 0; i < 100; i++)    // wait til it's playing for 10 seconds
                {
                    ch = chanMgr->findChannelByNameID(ch->info);

                    if (!ch)
                        return false;

                    if (ch->isPlaying())
                        break;

                    sys->sleep(100);
                }
            }else
                return false;
        }

        info = ch->info;    // get updated channel info

        return true;
    }else
    {
        if (relay)
        {
            ch = chanMgr->findAndRelay(info);
            if (ch)
            {
                info = ch->info; //get updated channel info
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------
int ServMgr::findChannel(ChanInfo &info)
{
#if 0
    char idStr[64];
    info.id.toStr(idStr);

    if (info.id.isSet())
    {
        // if we have an ID then try and connect to known hosts carrying channel.
        ServHost sh = getOutgoingServent(info.id);
        addOutgoing(sh.host, info.id, true);
    }

    GnuPacket pack;

    XML xml;
    XML::Node *n = info.createQueryXML();
    xml.setRoot(n);
    pack.initFind(NULL, &xml, servMgr->queryTTL);

    addReplyID(pack.id);
    int cnt = broadcast(pack, NULL);

    LOG_NETWORK("Querying network: %s %s - %d servents", info.name.cstr(), idStr, cnt);

    return cnt;
#endif
    return 0;
}

// --------------------------------------------------
// add outgoing network connection from string (ip:port format)
bool ServMgr::addOutgoing(Host h, GnuID &netid, bool pri)
{
#if 0
    if (h.ip)
    {
        if (!findServent(h.ip, h.port, netid))
        {
            Servent *sv = allocServent();
            if (sv)
            {
                if (pri)
                    sv->priorityConnect = true;
                sv->networkID = netid;
                sv->initOutgoing(h, Servent::T_OUTGOING);
                return true;
            }
        }
    }
#endif
    return false;
}

// --------------------------------------------------
Servent *ServMgr::findConnection(Servent::TYPE t, GnuID &sid)
{
    Servent *sv = servents;
    while (sv)
    {
        if (sv->isConnected())
            if (sv->type == t)
                if (sv->remoteID.isSame(sid))
                    return sv;
        sv=sv->next;
    }
    return NULL;
}

// --------------------------------------------------
void ServMgr::procConnectArgs(char *str, ChanInfo &info)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    char *args = strstr(str, "?");
    if (args)
        *args++=0;

    info.initNameID(str);

    if (args)
    {
        while (args=nextCGIarg(args, curr, arg))
        {
            LOG_DEBUG("cmd: %s, arg: %s", curr, arg);

            if (strcmp(curr, "sip")==0)
            // sip - add network connection to client with channel
            {
                Host h;
                h.fromStrName(arg, DEFAULT_PORT);
                if (addOutgoing(h, servMgr->networkID, true))
                    LOG_NETWORK("Added connection: %s", arg);
            }else if (strcmp(curr, "pip")==0)
            // pip - add private network connection to client with channel
            {
                Host h;
                h.fromStrName(arg, DEFAULT_PORT);
                if (addOutgoing(h, info.id, true))
                    LOG_NETWORK("Added private connection: %s", arg);
            }else if (strcmp(curr, "ip")==0)
            // ip - add hit
            {
                Host h;
                h.fromStrName(arg, DEFAULT_PORT);
                ChanHit hit;
                hit.init();
                hit.host = h;
                hit.rhost[0] = h;
                hit.rhost[1].init();
                hit.chanID = info.id;
                hit.recv = true;

                chanMgr->addHit(hit);
            }else if (strcmp(curr, "tip")==0)
            // tip - add tracker hit
            {
                Host h;
                h.fromStrName(arg, DEFAULT_PORT);
                chanMgr->addHit(h, info.id, true);
            }
        }
    }
}

// --------------------------------------------------
bool ServMgr::start()
{
    char idStr[64];

    const char *priv;
#if PRIVATE_BROADCASTER
    priv = "(private)";
#else
    priv = "";
#endif
    LOG_DEBUG("Peercast %s, %s %s", PCX_VERSTRING, peercastApp->getClientTypeOS(), priv);

    sessionID.toStr(idStr);
    LOG_DEBUG("SessionID: %s", idStr);

    chanMgr->broadcastID.toStr(idStr);
    LOG_DEBUG("BroadcastID: %s", idStr);

    checkForceIP();

    serverThread.func = ServMgr::serverProc;
    if (!sys->startThread(&serverThread))
        return false;

    idleThread.func = ServMgr::idleProc;
    if (!sys->startThread(&idleThread))
        return false;

    return true;
}

// --------------------------------------------------
int ServMgr::clientProc(ThreadInfo *thread)
{
#if 0
    thread->lock();

    GnuID netID;
    netID = servMgr->networkID;

    while (thread->active)
    {
        if (servMgr->autoConnect)
        {
            if (servMgr->needConnections() || servMgr->forceLookup)
            {
                if (servMgr->needHosts() || servMgr->forceLookup)
                {
                    // do lookup to find some hosts

                    Host lh;
                    lh.fromStrName(servMgr->connectHost, DEFAULT_PORT);

                    if (!servMgr->findServent(lh.ip, lh.port, netID))
                    {
                        Servent *sv = servMgr->allocServent();
                        if (sv)
                        {
                            LOG_DEBUG("Lookup: %s", servMgr->connectHost);
                            sv->networkID = netID;
                            sv->initOutgoing(lh, Servent::T_LOOKUP);
                            servMgr->forceLookup = false;
                        }
                    }
                }

                for (int i=0; i<MAX_TRYOUT; i++)
                {
                    if (servMgr->outUsedFull())
                        break;
                    if (servMgr->tryFull())
                        break;

                    ServHost sh = servMgr->getOutgoingServent(netID);

                    if (!servMgr->addOutgoing(sh.host, netID, false))
                        servMgr->deadHost(sh.host, ServHost::T_SERVENT);
                    sys->sleep(servMgr->tryoutDelay);
                    break;
                }
            }
        }else{
#if 0
            Servent *s = servMgr->servents;
            while (s)
            {
                if (s->type == Servent::T_OUTGOING)
                    s->thread.active = false;
                s=s->next;
            }
#endif
        }
        sys->sleepIdle();
    }
    thread->unlock();
#endif
    return 0;
}

// -----------------------------------
bool    ServMgr::acceptGIV(ClientSocket *sock)
{
    Servent *sv = servents;
    while (sv)
    {
        if (sv->type == Servent::T_COUT)
        {
            if (sv->acceptGIV(sock))
                return true;
        }
        sv=sv->next;
    }
    return false;
}

// -----------------------------------
int ServMgr::broadcastPushRequest(ChanHit &hit, Host &to, GnuID &chanID, Servent::TYPE type)
{
    ChanPacket pack;
    MemoryStream pmem(pack.data, sizeof(pack.data));
    AtomStream atom(pmem);

    atom.writeParent(PCP_BCST, 10);
        atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_ALL);
        atom.writeChar(PCP_BCST_HOPS, 0);
        atom.writeChar(PCP_BCST_TTL, 7);
        atom.writeBytes(PCP_BCST_DEST, hit.sessionID.id, 16);
        atom.writeBytes(PCP_BCST_FROM, servMgr->sessionID.id, 16);
        atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
        atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
        atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
        atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
        atom.writeParent(PCP_PUSH, 3);
            atom.writeInt(PCP_PUSH_IP, to.ip);
            atom.writeShort(PCP_PUSH_PORT, to.port);
            atom.writeBytes(PCP_PUSH_CHANID, chanID.id, 16);

    pack.len = pmem.pos;
    pack.type = ChanPacket::T_PCP;

    GnuID noID;

    return servMgr->broadcastPacket(pack, noID, servMgr->sessionID, hit.sessionID, type);
}

// --------------------------------------------------
void ServMgr::writeRootAtoms(AtomStream &atom, bool getUpdate)
{
    atom.writeParent(PCP_ROOT, 5 + (getUpdate?1:0));
        atom.writeInt(PCP_ROOT_UPDINT, chanMgr->hostUpdateInterval);
        atom.writeString(PCP_ROOT_URL, "download.php");
        atom.writeInt(PCP_ROOT_CHECKVER, PCP_ROOT_VERSION);
        atom.writeInt(PCP_ROOT_NEXT, chanMgr->hostUpdateInterval);
        atom.writeString(PCP_MESG_ASCII, rootMsg.cstr());
        if (getUpdate)
            atom.writeParent(PCP_ROOT_UPDATE, 0);
}

// --------------------------------------------------
void ServMgr::broadcastRootSettings(bool getUpdate)
{
    if (isRoot)
    {
        ChanPacket pack;
        MemoryStream mem(pack.data, sizeof(pack.data));
        AtomStream atom(mem);
        atom.writeParent(PCP_BCST, 9);
            atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_TRACKERS);
            atom.writeChar(PCP_BCST_HOPS, 0);
            atom.writeChar(PCP_BCST_TTL, 7);
            atom.writeBytes(PCP_BCST_FROM, sessionID.id, 16);
            atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
            atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
            atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
            atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
            writeRootAtoms(atom, getUpdate);

        mem.len = mem.pos;
        mem.rewind();
        pack.len = mem.len;

        GnuID noID;

        broadcastPacket(pack, noID, servMgr->sessionID, noID, Servent::T_CIN);
    }
}

// --------------------------------------------------
int ServMgr::broadcastPacket(ChanPacket &pack, GnuID &chanID, GnuID &srcID, GnuID &destID, Servent::TYPE type)
{
    int cnt=0;

    Servent *sv = servents;
    while (sv)
    {
        if (sv->sendPacket(pack, chanID, srcID, destID, type))
            cnt++;
        sv=sv->next;
    }
    return cnt;
}

// --------------------------------------------------
int ServMgr::idleProc(ThreadInfo *thread)
{
    sys->setThreadName(thread, "IDLE");

    // nothing much to do for the first couple of seconds, so just hang around.
    sys->sleep(2000);

    unsigned int lastBroadcastConnect = 0;
    unsigned int lastRootBroadcast = 0;

    unsigned int lastForceIPCheck = 0;

    while (thread->active)
    {
        stats.update();

        unsigned int ctime = sys->getTime();

        if (!servMgr->forceIP.isEmpty())
        {
            if ((ctime - lastForceIPCheck) > 60)
            {
                if (servMgr->checkForceIP())
                {
                    GnuID noID;
                    chanMgr->broadcastTrackerUpdate(noID, true);
                }
                lastForceIPCheck = ctime;
            }
        }

        if (chanMgr->isBroadcasting())
        {
            if ((ctime - lastBroadcastConnect) > 30)
            {
                servMgr->connectBroadcaster();
                lastBroadcastConnect = ctime;
            }
        }

        if (servMgr->isRoot)
        {
            // 1時間着信がなかったら終了する。…なぜ？
            if ((servMgr->lastIncoming) && (((int64_t)ctime - servMgr->lastIncoming) > 60*60))
            {
                peercastInst->saveSettings();
                sys->exit();
            }

            if ((ctime - lastRootBroadcast) > chanMgr->hostUpdateInterval)
            {
                servMgr->broadcastRootSettings(true);
                lastRootBroadcast = ctime;
            }
        }

        // デッドヒットをクリアする。オリジナルはトラッカーをクリアす
        // るが、開くチャンネルがこのサーバーに設定されている YP に掲
        // 載されているとは限らないので、トラッカーが消えると再び開く
        // ことができないので、トラッカーを残す。
        chanMgr->clearDeadHits(false);

        if (servMgr->shutdownTimer)
        {
            if (--servMgr->shutdownTimer <= 0)
            {
                peercastInst->saveSettings();
                sys->exit();
            }
        }

        // shutdown idle channels
        if (chanMgr->numIdleChannels() > ChanMgr::MAX_IDLE_CHANNELS)
            chanMgr->closeOldestIdle();

        // チャンネル一覧を取得する。
        servMgr->channelDirectory.update();

        sys->sleep(500);
    }

    sys->endThread(thread);
    return 0;
}

// --------------------------------------------------
int ServMgr::serverProc(ThreadInfo *thread)
{
    sys->setThreadName(thread, "SERVER");

    Servent *serv = servMgr->allocServent();
    Servent *serv2 = servMgr->allocServent();

    //unsigned int lastLookupTime=0;

    while (thread->active)
    {
        if (servMgr->restartServer)
        {
            serv->abort();      // force close
            serv2->abort();     // force close
            //servMgr->quit();

            servMgr->restartServer = false;
        }

        if (servMgr->autoServe)
        {
            serv->allow = servMgr->allowServer1;
            serv2->allow = servMgr->allowServer2;

            if ((!serv->sock) || (!serv2->sock))
            {
                LOG_DEBUG("Starting servers");

                if (servMgr->forceNormal)
                    servMgr->setFirewall(ServMgr::FW_OFF);
                else
                    servMgr->setFirewall(ServMgr::FW_UNKNOWN);

                Host h = servMgr->serverHost;

                if (!serv->sock)
                    if (!serv->initServer(h))
                    {
                        LOG_ERROR("Failed to start server on port %d. Exitting...", h.port);
                        sys->exit();
                    }

                h.port++;
                if (!serv2->sock)
                    if (!serv2->initServer(h))
                    {
                        LOG_ERROR("Failed to start server on port %d. Exitting...", h.port);
                        sys->exit();
                    }
            }
        }else{
            // stop server
            serv->abort();      // force close
            serv2->abort();     // force close

            // cancel incoming connectuions
            Servent *s = servMgr->servents;
            while (s)
            {
                if (s->type == Servent::T_INCOMING)
                    s->thread.active = false;
                s=s->next;
            }

            servMgr->setFirewall(ServMgr::FW_ON);
        }

        sys->sleepIdle();
    }

    sys->endThread(thread);
    return 0;
}

// -----------------------------------
void    ServMgr::setMaxRelays(int max)
{
    if (max < MIN_RELAYS)
        max = MIN_RELAYS;
    maxRelays = max;
}

// -----------------------------------
XML::Node *ServMgr::createServentXML()
{
    return new XML::Node("servent agent=\"%s\" ", PCX_AGENT);
}

// --------------------------------------------------
const char *ServHost::getTypeStr(TYPE t)
{
    switch(t)
    {
        case T_NONE: return "NONE";
        case T_STREAM: return "STREAM";
        case T_CHANNEL: return "CHANNEL";
        case T_SERVENT: return "SERVENT";
        case T_TRACKER: return "TRACKER";
    }
    return "UNKNOWN";
}

// --------------------------------------------------
ServHost::TYPE ServHost::getTypeFromStr(const char *s)
{
    if (Sys::stricmp(s, "NONE")==0)
        return T_NONE;
    else if (Sys::stricmp(s, "SERVENT")==0)
        return T_SERVENT;
    else if (Sys::stricmp(s, "STREAM")==0)
        return T_STREAM;
    else if (Sys::stricmp(s, "CHANNEL")==0)
        return T_CHANNEL;
    else if (Sys::stricmp(s, "TRACKER")==0)
        return T_TRACKER;

    return T_NONE;
}

// --------------------------------------------------
bool    ServFilter::writeVariable(Stream &out, const String &var)
{
    std::string buf;

    if (var == "network")
        buf = (flags & F_NETWORK) ? "1" : "0";
    else if (var == "private")
        buf = (flags & F_PRIVATE) ? "1" : "0";
    else if (var == "direct")
        buf = (flags & F_DIRECT) ? "1" : "0";
    else if (var == "banned")
        buf = (flags & F_BAN) ? "1" : "0";
    else if (var == "ip")
        buf = host.str(false); // without port
    else
        return false;

    out.writeString(buf);
    return true;
}

// --------------------------------------------------
bool    BCID::writeVariable(Stream &out, const String &var)
{
    std::string buf;

    if (var == "id")
        buf = id.str();
    else if (var == "name")
        buf = name.c_str();
    else if (var == "email")
        buf = email.c_str();
    else if (var == "url")
        buf = url.c_str();
    else if (var == "valid")
        buf = valid ? "Yes" : "No";
    else
        return false;

    out.writeString(buf);
    return true;
}

// --------------------------------------------------
bool ServMgr::writeVariable(Stream &out, const String &var)
{
    using namespace std;

    string buf;

    if (var == "version")
        buf = PCX_VERSTRING;
    else if (var == "uptime")
    {
        String str;
        str.setFromStopwatch(getUptime());
        buf = str.c_str();
    }else if (var == "numRelays")
        buf = to_string(numStreams(Servent::T_RELAY, true));
    else if (var == "numDirect")
        buf = to_string(numStreams(Servent::T_DIRECT, true));
    else if (var == "totalConnected")
        buf = to_string(totalConnected());
    else if (var == "numServHosts")
        buf = to_string(numHosts(ServHost::T_SERVENT));
    else if (var == "numServents")
        buf = to_string(numServents());
    else if (var == "serverName")
        buf = serverName.c_str();
    else if (var == "serverPort")
        buf = to_string(serverHost.port);
    else if (var == "serverIP")
        buf = serverHost.str(false);
    else if (var == "ypAddress")
        buf = rootHost.c_str();
    else if (var == "password")
        buf = password;
    else if (var == "isFirewalled")
        buf = getFirewall()==FW_ON ? "1" : "0";
    else if (var == "firewallKnown")
        buf = getFirewall()==FW_UNKNOWN ? "0" : "1";
    else if (var == "rootMsg")
        buf = rootMsg.c_str();
    else if (var == "isRoot")
        buf = to_string(isRoot ? 1 : 0);
    else if (var == "isPrivate")
        buf = (PCP_BROADCAST_FLAGS&1) ? "1" : "0";
    else if (var == "forceYP")
        buf = PCP_FORCE_YP ? "1" : "0";
    else if (var == "refreshHTML")
        buf = to_string(refreshHTML ? refreshHTML : 0x0fffffff);
    else if (var == "maxRelays")
        buf = to_string(maxRelays);
    else if (var == "maxDirect")
        buf = to_string(maxDirect);
    else if (var == "maxBitrateOut")
        buf = to_string(maxBitrateOut);
    else if (var == "maxControlsIn")
        buf = to_string(maxControl);
    else if (var == "maxServIn")
        buf = to_string(maxServIn);
    else if (var == "numFilters")
        buf = to_string(numFilters+1);
    else if (var == "maxPGNUIn")
        buf = to_string(maxGnuIncoming);
    else if (var == "minPGNUIn")
        buf = to_string(minGnuIncoming);
    else if (var == "numActive1")
        buf = to_string(numActiveOnPort(serverHost.port));
    else if (var == "numActive2")
        buf = to_string(numActiveOnPort(serverHost.port+1));
    else if (var == "numPGNU")
        buf = to_string(numConnected(Servent::T_PGNU));
    else if (var == "numCIN")
        buf = to_string(numConnected(Servent::T_CIN));
    else if (var == "numCOUT")
        buf = to_string(numConnected(Servent::T_COUT));
    else if (var == "numIncoming")
        buf = to_string(numActive(Servent::T_INCOMING));
    else if (var == "numValidBCID")
    {
        int cnt = 0;
        BCID *bcid = validBCID;
        while (bcid)
        {
            cnt++;
            bcid = bcid->next;
        }
        buf = to_string(cnt);
    }else if (var == "disabled")
        buf = to_string(isDisabled);
    else if (var == "serverPort1")
        buf = to_string(serverHost.port);
    else if (var == "serverLocalIP")
    {
        Host lh(ClientSocket::getIP(NULL), 0);
        buf = lh.str(false);
    }else if (var == "upgradeURL")
        buf = servMgr->downloadURL;
    else if (var == "serverPort2")
        buf = to_string(serverHost.port+1);
    else if (var.startsWith("allow."))
    {
        if (var == "allow.HTML1")
            buf = (allowServer1 & Servent::ALLOW_HTML) ? "1" : "0";
        else if (var == "allow.HTML2")
            buf = (allowServer2 & Servent::ALLOW_HTML) ? "1" : "0";
        else if (var == "allow.broadcasting1")
            buf = (allowServer1 & Servent::ALLOW_BROADCAST) ? "1" : "0";
        else if (var == "allow.broadcasting2")
            buf = (allowServer2 & Servent::ALLOW_BROADCAST) ? "1" : "0";
        else if (var == "allow.network1")
            buf = (allowServer1 & Servent::ALLOW_NETWORK) ? "1" : "0";
        else if (var == "allow.direct1")
            buf = (allowServer1 & Servent::ALLOW_DIRECT) ? "1" : "0";
    }else if (var.startsWith("auth."))
    {
        if (var == "auth.useCookies")
            buf = (authType==AUTH_COOKIE) ? "1" : "0";
        else if (var == "auth.useHTTP")
            buf = (authType==AUTH_HTTPBASIC) ? "1" : "0";
        else if (var == "auth.useSessionCookies")
            buf = (cookieList.neverExpire==false) ? "1" : "0";
    }else if (var.startsWith("log."))
    {
        if (var == "log.debug")
            buf = (showLog & (1<<LogBuffer::T_DEBUG)) ? "1" : "0";
        else if (var == "log.errors")
            buf = (showLog & (1<<LogBuffer::T_ERROR)) ? "1" : "0";
        else if (var == "log.gnet")
            buf = (showLog & (1<<LogBuffer::T_NETWORK)) ? "1" : "0";
        else if (var == "log.channel")
            buf = (showLog & (1<<LogBuffer::T_CHANNEL)) ? "1" : "0";
        else
            return false;
    }else if (var.startsWith("lang."))
    {
        const char* lang = var.c_str() + 5;

        if (strrchr(htmlPath, '/') &&
            strcmp(strrchr(htmlPath, '/') + 1, lang) == 0)
            buf = "1";
        else
            buf = "0";
    }else if (var == "numExternalChannels")
    {
        buf = to_string(channelDirectory.numChannels());
    }else if (var == "numChannelFeedsPlusOne")
    {
        buf = to_string(channelDirectory.numFeeds() + 1);
    }else if (var == "numChannelFeeds")
    {
        buf = to_string(channelDirectory.numFeeds());
    }else if (var.startsWith("channelDirectory."))
    {
        return channelDirectory.writeVariable(out, var + strlen("channelDirectory."));
    }else if (var == "publicDirectoryEnabled")
    {
        buf = to_string(publicDirectoryEnabled);
    }else if (var == "genrePrefix")
    {
        buf = genrePrefix;
    }else if (var == "test")
    {
        out.writeUTF8(0x304b);
        out.writeUTF8(0x304d);
        out.writeUTF8(0x304f);
        out.writeUTF8(0x3051);
        out.writeUTF8(0x3053);

        out.writeUTF8(0x0041);
        out.writeUTF8(0x0042);
        out.writeUTF8(0x0043);
        out.writeUTF8(0x0044);

        out.writeChar('a');
        out.writeChar('b');
        out.writeChar('c');
        out.writeChar('d');
        return true;
    }else
        return false;

    out.writeString(buf);
    return true;
}
