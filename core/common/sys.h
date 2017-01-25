// ------------------------------------------------
// File : sys.h
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

#ifndef _SYS_H
#define _SYS_H

#include <string.h>
#include "common.h"

#define RAND(a,b) (((a = 36969 * (a & 65535) + (a >> 16)) << 16) + \
                    (b = 18000 * (b & 65535) + (b >> 16))  )
extern char *stristr(const char *s1, const char *s2);
extern char *trimstr(char *s);

#define MAX_CGI_LEN 512

// ------------------------------------
class String
{
public:
	enum {
		MAX_LEN = 256
	};

	enum TYPE
	{
		T_UNKNOWN,
		T_ASCII,
		T_HTML,
		T_ESC,
		T_ESCSAFE,
		T_META,
		T_METASAFE,
		T_BASE64,
		T_UNICODE,
		T_UNICODESAFE
	};

	String() {clear();}
	String(const char *p, TYPE t=T_ASCII) 
	{
		set(p,t);
	}

	// set from straight null terminated string
	void set(const char *p, TYPE t=T_ASCII) 
	{
		strncpy(data,p,MAX_LEN-1);
		data[MAX_LEN-1] = 0;
		type = t;
	}

	// set from quoted or unquoted null terminated string
	void setFromString(const char *str, TYPE t=T_ASCII);
	
	// set from stopwatch
	void setFromStopwatch(unsigned int t);

	// set from time
	void setFromTime(unsigned int t);


	// from single word (end at whitespace)
	void setFromWord(const char *str)
	{
		int i;
		for(i=0; i<MAX_LEN-1; i++)
		{
			data[i] = *str++;
			if ((data[i]==0) || (data[i]==' '))
				break;
		}
		data[i]=0;
	}


	// set from null terminated string, remove first/last chars
	void setUnquote(const char *p, TYPE t=T_ASCII) 
	{
		int slen = strlen(p);
		if (slen > 2)
		{
			if (slen >= MAX_LEN) slen = MAX_LEN;
			strncpy(data,p+1,slen-2);
			data[slen-2]=0;
		}else
			clear();
		type = t;
	}

	void clear() 
	{
		data[0]=0;
		type = T_UNKNOWN;
	}
	void ASCII2ESC(const char *,bool);
	void ASCII2HTML(const char *);
	void ASCII2META(const char *,bool);
	void ESC2ASCII(const char *);
	void HTML2ASCII(const char *);
	void HTML2UNICODE(const char *);
	void BASE642ASCII(const char *);
	void UNKNOWN2UNICODE(const char *,bool);

	static	int	base64WordToChars(char *,const char *);

	static bool isSame(const char *s1, const char *s2) {return strcmp(s1,s2)==0;}

	bool startsWith(const char *s) const {return strncmp(data,s,strlen(s))==0;}
	bool isValidURL();
	bool isEmpty() {return data[0]==0;}
	bool isSame(::String &s) const {return strcmp(data,s.data)==0;}
	bool isSame(const char *s) const {return strcmp(data,s)==0;}
	bool contains(::String &s) {return stristr(data,s.data)!=NULL;}
	bool contains(const char *s) {return stristr(data,s)!=NULL;}
	void append(const char *s)
	{
		if ((strlen(s)+strlen(data) < (MAX_LEN-1)))
			strcat(data,s);
	}
	void append(char c)
	{
		char tmp[2];
		tmp[0]=c;
		tmp[1]=0;
		append(tmp);
	}

	void prepend(const char *s)
	{
		::String tmp;
		tmp.set(s);
		tmp.append(data);
		tmp.type = type;
		*this = tmp;
	}

	bool operator == (const char *s) const {return isSame(s);}
	bool operator != (const char *s) const {return !isSame(s);}

	operator const char *() const {return data;}

	void convertTo(TYPE t);

	char	*cstr() {return data;}

	static bool isWhitespace(char c) {return c==' ' || c=='\t';}

	TYPE	type;
	char	data[MAX_LEN];
};

// ------------------------------------
namespace peercast {
class Random {
public:
	Random(int s=0x14235465)
	{
		setSeed(s);
	}
	
	unsigned int next()
	{
		return RAND(a[0],a[1]);
	}

	void setSeed(int s)
	{
		a[0] = a[1] = s;
	}	
	
	unsigned long a[2];
};
}
// ------------------------------------
class Sys
{
public:
	Sys();



    virtual class ClientSocket	*createSocket() = 0;
	virtual bool			startThread(class ThreadInfo *) = 0;
	virtual void			sleep(int) = 0;
	virtual void			appMsg(long,long = 0) = 0;
	virtual unsigned int	getTime() = 0;		
	virtual double			getDTime() = 0;		
	virtual unsigned int	rnd() = 0;
	virtual void			getURL(const char *) = 0;
	virtual void			exit() = 0;
	virtual bool			hasGUI() = 0;
	virtual void			callLocalURL(const char *,int)=0;
	virtual void			executeFile(const char *) = 0;
	virtual void			endThread(ThreadInfo *) {}
	virtual void			waitThread(ThreadInfo *, int timeout = 30000) {}



#ifdef __BIG_ENDIAN__
	unsigned short	convertEndian(unsigned short v) { return SWAP2(v); }
	unsigned int	convertEndian(unsigned int v) { return SWAP4(v); }
#else
	unsigned short	convertEndian(unsigned short v) { return v; }
	unsigned int	convertEndian(unsigned int v) { return v; }
#endif


	void	sleepIdle();

	unsigned int idleSleepTime;
	unsigned int rndSeed;
	unsigned int numThreads;

	class LogBuffer	*logBuf;
};


#ifdef WIN32
#include <windows.h>

typedef __int64 int64_t;


// ------------------------------------
class WEvent
{
public:
	WEvent()
	{
		 event = ::CreateEvent(NULL, // no security attributes
                                  TRUE, // manual-reset
                                  FALSE,// initially non-signaled
                                  NULL);// anonymous
	}

	~WEvent()
	{
		::CloseHandle(event);
	}

	void	signal()
	{
		::SetEvent(event);
	}

	void	wait(int timeout = 30000)
	{
		switch(::WaitForSingleObject(event, timeout))
		{
          case WAIT_TIMEOUT:
              throw TimeoutException();
              break;
          //case WAIT_OBJECT_0:
              //break;
		}
	}

	void	reset()
	{
		::ResetEvent(event);
	}



	HANDLE event;
};



// ------------------------------------
typedef int (WINAPI *THREAD_FUNC)(ThreadInfo *);
typedef unsigned int THREAD_HANDLE;
#define THREAD_PROC int WINAPI
#define vsnprintf _vsnprintf

// ------------------------------------
class WLock
{
public:
	WLock()
	{
		InitializeCriticalSection(&cs);
	}


	void	on()
	{
		EnterCriticalSection(&cs);
	}

	void	off()
	{
		LeaveCriticalSection(&cs);
	}
	
	CRITICAL_SECTION cs;
};
#endif


#ifdef _UNIX
// ------------------------------------
#include <pthread.h>
#include <errno.h>

#ifdef __APPLE__
#include <sched.h>
#define _BIG_ENDIAN 1
#endif

typedef long long int64_t;

typedef int (*THREAD_FUNC)(ThreadInfo *);
#define THREAD_PROC int 
typedef pthread_t THREAD_HANDLE;

// ------------------------------------
#define stricmp strcasecmp
#define strnicmp strncasecmp


// ------------------------------------
class WEvent
{
public:

	WEvent()
	{
	}

	void	signal()
	{
	}

	void	wait(int timeout = 30000)
	{
	}

	void	reset()
	{
	}

};

// ------------------------------------
class WLock 
{
private:
	pthread_mutex_t mutex;
public:
	WLock()
	{
	    const pthread_mutexattr_t mattr = 
		{
#ifdef __APPLE__		
			PTHREAD_MUTEX_RECURSIVE
#else
			PTHREAD_MUTEX_RECURSIVE_NP
#endif
		 };
		 
        pthread_mutex_init( &mutex, &mattr );
	}

	~WLock()
	{
        pthread_mutex_destroy( &mutex );
	}


	void	on()
	{
		pthread_mutex_lock(&mutex);
	}

	void	off()
	{
		pthread_mutex_unlock(&mutex);
	}
	
};
#endif
// ------------------------------------
class ThreadInfo
{
public:
	//typedef int  (__stdcall *THREAD_FUNC)(ThreadInfo *);

	ThreadInfo()
	{
		active = false;
		id = 0;
		func = NULL;
		data = NULL;
	}

	void	shutdown();

	volatile bool	 active;
	int		id;
	THREAD_FUNC func;
	THREAD_HANDLE handle;

	void	*data;
};


// ------------------------------------
class LogBuffer 
{
public:
	enum TYPE
	{
		T_NONE,
		T_DEBUG,
		T_ERROR,
		T_NETWORK,
		T_CHANNEL,
	};

	LogBuffer(int i, int l)
	{
		lineLen = l;
		maxLines = i;
		currLine = 0;
		buf = new char[lineLen*maxLines];
		times = new unsigned int [maxLines];
		types = new TYPE [maxLines];
	}

	void	clear()
	{
		currLine = 0;
	}
	void	write(const char *, TYPE);
	static const char *getTypeStr(TYPE t) {return logTypes[t];}
	void	dumpHTML(class Stream &);

	char *buf;
	unsigned int *times;
	unsigned int currLine,maxLines,lineLen;
	TYPE	*types;
	WLock	lock;
	static	const char *logTypes[];

};


// ------------------------------------
extern Sys *sys;

// ------------------------------------


#if _BIG_ENDIAN
#define CHECK_ENDIAN2(v) v=SWAP2(v)
#define CHECK_ENDIAN3(v) v=SWAP3(v)
#define CHECK_ENDIAN4(v) v=SWAP4(v)
#else
#define CHECK_ENDIAN2
#define CHECK_ENDIAN3
#define CHECK_ENDIAN4
#endif


// ------------------------------------
#endif

