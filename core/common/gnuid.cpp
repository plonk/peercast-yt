// ------------------------------------------------
// File : gnuid.cpp
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

#include <algorithm>
#include "gnuid.h" // Host std::string
#include "sys.h" // sys

// ---------------------------
void GnuID::encode(Host *h, const char *salt1, const char *salt2, unsigned char salt3)
{
    for (int i=0; i<16; i++)
    {
        unsigned char ipb = id[i];

        // encode with IP address
        if (h)
            ipb ^= ((unsigned char *)&h->ip)[i&3];

        // plus some pepper
        ipb ^= salt3;

        id[i] = ipb;
    }

    int s1 = 0, s2 = 0;
    int n = ((std::max)(salt1?strlen(salt1):0, salt2?strlen(salt2):0)/16 + 1) * 16;
    for (int i = 0; i < n; i++)
    {
        unsigned char ipb = id[i%16];

        // add a bit of salt
        if (salt1)
        {
            if (salt1[s1])
                ipb ^= salt1[s1++];
            else
                s1=0;
        }

        // and some more
        if (salt2)
        {
            if (salt2[s2])
                ipb ^= salt2[s2++];
            else
                s2=0;
        }

        id[i%16] = ipb;
    }
}

// ---------------------------
void GnuID::toStr(char *str) const
{
    str[0] = 0;
    for (int i=0; i<16; i++)
    {
        char tmp[8];
        unsigned char ipb = id[i];

        snprintf(tmp, _countof(tmp), "%02X", ipb);
        strcat_s(str, 33, tmp);
    }
}

// ---------------------------
void GnuID::fromStr(const char *str)
{
    clear();

    if (strlen(str) < 32)
        return;

    char buf[8];

    buf[2] = 0;

    for (int i=0; i<16; i++)
    {
        buf[0] = str[i*2];
        buf[1] = str[i*2+1];
        id[i] = (unsigned char)strtoul(buf, NULL, 16);
    }
}

// ---------------------------
void GnuID::generate(unsigned char flags)
{
    clear();

    for (int i=0; i<16; i++)
        id[i] = sys->rnd();

    id[0] = flags;
}

// ---------------------------
unsigned char GnuID::getFlags()
{
    return id[0];
}

// ---------------------------
GnuIDList::GnuIDList(int max)
:ids(new GnuID[max])
{
    maxID = max;
    for (int i=0; i<maxID; i++)
        ids[i].clear();
}

// ---------------------------
GnuIDList::~GnuIDList()
{
    delete [] ids;
}

// ---------------------------
bool GnuIDList::contains(GnuID &id)
{
    for (int i=0; i<maxID; i++)
        if (ids[i].isSame(id))
            return true;
    return false;
}

// ---------------------------
int GnuIDList::numUsed()
{
    int cnt=0;
    for (int i=0; i<maxID; i++)
        if (ids[i].storeTime)
            cnt++;
    return cnt;
}

// ---------------------------
unsigned int GnuIDList::getOldest()
{
    unsigned int t=(unsigned int)-1;
    for (int i=0; i<maxID; i++)
        if (ids[i].storeTime)
            if (ids[i].storeTime < t)
                t = ids[i].storeTime;
    return t;
}

// ---------------------------
void GnuIDList::add(GnuID &id)
{
    unsigned int minTime = (unsigned int) -1;
    int minIndex = 0;

    // find same or oldest
    for (int i=0; i<maxID; i++)
    {
        if (ids[i].isSame(id))
        {
            ids[i].storeTime = sys->getTime();
            return;
        }
        if (ids[i].storeTime <= minTime)
        {
            minTime = ids[i].storeTime;
            minIndex = i;
        }
    }

    ids[minIndex] = id;
    ids[minIndex].storeTime = sys->getTime();
}

// ---------------------------
void GnuIDList::clear()
{
    for (int i=0; i<maxID; i++)
        ids[i].clear();
}
