// ------------------------------------------------
// File : http.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      クッキー認証。
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

#include "cookie.h"
#include "host.h"
#include "sys.h"

// -----------------------------------
void    CookieList::init()
{
    for (int i=0; i<MAX_COOKIES; i++)
        list[i].clear();

    neverExpire = false;
}

// -----------------------------------
bool    CookieList::contains(Cookie &c)
{
    if ((c.id[0]) && (c.ip))
        for (int i=0; i<MAX_COOKIES; i++)
            if (list[i].compare(c))
                return true;

    return false;
}

// -----------------------------------
void    Cookie::logDebug(const char *str, int ind)
{
    Host h;
    h.ip = ip;

    LOG_DEBUG("%s %d: %s - %s", str, ind, h.str().c_str(), id);
}

// -----------------------------------
bool    CookieList::add(Cookie &c)
{
    if (contains(c))
        return false;

    unsigned int oldestTime=(unsigned int)-1;
    int oldestIndex=0;

    for (int i=0; i<MAX_COOKIES; i++)
        if (list[i].time <= oldestTime)
        {
            oldestIndex = i;
            oldestTime = list[i].time;
        }

    c.logDebug("Added cookie", oldestIndex);
    c.time = sys->getTime();
    list[oldestIndex]=c;
    return true;
}

// -----------------------------------
void    CookieList::remove(Cookie &c)
{
    for (int i=0; i<MAX_COOKIES; i++)
        if (list[i].compare(c))
            list[i].clear();
}

// -----------------------------------
bool Cookie::compare(Cookie &c)
{
    if (c.ip == ip)
        if (strcmp(c.id, id)==0)
            return true;

    return false;
}

// -----------------------------------
void Cookie::set(const char *i, const IP& nip)
{
    strncpy(id, i, sizeof(id)-1);
    id[sizeof(id)-1]=0;
    ip = nip;
}

// -----------------------------------
void Cookie::clear()
{
    time = 0;
    ip = 0;
    id[0]=0;
}

// -----------------------------------
Cookie::Cookie()
{
    clear();
}
