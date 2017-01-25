// ------------------------------------------------
// File : mp3.h
// Date: 28-may-2003
// Author: giles
//
// (c) 2002-3 peercast.org
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

#ifndef _MP3_H
#define _MP3_H


#include "channel.h"

// ----------------------------------------------
class MP3Stream : public ChannelStream
{
public:
	virtual void readHeader(Stream &,Channel *);
	virtual int	 readPacket(Stream &,Channel *);
	virtual void readEnd(Stream &,Channel *);
};


#endif 
