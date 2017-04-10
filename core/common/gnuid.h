// ------------------------------------------------
// File : gnuid.h
// Author: giles
//
// (c) peercast.org
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

#ifndef _GNUID_H
#define _GNUID_H

#include "common.h"

// --------------------------------
class GnuID
{
public:
    GnuID()
    {
        clear();
    }

    GnuID(const std::string& str)
    {
        clear();
        fromStr(str.c_str());
    }

    bool    isSame(const GnuID &gid) const
    {
        return memcmp(id, gid.id, 16) == 0;
    }


    bool    isSet() const
    {
        for (int i=0; i<16; i++)
            if (id[i] != 0)
                return true;
        return false;
    }

    void    clear()
    {
        memset(id, 0, 16);
        storeTime = 0;
    }

    operator std::string ()
    {
        return this->str();
    }

    std::string str()
    {
        char buf[33];
        toStr(buf);
        return buf;
    }

    void    generate(unsigned char = 0);
    void    encode(class Host *, const char *, const char *, unsigned char);

    void    toStr(char *);
    void    fromStr(const char *);

    unsigned char   getFlags();

    unsigned char id[16];
    unsigned int storeTime;
};

// --------------------------------
class GnuIDList
{
public:
    GnuIDList(int);
    ~GnuIDList();

    void            clear();
    void            add(GnuID &);
    bool            contains(GnuID &);
    int             numUsed();
    unsigned int    getOldest();

    GnuID   *ids;
    int     maxID;
};

#endif
