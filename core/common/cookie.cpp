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
    list.clear();

    neverExpire = false;
}

// -----------------------------------
bool    CookieList::contains(Cookie &c)
{
    if ((c.id[0]) && (c.ip)) {
        auto it = std::find(list.begin(), list.end(), c);
        return (it != list.end());
    }

    return false;
}

// -----------------------------------
bool    CookieList::add(Cookie &c)
{
    if (contains(c))
        return false;

    list.push_back(c);
    LOG_DEBUG("Added cookie: %s - %s", c.ip.str().c_str(), c.id);
    while (list.size() > MAX_COOKIES)
        list.pop_front();
    
    return true;
}

// -----------------------------------
void    CookieList::remove(Cookie &c)
{
    list.remove(c);
}

// -----------------------------------
bool Cookie::operator ==(const Cookie &c)
{
    return (c.ip == ip && strcmp(c.id, id)==0);
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
    ip = 0;
    id[0]=0;
}

// -----------------------------------
Cookie::Cookie()
{
    clear();
}
