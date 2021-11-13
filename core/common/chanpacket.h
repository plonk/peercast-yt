// ------------------------------------------------
// File : chanpacket.h
// Date: 12-mar-2004
// Author: giles
//
// (c) 2002-4 peercast.org
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

#ifndef _CHANPACKET_H
#define _CHANPACKET_H

#include <vector>
#include <mutex>

// ----------------------------------
class Stream;

// ----------------------------------
class ChanPacket
{
public:
    enum
    {
        MAX_DATALEN = 16384
    };

    enum TYPE
    {
        T_UNKNOWN   = 0,
        T_HEAD      = 1,
        T_DATA      = 2,
        T_META      = 4,
        T_PCP       = 16,
        T_ALL       = 0xff
    };

    ChanPacket()
    {
        init();
    }

    void    init()
    {
        type = T_UNKNOWN;
        len  = 0;
        pos  = 0;
        sync = 0;
        cont = false;
    }

    void    init(TYPE type, const void *data, unsigned int length, unsigned int position);

    void    writeRaw(Stream &);

    ChanPacket& operator=(const ChanPacket& other);

    TYPE            type;
    unsigned int    len;
    unsigned int    pos;
    unsigned int    sync;
    bool            cont; // true if this is a continuation packet
    char            data[MAX_DATALEN];
};

// ----------------------------------
class ChanPacketBuffer
{
public:
    enum {
        MAX_PACKETS = 64,
        NUM_SAFEPACKETS = 56
    };

    ChanPacketBuffer()
    {
        init();
    }

    void    init()
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        lastPos = firstPos = safePos = 0;
        readPos = writePos = 0;
        accept = 0;
        lastWriteTime = 0;
    }

    bool    writePacket(ChanPacket &, bool = false);
    void    readPacket(ChanPacket &);

    bool    willSkip();

    int     numPending() { return writePos - readPos; }

    unsigned int    getLatestPos();
    unsigned int    getOldestPos();
    unsigned int    findOldestPos(unsigned int);
    bool            findPacket(unsigned int, ChanPacket &);
    unsigned int    getStreamPos(unsigned int);
    unsigned int    getLatestNonContinuationPos();
    unsigned int    getOldestNonContinuationPos();

    struct Stat
    {
        std::vector<unsigned int> packetLengths;
        int continuations;
        int nonContinuations;
    };

    Stat getStatistics()
    {
        std::lock_guard<std::recursive_mutex> cs_(lock);

        if (writePos == 0)
            return { {}, 0, 0 };

        std::vector<unsigned int> lens;
        int cs = 0, ncs = 0;
        for (unsigned int i = firstPos; i <= lastPos; i++)
        {
            lens.push_back(packets[i % MAX_PACKETS].len);
            if (packets[i % MAX_PACKETS].cont)
                cs++;
            else
                ncs++;
        }
        return { lens, cs, ncs };
    }

    ChanPacket              packets[MAX_PACKETS];
    volatile unsigned int   lastPos, firstPos, safePos;
    volatile unsigned int   readPos, writePos;
    unsigned int            accept;
    unsigned int            lastWriteTime;
    std::recursive_mutex    lock;
};

#endif
