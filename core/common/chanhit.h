// ------------------------------------------------
// File : chanhit.h
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

#ifndef _CHANHIT_H
#define _CHANHIT_H

#include <functional>

#include "chaninfo.h"
#include "xml.h"

class AtomStream;
class ChanHitSearch;

// ----------------------------------
class ChanHit
{
public:
    ChanHit () { init(); }

    void    init();
    void    initLocal(int numl, int numr, int nums, int uptm, bool, unsigned int, unsigned int, const Host& = Host());
    XML::Node *createXML();

    void    writeAtoms(AtomStream &, GnuID &);
    bool    writeVariable(Stream &, const String &);

    void    pickNearestIP(Host &);

    Host            host;
    Host            rhost[2];
    unsigned int    numListeners, numRelays, numHops;
    unsigned int    time, upTime, lastContact;
    unsigned int    hitID;
    GnuID           sessionID, chanID;
    unsigned int    version;
    unsigned int    oldestPos, newestPos;

    bool            firewalled;
    bool            stable;
    bool            tracker;
    bool            recv;
    bool            yp;
    bool            dead;
    bool            direct;
    bool            relay;
    bool            cin;

    // 上流ホストの情報。
    Host            uphost;
    unsigned int    uphostHops;

    unsigned int    versionVP;
    char            versionExPrefix[2];
    unsigned int    versionExNumber;

    std::string versionString();
    std::string str(bool withPort = false);

    ChanHit *next;
};

// ----------------------------------
class ChanHitList
{
public:
    ChanHitList();
    ~ChanHitList();

    int          contactTrackers(bool, int, int, int);

    ChanHit      *addHit(ChanHit &);
    void         delHit(ChanHit &);
    void         deadHit(ChanHit &);
    int          numHits();
    int          numListeners();
    int          numRelays();
    int          numFirewalled();
    int          numTrackers();
    int          closestHit();
    int          furthestHit();
    unsigned int newestHit();

    int          pickHits(ChanHitSearch &);

    bool         isUsed() { return used; }
    int          clearDeadHits(unsigned int, bool);
    XML::Node    *createXML(bool addHits = true);

    ChanHit      *deleteHit(ChanHit *);

    int          getTotalListeners();
    int          getTotalRelays();
    int          getTotalFirewalled();

    void         forEachHit(std::function<void(ChanHit*)> block);

    bool         used;
    ChanInfo     info;
    ChanHit      *hit;
    unsigned int lastHitTime;
    ChanHitList  *next;
};

// ----------------------------------
class ChanHitSearch
{
public:
    enum
    {
        MAX_RESULTS = 8
    };

    ChanHitSearch() { init(); }
    void init();

    ChanHit         best[MAX_RESULTS];
    Host            matchHost;
    unsigned int    waitDelay;
    bool            useFirewalled;
    bool            trackersOnly;
    bool            useBusyRelays, useBusyControls;
    GnuID           excludeID;
    int             numResults;
};

#endif
