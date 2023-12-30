// ------------------------------------------------
// File : socket.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _SOCKET_H
#define _SOCKET_H

#include "host.h"
#include "common.h"
#include "stream.h"
#include "config.h"
#include "ip.h"

#include <memory>

// --------------------------------------------------
class ClientSocket : public Stream
{
public:

    ClientSocket()
    {
        readTimeout = 30000;
        writeTimeout = 5000;
    }

    virtual ~ClientSocket() {}

    // required interface
    virtual void            open(const Host &) = 0;
    virtual void            bind(const Host &)       = 0;
    virtual void            connect()          = 0;
    virtual bool            active()           = 0;
    virtual std::shared_ptr<ClientSocket> accept()          = 0;
    virtual Host            getLocalHost()     = 0;

    void    setReadTimeout(unsigned int t) override
    {
        readTimeout = t;
    }
    void    setWriteTimeout(unsigned int t) override
    {
        writeTimeout = t;
    }
    virtual void    setBlocking(bool) = 0;

    static unsigned int getIP(const char *);

    bool eof() override
    {
        return active()==false;
    }

    Host            host;

    unsigned int    readTimeout, writeTimeout;
};

#endif
