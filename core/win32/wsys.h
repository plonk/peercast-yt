// ------------------------------------------------
// File : wsys.h
// Date: 4-apr-2002
// Author: giles
// Desc:
//      WSys derives from Sys to provide basic win32 functions such as starting threads.
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#ifndef _WSYS_H
#define _WSYS_H

// ------------------------------------
#include <winsock2.h>
#include <windows.h>
#include "socket.h"
#include "sys.h"

// ------------------------------------
class WSys : public Sys
{
public:
    WSys(HWND);

    std::shared_ptr<ClientSocket> createSocket() override;
    double          getDTime() override;
    unsigned int    rnd() override { return rndGen.next(); }
    void            getURL(const char *) override;
    void            exit() override;
    bool            hasGUI() override { return mainWindow!=NULL; }
    void            callLocalURL(const char *str, int port) override;
    void            executeFile(const char *) override;

    std::string     getHostname() override;
    std::vector<std::string> getIPAddresses(const std::string& name) override;
    std::vector<std::string> getAllIPAddresses() override;
    bool getHostnameByAddress(const IP& ip, std::string& out) override;

    std::string getExecutablePath() override;

    std::string realPath(const std::string& path) override;

    HWND    mainWindow;
    peercast::Random rndGen;
};

#endif
