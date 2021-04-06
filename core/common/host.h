// ------------------------------------------------
// File : host.h
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

#ifndef _HOST_H
#define _HOST_H

#include <netinet/in.h>
#include "common.h"
#include "ip.h"
#include <string>

class String;

// ----------------------------------
class Host
{
public:
    Host() { init(); }
    Host(unsigned int i, unsigned short p)
        : ip(i)
    {
        port = p;
    }
    Host(const std::string& hostname, uint16_t port)
    {
        fromStrName(hostname.c_str(), port);
    }
    Host(const IP& anIp, unsigned short p)
        : ip(anIp)
    {
        port = p;
    }

    // can this be private?
    void    init()
    {
        ip = IP(0);
        port = 0;
    }

    bool    isMemberOf(const Host &) const;

    bool    isSame(Host &h) const
    {
        return (h.ip == ip) && (h.port == port);
    }

    bool    classType() const { return globalIP(); }

    bool    globalIP() const
    {
        return ip.isGlobal();
    }

    bool    localIP() const
    {
        return !globalIP();
    }

    bool    loopbackIP() const
    {
        return ip.isIPv4Loopback() || ip.isIPv6Loopback();
    }

    bool    isValid() const
    {
        return !ip.isAny();
    }

    bool    isSameType(Host &h) const
    {
            return ( (globalIP() && h.globalIP()) ||
                     (!globalIP() && !h.globalIP()) );
    }

    ::String IPtoStr() const;

    std::string str(bool withPort = true) const
    {
        if (withPort) {
            if (ip.isIPv4Mapped()) {
                return ip.str() + ":" + std::to_string(static_cast<int>(port));
            } else {
                return "[" + ip.str() + "]:" + std::to_string(static_cast<int>(port));
            }
        } else {
            return ip.str();
        }
    }

    operator std::string () const
    {
        return str();
    }

    // why is this needed?
    bool operator < (const Host& other) const
    {
        if (ip == other.ip)
            return port < other.port;
        else
            return ip < other.ip;
    }

    bool operator == (const Host& other) const
    {
        return const_cast<Host*>(this)->isSame(const_cast<Host&>(other));
    }

    void    fromStrIP(const char *, int);
    void    fromStrName(const char *, int);

    // should go to IP
    bool    isLocalhost();

    static Host fromString(const std::string& str);

    IP              ip;
    unsigned short  port;
};

#endif
