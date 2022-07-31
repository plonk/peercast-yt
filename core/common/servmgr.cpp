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

#include <memory>

#include "servent.h"
#include "servmgr.h"
#include "inifile.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "atom.h"
#include "version2.h"
#include "rtmpmonit.h"
#include "chandir.h"
#include "uptest.h"
#include "portcheck.h"

// -----------------------------------
ServMgr::ServMgr()
    : relayBroadcast(30) // オリジナルでは未初期化。
    , channelDirectory(new ChannelDirectory())
    , publicDirectoryEnabled(false)
    , uptestServiceRegistry(new UptestServiceRegistry())
#ifdef WIN32
    , rtmpServerMonitor(std::string(peercastApp->getPath()) + "rtmp-server")
#else
    , rtmpServerMonitor(sys->joinPath({ sys->dirname(sys->getExecutablePath()), "rtmp-server" }))
#endif
    , flags(
        {
            {"randomizeBroadcastingChannelID", "配信するチャンネルのIDをランダムにする。", true },
            {"sendPortAtomWhenFirewallUnknown", "古いPeerCastStation相手に正常にポートチェックするにはオフにする。", false },
            {"forceFirewalled", "ファイアーウォール オンであるかの様に振る舞う。", false},
            {"startPlayingFromKeyFrame", "DIRECT接続でキーフレームまで継続パケットをスキップする。", true},
            {"banTrackersWhileBroadcasting", "配信中他の配信者による視聴をBANする。", false},
        })
    , preferredTheme("system")
    , accentColor("blue")
{
    authType = AUTH_COOKIE;

    serventNum = 0;

    startTime = sys->getTime();

    allowServer1 = Servent::ALLOW_ALL;

    clearHostCache(ServHost::T_NONE);
    password[0]=0;

    useFlowControl = true;

    maxServIn = 50;

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

    rootHost = "yp.pcgw.pgw.jp:7146";

    serverHost.fromStrIP("127.0.0.1", DEFAULT_PORT);
    serverHostIPv6 = Host(IP::parse("::1"), DEFAULT_PORT);

    firewalled = FW_UNKNOWN;
    firewalledIPv6 = FW_UNKNOWN;
    allowDirect = true;
    autoConnect = true;
    forceLookup = true;
    autoServe = true;
    forceNormal = false;

    maxControl = 3;

    totalStreams = 0;
    firewallTimeout = 30;
    pauseLog = false;
    m_logLevel = LogBuffer::T_INFO;

    shutdownTimer = 0;

    downloadURL[0] = 0;
    rootMsg.clear();

    restartServer=false;

    numFilters = 0;
    ensureCatchallFilters();

    servents = NULL;

    chanLog="";

    serverName = "";

    transcodingEnabled = false;
    preset = "veryfast";
    audioCodec = "mp3";

    wmvProtocol = "http";

    rtmpPort = 1935;

    channelDirectory->addFeed("http://yp.pcgw.pgw.jp/index.txt");

    uptestServiceRegistry->addURL("http://bayonet.ddo.jp/sp/yp4g.xml");

    chat = true;
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

// ------------------------------------
bool ServMgr::updateIPAddress(const IP& newIP)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    bool success = false;

    if (newIP.isIPv4Mapped()) {
        auto score = [](const IP& ip) -> int {
            if (ip.isIPv4Loopback())
                return 0;
            else if (ip.isIPv4Private())
                return 1;
            else
                return 2;
        };
        if (score(serverHost.ip) < score(newIP)) {
            IP oldIP = serverHost.ip;
            serverHost.ip = newIP;
            LOG_INFO("Server IPv4 address changed to %s (was %s)",
                     newIP.str().c_str(),
                     oldIP.str().c_str());
            success = true;
        }
    } else {
        auto score = [](const IP& ip) -> int {
            if (ip.isIPv6Loopback())
                return 0;
            else if (ip.isIPv6LinkLocal())
                return 1;
            else if (ip.isIPv6UniqueLocal())
                return 2;
            else
                return 3;
        };
        if (score(serverHostIPv6.ip) < score(newIP)) {
            IP oldIP = serverHostIPv6.ip;
            serverHostIPv6.ip = newIP;
            LOG_INFO("Server IPv6 address changed to %s (was %s)",
                     newIP.str().c_str(),
                     oldIP.str().c_str());
            success = true;
        }
    }
    return success;
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
void ServMgr::ensureCatchallFilters()
{
    bool hasV4Catchall = false;
    bool hasV6Catchall = false;

    for (int i = 0; i < numFilters; i++)
    {
        if (filters[i].getPattern() == "255.255.255.255")
            hasV4Catchall = true;
        else if (filters[i].getPattern() == "::/0")
            hasV6Catchall = true;
    }

    if (!hasV4Catchall)
    {
        filters[numFilters].setPattern("255.255.255.255");
        filters[numFilters].flags = ServFilter::F_NETWORK|ServFilter::F_DIRECT;
        numFilters++;
    }
    

    if (!hasV6Catchall)
    {
        filters[numFilters].setPattern("::/0");
        filters[numFilters].flags = ServFilter::F_NETWORK|ServFilter::F_DIRECT;
        numFilters++;
    }
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

    if (!sh)
        LOG_DEBUG("New host: %s - %s", h.str().c_str(), ServHost::getTypeStr(type));
    else
        LOG_DEBUG("Old host: %s - %s", h.str().c_str(), ServHost::getTypeStr(type));

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
Servent *ServMgr::findOldestServent(Servent::TYPE type, bool priv)
{
    Servent *oldest=NULL;

    Servent *s = servents;
    while (s)
    {
        if (s->type == type)
            if (s->thread.active())
                if (s->isOlderThan(oldest))
                    if (s->isPrivate() == priv)
                        oldest = s;
        s=s->next;
    }
    return oldest;
}

// -----------------------------------
Servent *ServMgr::findServent(Servent::TYPE type, Host &host, const GnuID &netid)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *s = servents;
    while (s)
    {
        if (s->type == type)
        {
            Host h = s->getHost();
            if (h.isSame(host) && s->networkID.isSame(netid))
            {
                return s;
            }
        }
        s = s->next;
    }

    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServent(unsigned int ip, unsigned short port, const GnuID &netid)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    Host target(ip, port);

    Servent *s = servents;
    while (s)
    {
        if (s->type != Servent::T_NONE)
        {
            if (s->getHost() == target && (s->networkID.isSame(netid)))
            {
                return s;
            }
        }
        s = s->next;
    }

    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServent(Servent::TYPE t)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *s = servents;
    while (s)
    {
        if (s->type == t)
            return s;
        s = s->next;
    }
    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServentByIndex(int id)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *s = servents;
    int cnt = 0;

    while (s)
    {
        if (cnt == id)
            return s;
        cnt++;
        s = s->next;
    }

    return NULL;
}

// -----------------------------------
Servent *ServMgr::findServentByID(int id)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *s = servents;

    while (s)
    {
        if (s->serventIndex == id)
            return s;
        s = s->next;
    }

    return NULL;
}

// -----------------------------------
Servent *ServMgr::allocServent()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *s = servents;
    while (s)
    {
        if (s->status == Servent::S_FREE)
            break;
        s = s->next;
    }

    if (!s)
    {
        int num = ++serventNum;
        s = new Servent(num);
        s->next = servents;
        servents = s;

        LOG_TRACE("allocated servent %d", num);
    }else
        LOG_TRACE("reused servent %d", s->serventIndex);

    s->reset();

    return s;
}

// --------------------------------------------------
void    ServMgr::closeConnections(Servent::TYPE type)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *sv = servents;
    while (sv)
    {
        if (sv->isConnected())
            if (sv->type == type)
                sv->thread.shutdown();
        sv = sv->next;
    }
}

// -----------------------------------
unsigned int ServMgr::numConnected(int type, bool priv, unsigned int uptime)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt=0;

    unsigned int ctime=sys->getTime();
    Servent *s = servents;
    while (s)
    {
        if (s->thread.active())
            if (s->isConnected())
                if (s->type == type)
                    if (s->isPrivate()==priv)
                        if ((ctime-s->lastConnect) >= uptime)
                            cnt++;

        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numConnected()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        if (s->thread.active())
            if (s->isConnected())
                cnt++;

        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numServents()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt = 0;

    Servent *s = servents;
    while (s)
    {
        cnt++;
        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numUsed(int type)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        std::lock_guard<std::recursive_mutex> cs(s->lock);
        if (s->type == type)
            cnt++;
        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numActiveOnPort(int port)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        std::lock_guard<std::recursive_mutex> cs(s->lock);
        if (s->thread.active() && s->sock && (s->servPort == port))
            cnt++;
        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::numActive(Servent::TYPE tp)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int cnt=0;

    Servent *s = servents;
    while (s)
    {
        std::lock_guard<std::recursive_mutex> cs(s->lock);

        if (s->thread.active() && s->sock && (s->type == tp))
            cnt++;
        s = s->next;
    }
    return cnt;
}

// -----------------------------------
unsigned int ServMgr::totalOutput(bool all)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    unsigned int tot = 0;
    Servent *s = servents;
    while (s)
    {
        if (s->isConnected())
            if (all || !s->isPrivate())
                if (s->sock)
                    tot += s->sock->bytesOutPerSec();
        s = s->next;
    }

    return tot;
}

// -----------------------------------
void ServMgr::quit()
{
    LOG_DEBUG("ServMgr is quitting..");

    serverThread.shutdown();
    idleThread.shutdown();

    LOG_DEBUG("Disabling RMTP server..");
    rtmpServerMonitor.disable();

    Servent *s = servents;
    while (s)
    {
        if (s->thread.active())
            s->thread.shutdown();

        s = s->next;
    }
}

// -----------------------------------
bool ServMgr::checkForceIP()
{
    if (!forceIP.isEmpty())
    {
        IP newIP(ClientSocket::getIP(forceIP.cstr()));
        if (serverHost.ip != newIP)
        {
            serverHost.ip = newIP;
            LOG_DEBUG("Server IP changed to %s", serverHost.str().c_str());
            return true;
        }
    }
    return false;
}

// -----------------------------------
void ServMgr::checkFirewall()
{
    if (this->rootHost.isEmpty())
    {
        LOG_ERROR("Root server is not set.");
    }else
    {
        LOG_DEBUG("Checking firewall..");
        Host host;
        host.fromStrName(this->rootHost.cstr(), DEFAULT_PORT);

        if (!host.ip.isIPv4Mapped()) {
            throw GeneralException("Not an IPv4 addresss");
        } else {
            LOG_DEBUG("Contacting %s (%s) ...", this->rootHost.c_str(), host.ip.str().c_str());
        }

        auto sock = sys->createSocket();
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
    }
}

// -----------------------------------
ServMgr::FW_STATE ServMgr::getFirewall(int ipv)
{
    if (ipv != 4 && ipv != 6)
        throw ArgumentException("getFirewall: Invalid IP version");

    if (this->flags.get("forceFirewalled"))
        return FW_ON;
        
    std::lock_guard<std::recursive_mutex> cs(lock);
    if (ipv == 4)
        return firewalled;
    else
        return firewalledIPv6;
}

// -----------------------------------
const char* ServMgr::getFirewallStateString(FW_STATE state)
{
    switch (state)
    {
    case FW_ON:
        return "ON";
    case FW_OFF:
        return "OFF";
    case FW_UNKNOWN:
    default:
        return "UNKNOWN";
    }
}

// -----------------------------------
void ServMgr::setFirewall(int ipv, FW_STATE state)
{
    if (ipv != 4 && ipv != 6)
        throw ArgumentException("setFirewall: Invalid IP version");

    std::lock_guard<std::recursive_mutex> cs(lock);

    auto str = getFirewallStateString(state);

    if (ipv == 4) {
        if (firewalled != state)
        {
            LOG_DEBUG("Firewall is set to %s (IPv4)", str);
            firewalled = state;
        }
    }else {
        if (firewalledIPv6 != state)
        {
            LOG_DEBUG("Firewall is set to %s (IPv6)", str);
            firewalledIPv6 = state;
        }
    }
}

// -----------------------------------
void ServMgr::checkFirewallIPv6()
{
    /* IPv6 で繋げられるYPが出現したら、以下のやり方で十分だろう。 */
#if 0
    if ((getFirewallIPv6() == FW_UNKNOWN) && !this->rootHost.isEmpty())
    {
        LOG_DEBUG("Checking firewall..");
        Host host;
        host.fromStrName(this->rootHost.cstr(), DEFAULT_PORT);

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
#endif

    IPv6PortChecker checker;

    try {
        LOG_DEBUG("Checking firewall.. (IPv6)");
        auto result = checker.run({serverHost.port});
        if (result.ports.size()) {
            setFirewall(6, FW_OFF);
        } else {
            setFirewall(6, FW_ON);
        }
        updateIPAddress(result.ip);
    } catch (SockException& e) {
        // network unreachable etc
        LOG_ERROR("checkFirewallIPv6: %s", e.what());
        setFirewall(6, FW_UNKNOWN);
    }
}

// -----------------------------------
bool ServMgr::isBlacklisted(const Host& h)
{
    if (flags.get("banTrackersWhileBroadcasting") && chanMgr->isBroadcasting())
    {
        for (auto& entry : channelDirectory->channels())
        {
            auto tracker = Host::fromString(entry.tip);
            if (tracker.ip && (tracker.ip == h.ip))
            {
                LOG_INFO("%s(%s) is being blocked", h.ip.str().c_str(), entry.name.c_str());
                return true;
            }
        }
    }
    return false;
}

// -----------------------------------
bool ServMgr::isFiltered(int fl, Host &h)
{
    if ((fl & ServFilter::F_BAN) && isBlacklisted(h))
        return true;

    for (int i=0; i<numFilters; i++)
        if (filters[i].matches(fl, h))
            return true;

    return false;
}

// --------------------------------------------------
static ini::Section writeServHost(ServHost &sh)
{
    return {
        "Host",
        {
            {"type", ServHost::getTypeStr(sh.type)},
            {"address", sh.host.str()},
            {"time", sh.time},
        },
        "End"
    };
}

// --------------------------------------------------
static ini::Section writeRelayChannel(std::shared_ptr<Channel> c)
{
    ini::Section sec("RelayChannel", {}, "End");
    auto& keys = sec.keys;

    keys.emplace_back("name", c->getName());
    keys.emplace_back("desc", c->info.desc);
    keys.emplace_back("genre", c->info.genre);
    keys.emplace_back("contactURL", c->info.url);
    keys.emplace_back("comment", c->info.comment);
    if (!c->sourceURL.isEmpty())
        keys.emplace_back("sourceURL", c->sourceURL);
    keys.emplace_back("sourceProtocol", ChanInfo::getProtocolStr(c->info.srcProtocol));
    keys.emplace_back("contentType", c->info.getTypeStr());
    keys.emplace_back("MIMEType", c->info.MIMEType);
    keys.emplace_back("streamExt", c->info.streamExt);
    keys.emplace_back("bitrate", c->info.bitrate);
    keys.emplace_back("id", c->info.id.str());
    keys.emplace_back("stayConnected", c->stayConnected);

    // トラッカーIPの書き出し。
    auto chl = chanMgr->findHitListByID(c->info.id);
    if (chl)
    {
        ChanHitSearch chs;
        chs.trackersOnly = true;
        if (chl->pickHits(chs))
        {
            keys.emplace_back("tracker", chs.best[0].host.str());
        }
    }

    // トラック情報の書き出し。
    keys.emplace_back("trackContact", c->info.track.contact);
    keys.emplace_back("trackTitle", c->info.track.title);
    keys.emplace_back("trackArtist", c->info.track.artist);
    keys.emplace_back("trackAlbum", c->info.track.album);
    keys.emplace_back("trackGenre", c->info.track.genre);

    keys.emplace_back("ipVersion", c->ipVersion);

    return sec;
}

// --------------------------------------------------
void ServMgr::saveSettings(const char *fn)
{
    FileStream iniFile;
    ini::Document settings = getSettings();
    std::string tmpname = str::STR(fn, ".tmp");

    try {
        iniFile.openWriteReplace(tmpname.c_str());
    } catch (StreamException&) {
        LOG_ERROR("Unable to open ini file");
    }
    // 改行コードを設定する。
#if defined(_LINUX) || defined(__APPLE__)
    iniFile.writeCRLF = false;
#endif

    LOG_DEBUG("Saving settings to: %s", fn);
    iniFile.writeString(ini::dump(settings));
    iniFile.close();

    try {
        sys->rename(tmpname, fn);
    } catch (GeneralException& e) {
        LOG_ERROR("rename failed: %s", e.what());
    }
}

// --------------------------------------------------
ini::Document ServMgr::getSettings()
{
    std::lock_guard<std::recursive_mutex> cs1(lock);
    std::lock_guard<std::recursive_mutex> cs2(chanMgr->lock);

    ini::Document doc;

    doc.push_back(
    {
        "Server",
        {
            {"serverName", this->serverName},
            {"serverPort", this->serverHost.port},
            {"autoServe", this->autoServe},
            {"forceIP", this->forceIP},
            {"isRoot", this->isRoot},
            {"maxBitrateOut", this->maxBitrateOut},
            {"maxRelays", this->maxRelays},
            {"maxDirect", this->maxDirect},
            {"maxRelaysPerChannel", chanMgr->maxRelaysPerChannel},
            {"firewallTimeout", firewallTimeout},
            {"forceNormal", forceNormal},
            {"rootMsg", rootMsg},
            {"authType", (this->authType == ServMgr::AUTH_COOKIE) ? "cookie" : "http-basic"},
            {"cookiesExpire", (this->cookieList.neverExpire) ? "never": "session"},
            {"htmlPath", this->htmlPath},
            {"maxServIn", this->maxServIn},
            {"chanLog", this->chanLog},
            {"publicDirectory", this->publicDirectoryEnabled},
            {"networkID", networkID.str()},
        }
    });

    doc.push_back(
    {
        "Broadcast",
        {
            {"broadcastMsgInterval", chanMgr->broadcastMsgInterval},
            {"broadcastMsg", chanMgr->broadcastMsg},
            {"icyMetaInterval", chanMgr->icyMetaInterval},
            {"broadcastID", chanMgr->broadcastID.str()},
            {"hostUpdateInterval", chanMgr->hostUpdateInterval},
            {"maxControlConnections", this->maxControl},
            {"rootHost", this->rootHost},
        }
    });

    doc.push_back(
    {
        "Client",
        {
            {"refreshHTML", refreshHTML},
            {"chat", chat},
            {"relayBroadcast", this->relayBroadcast},
            {"minBroadcastTTL", chanMgr->minBroadcastTTL},
            {"maxBroadcastTTL", chanMgr->maxBroadcastTTL},
            {"pushTries", chanMgr->pushTries},
            {"pushTimeout", chanMgr->pushTimeout},
            {"maxPushHops", chanMgr->maxPushHops},
            {"transcodingEnabled", this->transcodingEnabled},
            {"preset", this->preset},
            {"audioCodec", this->audioCodec},
            {"wmvProtocol", this->wmvProtocol},
            {"preferredTheme", this->preferredTheme},
            {"accentColor", this->accentColor},
        }
    });

    doc.push_back(
    {
        "Privacy",
        {
            {"password", this->password},
            {"maxUptime", chanMgr->maxUptime},
        }
    });

    for (int i = 0; i < this->numFilters; i++)
    {
        const auto& f = this->filters[i];
        doc.push_back(
        {
            "Filter",
            {
                {"ip", f.getPattern()},
                {"private", static_cast<bool>(f.flags & ServFilter::F_PRIVATE)},
                {"ban",     static_cast<bool>(f.flags & ServFilter::F_BAN)},
                {"network", static_cast<bool>(f.flags & ServFilter::F_NETWORK)},
                {"direct",  static_cast<bool>(f.flags & ServFilter::F_DIRECT)},
            },
            "End"
        });
    }

    // チャンネルフィード
    for (auto feed : this->channelDirectory->feeds())
    {
        doc.push_back(
        {
            "Feed",
            { {"url", feed.url} },
            "End",
        });
    }

    // 帯域チェック
    for (auto url : this->uptestServiceRegistry->getURLs())
    {
        doc.push_back(
        {
            "Uptest",
            { {"url", url} },
            "End"
        });
    }

    doc.push_back(
    {
        "Notify",
        {
            {"PeerCast", static_cast<bool>(notifyMask & NT_PEERCAST)},
            {"Broadcasters", static_cast<bool>(notifyMask & NT_BROADCASTERS)},
            {"TrackInfo", static_cast<bool>(notifyMask & NT_TRACKINFO)},
        },
        "End"
    });

    doc.push_back(
    {
        "Server1",
        {
            {"allowHTML", static_cast<bool>(allowServer1 & Servent::ALLOW_HTML)},
            {"allowBroadcast", static_cast<bool>(allowServer1 & Servent::ALLOW_BROADCAST)},
            {"allowNetwork", static_cast<bool>(allowServer1 & Servent::ALLOW_NETWORK)},
            {"allowDirect", static_cast<bool>(allowServer1 & Servent::ALLOW_DIRECT)},
        },
        "End"
    });

    doc.push_back(
    {
        "Debug",
        {
            {"logLevel", logLevel()},
            {"pauseLog", pauseLog},
            {"idleSleepTime", sys->idleSleepTime},
        }
    });


    std::shared_ptr<Channel> c = chanMgr->channel;
    while (c)
    {
        if (c->isActive() && c->stayConnected)
            doc.push_back(writeRelayChannel(c));

        c = c->next;
    }

    for (int i = 0; i < ServMgr::MAX_HOSTCACHE; i++)
    {
        ServHost *sh = &this->hostCache[i];
        if (sh->type != ServHost::T_NONE)
            doc.push_back(writeServHost(*sh));
    }

    std::vector<ini::Key> keys;
    this->flags.forEachFlag([&](Flag& flag)
                            {
                                keys.emplace_back(flag.name, flag.currentValue);
                            });
    doc.push_back(
    {
        "Flags",
        keys,
        "End"
    });

    return doc;
}

// --------------------------------------------------
unsigned int readServerSettings(IniFileBase &iniFile, unsigned int a)
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
void readFilterSettings(IniFileBase &iniFile, ServFilter &sv)
{
    sv.init();

    while (iniFile.readNext())
    {
        if (iniFile.isName("[End]"))
            break;
        else if (iniFile.isName("ip"))
            sv.setPattern(iniFile.getStrValue());
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

    this->numFilters = 0;

    std::lock_guard<std::recursive_mutex> cs(this->uptestServiceRegistry->m_lock);
    this->uptestServiceRegistry->clear();

    std::lock_guard<std::recursive_mutex> cs1(this->channelDirectory->m_lock);
    this->channelDirectory->clearFeeds();

    if (iniFile.openReadOnly(fn))
    {
        while (iniFile.readNext())
        {
            // server settings
            if (iniFile.isName("serverName"))
                this->serverName = iniFile.getStrValue();
            else if (iniFile.isName("serverPort"))
                this->serverHostIPv6.port = this->serverHost.port = iniFile.getIntValue();
            else if (iniFile.isName("autoServe"))
                this->autoServe = iniFile.getBoolValue();
            else if (iniFile.isName("autoConnect"))
                this->autoConnect = iniFile.getBoolValue();
            else if (iniFile.isName("icyPassword"))     // depreciated
                strcpy(this->password, iniFile.getStrValue());
            else if (iniFile.isName("forceIP"))
                this->forceIP = iniFile.getStrValue();
            else if (iniFile.isName("isRoot"))
                this->isRoot = iniFile.getBoolValue();
            else if (iniFile.isName("broadcastID"))
            {
                chanMgr->broadcastID.fromStr(iniFile.getStrValue());
            }else if (iniFile.isName("htmlPath"))
                strcpy(this->htmlPath, iniFile.getStrValue());
            else if (iniFile.isName("maxControlConnections"))
            {
                this->maxControl = iniFile.getIntValue();
            }
            else if (iniFile.isName("maxBitrateOut"))
                this->maxBitrateOut = iniFile.getIntValue();

            else if (iniFile.isName("maxStreamsOut"))       // depreciated
                this->setMaxRelays(iniFile.getIntValue());
            else if (iniFile.isName("maxRelays"))
                this->setMaxRelays(iniFile.getIntValue());
            else if (iniFile.isName("maxDirect"))
                this->maxDirect = iniFile.getIntValue();

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
                this->maxServIn = iniFile.getIntValue();
            else if (iniFile.isName("chanLog"))
                this->chanLog.set(iniFile.getStrValue(), String::T_ASCII);
            else if (iniFile.isName("publicDirectory"))
                this->publicDirectoryEnabled = iniFile.getBoolValue();

            else if (iniFile.isName("rootMsg"))
                rootMsg.set(iniFile.getStrValue());
            else if (iniFile.isName("networkID"))
                networkID.fromStr(iniFile.getStrValue());
            else if (iniFile.isName("authType"))
            {
                const char *t = iniFile.getStrValue();
                if (Sys::stricmp(t, "cookie")==0)
                    this->authType = ServMgr::AUTH_COOKIE;
                else if (Sys::stricmp(t, "http-basic")==0)
                    this->authType = ServMgr::AUTH_HTTPBASIC;
            }else if (iniFile.isName("cookiesExpire"))
            {
                const char *t = iniFile.getStrValue();
                if (Sys::stricmp(t, "never")==0)
                    this->cookieList.neverExpire = true;
                else if (Sys::stricmp(t, "session")==0)
                    this->cookieList.neverExpire = false;
            }

            // privacy settings
            else if (iniFile.isName("password"))
                strcpy(this->password, iniFile.getStrValue());
            else if (iniFile.isName("maxUptime"))
                chanMgr->maxUptime = iniFile.getIntValue();

            // client settings

            else if (iniFile.isName("rootHost"))
            {
                this->rootHost = iniFile.getStrValue();
            }else if (iniFile.isName("deadHitAge"))
                chanMgr->deadHitAge = iniFile.getIntValue();
            else if (iniFile.isName("tryoutDelay"))
                this->tryoutDelay = iniFile.getIntValue();
            else if (iniFile.isName("refreshHTML"))
                refreshHTML = iniFile.getIntValue();
            else if (iniFile.isName("chat"))
                this->chat = iniFile.getBoolValue();
            else if (iniFile.isName("relayBroadcast"))
            {
                this->relayBroadcast = iniFile.getIntValue();
                if (this->relayBroadcast < 30)
                    this->relayBroadcast = 30;
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
            else if (iniFile.isName("transcodingEnabled"))
                this->transcodingEnabled = iniFile.getBoolValue();
            else if (iniFile.isName("preset"))
                this->preset = iniFile.getStrValue();
            else if (iniFile.isName("audioCodec"))
                this->audioCodec = iniFile.getStrValue();
            else if (iniFile.isName("wmvProtocol"))
                this->wmvProtocol = iniFile.getStrValue();
            else if (iniFile.isName("preferredTheme"))
                this->preferredTheme = iniFile.getStrValue();
            else if (iniFile.isName("accentColor"))
                this->accentColor = iniFile.getStrValue();

            // debug
            else if (iniFile.isName("logLevel"))
                logLevel(iniFile.getIntValue());
            else if (iniFile.isName("pauseLog"))
                pauseLog = iniFile.getBoolValue();
            else if (iniFile.isName("idleSleepTime"))
                sys->idleSleepTime = iniFile.getIntValue();
            else if (iniFile.isName("[Server1]"))
                allowServer1 = readServerSettings(iniFile, allowServer1);
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
                        this->channelDirectory->addFeed(iniFile.getStrValue());
                }
                feedIndex++;
            }
            else if (iniFile.isName("[Uptest]"))
            {
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("url"))
                        this->uptestServiceRegistry->addURL(iniFile.getStrValue());
                }
            }else if (iniFile.isName("[Notify]"))
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
                Channel::IP_VERSION ipv = Channel::IP_V4;

                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]"))
                        break;
                    else if (iniFile.isName("name"))
                        info.name.set(iniFile.getStrValue());
                    else if (iniFile.isName("desc"))
                        info.desc.set(iniFile.getStrValue());
                    else if (iniFile.isName("genre"))
                        info.genre.set(iniFile.getStrValue());
                    else if (iniFile.isName("contactURL"))
                        info.url.set(iniFile.getStrValue());
                    else if (iniFile.isName("comment"))
                        info.comment.set(iniFile.getStrValue());
                    else if (iniFile.isName("id"))
                        info.id.fromStr(iniFile.getStrValue());
                    else if (iniFile.isName("sourceType"))
                        info.srcProtocol = ChanInfo::getProtocolFromStr(iniFile.getStrValue());
                    else if (iniFile.isName("contentType"))
                        info.contentType = iniFile.getStrValue();
                    else if (iniFile.isName("MIMEType"))
                        info.MIMEType = iniFile.getStrValue();
                    else if (iniFile.isName("streamExt"))
                        info.streamExt = iniFile.getStrValue();
                    else if (iniFile.isName("stayConnected"))
                        stayConnected = iniFile.getBoolValue();
                    else if (iniFile.isName("sourceURL"))
                        sourceURL.set(iniFile.getStrValue());
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
                    else if (iniFile.isName("trackContact"))
                        info.track.contact = iniFile.getStrValue();
                    else if (iniFile.isName("trackTitle"))
                        info.track.title = iniFile.getStrValue();
                    else if (iniFile.isName("trackArtist"))
                        info.track.artist = iniFile.getStrValue();
                    else if (iniFile.isName("trackAlbum"))
                        info.track.album = iniFile.getStrValue();
                    else if (iniFile.isName("trackGenre"))
                        info.track.genre = iniFile.getStrValue();
                    else if (iniFile.isName("ipVersion"))
                        ipv = (iniFile.getIntValue() == 6) ? Channel::IP_V6 : Channel::IP_V4;
                }
                if (sourceURL.isEmpty())
                {
                    chanMgr->createRelay(info, stayConnected);
                }else
                {
                    info.bcID = chanMgr->broadcastID;
                    auto c = chanMgr->createChannel(info, NULL);
                    c->ipVersion = ipv;
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
                this->addHost(h, type, time);
            }

            // Experimental feature flags
            else if (iniFile.isName("[Flags]"))
            {
                while (iniFile.readNext())
                {
                    if (iniFile.isName("[End]")) {
                        break;
                    } else {
                        try {
                            this->flags.get(iniFile.getName()) = iniFile.getBoolValue();
                        } catch (std::out_of_range&) {
                            LOG_ERROR("Flag %s not found", iniFile.getName());
                        }
                    }
                }
            }
        }
    }

    ensureCatchallFilters();
}

// --------------------------------------------------
unsigned int ServMgr::numStreams(const GnuID &cid, Servent::TYPE tp, bool all)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    int cnt = 0;
    Servent *sv = servents;
    while (sv)
    {
        std::lock_guard<std::recursive_mutex> cs(sv->lock);
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
    std::lock_guard<std::recursive_mutex> cs(lock);

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

    auto ch = chanMgr->findChannelByNameID(info);
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
Servent *ServMgr::findConnection(Servent::TYPE t, const GnuID &sid)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

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

    const char *args = strstr(str, "?");
    if (args)
    {
        *strstr(str, "?") = '\0';
        args++;
    }

    info.initNameID(str);

    if (args)
    {
        while ((args = nextCGIarg(args, curr, arg)) != nullptr)
        {
            LOG_DEBUG("cmd: %s, arg: %s", curr, arg);

            if (strcmp(curr, "ip")==0)
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
                Host h = Host::fromString(arg);
                if (h.port == 0)
                    h.port = DEFAULT_PORT;
                chanMgr->addHit(h, info.id, true);
            }
        }
    }
}

// --------------------------------------------------
bool ServMgr::start()
{
    LOG_INFO("Peercast %s, %s", PCX_VERSTRING, peercastApp->getClientTypeOS());

    LOG_INFO("SessionID: %s", sessionID.str().c_str());
    LOG_INFO("BroadcastID: %s", chanMgr->broadcastID.str().c_str());

    auto v = sys->getAllIPAddresses();
    for (auto it = v.begin(); it != v.end(); ++it) {
        IP ip;
        if (IP::tryParse(*it, ip)) {
            LOG_DEBUG("New address discovered: %s", it->c_str());
            updateIPAddress(ip);
        } else {
            LOG_DEBUG("\"%s\" could not be parsed", it->c_str());
        }
    }

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
    netID = this->networkID;

    while (thread->active)
    {
        if (this->autoConnect)
        {
            if (this->needConnections() || this->forceLookup)
            {
                if (this->needHosts() || this->forceLookup)
                {
                    // do lookup to find some hosts

                    Host lh;
                    lh.fromStrName(this->connectHost, DEFAULT_PORT);

                    if (!this->findServent(lh.ip, lh.port, netID))
                    {
                        Servent *sv = this->allocServent();
                        if (sv)
                        {
                            LOG_DEBUG("Lookup: %s", this->connectHost);
                            sv->networkID = netID;
                            sv->initOutgoing(lh, Servent::T_LOOKUP);
                            this->forceLookup = false;
                        }
                    }
                }

                for (int i=0; i<MAX_TRYOUT; i++)
                {
                    if (this->outUsedFull())
                        break;
                    if (this->tryFull())
                        break;

                    ServHost sh = this->getOutgoingServent(netID);

                    if (!this->addOutgoing(sh.host, netID, false))
                        this->deadHost(sh.host, ServHost::T_SERVENT);
                    sys->sleep(this->tryoutDelay);
                    break;
                }
            }
        }else{
#if 0
            Servent *s = this->servents;
            while (s)
            {
                if (s->type == Servent::T_OUTGOING)
                    s->thread.shutdown();
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
bool    ServMgr::acceptGIV(std::shared_ptr<ClientSocket> sock)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    Servent *sv = servents;
    while (sv)
    {
        if (sv->type == Servent::T_COUT)
        {
            if (sv->acceptGIV(sock))
                return true;
        }
        sv = sv->next;
    }
    return false;
}

// -----------------------------------
int ServMgr::broadcastPushRequest(ChanHit &hit, Host &to, const GnuID &chanID, Servent::TYPE type)
{
    ChanPacket pack;
    MemoryStream pmem(pack.data, sizeof(pack.data));
    AtomStream atom(pmem);

    atom.writeParent(PCP_BCST, 10);
        atom.writeChar(PCP_BCST_GROUP, PCP_BCST_GROUP_ALL);
        atom.writeChar(PCP_BCST_HOPS, 0);
        atom.writeChar(PCP_BCST_TTL, 7);
        atom.writeBytes(PCP_BCST_DEST, hit.sessionID.id, 16);
        atom.writeBytes(PCP_BCST_FROM, this->sessionID.id, 16);
        atom.writeInt(PCP_BCST_VERSION, PCP_CLIENT_VERSION);
        atom.writeInt(PCP_BCST_VERSION_VP, PCP_CLIENT_VERSION_VP);
        atom.writeBytes(PCP_BCST_VERSION_EX_PREFIX, PCP_CLIENT_VERSION_EX_PREFIX, 2);
        atom.writeShort(PCP_BCST_VERSION_EX_NUMBER, PCP_CLIENT_VERSION_EX_NUMBER);
        atom.writeParent(PCP_PUSH, 3);
            atom.writeAddress(PCP_PUSH_IP, to.ip);
            atom.writeShort(PCP_PUSH_PORT, to.port);
            atom.writeBytes(PCP_PUSH_CHANID, chanID.id, 16);

    pack.len = pmem.pos;
    pack.type = ChanPacket::T_PCP;

    return this->broadcastPacket(pack, GnuID(), this->sessionID, hit.sessionID, type);
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

        broadcastPacket(pack, GnuID(), this->sessionID, GnuID(), Servent::T_CIN);
    }
}

// --------------------------------------------------
int ServMgr::broadcastPacket(ChanPacket &pack, const GnuID &chanID, const GnuID &srcID, const GnuID &destID, Servent::TYPE type)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

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
    sys->setThreadName("IDLE");

    unsigned int lastBroadcastConnect = 0;
    unsigned int lastRootBroadcast = 0;

    unsigned int lastForceIPCheck = 0;

    while (thread->active())
    {
        stats.update();

        unsigned int ctime = sys->getTime();

        if (!servMgr->forceIP.isEmpty())
        {
            if ((ctime - lastForceIPCheck) > 60)
            {
                if (servMgr->checkForceIP())
                {
                    chanMgr->broadcastTrackerUpdate(GnuID(), true);
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
            if ((ctime - lastRootBroadcast) > chanMgr->hostUpdateInterval)
            {
                servMgr->broadcastRootSettings(true);
                lastRootBroadcast = ctime;
            }
        }

        chanMgr->clearDeadHits(true);

        if (servMgr->shutdownTimer)
        {
            if (--servMgr->shutdownTimer <= 0)
            {
                peercastInst->saveSettings();
                peercastInst->quit();
                sys->exit();
            }
        }

        // shutdown idle channels
        if (chanMgr->numIdleChannels() > ChanMgr::MAX_IDLE_CHANNELS)
            chanMgr->closeOldestIdle();

        // チャンネル一覧を取得する。
        servMgr->channelDirectory->update();

        servMgr->rtmpServerMonitor.update();

        servMgr->uptestServiceRegistry->update();

        sys->sleep(500);
    }

    return 0;
}

// --------------------------------------------------
int ServMgr::serverProc(ThreadInfo *thread)
{
    sys->setThreadName("SERVER");

    Servent *serv = servMgr->allocServent();

    //unsigned int lastLookupTime=0;

    std::unique_lock<std::recursive_mutex> cs(servMgr->lock, std::defer_lock);
    while (thread->active())
    {
        cs.lock();
        if (servMgr->restartServer)
        {
            serv->abort();      // force close

            servMgr->restartServer = false;
        }

        if (servMgr->autoServe)
        {
            std::lock_guard<std::recursive_mutex> cs1(serv->lock);

            // サーバーが既に起動している最中に allow を書き換え続ける
            // の気持ち悪いな。
            serv->allow = servMgr->allowServer1;

            if (!serv->sock)
            {
                LOG_DEBUG("Starting servers");

                if (servMgr->forceNormal)
                    servMgr->setFirewall(4, ServMgr::FW_OFF);
                else
                    servMgr->setFirewall(4, ServMgr::FW_UNKNOWN);

                Host h = servMgr->serverHost;

                if (!serv->sock)
                    if (!serv->initServer(h))
                    {
                        LOG_ERROR("Failed to start server on port %d. Exitting...", h.port);
                        peercastInst->quit();
                        sys->exit();
                    }
            }
        }else{
            // stop server
            serv->abort();      // force close

            // cancel incoming connectuions
            Servent *s = servMgr->servents;
            while (s)
            {
                if (s->type == Servent::T_INCOMING)
                    s->thread.shutdown();
                s = s->next;
            }

            servMgr->setFirewall(4, ServMgr::FW_ON);
            servMgr->setFirewall(6, ServMgr::FW_ON);
        }

        cs.unlock();
        sys->sleepIdle();
    }

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
amf0::Value ServMgr::getState()
{
    using std::to_string;

    std::vector<amf0::Value> filterArray;
    for (int i = 0; i < this->numFilters; i++)
        filterArray.push_back(this->filters[i].getState());

    std::vector<amf0::Value> serventArray;
    for (auto sv = servents; sv != nullptr; sv = sv->next)
        serventArray.push_back(sv->getState());

    return amf0::Value::object(
        {
            {"version", PCX_VERSTRING},
            {"buildDateTime", __DATE__ " " __TIME__},
            {"uptime", String().setFromStopwatch(getUptime()).c_str()},
            {"numRelays", to_string(numStreams(Servent::T_RELAY, true))},
            {"numDirect", to_string(numStreams(Servent::T_DIRECT, true))},
            {"totalConnected", to_string(totalConnected())},
            {"numServHosts", to_string(numHosts(ServHost::T_SERVENT))},
            {"numServents", to_string(numServents())},
            {"servents", serventArray},
            {"serverName", serverName.c_str()},
            {"serverPort", to_string(serverHost.port)},
            {"serverIP", serverHost.str(false)},
            {"serverIPv6", serverHostIPv6.str(false)},
            {"ypAddress", rootHost.c_str()},
            {"password", password},
            {"isFirewalled", getFirewall(4)==FW_ON ? "1" : "0"},
            {"firewallKnown", getFirewall(4)==FW_UNKNOWN ? "0" : "1"},
            {"isFirewalledIPv6", getFirewall(6)==FW_ON ? "1" : "0"},
            {"firewallKnownIPv6", getFirewall(6)==FW_UNKNOWN ? "0" : "1"},
            {"rootMsg", rootMsg.c_str()},
            {"isRoot", to_string(isRoot ? 1 : 0)},
            {"isPrivate", "0"},
            {"forceYP", "0"},
            {"refreshHTML", to_string(refreshHTML ? refreshHTML : 0x0fffffff)},
            {"maxRelays", to_string(maxRelays)},
            {"maxDirect", to_string(maxDirect)},
            {"maxBitrateOut", to_string(maxBitrateOut)},
            {"maxControlsIn", to_string(maxControl)},
            {"maxServIn", to_string(maxServIn)},
            {"numFilters", to_string(numFilters+1)},
            {"filters", filterArray },
            {"numActive1", to_string(numActiveOnPort(serverHost.port))},
            {"numCIN", to_string(numConnected(Servent::T_CIN))},
            {"numCOUT", to_string(numConnected(Servent::T_COUT))},
            {"numIncoming", to_string(numActive(Servent::T_INCOMING))},
            {"disabled", to_string(isDisabled)},
            {"serverPort1", to_string(serverHost.port)},
            {"serverLocalIP",Host(sys->getInterfaceIPv4Address(), 0).str(false) },
            {"upgradeURL", this->downloadURL},
            {"allow", amf0::Value::object(
                    {
                        {"HTML1", (allowServer1 & Servent::ALLOW_HTML) ? "1" : "0"},
                        {"broadcasting1", (allowServer1 & Servent::ALLOW_BROADCAST) ? "1" : "0"},
                        {"network1", (allowServer1 & Servent::ALLOW_NETWORK) ? "1" : "0"},
                        {"direct1", (allowServer1 & Servent::ALLOW_DIRECT) ? "1" : "0"},
                    })},
            {"auth", amf0::Value::object(
                    {
                        {"useCookies", (authType==AUTH_COOKIE) ? "1" : "0"},
                        {"useHTTP", (authType==AUTH_HTTPBASIC) ? "1" : "0"},
                        {"useSessionCookies", (cookieList.neverExpire==false) ? "1" : "0"},
                    })},
            {"log", amf0::Value::object(
                    {
                        {"level", std::to_string(logLevel())}
                    })},
            {"lang", this->htmlPath + strlen("html/") },
            {"numExternalChannels", to_string(channelDirectory->numChannels())},
            {"numChannelFeedsPlusOne", channelDirectory->numFeeds() + 1},
            {"numChannelFeeds", channelDirectory->numFeeds()},
            {"channelDirectory", channelDirectory->getState()},
            {"uptestServiceRegistry", uptestServiceRegistry->getState()},
            {"publicDirectoryEnabled", to_string(publicDirectoryEnabled)},
            {"transcodingEnabled", to_string(this->transcodingEnabled)},
            {"preset", this->preset},
            {"audioCodec", this->audioCodec},
            {"wmvProtocol", this->wmvProtocol},
            {"defaultChannelInfo", this->defaultChannelInfo.getState()},
            {"rtmpServerMonitor", this->rtmpServerMonitor.getState()},
            {"rtmpPort", std::to_string(this->rtmpPort)},
            {"hasUnsafeFilterSettings", std::to_string(this->hasUnsafeFilterSettings())},
            {"chat", to_string(this->chat)},
            {"randomizeBroadcastingChannelID", to_string(this->flags.get("randomizeBroadcastingChannelID"))},
            {"flags", this->flags.getState()},
            {"installationDirectory", []()
                                      {
                                          try {
                                              return sys->realPath(peercastApp->getPath());
                                          } catch (GeneralException& e) {
                                              LOG_ERROR("installationDirectory: %s", e.what());
                                              return std::string("[Error]");
                                          }
                                      }()},
            {"configurationFile", peercastApp->getIniFilename()},
            {"preferredTheme", this->preferredTheme},
            {"accentColor", this->accentColor},
        });
}

// --------------------------------------------------
void ServMgr::logLevel(int newLevel)
{
    if (1 <= newLevel && newLevel <= 7) // (T_TRACE, T_OFF)
    {
        m_logLevel = newLevel;
    }else
        LOG_ERROR("Trying to set log level outside valid range. Ignored");
}

// --------------------------------------------------
bool ServMgr::hasUnsafeFilterSettings()
{
    for (int i = 0; i < this->numFilters; ++i)
    {
        if (filters[i].isGlobal() && (filters[i].flags & ServFilter::F_PRIVATE) != 0)
            return true;
    }
    return false;
}
