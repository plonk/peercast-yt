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

// ----------------------------------

const int MIN_YP_RETRY = 20;
const int MIN_TRACKER_RETRY = 10;
const int MIN_RELAY_RETRY = 5;

// ----------------------------------
class BCID : public VariableWriter
{
public:
    BCID()
    :next(0), valid(true)
    {}

    bool    writeVariable(Stream &, const String &) override;

    GnuID   id;
    String  name, email, url;
    bool    valid;
    BCID    *next;
};

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
class ServFilter : public VariableWriter
{
public:
    enum
    {
        F_PRIVATE  = 0x01,
        F_BAN      = 0x02,
        F_NETWORK  = 0x04,
        F_DIRECT   = 0x08
    };

    ServFilter() { init(); }
    void    init()
    {
        flags = 0;
        host.init();
    }
    bool    writeVariable(Stream &, const String &) override;
    bool    matches(int fl, const Host& h) const;

    Host host;
    unsigned int flags;
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
    ~ServMgr();

    bool    start();

    Servent             *findServent(unsigned int, unsigned short, GnuID &);
    Servent             *findServent(Servent::TYPE);
    Servent             *findServent(Servent::TYPE, Host &, GnuID &);
    Servent             *findOldestServent(Servent::TYPE, bool);
    Servent             *findServentByIndex(int);
    Servent             *findServentByID(int id);

    bool                writeVariable(Stream &, const String &) override;
    Servent             *allocServent();

    unsigned int        numUsed(int);
    unsigned int        numStreams(GnuID &, Servent::TYPE, bool);
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
        //return numConnected(Servent::T_OUTGOING) + numConnected(Servent::T_INCOMING);
        return numConnected();
    }
    bool                isFiltered(int, Host &h);
    bool                hasUnsafeFilterSettings();
    bool                addOutgoing(Host, GnuID &, bool);
    Servent             *findConnection(Servent::TYPE, GnuID &);

    static THREAD_PROC  serverProc(ThreadInfo *);
    static THREAD_PROC  clientProc(ThreadInfo *);
    static THREAD_PROC  trackerProc(ThreadInfo *);
    static THREAD_PROC  idleProc(ThreadInfo *);

    int                 broadcast(GnuPacket &, Servent * = NULL);
    int                 route(GnuPacket &, GnuID &, Servent * = NULL);

    XML::Node           *createServentXML();

    void                connectBroadcaster();
    void                procConnectArgs(char *, ChanInfo &);

    void                quit();
    void                closeConnections(Servent::TYPE);

    void                checkFirewall();

    // host cache
    void            addHost(Host &, ServHost::TYPE, unsigned int);
    int             getNewestServents(Host *, int, Host &);
    ServHost        getOutgoingServent(GnuID &);
    void            deadHost(Host &, ServHost::TYPE);
    unsigned int    numHosts(ServHost::TYPE);
    void            clearHostCache(ServHost::TYPE);
    bool            seenHost(Host &, ServHost::TYPE, unsigned int);

    void            setMaxRelays(int);
    void            setFirewall(FW_STATE);
    bool            checkForceIP();
    FW_STATE        getFirewall();
    void            saveSettings(const char *);
    void            doSaveSettings(IniFileBase& iniFile);
    void            loadSettings(const char *);
    void            setPassiveSearch(unsigned int);
    int             findChannel(ChanInfo &);
    bool            getChannel(char *, ChanInfo &, bool);
    void            setFilterDefaults();

    bool            acceptGIV(ClientSocket *);
    void            addVersion(unsigned int);

    void            broadcastRootSettings(bool);
    int             broadcastPushRequest(ChanHit &, Host &, GnuID &, Servent::TYPE);
    void            writeRootAtoms(AtomStream &, bool);

    int             broadcastPacket(ChanPacket &, GnuID &, GnuID &, GnuID &, Servent::TYPE type);

    void            addValidBCID(BCID *);
    void            removeValidBCID(GnuID &);
    BCID            *findValidBCID(GnuID &);
    BCID            *findValidBCID(int);

    unsigned int    getUptime()
    {
        return sys->getTime()-startTime;
    }

    bool    seenPacket(GnuPacket &);

    bool    needHosts()
    {
        return false;
        //return numHosts(ServHost::T_SERVENT) < maxTryout;
    }

    unsigned int    numActiveOnPort(int);
    unsigned int    numActive(Servent::TYPE);

    bool    needConnections()
    {
        return numConnected(Servent::T_PGNU, 60) < minGnuIncoming;
    }
    bool    tryFull()
    {
        return false;
        //return maxTryout ? numUsed(Servent::T_OUTGOING) > maxTryout: false;
    }

    bool    pubInOver()
    {
        return numConnected(Servent::T_PGNU) > maxGnuIncoming;
//      return maxIncoming ? numConnected(Servent::T_INCOMING, false) > maxIncoming : false;
    }
    bool    pubInFull()
    {
        return numConnected(Servent::T_PGNU) >= maxGnuIncoming;
//      return maxIncoming ? numConnected(Servent::T_INCOMING, false) >= maxIncoming : false;
    }

    bool    outUsedFull()
    {
        return false;
//      return maxOutgoing ? numUsed(Servent::T_OUTGOING) >= maxOutgoing: false;
    }
    bool    outOver()
    {
        return false;
//      return maxOutgoing ? numConnected(Servent::T_OUTGOING) > maxOutgoing : false;
    }

    bool    controlInFull()
    {
        return numConnected(Servent::T_CIN)>=maxControl;
    }

    bool    outFull()
    {
        return false;
//      return maxOutgoing ? numConnected(Servent::T_OUTGOING) >= maxOutgoing : false;
    }

    bool    relaysFull()
    {
        return numStreams(Servent::T_RELAY, false) >= maxRelays;
    }
    bool    relaysFull2()
    {
      return numStreams(Servent::T_RELAY, false) >= chanMgr->maxRelaysPerChannel;
    }
    bool    directFull()
    {
        return numStreams(Servent::T_DIRECT, false) >= maxDirect;
    }

    bool    bitrateFull(unsigned int br)
    {
        return maxBitrateOut ? (BYTES_TO_KBPS(totalOutput(false))+br) > maxBitrateOut  : false;
    }

    void logLevel(int newLevel);
    int logLevel()
    {
        return m_logLevel;
    }

    unsigned int    totalOutput(bool);

    ThreadInfo          serverThread;
    ThreadInfo          idleThread;

    Servent             *servents;
    std::recursive_mutex lock;

    ServHost            hostCache[MAX_HOSTCACHE];

    char                password[64];

    bool                allowGnutella;

    unsigned int        maxBitrateOut, maxControl, maxRelays, maxDirect;
    unsigned int        minGnuIncoming, maxGnuIncoming;
    unsigned int        maxServIn;

    bool                isDisabled;
    std::atomic_bool    isRoot;
    int                 totalStreams;

    Host                serverHost;
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
    int                 queryTTL;

    unsigned int        allowServer1, allowServer2;
    unsigned int        startTime;
    unsigned int        tryoutDelay;
    unsigned int        refreshHTML;
    unsigned int        relayBroadcast;

    unsigned int        notifyMask;

    BCID                *validBCID;
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

    String              serverName;

    std::string         genrePrefix;

    bool                transcodingEnabled;
    std::string         preset;
    std::string         audioCodec;

    std::string         wmvProtocol;

    RTMPServerMonitor   rtmpServerMonitor;
    uint16_t            rtmpPort;
    ChanInfo            defaultChannelInfo;
};

// ----------------------------------
extern ServMgr *servMgr;

#endif
