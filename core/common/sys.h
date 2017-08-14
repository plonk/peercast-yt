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

#include <string>
#include <atomic>
#include <mutex>
#include <thread>

#include <string.h>
#include <stdarg.h>
#include "common.h"

#define RAND(a, b) (((a = 36969 * (a & 65535) + (a >> 16)) << 16) + \
                    (b = 18000 * (b & 65535) + (b >> 16))  )
extern char *stristr(const char *s1, const char *s2);
extern char *trimstr(char *s);

#define MAX_CGI_LEN 512

#include "_string.h"
#include "varwriter.h"

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
        return RAND(a[0], a[1]);
    }

    void setSeed(int s)
    {
        a[0] = a[1] = s;
    }

    unsigned long a[2];
};
}

// ------------------------------------
class Sys : public VariableWriter
{
public:
    Sys();
    virtual ~Sys();

    virtual class ClientSocket  *createSocket() = 0;
    virtual bool            startThread(class ThreadInfo *);
    virtual void            sleep(int) = 0;
    virtual void            appMsg(long, long = 0) = 0;
    virtual unsigned int    getTime() = 0;
    virtual double          getDTime() = 0;
    virtual unsigned int    rnd() = 0;
    virtual void            getURL(const char *) = 0;
    virtual void            exit() = 0;
    virtual bool            hasGUI() = 0;
    virtual void            callLocalURL(const char *, int)=0;
    virtual void            executeFile(const char *) = 0;
    virtual void            waitThread(ThreadInfo *, int timeout = 30000) {}
    virtual void            setThreadName(ThreadInfo *, const char* name) {}

#ifdef __BIG_ENDIAN__
    unsigned short  convertEndian(unsigned short v) { return SWAP2(v); }
    unsigned int    convertEndian(unsigned int v) { return SWAP4(v); }
#else
    unsigned short  convertEndian(unsigned short v) { return v; }
    unsigned int    convertEndian(unsigned int v) { return v; }
#endif

    void    sleepIdle();

    bool writeVariable(Stream&, const String&) override;

    static char* strdup(const char *s);
    static int   stricmp(const char* s1, const char* s2);
    static int   strnicmp(const char* s1, const char* s2, size_t n);
    static char* strcpy_truncate(char* dest, size_t destsize, const char* src);

    unsigned int idleSleepTime;
    unsigned int rndSeed;

    class LogBuffer *logBuf;
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
        event = ::CreateEvent(NULL,  // no security attributes
                              TRUE,  // manual-reset
                              FALSE, // initially non-signaled
                              NULL); // anonymous
    }

    ~WEvent()
    {
        ::CloseHandle(event);
    }

    void    signal()
    {
        ::SetEvent(event);
    }

    void    wait(int timeout = 30000)
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

    void    reset()
    {
        ::ResetEvent(event);
    }

    HANDLE event;
};
#endif

#ifdef _UNIX
// ------------------------------------
#include <errno.h>

#ifdef __APPLE__
#include <sched.h>
#define _BIG_ENDIAN 1
#endif

// ------------------------------------
class WEvent
{
public:

    WEvent()
    {
    }

    void    signal()
    {
    }

    void    wait(int timeout = 30000)
    {
    }

    void    reset()
    {
    }
};
#endif

// ------------------------------------
class WLock
{
private:
    std::recursive_mutex m_mutex;

public:
    void    on()
    {
        m_mutex.lock();
    }

    void    off()
    {
        m_mutex.unlock();
    }
};

// ------------------------------------
typedef int (*THREAD_FUNC)(ThreadInfo *);
#define THREAD_PROC int
typedef std::thread THREAD_HANDLE;

class ThreadInfo
{
public:
    ThreadInfo()
        : m_active(false)
    {
        func         = NULL;
        data         = NULL;
        nativeHandle = std::thread::native_handle_type();
    }

    void    shutdown();

    bool active() { return m_active.load(); }

    std::atomic<bool>   m_active;
    THREAD_FUNC     func;
    void            *data;

    THREAD_HANDLE   handle;
    std::thread::native_handle_type nativeHandle;
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

    ~LogBuffer()
    {
        delete[] buf;
        delete[] times;
        delete[] types;
    }

    void    clear()
    {
        currLine = 0;
    }

    void                write(const char *, TYPE);
    static const char   *getTypeStr(TYPE t) { return logTypes[t]; }
    void                dumpHTML(class Stream &);

    static void         escapeHTML(char* dest, char* src);

    char            *buf;
    unsigned int    *times;
    unsigned int    currLine, maxLines, lineLen;
    TYPE            *types;
    WLock           lock;
    static          const char *logTypes[];
};

// ------------------------------------
extern Sys *sys;

// ------------------------------------

#if _BIG_ENDIAN
#define CHECK_ENDIAN2(v) v=SWAP2(v)
#define CHECK_ENDIAN3(v) v=SWAP3(v)
#define CHECK_ENDIAN4(v) v=SWAP4(v)
#else
#define CHECK_ENDIAN2(v)
#define CHECK_ENDIAN3(v)
#define CHECK_ENDIAN4(v)
#endif

// ------------------------------------
#endif
