#ifndef _IP_H
#define _IP_H

#include "common.h"
#include <arpa/inet.h>
#include <memory.h>
#include <algorithm>

class IP
{
 public:

    inline unsigned int ip3() const
    {
        if (!isIPv4Mapped())
            throw GeneralException("ip3: Not an IPv4 address");
        return addr[12];
    }
    inline unsigned int ip2() const
    {
        if (!isIPv4Mapped())
            throw GeneralException("ip2: Not an IPv4 address");
        return addr[13];
    }
    inline unsigned int ip1() const
    {
        if (!isIPv4Mapped())
            throw GeneralException("ip1: Not an IPv4 address");
        return addr[14];
    }
    inline unsigned int ip0() const
    {
        if (!isIPv4Mapped())
            throw GeneralException("ip0: Not an IPv4 address");
        return addr[15];
    }

    bool isIPv4Mapped() const
    {
        static unsigned char prefix[12] = { 0,0,0,0,0,0,0,0,0,0,0xff,0xff };
        return memcmp(prefix, addr, 12) == 0;
    }

    IP()
    {
    }
    
    IP(unsigned int ipv4)
        : addr {0,0,0,0,0,0,0,0,0,0,0xff,0xff}
    {
        *reinterpret_cast<unsigned int *>(addr + 12) = htonl(ipv4);
    }
    
    IP(in6_addr& anAddr)
    {
        memcpy(addr, anAddr.s6_addr, 16);
    }

    in6_addr serialize() const
    {
        in6_addr res = {};
        memcpy(res.s6_addr, addr, 16);
        return res;
    }

    bool operator ==(const IP& other) const
    {
        return memcmp(addr, other.addr, 16) == 0;
    }

    bool operator <(const IP& other) const
    {
        return memcmp(addr, other.addr, 16) < 0;
    }

    bool isGlobal() const
    {
        if (isIPv4Mapped()) {
            // local host
            if ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1))
                return false;

            // class A
            if (ip3() == 10)
                return false;

            // class B
            if ((ip3() == 172) && (ip2() >= 16) && (ip2() <= 31))
                return false;

            // class C
            if ((ip3() == 192) && (ip2() == 168))
                return false;
        } else {
            if (isIPv6Loopback())
                return false;

            // XXX: etc...
        }

        return true;
    }

    bool isIPv4Loopback() const
    {
        return isIPv4Mapped() &&
            ((ip3() == 127) && (ip2() == 0) && (ip1() == 0) && (ip0() == 1));
    }

    bool isIPv6Loopback() const
    {
        unsigned char loopback[16] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1 };
        return memcmp(addr, loopback, 16) == 0;
    }

    bool isIPv6Any() const
    {
        static unsigned char any[16] = {};
        return memcmp(addr, any, 16) == 0;
    }

    bool isIPv4Any() const
    {
        return isIPv4Mapped() &&
            (ip3() == 0 && ip2() == 0 && ip1() == 0 && ip0() == 0);
    }
    
    bool isAny() const
    {
        return isIPv6Any() || isIPv4Any();
    }

    std::string str() const
    {
        if (isIPv4Mapped()) {
            char buf[64];
            sprintf(buf, "%d.%d.%d.%d", ip3(), ip2(), ip1(), ip0());
            return buf;
        } else {
            in6_addr a = serialize();
            char buf[64];
            if (inet_ntop(AF_INET6, &a, buf, sizeof(buf)) == NULL) {
                throw GeneralException("inet_ntop");
            } else {
                return buf;
            }
        }
    }

    operator bool() const
    {
        return !isAny();
    }

    unsigned int ipv4() const
    {
        if (!isIPv4Mapped()) {
            throw GeneralException("ipv4: Not an IPv4 address");
        }
        return addr[12]<<24 | addr[13]<<16 | addr[14]<<8 | addr[15];
    }

    static bool tryParse(const std::string& str, IP& ip)
    {
        if (std::find(str.begin(), str.end(), ':') != str.end()) {
            in6_addr addr;
            if (inet_pton(AF_INET6, str.c_str(), addr.s6_addr) == 1) {
                // success
                ip = IP(addr);
                return true;
            }
        } else {
            // str could be an IPv4 address
            in6_addr addr;
            if (inet_pton(AF_INET6, ("::FFFF:" + str).c_str(), addr.s6_addr) == 1) {
                ip = IP(addr);
                return true;
            }
        }
        ip = IP();
        return false;
    }

    unsigned char addr[16];
};

#endif
