// ------------------------------------------------
// File : cookie.h
// Date: 4-apr-2002
// Author: giles
// Desc:
//         クッキー認証。
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

#ifndef _COOKIE_H
#define _COOKIE_H

#include <list>
#include "ip.h"

// --------------------------------------------
class Cookie
{
public:
    Cookie();

    void    clear();
    void    set(const char *i, const IP& nip);
    bool    operator ==(const Cookie &c);

    IP            ip;
    char          id[64];
};

// --------------------------------------------
class CookieList
{
public:
    enum {
        MAX_COOKIES = 32
    };

    void    init();
    bool    add(Cookie &);
    void    remove(Cookie &);
    bool    contains(Cookie &);

    std::list<Cookie> list;
    bool    neverExpire;
};

#endif
