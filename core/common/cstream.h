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

// ----------------------------------

class Channel;
class ChanPacket;
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
		T_UNKNOWN	= 0,
		T_HEAD		= 1,
		T_DATA		= 2,
		T_META		= 4,
		T_PCP		= 16,
		T_ALL		= 0xff
	};

	ChanPacket() 
	{
		init();
	}

	void	init()
	{
		type = T_UNKNOWN;
		len = 0;
		pos = 0;
		sync = 0;
	}
	void	init(TYPE t, const void *, unsigned int , unsigned int );

	void	writeRaw(Stream &);
	void	writePeercast(Stream &);
	void	readPeercast(Stream &);


	unsigned int sync;
	unsigned int pos;
	TYPE type;
	unsigned int len;
	char data[MAX_DATALEN];

};
// ----------------------------------
class ChanPacketBuffer 
{
public:
	enum {
		MAX_PACKETS = 64,
		NUM_SAFEPACKETS = 56
	};

	void	init()
	{
		lock.on();
		lastPos = firstPos = safePos = 0;
		readPos = writePos = 0;
		accept = 0;
		lastWriteTime = 0;
		lock.off();
	}

	int copyFrom(ChanPacketBuffer &,unsigned in);

	bool	writePacket(ChanPacket &,bool = false);
	void	readPacket(ChanPacket &);

	bool	willSkip();

	int		numPending() {return writePos-readPos;}

	unsigned int	getLatestPos();
	unsigned int	getOldestPos();
	unsigned int	findOldestPos(unsigned int);
	bool	findPacket(unsigned int,ChanPacket &);
	unsigned int	getStreamPos(unsigned int);
	unsigned int	getStreamPosEnd(unsigned int);
	unsigned int	getLastSync();

	ChanPacket	packets[MAX_PACKETS];
	volatile unsigned int lastPos,firstPos,safePos;
	volatile unsigned int readPos,writePos;
	unsigned int accept;
	unsigned int lastWriteTime;
	WLock lock;
};

// ----------------------------------
class ChannelStream
{
public:
	ChannelStream()
	:numListeners(0)
	,numRelays(0) 
	,isPlaying(false)
	,fwState(0)
	,lastUpdate(0) 
	{}

	void updateStatus(Channel *);
	bool getStatus(Channel *,ChanPacket &);

	virtual void kill() {}
	virtual bool sendPacket(ChanPacket &,GnuID &) {return false;}
	virtual void flush(Stream &) {}
	virtual void readHeader(Stream &,Channel *)=0;
	virtual int  readPacket(Stream &,Channel *)=0;
	virtual void readEnd(Stream &,Channel *)=0;

	void	readRaw(Stream &,Channel *);

	int		numRelays;
	int		numListeners;
	bool	isPlaying;
	int	fwState;
	unsigned int lastUpdate;

};

#endif 

