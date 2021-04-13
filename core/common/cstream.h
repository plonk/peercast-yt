// ------------------------------------------------
// File : cstream.h
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

#ifndef _CSTREAM_H
#define _CSTREAM_H

#include <vector>

#include "gnuid.h"
#include "atom.h"
#include "sys.h"

// ----------------------------------

class Channel;
class Stream;
class ChanPacket;

// ----------------------------------
class ChannelStream
{
public:
    ChannelStream()
    : numRelays(0)
    , numListeners(0)
    , isPlaying(false)
    , fwState(0)
    , lastUpdate(0)
    {}

    virtual ~ChannelStream() {}

    void updateStatus(std::shared_ptr<Channel>);
    bool getStatus(std::shared_ptr<Channel>, std::shared_ptr<ChanPacket> &);

    virtual void kill() {}
    virtual bool sendPacket(std::shared_ptr<ChanPacket>, const GnuID &) { return false; }
    virtual void flush(Stream &) {}
    virtual void readHeader(Stream &, std::shared_ptr<Channel>) = 0;
    virtual int  readPacket(Stream &, std::shared_ptr<Channel>) = 0;
    virtual void readEnd(Stream &, std::shared_ptr<Channel>) = 0;

    void    readRaw(Stream &, std::shared_ptr<Channel>);

    int             numRelays;
    int             numListeners;
    bool            isPlaying;
    int             fwState;
    unsigned int    lastUpdate;
};

#endif

