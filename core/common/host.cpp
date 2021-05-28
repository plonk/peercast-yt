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
#include "str.h"
#include "regexp.h"

// ------------------------------------------
bool Host::isLocalhost()
{
    return loopbackIP() || (ip == sys->getInterfaceIPv4Address());
}

// ------------------------------------------
Host Host::fromString(const std::string& str)
{
    uint16_t port = 0;
    IP ip = 0;

    auto it = str.begin();

    if (*it == '[') {
        // ipv6?
        ++it;
        it = std::find(it, str.end(), ']');
        if (it != str.end()) {
            IP::tryParse(std::string(str.begin() + 1, it), ip);
            ++it;
            if (*it == ':') {
                ++it;
                port = atoi(std::string(it, str.end()).c_str());
            }
        }
    } else {
        // ipv4
        auto it2 = std::find(it, str.end(), ':');
        if (it2 != str.end()) {
            ip = ClientSocket::getIP(std::string(it, it2).c_str());
            ++it2;
            port = atoi(std::string(it2, str.end()).c_str());
        } else {
            ip = ClientSocket::getIP(std::string(it, it2).c_str());
        }
    }

    return Host(ip, port);
}

// ------------------------------------------
void Host::fromStrName(const char *str, int p)
{
    static const Regexp IPV4_PATTERN("^\\d+\\.\\d+\\.\\d+\\.\\d+$");
    static const Regexp IPV4_PATTERN_PORT("^\\d+\\.\\d+\\.\\d+\\.\\d+:\\d+$");
#define IPV6 "(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][fF][fF][fF](:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))"
    static const Regexp IPV6_PATTERN("^" IPV6 "$");
    static const Regexp IPV6_PATTERN_BRACKETED("^\\[" IPV6 "\\]$");
    static const Regexp IPV6_PATTERN_BRACKETED_PORT("^\\[" IPV6 "\\]:\\d+$");
#undef IPV6

    if (!strlen(str))
    {
        port = 0;
        ip = 0;
        return;
    }

    if (IPV4_PATTERN.exec(str).size()) {
        ip = IP::parse(str);
        port = p;
    } else if (IPV4_PATTERN_PORT.exec(str).size()) {
        auto v = str::split(str, ":");
        ip = IP::parse(v[0]);
        port = atoi(v[1].c_str());
    } else if (IPV6_PATTERN.exec(str).size()) {
        ip = IP::parse(str);
        port = p;
    } else if (IPV6_PATTERN_BRACKETED.exec(str).size()) {
        ip = IP::parse(std::string(str + 1, strlen(str) - 2));
        port = p;
    } else if (IPV6_PATTERN_BRACKETED_PORT.exec(str).size()) {
        const char *q = strchr(str, ']');
        ip = IP::parse(std::string(str + 1, q));
        port = atoi(q + 2);
    } else {
        auto v = str::split(str, ":");
        if (v.size() <= 2) {
            if (!IP::tryParse(v[0], ip)) {
                ip = IP();
                for (std::string& s : sys->getIPAddresses(v[0])) {
                    ip = IP::parse(s);
                    break;
                }
            }
            if (v.size() == 2)
                port = atoi(v[1].c_str());
            else
                port = p;
        } else {
            LOG_ERROR("fromStrName: Parse error: %s", str);
            ip = 0;
            port = 0;
        }
    }
}

// ------------------------------------------
::String Host::IPtoStr() const
{
    return ip.str().c_str();
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
bool Host::isMemberOf(const Host &h) const
{
    if (!h.ip)
        return false;

    if ( h.ip.ip0() != 255 && ip.ip0() != h.ip.ip0() )
        return false;
    if ( h.ip.ip1() != 255 && ip.ip1() != h.ip.ip1() )
        return false;
    if ( h.ip.ip2() != 255 && ip.ip2() != h.ip.ip2() )
        return false;
    if ( h.ip.ip3() != 255 && ip.ip3() != h.ip.ip3() )
        return false;

    return true;
}
