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
#include <map>
#include <memory>

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
        time = 0;
    }

    void    init(TYPE type, const void *data, unsigned int length, unsigned int position);

    void    writeRaw(Stream &);

    TYPE            type;
    unsigned int    len;
    unsigned int    pos;
    unsigned int    sync;
    bool            cont; // true if this is a continuation packet
    char            data[MAX_DATALEN];

    unsigned int    time;
};

// ----------------------------------
class ChanPacketBuffer
{
public:
    ChanPacketBuffer()
    {
        init();
    }

    void    init()
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        packets.clear();
        packets.resize(64);
        m_maxPackets = 64;
        m_safePackets = 56;

        lastPos = firstPos = safePos = 0;
        readPos = writePos = 0;
        accept = 0;
        lastWriteTime = 0;
    }

    void resize(unsigned int newSize);

    bool    writePacket(ChanPacket &, bool = false);
    void    readPacket(ChanPacket &);

    bool    willSkip();

    int     numPending()
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        return writePos - readPos;
    }

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

    Stat getStatistics();

    std::vector<std::shared_ptr<ChanPacket>> packets;
    volatile unsigned int   lastPos, firstPos, safePos;
    volatile unsigned int   readPos, writePos;
    unsigned int            accept;
    unsigned int            lastWriteTime;
    std::recursive_mutex    lock;

    unsigned int m_maxPackets;
    unsigned int m_safePackets;
};

#endif
