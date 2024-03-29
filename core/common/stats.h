// ------------------------------------------------
// File : stats.h
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

#ifndef _STATS_H
#define _STATS_H

#include "varwriter.h"
#include "threading.h"

// ------------------------------------------------------
class Stats : public VariableWriter
{
public:

    void    clear();
    void    update();

    enum STAT
    {
        NONE,

        PACKETSSTART,
        NUMQUERYIN, NUMQUERYOUT,
        NUMPINGIN, NUMPINGOUT,
        NUMPONGIN, NUMPONGOUT,
        NUMPUSHIN, NUMPUSHOUT,
        NUMHITIN, NUMHITOUT,
        NUMOTHERIN, NUMOTHEROUT,
        NUMDROPPED,
        NUMDUP,
        NUMACCEPTED,
        NUMOLD,
        NUMBAD,
        NUMHOPS1, NUMHOPS2, NUMHOPS3, NUMHOPS4, NUMHOPS5, NUMHOPS6, NUMHOPS7, NUMHOPS8, NUMHOPS9, NUMHOPS10,
        NUMPACKETSIN,
        NUMPACKETSOUT,
        NUMROUTED,
        NUMBROADCASTED,
        NUMDISCARDED,
        NUMDEAD,
        PACKETDATAIN,
        PACKETDATAOUT,
        PACKETSEND,

        BYTESIN,
        BYTESOUT,
        LOCALBYTESIN,
        LOCALBYTESOUT,

        MAX
    };

    amf0::Value    getState() override;

    void    clearRange(STAT s, STAT e)
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        for (int i=s; i<=e; i++)
            current[i] = 0;
    }
    void    clear(STAT s)
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        current[s] = 0;
    }
    void    add(STAT s, int n=1)
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        current[s] += n;
    }
    unsigned int getPerSecond(STAT s) const
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        return perSec[s];
    }
    unsigned int getCurrent(STAT s) const
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        return current[s];
    }

    unsigned int    current[Stats::MAX], last[Stats::MAX], perSec[Stats::MAX];
    unsigned int    lastUpdate;
    mutable std::recursive_mutex lock;
};

extern Stats stats;

#endif

