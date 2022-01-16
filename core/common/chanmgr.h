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
#include "varwriter.h"

class Servent;

// ----------------------------------
class ChanMgr : public VariableWriter
{
public:
    enum
    {
        MAX_IDLE_CHANNELS = 8,      // max. number of channels that can be left idle
        MAX_METAINT = 8192          // must be at least smaller than ChanPacket data len (ie. about half)
    };

    ChanMgr();
    ~ChanMgr();

    void deleteChannel(std::shared_ptr<Channel>);

    std::shared_ptr<Channel> createChannel(ChanInfo &, const char *);
    std::shared_ptr<Channel> findChannelByName(const char *);
    std::shared_ptr<Channel> findChannelByIndex(int);
    std::shared_ptr<Channel> findChannelByMount(const char *);
    std::shared_ptr<Channel> findChannelByID(const GnuID &);
    std::shared_ptr<Channel> findChannelByNameID(ChanInfo &);
    std::shared_ptr<Channel> findPushChannel(int);

    void    broadcastTrackerSettings();
    void    setUpdateInterval(unsigned int v);

    int     broadcastPacketUp(ChanPacket &, const GnuID &, const GnuID &, const GnuID &);
    void    broadcastTrackerUpdate(const GnuID &, bool = false);

    int     findChannels(ChanInfo &, std::shared_ptr<Channel> *, int);
    int     findChannelsByStatus(std::shared_ptr<Channel> *, int, Channel::STATUS);

    int     numIdleChannels();
    int     numChannels();

    void    closeIdles();
    void    closeOldestIdle();
    void    closeAll();
    void    quit();

    void    addHit(Host &, const GnuID &, bool);
    std::shared_ptr<ChanHit> addHit(ChanHit &);
    void    delHit(ChanHit &);
    void    deadHit(ChanHit &);
    void    setFirewalled(Host &);

    std::shared_ptr<ChanHitList> findHitList(ChanInfo &);
    std::shared_ptr<ChanHitList> findHitListByID(const GnuID &);
    std::shared_ptr<ChanHitList> addHitList(ChanInfo &);

    void        clearHitLists();
    void        clearDeadHits(bool);
    int         numHitLists();

    void        setBroadcastMsg(::String &);

    std::shared_ptr<Channel> createRelay(ChanInfo &, bool);
    std::shared_ptr<Channel> findAndRelay(ChanInfo &);

    void        playChannel(ChanInfo &);
    void        findAndPlayChannel(ChanInfo &, bool);

    bool        isBroadcasting(const GnuID &);
    bool        isBroadcasting();

    int         pickHits(ChanHitSearch &);

    std::string authSecret(const GnuID& id);
    std::string authToken(const GnuID& id);

    amf0::Value getState() override;

    std::shared_ptr<Channel> channel;
    std::shared_ptr<ChanHitList> hitlist;

    GnuID           broadcastID;

    ::String        broadcastMsg;
    unsigned int    broadcastMsgInterval;
    unsigned int    maxUptime;
    unsigned int    deadHitAge;
    int             icyMetaInterval;
    int             maxRelaysPerChannel;
    std::recursive_mutex lock;
    int             minBroadcastTTL, maxBroadcastTTL;
    int             pushTimeout, pushTries, maxPushHops;
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
