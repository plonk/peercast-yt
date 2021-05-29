// ------------------------------------------------
// File : servmgr.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _SERVMGR_H
#define _SERVMGR_H

#include "servent.h"
#include "varwriter.h"
#include "rtmpmonit.h"
#include "inifile.h"
#include "servfilter.h"
#include "chanmgr.h"
#include "ini.h"
#include "flag.h"

// ----------------------------------

static const int DEFAULT_PORT   = 7144;

// ----------------------------------

const int MIN_YP_RETRY = 20;
const int MIN_TRACKER_RETRY = 10;
const int MIN_RELAY_RETRY = 5;

// ----------------------------------
class ServHost
{
public:
    enum TYPE
    {
        T_NONE,
        T_STREAM,
        T_CHANNEL,
        T_SERVENT,
        T_TRACKER
    };

    ServHost() { init(); }
    void init()
    {
        host.init();
        time = 0;
        type = T_NONE;
    }
    void init(Host &h, TYPE tp, unsigned int tim)
    {
        init();
        host = h;
        type = tp;
        if (tim)
            time = tim;
        else
            time = sys->getTime();
    }

    static const char   *getTypeStr(TYPE);
    static TYPE         getTypeFromStr(const char *);

    TYPE            type;
    Host            host;
    unsigned int    time;
};

// ----------------------------------
// ServMgr keeps track of Servents
class ServMgr : public VariableWriter
{
public:
    enum NOTIFY_TYPE
    {
        NT_UPGRADE          = 0x0001,
        NT_PEERCAST         = 0x0002,
        NT_BROADCASTERS     = 0x0004,
        NT_TRACKINFO        = 0x0008
    };

    enum FW_STATE
    {
        FW_OFF,
        FW_ON,
        FW_UNKNOWN
    };

    enum {
        MAX_HOSTCACHE = 100,        // max. amount of hosts in cache
        MIN_HOSTS   = 3,            // min. amount of hosts that should be kept in cache

        MAX_OUTGOING = 3,           // max. number of outgoing servents to use
        MAX_INCOMING = 6,           // max. number of public incoming servents to use
        MAX_TRYOUT   = 10,          // max. number of outgoing servents to try connect
        MIN_CONNECTED = 3,          // min. amount of connected hosts that should be kept
        MIN_RELAYS = 2,

        MAX_FILTERS = 50,

        MAX_PREVIEWTIME = 300,      // max. seconds preview per channel available (direct connections)
        MAX_PREVIEWWAIT = 300,      // max. seconds wait between previews
    };

    enum AUTH_TYPE
    {
        AUTH_COOKIE,
        AUTH_HTTPBASIC
    };

    ServMgr();
    ~ServMgr() override;

    bool    start();

    Servent             *findServent(unsigned int, unsigned short, const GnuID &);
    Servent             *findServent(Servent::TYPE);
    Servent             *findServent(Servent::TYPE, Host &, const GnuID &);
    Servent             *findOldestServent(Servent::TYPE, bool);
    Servent             *findServentByIndex(int);
    Servent             *findServentByID(int id);

    bool                writeVariable(Stream &, const String &) override;
    Servent             *allocServent();

    unsigned int        numUsed(int);
    unsigned int        numStreams(const GnuID &, Servent::TYPE, bool);
    unsigned int        numStreams(Servent::TYPE, bool);
    unsigned int        numConnected(int, bool, unsigned int);
    unsigned int        numConnected(int t, int tim = 0)
    {
        return numConnected(t, false, tim)+numConnected(t, true, tim);
    }
    unsigned int        numConnected();
    unsigned int        numServents();

    unsigned int        totalConnected()
    {
        return numConnected();
    }
    bool                isFiltered(int, Host &h);
    bool                hasUnsafeFilterSettings();

    Servent             *findConnection(Servent::TYPE, const GnuID &);

    static THREAD_PROC  serverProc(ThreadInfo *);
    static THREAD_PROC  clientProc(ThreadInfo *);
    static THREAD_PROC  trackerProc(ThreadInfo *);
    static THREAD_PROC  idleProc(ThreadInfo *);

    XML::Node           *createServentXML();

    void                connectBroadcaster();
    void                procConnectArgs(char *, ChanInfo &);

    void                quit();
    void                closeConnections(Servent::TYPE);

    void                checkFirewall();
    void                setFirewall(int, FW_STATE);
    FW_STATE            getFirewall(int);
    void                checkFirewallIPv6();

    // host cache
    void            addHost(Host &, ServHost::TYPE, unsigned int);
    int             getNewestServents(Host *, int, Host &);
    void            deadHost(Host &, ServHost::TYPE);
    unsigned int    numHosts(ServHost::TYPE);
    void            clearHostCache(ServHost::TYPE);
    bool            seenHost(Host &, ServHost::TYPE, unsigned int);

    void            setMaxRelays(int);
    bool            checkForceIP();
    void            saveSettings(const char *);
    ini::Document   getSettings();
    void            loadSettings(const char *);
    void            setPassiveSearch(unsigned int);
    int             findChannel(ChanInfo &);
    bool            getChannel(char *, ChanInfo &, bool);
    void            setFilterDefaults();

    bool            acceptGIV(std::shared_ptr<ClientSocket>);
    void            addVersion(unsigned int);

    void            broadcastRootSettings(bool);
    int             broadcastPushRequest(ChanHit &, Host &, const GnuID &, Servent::TYPE);
    void            writeRootAtoms(AtomStream &, bool);

    int             broadcastPacket(ChanPacket &, const GnuID &, const GnuID &, const GnuID &, Servent::TYPE type);

    unsigned int    getUptime()
    {
        return sys->getTime() - startTime;
    }

    bool    needHosts()
    {
        return false;
        //return numHosts(ServHost::T_SERVENT) < maxTryout;
    }

    unsigned int    numActiveOnPort(int);
    unsigned int    numActive(Servent::TYPE);

    bool    controlInFull()
    {
        return numConnected(Servent::T_CIN) >= maxControl;
    }

    bool    relaysFull()
    {
        return numStreams(Servent::T_RELAY, false) >= maxRelays;
    }
    bool    directFull()
    {
        return numStreams(Servent::T_DIRECT, false) >= maxDirect;
    }

    bool    bitrateFull(unsigned int br)
    {
        return maxBitrateOut ? (BYTES_TO_KBPS(totalOutput(false)) + br) > maxBitrateOut  : false;
    }

    void logLevel(int newLevel);
    int logLevel()
    {
        return m_logLevel;
    }

    unsigned int    totalOutput(bool);

    bool updateIPAddress(const IP& newIP);

    static const char* getFirewallStateString(FW_STATE);

    ThreadInfo          serverThread;
    ThreadInfo          idleThread;

    Servent             *servents;
    std::recursive_mutex lock;

    ServHost            hostCache[MAX_HOSTCACHE];

    char                password[64];

    unsigned int        maxBitrateOut, maxControl, maxRelays, maxDirect;
    unsigned int        maxServIn;

    bool                isDisabled;
    std::atomic_bool    isRoot;
    int                 totalStreams;

    Host                serverHost;
    Host                serverHostIPv6;
    String              rootHost;

    char                downloadURL[128];
    String              rootMsg;
    String              forceIP;
    char                connectHost[128];
    GnuID               networkID;
    unsigned int        firewallTimeout;
    std::atomic<int>    m_logLevel;
    int                 shutdownTimer;
    bool                pauseLog;
    bool                forceNormal;
    bool                useFlowControl;
    unsigned int        lastIncoming;

    bool                restartServer;
    bool                allowDirect;
    bool                autoConnect, autoServe, forceLookup;

    unsigned int        allowServer1;
    unsigned int        startTime;
    unsigned int        tryoutDelay;
    unsigned int        refreshHTML;
    unsigned int        relayBroadcast;

    unsigned int        notifyMask;

    GnuID               sessionID;

    ServFilter          filters[MAX_FILTERS];
    int                 numFilters;

    CookieList          cookieList;
    AUTH_TYPE           authType;

    char                htmlPath[128];

    std::atomic_int     serventNum;

    String              chanLog;

    const std::unique_ptr<class ChannelDirectory>
                        channelDirectory;
    bool                publicDirectoryEnabled;

    const std::unique_ptr<class UptestServiceRegistry>
                        uptestServiceRegistry;

    FW_STATE            firewalled;
    FW_STATE            firewalledIPv6;

    String              serverName;

    bool                transcodingEnabled;
    std::string         preset;
    std::string         audioCodec;

    std::string         wmvProtocol;

    RTMPServerMonitor   rtmpServerMonitor;
    uint16_t            rtmpPort;
    ChanInfo            defaultChannelInfo;

    bool                chat;

    FlagRegistory       flags;
};

// ----------------------------------
extern ServMgr *servMgr;

#endif
