// ------------------------------------------------
// File : ogg.h
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

#ifndef _OGG_H
#define _OGG_H


#include "channel.h"
// ----------------------------------------------
class OggPage;

// ----------------------------------
class OggPacket
{
public:
	enum 
	{
		MAX_BODYLEN = 65536,		// probably too small
		MAX_PACKETS = 256			// prolly too small too, but realloc?!?!?!
	};

	void	addLacing(OggPage &);

	int	bodyLen;
	unsigned char body[MAX_BODYLEN];


	int	numPackets;
	unsigned int packetSizes[MAX_PACKETS];
};

// ----------------------------------------------
class OggSubStream
{
public:
	OggSubStream()
	:maxHeaders(0),serialNo(0),bitrate(0)
	{}

	bool needHeader()
	{
		return maxHeaders && (pack.numPackets < maxHeaders);
	}

	void eos()
	{
		maxHeaders=0;
		serialNo=0;
	}

	void bos(unsigned int ser)
	{
		maxHeaders = 3;
		pack.numPackets=0;
		pack.packetSizes[0]=0;
		pack.bodyLen = 0;
		serialNo = ser;
		bitrate = 0;
	}

	bool	isActive() {return serialNo!=0;}

	void readHeader(Channel *,OggPage &);

	virtual void procHeaders(Channel *) = 0;

	int	bitrate;

	OggPacket	pack;
	int	maxHeaders;
	unsigned int serialNo;
};
// ----------------------------------------------
class OggVorbisSubStream : public OggSubStream
{
public:
	OggVorbisSubStream()
	:samplerate(0)
	{}

	virtual void procHeaders(Channel *);

	void	readIdent(Stream &, ChanInfo &);
	void	readSetup(Stream &);
	void	readComment(Stream &, ChanInfo &);

	double	getTime(OggPage &);

	int samplerate;

};
// ----------------------------------------------
class OggTheoraSubStream : public OggSubStream
{
public:
	OggTheoraSubStream() : granposShift(0), frameTime(0) {}

	virtual void procHeaders(Channel *);

	void readInfo(Stream &, ChanInfo &);

	double	getTime(OggPage &);

	int granposShift;
	double frameTime;
};

// ----------------------------------------------
class OGGStream : public ChannelStream
{
public:
	OGGStream()
	{}


	virtual void readHeader(Stream &,Channel *);
	virtual int readPacket(Stream &,Channel *);
	virtual void readEnd(Stream &,Channel *);


	void	readHeaders(Stream &,Channel *, OggPage &);

	OggVorbisSubStream	vorbis;
	OggTheoraSubStream	theora;	
};

// ----------------------------------
class OggPage
{
public:
	enum 
	{
		MAX_BODYLEN = 65536,
		MAX_HEADERLEN = 27+256
	};

	void	read(Stream &);
	bool	isBOS();
	bool	isEOS();
	bool	isNewPacket();
	bool	isHeader();
	unsigned int getSerialNo();

	bool	detectVorbis();
	bool	detectTheora();


	int64_t granPos;
	int headLen,bodyLen;
	unsigned char data[MAX_HEADERLEN+MAX_BODYLEN];
};


#endif 
