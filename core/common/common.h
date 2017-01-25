// ------------------------------------------------
// File : common.h
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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <string.h>

#ifndef NULL
#define NULL 0
#endif

// ----------------------------------
class GeneralException
{
public:
    GeneralException(const char *m, int e = 0) 
	{
		strcpy(msg,m);
		err=e;
	}
    char msg[128];
	int err;
};

// -------------------------------------
class StreamException : public GeneralException
{
public:
	StreamException(const char *m) : GeneralException(m) {}
	StreamException(const char *m,int e) : GeneralException(m,e) {}
};

// ----------------------------------
class SockException : public StreamException
{
public:
    SockException(const char *m="Socket") : StreamException(m) {}
    SockException(const char *m, int e) : StreamException(m,e) {}
};
// ----------------------------------
class EOFException : public StreamException
{
public:
    EOFException(const char *m="EOF") : StreamException(m) {}
    EOFException(const char *m, int e) : StreamException(m,e) {}
};

// ----------------------------------
class CryptException : public StreamException
{
public:
    CryptException(const char *m="Crypt") : StreamException(m) {}
    CryptException(const char *m, int e) : StreamException(m,e) {}
};


// ----------------------------------
class TimeoutException : public StreamException
{
public:
    TimeoutException(const char *m="Timeout") : StreamException(m) {}
};
// --------------------------------
class GnuID
{
public:
	bool	isSame(GnuID &gid)
	{
		for(int i=0; i<16; i++)
			if (gid.id[i] != id[i])
				return false;
		return true;
	}


	bool	isSet()
	{
		for(int i=0; i<16; i++)
			if (id[i] != 0)
				return true;
		return false;
	}

	void	clear()
	{
		for(int i=0; i<16; i++)
			id[i] = 0;
		storeTime = 0;
	}


	void	generate(unsigned char = 0);
	void	encode(class Host *, const char *,const char *,unsigned char);

	void	toStr(char *);
	void	fromStr(const char *);

	unsigned char 	getFlags();

	unsigned char id[16];
	unsigned int storeTime;
};
// --------------------------------
class GnuIDList 
{
public:
	GnuIDList(int);
	~GnuIDList();
	void	clear();
	void	add(GnuID &);
	bool	contains(GnuID &);
	int		numUsed();
	unsigned int getOldest();

	GnuID	*ids;
	int		maxID;
};


// ----------------------------------
class Host
{
    inline unsigned int ip3()
    {
        return (ip>>24);
    }
    inline unsigned int ip2()
    {
        return (ip>>16)&0xff;
    }
    inline unsigned int ip1()
    {
        return (ip>>8)&0xff;
    }
    inline unsigned int ip0()
    {
        return ip&0xff;
    }

public:
	Host(){init();}
	Host(unsigned int i, unsigned short p)
	{
		ip = i;
		port = p;
		value = 0;
	}

	void	init()
	{
		ip = 0;
		port = 0;
		value = 0;
	}


	bool	isMemberOf(Host &);

	bool	isSame(Host &h)
	{
		return (h.ip == ip) && (h.port == port);
	}

	bool classType() {return globalIP();}

	bool	globalIP()
	{
		// local host
		if ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1))
			return false;

		// class A
		if (ip3() == 10)
			return false;

		// class B
		if ((ip3() == 172) && (ip2() >= 16) && (ip2() <= 31))
			return false;

		// class C
		if ((ip3() == 192) && (ip2() == 168))
			return false;

		return true;
	}
	bool	localIP()
	{
		return !globalIP();
	}

	bool	loopbackIP()
	{
//		return ((ipByte[3] == 127) && (ipByte[2] == 0) && (ipByte[1] == 0) && (ipByte[0] == 1));
		return ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1));
	}

	bool	isValid()
	{
		return (ip != 0);
	}


	bool	isSameType(Host &h)
	{
			return ( (globalIP() && h.globalIP()) ||
			         (!globalIP() && !h.globalIP()) ); 
	}

	void	IPtoStr(char *str)
	{
		sprintf(str,"%d.%d.%d.%d",(ip>>24)&0xff,(ip>>16)&0xff,(ip>>8)&0xff,(ip)&0xff);
	}

	void	toStr(char *str)
	{
		sprintf(str,"%d.%d.%d.%d:%d",(ip>>24)&0xff,(ip>>16)&0xff,(ip>>8)&0xff,(ip)&0xff,port);
	}

	void	fromStrIP(const char *,int);
	void	fromStrName(const char *,int);

	bool	isLocalhost();


	union
	{
		unsigned int ip;
//		unsigned char ipByte[4];
	};

    unsigned short port;
	unsigned int value;
};
// ----------------------------------
#define SWAP2(v) ( ((v&0xff)<<8) | ((v&0xff00)>>8) )
#define SWAP3(v) (((v&0xff)<<16) | ((v&0xff00)) | ((v&0xff0000)>>16) )
#define SWAP4(v) (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24))
#define TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c)+'A'-'a' : (c))
#define TONIBBLE(c) ((((c) >= 'A')&&((c) <= 'F')) ? (((c)-'A')+10) : ((c)-'0'))
#define BYTES_TO_KBPS(n) (float)(((((float)n)*8.0f)/1024.0f))

// ----------------------------------
inline bool isWhiteSpace(char c)
{
	return (c == ' ') || (c == '\r') || (c == '\n') || (c == '\t');
}

// ----------------------------------
inline int strToID(char *str)
{
	union {
    	int i;
        char s[8];
    };
    strncpy(s,str,4);
    return i;
}

// -----------------------------------
char *getCGIarg(const char *str, const char *arg);
bool cmpCGIarg(char *str, char *arg, char *value);
bool hasCGIarg(char *str, char *arg);

// ----------------------------------
extern void LOG(const char *fmt,...);

extern void LOG_ERROR(const char *fmt,...);
extern void LOG_DEBUG(const char *fmt,...);
extern void LOG_NETWORK(const char *fmt,...);
extern void LOG_CHANNEL(const char *fmt,...);


#endif

