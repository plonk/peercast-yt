// ------------------------------------------------
// File : chanmgr.h
// Date: 4-apr-2002
// Author: giles
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

#ifndef _CHANMGR_H
#define _CHANMGR_H

#include "channel.h"

// ----------------------------------
class ChanMgr
{
public:
    enum
    {
        MAX_IDLE_CHANNELS = 8,      // max. number of channels that can be left idle
        MAX_METAINT = 8192          // must be at least smaller than ChanPacket data len (ie. about half)
    };

    ChanMgr();
    ~ChanMgr();

    Channel *deleteChannel(Channel *);

    Channel *createChannel(ChanInfo &, const char *);
    Channel *findChannelByName(const char *);
    Channel *findChannelByIndex(int);
    Channel *findChannelByMount(const char *);
    Channel *findChannelByID(const GnuID &);
    Channel *findChannelByNameID(ChanInfo &);
    Channel *findPushChannel(int);

    void    broadcastTrackerSettings();
    void    setUpdateInterval(unsigned int v);
    void    broadcastRelays(Servent *, int, int);

    int     broadcastPacketUp(ChanPacket &, GnuID &, GnuID &, GnuID &);
    void    broadcastTrackerUpdate(GnuID &, bool = false);

    bool    writeVariable(Stream &, const String &, int);

    int     findChannels(ChanInfo &, Channel **, int);
    int     findChannelsByStatus(Channel **, int, Channel::STATUS);

    int     numIdleChannels();
    int     numChannels();

    void    closeOldestIdle();
    void    closeAll();
    void    quit();

    void    addHit(Host &, GnuID &, bool);
    ChanHit *addHit(ChanHit &);
    void    delHit(ChanHit &);
    void    deadHit(ChanHit &);
    void    setFirewalled(Host &);

    ChanHitList *findHitList(ChanInfo &);
    ChanHitList *findHitListByID(GnuID &);
    ChanHitList *addHitList(ChanInfo &);

    void        clearHitLists();
    void        clearDeadHits(bool);
    int         numHitLists();

    void        setBroadcastMsg(::String &);

    Channel     *createRelay(ChanInfo &, bool);
    Channel     *findAndRelay(ChanInfo &);
    void        startSearch(ChanInfo &);

    void        playChannel(ChanInfo &);
    void        findAndPlayChannel(ChanInfo &, bool);

    bool        isBroadcasting(GnuID &);
    bool        isBroadcasting();

    int         pickHits(ChanHitSearch &);

    std::string authSecret(const GnuID& id);
    std::string authToken(const GnuID& id);

    Channel         *channel;
    ChanHitList     *hitlist;

    GnuID           broadcastID;

    ChanInfo        searchInfo;

    int             numFinds;
    ::String        broadcastMsg;
    unsigned int    broadcastMsgInterval;
    unsigned int    lastHit, lastQuery;
    unsigned int    maxUptime;
    bool            searchActive;
    unsigned int    deadHitAge;
    int             icyMetaInterval;
    int             maxRelaysPerChannel;
    WLock           lock;
    int             minBroadcastTTL, maxBroadcastTTL;
    int             pushTimeout, pushTries, maxPushHops;
    unsigned int    autoQuery;
    unsigned int    prefetchTime;
    unsigned int    lastYPConnect;
    unsigned int    icyIndex;

    unsigned int    hostUpdateInterval;
    unsigned int    bufferTime;

    GnuID           currFindAndPlayChannel;
};

// ----------------------------------

extern ChanMgr *chanMgr;

#endif
