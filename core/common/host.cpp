// ------------------------------------------------
// File : host.cpp
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

#include "common.h"
#include "host.h"
#include "socket.h"

// ------------------------------------------
bool Host::isLocalhost()
{
    return loopbackIP() || (ip == ClientSocket::getIP(NULL));
}

// ------------------------------------------
void Host::fromStrName(const char *str, int p)
{
    if (!strlen(str))
    {
        port = 0;
        ip = 0;
        return;
    }

    char name[128];
    strncpy(name, str, sizeof(name)-1);
    port = p;
    char *pp = strstr(name, ":");
    if (pp)
    {
        port = atoi(pp+1);
        pp[0] = 0;
    }

    ip = ClientSocket::getIP(name);
}

// ------------------------------------------
::String Host::IPtoStr()
{
    ::String result;
    this->IPtoStr(result.data);
    return result;
}

// ------------------------------------------
void Host::fromStrIP(const char *str, int p)
{
    unsigned int ipb[4];
    unsigned int ipp;

    if (strstr(str, ":"))
    {
        if (sscanf(str, "%03d.%03d.%03d.%03d:%d", &ipb[0], &ipb[1], &ipb[2], &ipb[3], &ipp) == 5)
        {
            ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
            port = ipp;
        }else
        {
            ip = 0;
            port = 0;
        }
    }else{
        port = p;
        if (sscanf(str, "%03d.%03d.%03d.%03d", &ipb[0], &ipb[1], &ipb[2], &ipb[3]) == 4)
            ip = ((ipb[0]&0xff) << 24) | ((ipb[1]&0xff) << 16) | ((ipb[2]&0xff) << 8) | ((ipb[3]&0xff));
        else
            ip = 0;
    }
}

// -----------------------------------
bool Host::isMemberOf(Host &h)
{
    if (h.ip==0)
        return false;

    if ( h.ip0() != 255 && ip0() != h.ip0() )
        return false;
    if ( h.ip1() != 255 && ip1() != h.ip1() )
        return false;
    if ( h.ip2() != 255 && ip2() != h.ip2() )
        return false;
    if ( h.ip3() != 255 && ip3() != h.ip3() )
        return false;

    return true;
}
