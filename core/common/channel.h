// ------------------------------------------------
// File : channel.h
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

#ifndef _CHANNEL_H
#define _CHANNEL_H

#include "sys.h"
#include "stream.h"
#include "gnutella.h"
#include "xml.h"
#include "asf.h"
#include "cstream.h"

class AtomStream;
class ChanHitSearch;

// --------------------------------------------------
struct MP3Header
{
	int lay;
	int version;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode;
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
};

// ----------------------------------
class TrackInfo
{
public:
	void	clear()
	{
		contact.clear();
		title.clear();
		artist.clear();
		album.clear();
		genre.clear();
	}

	void	convertTo(String::TYPE t)
	{
		contact.convertTo(t);
		title.convertTo(t);
		artist.convertTo(t);
		album.convertTo(t);
		genre.convertTo(t);
	}

	bool	update(TrackInfo &);

	::String	contact,title,artist,album,genre;
};



// ----------------------------------
class ChanInfo
{
public:
	enum TYPE
	{
		T_UNKNOWN,

		T_RAW,
		T_MP3,
		T_OGG,
		T_OGM,
		T_MOV,
		T_MPG,
		T_NSV,

		T_WMA,
		T_WMV,

		T_PLS,
		T_ASX
	};


	enum PROTOCOL
	{
		SP_UNKNOWN,
		SP_PEERCAST,
		SP_HTTP,
		SP_FILE,
		SP_MMS,
		SP_PCP
	};


	enum STATUS
	{
		S_UNKNOWN,
		S_PLAY
	};

	ChanInfo() {init();}

	void	init();
	void	init(const char *);
	void	init(const char *, GnuID &, TYPE, int);
	void	init(XML::Node *);
	void	initNameID(const char *);

	void	updateFromXML(XML::Node *);

	void	readTrackXML(XML::Node *);
	void	readServentXML(XML::Node *);
	bool	update(ChanInfo &);
	XML::Node *createQueryXML();
	XML::Node *createChannelXML();
	XML::Node *createRelayChannelXML();
	XML::Node *createTrackXML();
	bool	match(XML::Node *);
	bool	match(ChanInfo &);
	bool	matchNameID(ChanInfo &);

	void	writeInfoAtoms(AtomStream &atom);
	void	writeTrackAtoms(AtomStream &atom);

	void	readInfoAtoms(AtomStream &,int);
	void	readTrackAtoms(AtomStream &,int);

	unsigned int getUptime();
	unsigned int getAge();
	bool	isActive() {return id.isSet();}
	bool	isPrivate() {return bcID.getFlags() & 1;}
	static	const	char *getTypeStr(TYPE);
	static	const	char *getProtocolStr(PROTOCOL);
	static	const	char *getTypeExt(TYPE);
	static	TYPE		getTypeFromStr(const char *str);
	static	PROTOCOL	getProtocolFromStr(const char *str);

	::String	name;
	GnuID	id,bcID;
	int		bitrate;
	TYPE	contentType;
	PROTOCOL	srcProtocol;
	unsigned int lastPlayStart,lastPlayEnd;
	unsigned int numSkips;
	unsigned int createdTime;

	STATUS  status;

	TrackInfo	track;
	::String	desc,genre,url,comment;

};


// ----------------------------------
class ChanHit
{
public:
	void	init();
	void	initLocal(int numl,int numr,int nums,int uptm,bool,unsigned int,unsigned int);
	XML::Node *createXML();

	void	writeAtoms(AtomStream &,GnuID &);
	bool	writeVariable(Stream &, const String &);

	void	pickNearestIP(Host &);

	Host				host;
	Host				rhost[2];
	unsigned int		numListeners,numRelays,numHops;
	unsigned int		time,upTime,lastContact;
	unsigned int		hitID;
	GnuID				sessionID,chanID;
	unsigned int		version;
	unsigned int		oldestPos,newestPos;

	bool	firewalled:1,stable:1,tracker:1,recv:1,yp:1,dead:1,direct:1,relay:1,cin:1;

	ChanHit *next;

};
// ----------------------------------
class ChanHitList
{
public:
	ChanHitList();
	~ChanHitList();

	int		contactTrackers(bool,int,int,int);

	ChanHit	*addHit(ChanHit &);
	void	delHit(ChanHit &);
	void	deadHit(ChanHit &);
	int		numHits();
	int		numListeners();
	int		numRelays();
	int		numFirewalled();
	int		numTrackers();
	int		closestHit();
	int		furthestHit();
	unsigned int		newestHit();

	int			pickHits(ChanHitSearch &);

	bool	isUsed() {return used;}
	int		clearDeadHits(unsigned int,bool);
	XML::Node *createXML(bool addHits = true);

	ChanHit *deleteHit(ChanHit *);

	int		getTotalListeners();
	int		getTotalRelays();
	int		getTotalFirewalled();

	bool	used;
	ChanInfo	info;
	ChanHit	*hit;
	unsigned int lastHitTime;
	ChanHitList *next;


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

	ChanHit	best[MAX_RESULTS];
	Host	matchHost;
	unsigned int waitDelay;
	bool	useFirewalled;
	bool	trackersOnly;
	bool	useBusyRelays,useBusyControls;
	GnuID	excludeID;
	int		numResults;
};

// ----------------------------------
class ChanMeta
{
public:
	enum {
		MAX_DATALEN = 65536
	};

	void	init()
	{
		len = 0;
		cnt = 0;
		startPos = 0;
	}

	void	fromXML(XML &);
	void	fromMem(void *,int);
	void	addMem(void *,int);

	unsigned int len,cnt,startPos;
	char	data[MAX_DATALEN];
};



// ------------------------------------------
class RawStream : public ChannelStream
{
public:
	virtual void readHeader(Stream &,Channel *);
	virtual int readPacket(Stream &,Channel *);
	virtual void readEnd(Stream &,Channel *);

};

// ------------------------------------------
class PeercastStream : public ChannelStream
{
public:
	virtual void readHeader(Stream &,Channel *);
	virtual int  readPacket(Stream &,Channel *);
	virtual void readEnd(Stream &,Channel *);
};

// ------------------------------------------
class ChannelSource
{
public:
	virtual void stream(Channel *) = 0;

	virtual int getSourceRate() {return 0;}

};
// ------------------------------------------
class PeercastSource : public ChannelSource
{
public:


	virtual void stream(Channel *);

};


// ----------------------------------
class Channel
{
public:
	
	enum STATUS
	{
		S_NONE,
		S_WAIT,
		S_CONNECTING,
		S_REQUESTING,
		S_CLOSING,
		S_RECEIVING,
		S_BROADCASTING,
		S_ABORT,
		S_SEARCHING,
		S_NOHOSTS,
		S_IDLE,
		S_ERROR,
		S_NOTFOUND
	};

	enum TYPE
	{
		T_NONE,
		T_ALLOCATED,
		T_BROADCAST,
		T_RELAY
	};

	enum SRC_TYPE
	{
		SRC_NONE,
		SRC_PEERCAST,
		SRC_SHOUTCAST,
		SRC_ICECAST,
		SRC_URL
	};


	Channel();
	void	reset();
	void	endThread();

	void	startMP3File(char *);
	void	startGet();
	void	startICY(ClientSocket *,SRC_TYPE);
	void	startURL(const char *);


	ChannelStream	*createSource();

	void	resetPlayTime();

	bool	notFound()
	{
		return (status == S_NOTFOUND);
	}

	bool	isPlaying()
	{
		return (status == S_RECEIVING) || (status == S_BROADCASTING);
	}

	bool	isReceiving()
	{
		return (status == S_RECEIVING);
	}

	bool	isBroadcasting()
	{
		return (status == S_BROADCASTING);
	}

	bool	isFull();

	bool	checkBump();

	bool	checkIdle();
	void	sleepUntil(double);

	bool	isActive()
	{
		return type != T_NONE;
	}


	void	connectFetch();
	int		handshakeFetch();

	bool	isIdle() {return isActive() && (status==S_IDLE);}

	static THREAD_PROC	stream(ThreadInfo *);


	void	setStatus(STATUS s);
	const char  *getSrcTypeStr() {return srcTypes[srcType];}
	const char	*getStatusStr() {return statusMsgs[status];}
	const char	*getName() {return info.name.cstr();}
	GnuID	getID() {return info.id;}
	int		getBitrate() {return info.bitrate; }
	void	getIDStr(char *s) {info.id.toStr(s);}
	void	getStreamPath(char *);

	void	broadcastTrackerUpdate(GnuID &,bool = false);
	bool	sendPacketUp(ChanPacket &,GnuID &,GnuID &,GnuID &);

	bool	writeVariable(Stream &, const String &,int);

	bool	acceptGIV(ClientSocket *);

	void	updateInfo(ChanInfo &);

	int		readStream(Stream &,ChannelStream *);
	
	void	checkReadDelay(unsigned int);

	void	processMp3Metadata(char *);

	void	readHeader();

	void	startStream();

	XML::Node *createRelayXML(bool);

	void	newPacket(ChanPacket &);

	int		localListeners();
	int		localRelays();

	int		totalListeners();
	int		totalRelays();

	::String mount;
	ChanMeta	insertMeta;
	ChanPacket	headPack;

	ChanPacketBuffer	rawData;

	ChannelStream *sourceStream;
	unsigned int streamIndex;
	

	ChanInfo	info;
	ChanHit		sourceHost;

	GnuID		remoteID;


	::String  sourceURL;

	bool	bump,stayConnected;
	int		icyMetaInterval;
	unsigned int streamPos;
	bool	readDelay;

	TYPE	type;
	ChannelSource *sourceData;

	SRC_TYPE	srcType;

	MP3Header mp3Head;
	ThreadInfo	thread;

	unsigned int lastIdleTime;
	int		status;
	static	char *statusMsgs[],*srcTypes[];

	ClientSocket	*sock;
	ClientSocket	*pushSock;

	unsigned int lastTrackerUpdate;
	unsigned int lastMetaUpdate;

	double	startTime,syncTime;

	WEvent	syncEvent;

	Channel *next;
};

// ----------------------------------
class ChanMgr
{
public:
	enum {
		MAX_IDLE_CHANNELS = 8,		// max. number of channels that can be left idle
		MAX_METAINT = 8192			// must be at least smaller than ChanPacket data len (ie. about half)
		
	};


	ChanMgr();

	Channel	*deleteChannel(Channel *);

	Channel	*createChannel(ChanInfo &,const char *);
	Channel *findChannelByName(const char *);
	Channel *findChannelByIndex(int);
	Channel *findChannelByMount(const char *);
	Channel *findChannelByID(GnuID &);
	Channel	*findChannelByNameID(ChanInfo &);
	Channel *findPushChannel(int);

	void	broadcastTrackerSettings();
	void	setUpdateInterval(unsigned int v);
	void	broadcastRelays(Servent *,int,int);

	int		broadcastPacketUp(ChanPacket &,GnuID &,GnuID &,GnuID &);
	void	broadcastTrackerUpdate(GnuID &,bool = false);

	bool	writeVariable(Stream &, const String &,int);

	int		findChannels(ChanInfo &,Channel **,int);
	int		findChannelsByStatus(Channel **,int,Channel::STATUS);
	
	int		numIdleChannels();
	int		numChannels();

	void	closeOldestIdle();
	void	closeAll();
	void	quit();

	void	addHit(Host &,GnuID &,bool);
	ChanHit	*addHit(ChanHit &);
	void	delHit(ChanHit &);
	void	deadHit(ChanHit &);
	void	setFirewalled(Host &);

	ChanHitList *findHitList(ChanInfo &);
	ChanHitList *findHitListByID(GnuID &);
	ChanHitList	*addHitList(ChanInfo &);

	void		clearHitLists();
	void		clearDeadHits(bool);
	int			numHitLists();

	void		setBroadcastMsg(::String &);

	Channel		*createRelay(ChanInfo &,bool);
	Channel		*findAndRelay(ChanInfo &);
	void		startSearch(ChanInfo &);

	void		playChannel(ChanInfo &);
	void		findAndPlayChannel(ChanInfo &,bool);

	bool		isBroadcasting(GnuID &);
	bool		isBroadcasting();

	int			pickHits(ChanHitSearch &);



	Channel		*channel;
	ChanHitList	*hitlist;

	GnuID	broadcastID;

	ChanInfo	searchInfo;

	int		numFinds;
	::String	broadcastMsg;
	unsigned int		broadcastMsgInterval;
	unsigned int lastHit,lastQuery;
	unsigned int maxUptime;
	bool	searchActive;
	unsigned int		deadHitAge;
	int		icyMetaInterval;
	int		maxRelaysPerChannel;
	WLock	lock;
	int		minBroadcastTTL,maxBroadcastTTL;
	int		pushTimeout,pushTries,maxPushHops;
	unsigned int		autoQuery;
	unsigned int	prefetchTime;
	unsigned int	lastYPConnect;
	unsigned int	icyIndex;

	unsigned int	hostUpdateInterval;
	unsigned int bufferTime;

	GnuID	currFindAndPlayChannel;

};
// ----------------------------------
class PlayList
{
public:

	enum TYPE
	{
		T_NONE,
		T_SCPLS,
		T_PLS,
		T_ASX,
		T_RAM,
	};

	PlayList(TYPE t, int max)
	{
		maxURLs = max;
		numURLs = 0;
		type = t;
		urls = new ::String[max];
		titles = new ::String[max];
	}

	~PlayList()
	{
		delete [] urls;
		delete [] titles;
	}

	void	addURL(const char *url, const char *tit)
	{
		if (numURLs < maxURLs)
		{
			urls[numURLs].set(url);
			titles[numURLs].set(tit);
			numURLs++;
		}
	}
	void	addChannels(const char *,Channel **,int);
	void	addChannel(const char *,ChanInfo &);

	void	writeSCPLS(Stream &);
	void	writePLS(Stream &);
	void	writeASX(Stream &);
	void	writeRAM(Stream &);

	void	readSCPLS(Stream &);
	void	readPLS(Stream &);
	void	readASX(Stream &);

	void	read(Stream &s)
	{
		try
		{
			switch (type)
			{
				case T_SCPLS: readSCPLS(s); break;
				case T_PLS: readPLS(s); break;
				case T_ASX: readASX(s); break;
			}
		}catch(StreamException &) {}	// keep pls regardless of errors (eof isn`t handled properly in sockets)
	}

	void	write(Stream &s)
	{
		switch (type)
		{
			case T_SCPLS: writeSCPLS(s); break;
			case T_PLS: writePLS(s); break;
			case T_ASX: writeASX(s); break;
			case T_RAM: writeRAM(s); break;
		}
	}

	TYPE	type;
	int		numURLs,maxURLs;
	::String	*urls,*titles;
};

// ----------------------------------

extern ChanMgr *chanMgr;


#endif
