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
#include <vector>
#include <memory>

#include "common.h"

#define RAND(a, b) (((a = 36969 * (a & 65535) + (a >> 16)) << 16) + \
                    (b = 18000 * (b & 65535) + (b >> 16))  )
extern char *stristr(const char *s1, const char *s2);
extern char *trimstr(char *s);

#define MAX_CGI_LEN 512

#include "_string.h"
#include "varwriter.h"

#include "ip.h"

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
class ThreadInfo;
class Sys : public VariableWriter
{
public:
    Sys();
    virtual ~Sys();

    virtual std::shared_ptr<class ClientSocket>  createSocket() = 0;
    virtual bool            startThread(class ThreadInfo *);
    virtual bool            startWaitableThread(class ThreadInfo *);
    virtual void            waitThread(ThreadInfo *);
    virtual void            sleep(int);
    virtual unsigned int    getTime();
    virtual double          getDTime() = 0;
    virtual unsigned int    rnd() = 0;
    virtual void            getURL(const char *) = 0;
    virtual void            exit() = 0;
    virtual bool            hasGUI() = 0;
    virtual void            callLocalURL(const char *, int)=0;
    virtual void            executeFile(const char *) = 0;
    void                    executeFile(const std::string& file) { executeFile(file.c_str()); }

    virtual void            setThreadName(const char* name) {}
    virtual std::string     getThreadName() { return ""; }
    std::string             getThreadIdString();

    virtual std::string     getHostname() { return "localhost"; }
    virtual std::vector<std::string> getIPAddresses(const std::string& name) { return {}; }
    virtual std::vector<std::string> getAllIPAddresses() { return {}; }
    virtual bool getHostnameByAddress(const IP& ip, std::string& out)
    {
        out = "";
        return false;
    }

    virtual IP getInterfaceIPv4Address() const;
    
#ifdef __BIG_ENDIAN__
    unsigned short  convertEndian(unsigned short v) { return SWAP2(v); }
    unsigned int    convertEndian(unsigned int v) { return SWAP4(v); }
#else
    unsigned short  convertEndian(unsigned short v) { return v; }
    unsigned int    convertEndian(unsigned int v) { return v; }
#endif

    void    sleepIdle();

    amf0::Value getState() override;

    static char* strdup(const char *s);
    static int   stricmp(const char* s1, const char* s2);
    static int   strnicmp(const char* s1, const char* s2, size_t n);
    static char* strcpy_truncate(char* dest, size_t destsize, const char* src);

    virtual std::string getExecutablePath()
    {
        throw NotImplementedException(__func__);
    }

    virtual std::string dirname(const std::string&)
    {
        throw NotImplementedException(__func__);
    }

    virtual std::string joinPath(const std::vector<std::string>&)
    {
        throw NotImplementedException(__func__);
    }

    virtual std::string realPath(const std::string& path) = 0;

    virtual std::string getDirectorySeparator()
    {
        return "/";
    }

    virtual void rename(const std::string& oldpath, const std::string& newpath) = 0;

    virtual std::string fromFilenameEncoding(const std::string& path)
    {
        return path;
    }
    virtual std::string getCurrentWorkingDirectory() = 0;

    unsigned int idleSleepTime;

    class LogBuffer *logBuf;
};

// ------------------------------------
extern Sys *sys;

// ------------------------------------
#ifdef _UNIX

#ifdef __APPLE__
#include <sched.h>
#define _BIG_ENDIAN 1
#endif

#endif

#ifdef WIN32
typedef __int64 int64_t;
#endif

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
