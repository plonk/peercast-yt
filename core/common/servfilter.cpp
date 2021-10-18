// ------------------------------------------------
// File : servfilter.cpp (derived from servmgr.cpp)
// Desc:
//      IPアドレスマッチング。
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

#include "servfilter.h"
#include "_string.h"
#include "stream.h"
#include "regexp.h"
#include "str.h"
#include "socket.h"

static const Regexp IPV4_PATTERN("^\\d+\\.\\d+\\.\\d+\\.\\d+$");
static const Regexp IPV6_PATTERN("^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][fF][fF][fF](:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))$");
static const Regexp IPV4_WITH_NETMASK_PATTERN("^\\d+\\.\\d+\\.\\d+\\.\\d+/\\d+$");
static const Regexp IPV6_WITH_NETMASK_PATTERN("^(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|[fF][eE]80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::([fF][fF][fF][fF](:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))/\\d+$");

bool    ServFilter::writeVariable(Stream &out, const String &var)
{
    std::string buf;

    if (var == "network")
        buf = (flags & F_NETWORK) ? "1" : "0";
    else if (var == "private")
        buf = (flags & F_PRIVATE) ? "1" : "0";
    else if (var == "direct")
        buf = (flags & F_DIRECT) ? "1" : "0";
    else if (var == "banned")
        buf = (flags & F_BAN) ? "1" : "0";
    else if (var == "ip")
        buf = getPattern();
    else
        return false;

    out.writeString(buf);
    return true;
}

// --------------------------------------------------
bool ServFilter::matches(int fl, const Host& h) const
{
    if ((flags&fl) == 0)
        return false;

    switch (type)
    {
    case T_IP:
        return h.isMemberOf(host);
    case T_IPV6:
        return host.ip == h.ip;
    case T_HOSTNAME:
        return h.ip == IP(ClientSocket::getIP(pattern.c_str()));
    case T_SUFFIX:
        {
            std::string str;
            if (sys->getHostnameByAddress(h.ip, str)) {
                return str::has_suffix(str, pattern);
            } else {
                return false;
            }
        }
    case T_IPV4_WITH_NETMASK:
        {
            if (!h.ip.isIPv4Mapped())
                return false;

            uint32_t mask = (uint32_t) -1 << (32 - netmask);

            return (host.ip.ipv4() & mask) == (h.ip.ipv4() & mask);
        }
    case T_IPV6_WITH_NETMASK:
        {
            const unsigned char* addr1 = host.ip.addr;
            const unsigned char* addr2 = h.ip.addr;

            int n = netmask;
            for (int i = 0; i < 16; i++)
            {
                if (n >= 8) {
                    if (addr1[i] != addr2[i])
                        return false;
                    n -= 8;
                } else {
                    unsigned char mask = (unsigned char) -1 << (8 - n);
                    if ((addr1[i] & mask) != (addr2[i] & mask))
                        return false;
                    n = 0;
                }
                if (n <= 0)
                    break;
            }
            return true;
        }
    default:
        throw LogicError("matches: Unknown ServFilter type");
    }
}

// --------------------------------------------------
void ServFilter::setPattern(const char* str)
{
    if (*str == '\0')
    {
        this->init();
        return;
    }else if (str[0] == '.')
    {
        type = T_SUFFIX;
        pattern = str;
    }else if (IPV4_PATTERN.matches(str))
    {
        type = T_IP;
        host.fromStrIP(str, 0);
    }else if (IPV6_PATTERN.matches(str))
    {
        type = T_IPV6;
        host = Host(IP::parse(str), 0);
    }else if (IPV4_WITH_NETMASK_PATTERN.matches(str))
    {
        type = T_IPV4_WITH_NETMASK;
        auto vec = str::split(str, "/", 2);
        host.fromStrIP(vec[0].c_str(), 0);
        netmask = std::min(32, atoi(vec[1].c_str()));
    }else if (IPV6_WITH_NETMASK_PATTERN.matches(str))
    {
        type = T_IPV6_WITH_NETMASK;
        auto vec = str::split(str, "/", 2);
        host = Host(IP::parse(vec[0]), 0);
        netmask = std::min(128, atoi(vec[1].c_str()));
    }else
    {
        type = T_HOSTNAME;
        pattern = str;
    }
}

// --------------------------------------------------
std::string ServFilter::getPattern() const
{
    switch (type)
    {
    case T_IP:
        return host.str(false);
    case T_IPV6:
        return host.ip.str();
    case T_HOSTNAME:
        return pattern;
    case T_SUFFIX:
        return pattern;
    case T_IPV4_WITH_NETMASK:
    case T_IPV6_WITH_NETMASK:
        return host.ip.str() + "/" + std::to_string(netmask);
    default:
        throw LogicError("getPattern: Unknown ServFilter type");
    }
}

// --------------------------------------------------
bool ServFilter::isGlobal()
{
    return type == T_IP && host.str(false) == "255.255.255.255";
}

// --------------------------------------------------
bool ServFilter::isSet()
{
    return !(type == T_IP && !host.ip);
}
