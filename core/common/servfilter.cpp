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
        buf = host.str(false); // without port
    else
        return false;

    out.writeString(buf);
    return true;
}

// --------------------------------------------------
bool ServFilter::matches(int fl, const Host& h) const
{
    return (flags&fl) != 0 && h.isMemberOf(host);
}

// --------------------------------------------------
void ServFilter::setPattern(const char* str)
{
    host.fromStrIP(str, 0);
}

// --------------------------------------------------
std::string ServFilter::getPattern()
{
    char ipstr[64];
    host.IPtoStr(ipstr);
    return ipstr;
}

// --------------------------------------------------
bool ServFilter::isGlobal()
{
    const std::string global = "255.255.255.255";
    return host.IPtoStr().c_str() == global;
}

// --------------------------------------------------
bool ServFilter::isSet()
{
    return host.ip != 0;
}
