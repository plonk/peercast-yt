// ------------------------------------------------
// File : sys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Sys is a base class for all things systemy, like starting threads, creating sockets etc..
//      Lock is a very basic cross platform CriticalSection class
//      SJIS-UTF8 conversion by ????
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

#include "common.h"
#include "sys.h"
#include "gnutella.h"
#include "logbuf.h"
#include <chrono>

// ------------------------------------------
Sys::Sys()
{
    idleSleepTime = 10;
    logBuf = new LogBuffer(1000, 100);
}

// ------------------------------------------
Sys::~Sys()
{
    delete logBuf;
}

// ------------------------------------------
void Sys::sleep(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// ------------------------------------------
void Sys::sleepIdle()
{
    sleep(idleSleepTime);
}

// -----------------------------------
bool Sys::writeVariable(Stream& s, const String& varName)
{
    if (varName == "log.dumpHTML")
    {
        logBuf->dumpHTML(s);
        return true;
    }else if (varName == "time")
    {
        s.writeString(std::to_string(sys->getTime()).c_str());
        return true;
    }

    return false;
}

// -----------------------------------
char *trimstr(char *s1)
{
    while (*s1)
    {
        if ((*s1 == ' ') || (*s1 == '\t'))
            s1++;
        else
            break;
    }

    char *s = s1;

    s1 = s1+strlen(s1);

    while (--s1 >= s)
        if ((*s1 != ' ') && (*s1 != '\t'))
            break;

    s1[1] = 0;

    return s;
}

// -----------------------------------
char *stristr(const char *s1, const char *s2)
{
    while (*s1)
    {
        if (TOUPPER(*s1) == TOUPPER(*s2))
        {
            const char *c1 = s1;
            const char *c2 = s2;

            while (*c1 && *c2)
            {
                if (TOUPPER(*c1) != TOUPPER(*c2))
                    break;
                c1++;
                c2++;
            }
            if (*c2==0)
                return (char *)s1;
        }

        s1++;
    }
    return NULL;
}

// -----------------------------------
const char *getCGIarg(const char *str, const char *arg)
{
    if (!str)
        return NULL;

    const char *s = strstr(str, arg);

    if (!s)
        return NULL;

    s += strlen(arg);

    return s;
}

// -----------------------------------
bool cmpCGIarg(const char *str, const char *arg, const char *value)
{
    if ((!str) || (!strlen(value)))
        return false;

    if (Sys::strnicmp(str, arg, strlen(arg)) == 0)
    {
        str += strlen(arg);

        return strncmp(str, value, strlen(value))==0;
    }else
        return false;
}

// -----------------------------------
bool hasCGIarg(const char *str, const char *arg)
{
    if (!str)
        return false;

    const char *s = strstr(str, arg);

    if (!s)
        return false;

    return true;
}

// ---------------------------
void    ThreadInfo::shutdown()
{
    m_active.store(false);
    //sys->waitThread(this);
}

// ---------------------------
char* Sys::strdup(const char *src)
{
    size_t len = strlen(src);
    char *res = (char*) malloc(len+1);
    memcpy(res, src, len+1);
    return res;
}

// ---------------------------
int Sys::stricmp(const char* s1, const char* s2)
{
    while (*s1 && *s2 && TOUPPER(*s1) == TOUPPER(*s2))
        s1++, s2++;

    return TOUPPER(*s1) - TOUPPER(*s2);
}

// ---------------------------
int Sys::strnicmp(const char* s1, const char* s2, size_t n)
{
    while (*s1 && *s2 && n > 0 && TOUPPER(*s1) == TOUPPER(*s2))
        s1++, s2++, n--;

    if (n == 0)
        return 0;
    else
        return TOUPPER(*s1) - TOUPPER(*s2);
}

// ---------------------------
char* Sys::strcpy_truncate(char* dest, size_t destsize, const char* src)
{
    if (destsize == 0)
    {
        LOG_ERROR("strcpy_truncate: destsize == 0");
        return dest;
    }

    if (destsize < strlen(src) + 1)
    {
        LOG_ERROR("strcpy_truncate: destsize[%d bytes] not large enough to hold src[%d bytes]",
                  (int)destsize, (int)strlen(src));
    }

    strncpy(dest, src, destsize);
    dest[destsize - 1] = '\0';

    return dest;
}

// ---------------------------------
bool    Sys::startThread(ThreadInfo *info)
{
    info->m_active.store(true);

    try {
        info->handle = std::thread([info]()
                                   {
                                       sys->setThreadName("new thread");
                                       info->func(info);
                                   });
        info->handle.detach();
        return true;
    } catch (std::system_error& e)
    {
        LOG_ERROR("Error creating thread");
        return false;
    }
}

// ---------------------------------
bool    Sys::startWaitableThread(ThreadInfo *info)
{
    info->m_active.store(true);

    try {
        info->handle = std::thread([info]()
                                   {
                                       sys->setThreadName("new thread");
                                       info->func(info);
                                   });
        return true;
    } catch (std::system_error& e)
    {
        LOG_ERROR("Error creating thread");
        return false;
    }
}

// ---------------------------------
unsigned int Sys::getTime()
{
    return time(NULL);
}

// ---------------------------------
void Sys::waitThread(ThreadInfo* info)
{
    if (info->handle.joinable())
    {
        info->handle.join();
    }else
    {
        LOG_ERROR("waitThread called on non-joinable thread");
    }
}
