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

#ifndef _SSLCLIENTSOCKET_H
#define _SSLCLIENTSOCKET_H

#include <openssl/ssl.h>

#include "socket.h"

class SslClientSocket : public ClientSocket
{
public:
    SslClientSocket();
    ~SslClientSocket();

    // setReadTimeout setWriteTimeout は open の前に呼び出す必要があり、
    // 一度 open したらタイムアウト値を変更しても効果はない。

    // ClientSocket interface
    void open(const Host &) override;
    void bind(const Host &) override;
    void connect() override;
    bool active() override;
    std::shared_ptr<ClientSocket> accept() override;
    Host getLocalHost() override;
    void setBlocking(bool) override;

    // Stream interface
    int read(void *, int) override;
    int readUpto(void *, int) override;
    void write(const void *, int) override;

    static std::shared_ptr<SslClientSocket> upgrade(std::shared_ptr<ClientSocket>);
    void setTimeoutOptions();

    struct sockaddr_in6 m_remoteAddr;
    int m_socket;
    SSL_CTX* m_ctx;
    SSL* m_ssl;
};

#endif
