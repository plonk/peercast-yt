// ------------------------------------------------
// File : pcp.h
// Date: 1-mar-2004
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

#ifndef _PCP_H
#define _PCP_H

// ------------------------------------------------


#include "id.h"
#include "cstream.h"

// ------------------------------------------------

class Servent;


// ------------------------------------------------

static const ID4 PCP_CONNECT		= "pcp\n";

static const ID4 PCP_OK				= "ok";

static const ID4 PCP_HELO			= "helo";
static const ID4 PCP_HELO_AGENT		= "agnt";
static const ID4 PCP_HELO_OSTYPE	= "ostp";
static const ID4 PCP_HELO_SESSIONID	= "sid";
static const ID4 PCP_HELO_PORT		= "port";
static const ID4 PCP_HELO_PING		= "ping";
static const ID4 PCP_HELO_PONG		= "pong";
static const ID4 PCP_HELO_REMOTEIP	= "rip";
static const ID4 PCP_HELO_VERSION	= "ver";
static const ID4 PCP_HELO_BCID		= "bcid";
static const ID4 PCP_HELO_DISABLE	= "dis";

static const ID4 PCP_OLEH			= "oleh";

static const ID4 PCP_MODE			= "mode";
static const ID4 PCP_MODE_GNUT06	= "gn06";

static const ID4 PCP_ROOT			= "root";
static const ID4 PCP_ROOT_UPDINT	= "uint";
static const ID4 PCP_ROOT_CHECKVER	= "chkv";
static const ID4 PCP_ROOT_URL		= "url";
static const ID4 PCP_ROOT_UPDATE	= "upd";
static const ID4 PCP_ROOT_NEXT		= "next";


static const ID4 PCP_OS_LINUX		= "lnux";
static const ID4 PCP_OS_WINDOWS		= "w32";
static const ID4 PCP_OS_OSX			= "osx";
static const ID4 PCP_OS_WINAMP		= "wamp";
static const ID4 PCP_OS_ZAURUS		= "zaur";

static const ID4 PCP_GET			= "get";
static const ID4 PCP_GET_ID			= "id";
static const ID4 PCP_GET_NAME		= "name";

static const ID4 PCP_HOST			= "host";
static const ID4 PCP_HOST_ID		= "id";
static const ID4 PCP_HOST_IP		= "ip";
static const ID4 PCP_HOST_PORT		= "port";
static const ID4 PCP_HOST_NUML		= "numl";
static const ID4 PCP_HOST_NUMR		= "numr";
static const ID4 PCP_HOST_UPTIME	= "uptm";
static const ID4 PCP_HOST_TRACKER	= "trkr";
static const ID4 PCP_HOST_CHANID	= "cid";
static const ID4 PCP_HOST_VERSION	= "ver";
static const ID4 PCP_HOST_FLAGS1	= "flg1";
static const ID4 PCP_HOST_OLDPOS	= "oldp";
static const ID4 PCP_HOST_NEWPOS	= "newp";


static const ID4 PCP_QUIT			= "quit";

static const ID4 PCP_CHAN			= "chan";
static const ID4 PCP_CHAN_ID		= "id";
static const ID4 PCP_CHAN_BCID		= "bcid";
static const ID4 PCP_CHAN_KEY		= "key";

static const ID4 PCP_CHAN_PKT		= "pkt";
static const ID4 PCP_CHAN_PKT_TYPE	= "type";
static const ID4 PCP_CHAN_PKT_POS	= "pos";
static const ID4 PCP_CHAN_PKT_HEAD	= "head";
static const ID4 PCP_CHAN_PKT_DATA	= "data";
static const ID4 PCP_CHAN_PKT_META	= "meta";

static const ID4 PCP_CHAN_INFO			= "info";
static const ID4 PCP_CHAN_INFO_TYPE		= "type";
static const ID4 PCP_CHAN_INFO_BITRATE	= "bitr";
static const ID4 PCP_CHAN_INFO_GENRE	= "gnre";
static const ID4 PCP_CHAN_INFO_NAME		= "name";
static const ID4 PCP_CHAN_INFO_URL		= "url";
static const ID4 PCP_CHAN_INFO_DESC		= "desc";
static const ID4 PCP_CHAN_INFO_COMMENT	= "cmnt";

static const ID4 PCP_CHAN_TRACK			= "trck";
static const ID4 PCP_CHAN_TRACK_TITLE	= "titl";
static const ID4 PCP_CHAN_TRACK_CREATOR	= "crea";
static const ID4 PCP_CHAN_TRACK_URL		= "url";
static const ID4 PCP_CHAN_TRACK_ALBUM	= "albm";

static const ID4 PCP_MESG				= "mesg";
static const ID4 PCP_MESG_ASCII			= "asci";		// ascii/sjis to be depreciated.. utf8/unicode is the only supported format from now.
static const ID4 PCP_MESG_SJIS			= "sjis";

static const ID4 PCP_BCST				= "bcst";	
static const ID4 PCP_BCST_TTL			= "ttl";	
static const ID4 PCP_BCST_HOPS			= "hops";	
static const ID4 PCP_BCST_FROM			= "from";	
static const ID4 PCP_BCST_DEST			= "dest";	
static const ID4 PCP_BCST_GROUP			= "grp";	
static const ID4 PCP_BCST_CHANID		= "cid";	
static const ID4 PCP_BCST_VERSION		= "vers";	

static const ID4 PCP_PUSH				= "push";	
static const ID4 PCP_PUSH_IP			= "ip";	
static const ID4 PCP_PUSH_PORT			= "port";	
static const ID4 PCP_PUSH_CHANID		= "cid";	

static const ID4 PCP_SPKT				= "spkt";

static const ID4 PCP_ATOM				= "atom";	

static const ID4 PCP_SESSIONID			= "sid";

static const int PCP_BCST_GROUP_ALL			= (char)0xff;	
static const int PCP_BCST_GROUP_ROOT		= 1;	
static const int PCP_BCST_GROUP_TRACKERS	= 2;	
static const int PCP_BCST_GROUP_RELAYS		= 4;	


static const int PCP_ERROR_QUIT			= 1000;
static const int PCP_ERROR_BCST			= 2000;
static const int PCP_ERROR_READ			= 3000;
static const int PCP_ERROR_WRITE		= 4000;
static const int PCP_ERROR_GENERAL		= 5000;

static const int PCP_ERROR_SKIP				= 1;
static const int PCP_ERROR_ALREADYCONNECTED	= 2;
static const int PCP_ERROR_UNAVAILABLE		= 3;
static const int PCP_ERROR_LOOPBACK			= 4;
static const int PCP_ERROR_NOTIDENTIFIED	= 5;
static const int PCP_ERROR_BADRESPONSE		= 6;
static const int PCP_ERROR_BADAGENT			= 7;
static const int PCP_ERROR_OFFAIR			= 8;
static const int PCP_ERROR_SHUTDOWN			= 9;
static const int PCP_ERROR_NOROOT			= 10;
static const int PCP_ERROR_BANNED			= 11;

static const int PCP_HOST_FLAGS1_TRACKER	= 0x01;
static const int PCP_HOST_FLAGS1_RELAY		= 0x02;
static const int PCP_HOST_FLAGS1_DIRECT		= 0x04;
static const int PCP_HOST_FLAGS1_PUSH		= 0x08;
static const int PCP_HOST_FLAGS1_RECV		= 0x10;
static const int PCP_HOST_FLAGS1_CIN		= 0x20;
static const int PCP_HOST_FLAGS1_PRIVATE	= 0x40;


// ----------------------------------------------
class BroadcastState 
{
public:
	BroadcastState()
	:numHops(0)
	,forMe(false) 
	,streamPos(0)
	,group(0)
	{
		chanID.clear();
		bcID.clear();
	}


	void initPacketSettings()
	{
		forMe = false;
		group = 0;
		numHops = 0;
		bcID.clear();
		chanID.clear();
	}


	GnuID chanID,bcID;
	int numHops;
	bool forMe;
	unsigned int streamPos;
	int group;
};

// ----------------------------------------------
class PCPStream : public ChannelStream
{
public:
	PCPStream(GnuID &rid) 
	:routeList(1000)
	{
		init(rid);
	}

	void	init(GnuID &);

	virtual void kill()
	{
		inData.lock.on();
		outData.lock.on();
	}

	virtual bool sendPacket(ChanPacket &,GnuID &);
	virtual void flush(Stream &);
	virtual void readHeader(Stream &,Channel *);
	virtual int readPacket(Stream &,Channel *);
	virtual void readEnd(Stream &,Channel *);

	int		readPacket(Stream &,BroadcastState &);
	void	flushOutput(Stream &in,BroadcastState &);
	static void	readVersion(Stream &);

	int		procAtom(AtomStream &,ID4,int,int,BroadcastState &);
	int		readAtom(AtomStream &,BroadcastState &);
	void	readChanAtoms(AtomStream &,int,BroadcastState &);
	void	readHostAtoms(AtomStream &, int, BroadcastState &);
	void	readPushAtoms(AtomStream &, int,BroadcastState &);

	void	readPktAtoms(Channel *,AtomStream &,int,BroadcastState &);
	void	readRootAtoms(AtomStream &, int,BroadcastState &);

	int		readBroadcastAtoms(AtomStream &,int,BroadcastState &);

	ChanPacketBuffer inData,outData;
	unsigned int lastPacketTime;
	unsigned int nextRootPacket;	

	//int	error;
	GnuIDList	routeList;
	GnuID	remoteID;

};

#endif
