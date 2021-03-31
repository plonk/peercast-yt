// ------------------------------------------------
// File : wsocket.h
// Date: 4-apr-2002
// Author: giles
// Desc:
//      see .cpp for details
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

#ifndef _USOCKET_H
#define _USOCKET_H

#include "socket.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <string.h>
#include <netdb.h>

// --------------------------------------------------
class UClientSocket : public ClientSocket
{
public:
    static void init();

    UClientSocket()
    :sockNum(0)
    {
    }

    ~UClientSocket()
    {
        close();
    }

    void    open(const Host &) override;
    int     read(void *, int) override;
    int     readUpto(void *, int) override;
    void    write(const void *, int) override;
    void    bind(const Host &) override;
    void    connect() override;
    void    close() override;
    ClientSocket *accept() override;
    bool    active() override { return sockNum != 0; }
    bool    readReady(int milliSeconds = 0) override;
    int     numPending() override;

    Host    getLocalHost() override;
    void    setBlocking(bool) override;
    void    setReuse(bool);
    void    setNagle(bool);
    void    setLinger(int);

    static hostent  *resolveHost(const char *);

    void    checkTimeout(bool, bool);

    int sockNum;
    struct sockaddr_in6 remoteAddr;
};

#endif
