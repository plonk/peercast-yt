// ------------------------------------------------
// File : gnutella.h
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

#ifndef _GNUTELLA_H
#define _GNUTELLA_H

// --------------------------------
#include "stream.h"
#include "sys.h"



#define GNUTELLA_SETUP 0


// --------------------------------
static const int GNU_FUNC_PING	= 0;
static const int GNU_FUNC_PONG	= 1;
static const int GNU_FUNC_QUERY = 128;
static const int GNU_FUNC_HIT	= 129;
static const int GNU_FUNC_PUSH	= 64;

extern const char *GNU_FUNC_STR(int);

// --------------------------------
static const char *GNU_PEERCONN		= "PEERCAST CONNECT/0.1";
static const char *GNU_CONNECT 		= "GNUTELLA CONNECT/0.6";
static const char *GNU_OK 			= "GNUTELLA/0.6 200 OK";
static const char *PCX_PCP_CONNECT	= "pcp";

static const char *PCX_HS_OS 		= "x-peercast-os:";
static const char *PCX_HS_DL		= "x-peercast-download:"; 
static const char *PCX_HS_ID		= "x-peercast-id:"; 
static const char *PCX_HS_CHANNELID	= "x-peercast-channelid:"; 
static const char *PCX_HS_NETWORKID	= "x-peercast-networkid:"; 
static const char *PCX_HS_MSG		= "x-peercast-msg:"; 
static const char *PCX_HS_SUBNET	= "x-peercast-subnet:"; 
static const char *PCX_HS_FULLHIT	= "x-peercast-fullhit:"; 
static const char *PCX_HS_MINBCTTL	= "x-peercast-minbcttl:"; 
static const char *PCX_HS_MAXBCTTL	= "x-peercast-maxbcttl:"; 
static const char *PCX_HS_RELAYBC	= "x-peercast-relaybc:"; 
static const char *PCX_HS_PRIORITY	= "x-peercast-priority:"; 
static const char *PCX_HS_FLOWCTL	= "x-peercast-flowctl:"; 
static const char *PCX_HS_PCP		= "x-peercast-pcp:"; 
static const char *PCX_HS_PINGME	= "x-peercast-pingme:"; 
static const char *PCX_HS_PORT		= "x-peercast-port:"; 
static const char *PCX_HS_REMOTEIP	= "x-peercast-remoteip:"; 
static const char *PCX_HS_POS		= "x-peercast-pos:"; 
static const char *PCX_HS_SESSIONID	= "x-peercast-sessionid:"; 

// official version number sent to relay to check for updates 
static const char *PCX_OS_WIN32 	= "Win32";
static const char *PCX_OS_LINUX 	= "Linux";
static const char *PCX_OS_MACOSX 	= "Apple-OSX";
static const char *PCX_OS_WINAMP2 	= "Win32-WinAmp2";
static const char *PCX_OS_ACTIVEX 	= "Win32-ActiveX";

static const char *PCX_DL_URL		= "http://www.peercast.org/download.php"; 

// version number sent to other clients
static const char *PCX_OLDAGENT 	= "PeerCast/0.119E";





// version number used inside packets GUIDs
static const int PEERCAST_PACKETID	= 0x0000119E;

static const char *MIN_ROOTVER		= "0.119E";

static const char *MIN_CONNECTVER	= "0.119D";
static const int MIN_PACKETVER	    = 0x0000119D;

static const char *ICY_OK	= "ICY 200 OK";

// --------------------------------

static const int DEFAULT_PORT	= 7144;

// --------------------------------

class Servent;
class Channel;
class ChanHit;


// --------------------------------
class GnuPacket
{
public:


	// --------------------------------
	class Hash
	{
	public:

		bool	isSame(Hash &h)
		{
			return (idChecksum == h.idChecksum) && (dataChecksum == h.dataChecksum);
		}

		bool	isSameID(Hash &h)
		{
			return (idChecksum == h.idChecksum);
		}

		unsigned int idChecksum;
		unsigned int dataChecksum;

	};
	// --------------------------------

	enum {
		MAX_DATA = 2000
	};

	void	initPing(int);
	void	initPong(Host &, bool, GnuPacket &);
	void	initFind(const char *, class XML *,int);
	bool	initHit(Host &, Channel *, GnuPacket *,bool,bool,bool,bool,int);
	void	initPush(ChanHit &, Host &);


	void makeChecksumID();

	unsigned char func;
	unsigned char ttl;
	unsigned char hops;
	unsigned int	len;
	Hash	hash;
	GnuID	id;

	char data[MAX_DATA];
};
// --------------------------------
class GnuPacketBuffer
{
public:
	GnuPacketBuffer(int s) 
	:size(s)
	,packets(new GnuPacket[size])
	{
		reset();
	}
	~GnuPacketBuffer()
	{
		delete [] packets;
	}

	void	reset()
	{
		readPtr = writePtr = 0;
	}

	GnuPacket *curr()
	{
		if (numPending())
			return &packets[readPtr%size];
		else
			return NULL;

	}
	void	next()
	{
		readPtr++;
	}

	int findMinHop()
	{
		int min=100;
		int n = numPending();
		for(int i=0; i<n; i++)
		{
			int idx = (readPtr+i)%size;
			if (packets[idx].hops < min)
				min = packets[idx].hops;
		}
		return min;
	}
	int findMaxHop()
	{
		int max=0;
		int n = numPending();
		for(int i=0; i<n; i++)
		{
			int idx = (readPtr+i)%size;
			if (packets[idx].hops > max)
				max = packets[idx].hops;
		}
		return max;
	}

	int percentFull()
	{
		return (numPending()*100) / size;
	}

	
	int sizeOfPending()
	{
		int tot=0;
		int n = numPending();
		for(int i=0; i<n; i++)
			tot+=packets[(readPtr+i)%size].len;
		return tot;
	}

	int	numPending()
	{
		return writePtr-readPtr;
	}

	bool	write(GnuPacket &p) 
	{
		if ((writePtr-readPtr) >= size)
			return false;
		else
		{
			packets[writePtr%size] = p;
			writePtr++;
			return true;
		}
	}

	int	size;
	GnuPacket *packets;
	int	readPtr,writePtr;
};



// --------------------------------
class GnuStream : public IndirectStream
{
public:

	enum R_TYPE
	{
		R_PROCESS,
		R_DEAD,
		R_DISCARD,
		R_ACCEPTED,
		R_BROADCAST,
		R_ROUTE,
		R_DUPLICATE,
		R_BADVERSION,
		R_DROP
	};

	GnuStream()
	{
		init(NULL);
	}

	void	init(Stream *s)  
	{
		IndirectStream::init(s);
		packetsIn = packetsOut = 0;
	}

	bool	readPacket(GnuPacket &);
	void	sendPacket(GnuPacket &);
	R_TYPE	processPacket(GnuPacket &, Servent *, GnuID &);

	static const char *getRouteStr(R_TYPE);
	bool	readHit(Stream &data, ChanHit &ch,int,GnuID &);


	
	void	ping(int);

	int		packetsIn,packetsOut;
	WLock	lock;
};


#endif
