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

#ifndef _WSOCKET_H
#define _WSOCKET_H

#include "socket.h"
#include <windows.h>

// --------------------------------------------------
class WSAClientSocket : public ClientSocket
{
public:
    static void init();

    WSAClientSocket()
    :sockNum(INVALID_SOCKET)
    {
    }

    ~WSAClientSocket()
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
    std::shared_ptr<ClientSocket> accept() override;
    bool    active() override { return sockNum != INVALID_SOCKET; }
    bool    readReady(int timeoutMilliseconds) override;
    Host    getLocalHost() override;
    void    setBlocking(bool) override;
    void    setReuse(bool);
    void    setNagle(bool);
    void    setLinger(int);

    static  HOSTENT     *resolveHost(const char *);

    void    checkTimeout(bool,bool);

    int getDescriptor() const override { return sockNum; }
    void detach() override { sockNum = INVALID_SOCKET; remoteAddr = {}; }
    char peekChar() override;

    SOCKET sockNum;
    struct sockaddr_in6 remoteAddr;
};

#endif
