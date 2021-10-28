// ------------------------------------------------
// File : lsocket.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Linux version of ClientSocket. Handles the nitty gritty of actually
//      reading and writing TCP
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

// TODO: fix socket closing

#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "usocket.h"
#include "stats.h"
#include "strerror.h"

#ifdef __APPLE__
#include <netinet/in_systm.h> // for n_long definition
#endif

#include <netinet/ip.h>
#include <netinet/tcp.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// --------------------------------------------------
void UClientSocket::init()
{
    LOG_DEBUG("LStartup:  OK");
}

// --------------------------------------------------
// gethostbyname/gethostbyaddr のデータへのアクセスをシリアライズするためのロック。
static std::recursive_mutex staticDataLock;

// --------------------------------------------------
bool ClientSocket::getHostname(char *str, unsigned int ip)
{
    std::lock_guard<std::recursive_mutex> cs(staticDataLock);

    hostent *he;

    ip = htonl(ip);

    he = gethostbyaddr((char *)&ip, sizeof(ip), AF_INET);

    if (he)
    {
        strcpy(str, he->h_name);
        return true;
    }else
        return false;
}

// --------------------------------------------------
unsigned int ClientSocket::getIP(const char *name)
{
    std::lock_guard<std::recursive_mutex> cs(staticDataLock);

    char szHostName[256];

    if (!name)
    {
        if (gethostname(szHostName, sizeof(szHostName))==0)
            name = szHostName;
        else
            return 0;
    }

    hostent *he = UClientSocket::resolveHost(name);

    if (!he)
        return 0;

    char* lpAddr = he->h_addr_list[0];
    if (lpAddr)
    {
        struct in_addr  inAddr;
        memmove (&inAddr, lpAddr, 4);

        return ntohl(inAddr.s_addr);
    }
    return 0;
}

// --------------------------------------------------
void UClientSocket::setLinger(int sec)
{
    linger linger;
    linger.l_onoff = (sec>0)?1:0;
    linger.l_linger = sec;

    if (setsockopt(sockNum, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof (linger)) == -1)
        throw SockException("Unable to set LINGER");
}

// --------------------------------------------------
void UClientSocket::setNagle(bool on)
{
     int nodelay = (on==false);
     if (setsockopt(sockNum, IPPROTO_TCP, TCP_NODELAY, (void*) &nodelay, sizeof(nodelay)) < 0)
        throw SockException("Unable to set NODELAY");
}

// --------------------------------------------------
void UClientSocket::setBlocking(bool block)
{
    int fl = fcntl(sockNum, F_GETFL);

    if (block)
        fl &= ~O_NONBLOCK;
    else
        fl |= O_NONBLOCK;

    if (fcntl(sockNum, F_SETFL, fl) == -1)
        throw SockException("Unable to set NONBLOCK");
}

// --------------------------------------------------
void UClientSocket::setReuse(bool yes)
{
    unsigned long op = yes ? 1 : 0;
    if (setsockopt(sockNum, SOL_SOCKET, SO_REUSEADDR, (char *)&op, sizeof(op)) < 0)
        throw SockException("Unable to set REUSE");
}

// --------------------------------------------------
hostent *UClientSocket::resolveHost(const char *hostName)
{
    hostent *he;

    if ((he = gethostbyname(hostName)) == NULL)
    {
        // if failed, try using gethostbyaddr instead

        unsigned long ip = inet_addr(hostName);

        if (ip == INADDR_NONE)
            return NULL;

        if ((he = gethostbyaddr((char *)&ip, sizeof(ip), AF_INET)) == NULL)
            return NULL;
    }
    return he;
}

// --------------------------------------------------
void UClientSocket::open(const Host &rh)
{
    sockNum = socket (AF_INET6, SOCK_STREAM, 0);

    if (sockNum == INVALID_SOCKET)
        throw SockException("Can`t open socket");

    setBlocking(false);
#ifdef DISABLE_NAGLE
    setNagle(false);
#endif

    host = rh;

    memset(&remoteAddr, 0, sizeof(remoteAddr));

    remoteAddr.sin6_family = AF_INET6;
    remoteAddr.sin6_port = htons(host.port);
    remoteAddr.sin6_addr = host.ip.serialize();
}

// --------------------------------------------------
void UClientSocket::checkTimeout(bool r, bool w)
{
    if ((r && w) || !(r || w))
        throw ArgumentException("Either r or w but no both must be true");

    int err = errno;
    if ((err == EAGAIN) || (err == EINPROGRESS))
    {
        //LOG("checktimeout %d %d", (int)r, (int)w);

        timeval timeout;
        fd_set read_fds;
        fd_set write_fds;
        fd_set except_fds;

        timeout.tv_sec = 0;
        timeout.tv_usec = 0;

        FD_ZERO(&write_fds);
        FD_ZERO(&read_fds);
        FD_ZERO(&except_fds);

        if (w)
        {
            timeout.tv_sec = this->writeTimeout/1000;
            timeout.tv_usec = this->writeTimeout%1000*1000;
            FD_SET(sockNum, &write_fds);
        }

        if (r)
        {
            timeout.tv_sec = this->readTimeout/1000;
            timeout.tv_usec = this->readTimeout%1000*1000;
            FD_SET(sockNum, &read_fds);
        }

        FD_SET(sockNum, &except_fds);

        timeval *tp;
        if (timeout.tv_sec != 0  || timeout.tv_usec != 0)
            tp = &timeout;
        else
            tp = NULL;

        int r = select(sockNum+1, &read_fds, &write_fds, &except_fds, tp);

        if (r == 0)
            throw TimeoutException();
        else if (r == SOCKET_ERROR)
            throw SockException("select failed.");
        else{
            int err;
            socklen_t size = sizeof(int);
            if (getsockopt(sockNum, SOL_SOCKET, SO_ERROR, (char*) &err, &size) != 0)
                throw SockException("getsockopt failed");

            if (err != 0)
                throw SockException(str::strerror(err).c_str());
        }
    }else{
        throw SockException(str::strerror(err).c_str());
    }
}

// --------------------------------------------------
void UClientSocket::connect()
{
    if (::connect(sockNum, (struct sockaddr *)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR)
        checkTimeout(false, true);
}

// --------------------------------------------------
int UClientSocket::read(void *p, int l)
{
    int bytesRead=0;
    while (l)
    {
        int r = recv(sockNum, (char *)p, l, MSG_NOSIGNAL);
        if (r == SOCKET_ERROR)
        {
            // non-blocking sockets always fall through to here
            checkTimeout(true, false);
        }else if (r == 0)
        {
            throw EOFException("Closed on read");
        }else
        {
            stats.add(Stats::BYTESIN, r);
            if (host.localIP())
                stats.add(Stats::LOCALBYTESIN, r);
            updateTotals(r, 0);
            bytesRead+=r;
            l -= r;
            p = (char *)p+r;
        }
    }

    return bytesRead;
}

// --------------------------------------------------
int UClientSocket::readUpto(void *p, int l)
{
    int bytesRead=0;
    while (l)
    {
        int r = recv(sockNum, (char *)p, l, MSG_NOSIGNAL);
        if (r == SOCKET_ERROR)
        {
            // non-blocking sockets always fall through to here
            checkTimeout(true, false);
        }else if (r == 0)
        {
            break;
        }else
        {
            stats.add(Stats::BYTESIN, r);
            if (host.localIP())
                stats.add(Stats::LOCALBYTESIN, r);
            updateTotals(r, 0);
            bytesRead+=r;
            l -= r;
            p = (char *)p+r;
        }
    }

    return bytesRead;
}

// --------------------------------------------------
void UClientSocket::write(const void *p, int l)
{
    while (l)
    {
        int r = send(sockNum, (char *)p, l, MSG_DONTWAIT|MSG_NOSIGNAL);
        if (r == SOCKET_ERROR)
        {
            // non-blocking sockets always fall through to here
            checkTimeout(false, true);
        }else if (r == 0)
        {
            throw SockException("Closed on write");
        }else
        {
            // LOG_DEBUG("UClientSocket: write %d bytes", r);
            stats.add(Stats::BYTESOUT, r);
            if (host.localIP())
                stats.add(Stats::LOCALBYTESOUT, r);
            updateTotals(0, r);
            l -= r;
            p = (char *)p+r;
        }
    }
}

// --------------------------------------------------
void UClientSocket::bind(const Host &h)
{
    struct sockaddr_in6 localAddr = {};

    if ((sockNum = socket (AF_INET6, SOCK_STREAM, 0)) == -1)
        throw SockException("Can`t open socket");

    if (fcntl(sockNum, F_SETFD, FD_CLOEXEC) == -1)
        throw SockException("Can`t set close-on-exec flag");

    setReuse(true);
    setBlocking(false);

    memset(&localAddr, 0, sizeof(localAddr));
    localAddr.sin6_family = AF_INET6;
    localAddr.sin6_port = htons(h.port);
    localAddr.sin6_addr = in6addr_any;

    if ( ::bind (sockNum, (sockaddr *)&localAddr, sizeof(localAddr)) == -1)
        throw SockException("Can`t bind socket");

    if (::listen(sockNum, SOMAXCONN))
        throw SockException("Can`t listen");

    host = h;
}

// --------------------------------------------------
std::shared_ptr<ClientSocket> UClientSocket::accept()
{
    socklen_t fromSize = sizeof(sockaddr_in6);
    sockaddr_in6 from;

    int conSock = ::accept(sockNum, (sockaddr *)&from, &fromSize);

    if (conSock ==  INVALID_SOCKET)
        return NULL;

    auto cs = std::make_shared<UClientSocket>();
    cs->sockNum = conSock;

    cs->host.port = ntohs(from.sin6_port);
    cs->host.ip = IP(from.sin6_addr);

    cs->setBlocking(false);
#ifdef DISABLE_NAGLE
    cs->setNagle(false);
#endif

    return cs;
}

// --------------------------------------------------
Host UClientSocket::getLocalHost()
{
    struct sockaddr_in6 localAddr;

    socklen_t len = sizeof(localAddr);
    if (getsockname(sockNum, (sockaddr *)&localAddr, &len) == 0) {
        return Host(IP(localAddr.sin6_addr), 0);
    } else {
        throw SockException("getsockname failed", errno);
    }
}

// --------------------------------------------------
void UClientSocket::close()
{
    if (sockNum)
    {
        // signal shutdown
        shutdown(sockNum, SHUT_WR);

        // skip remaining data and wait for 0 from recv
        setReadTimeout(2000);
        try
        {
            //char c;
            //while (readUpto(&c, 1)!=0);
            //readUpto(&c, 1);
        }catch (StreamException &e)
        {
            LOG_ERROR("Socket close: %s", e.msg);
        }
        // close handle
        ::close(sockNum);

        sockNum = 0;
    }
}

// --------------------------------------------------
bool    UClientSocket::readReady(int timeoutMilliseconds)
{
    timeval timeout;
    fd_set read_fds;

    timeout.tv_sec = timeoutMilliseconds / 1000;
    timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;

    FD_ZERO (&read_fds);
    FD_SET (sockNum, &read_fds);

    return select (sockNum+1, &read_fds, NULL, NULL, &timeout) == 1;
}

// --------------------------------------------------
int UClientSocket::numPending()
{
    size_t len;

    if (ioctl( sockNum, FIONREAD, (char *)&len ) < 0)
        throw StreamException("numPending");

    return (int)len;
}
