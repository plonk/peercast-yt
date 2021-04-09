// ------------------------------------------------
// File : wsys.cpp
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

#include <process.h>
#include <time.h>
#include "win32/wsys.h"
#include "win32/wsocket.h"
#include <windows.h>
#include "stats.h"
#include "peercast.h"
#include <sys/timeb.h>
#include <time.h>

// ---------------------------------
WSys::WSys(HWND w)
{
    stats.clear();

    rndGen.setSeed(getTime());

    mainWindow = w;
    WSAClientSocket::init();
}

// ---------------------------------
double WSys::getDTime()
{
   struct _timeb timebuffer;

   _ftime( &timebuffer );

   return (double)timebuffer.time+(((double)timebuffer.millitm)/1000);
}

// ---------------------------------
ClientSocket *WSys::createSocket()
{
    return new WSAClientSocket();
}

// --------------------------------------------------
void WSys::callLocalURL(const char *str,int port)
{
    char cmd[512];
    sprintf(cmd,"http://localhost:%d/%s",port,str);
    ShellExecuteA(mainWindow, NULL, cmd, NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------
void WSys::getURL(const char *url)
{
    if (mainWindow)
        if (Sys::strnicmp(url,"http://",7) || Sys::strnicmp(url,"mailto:",7)) // XXX: ==0 が抜けてる？
            ShellExecuteA(mainWindow, NULL, url, NULL, NULL, SW_SHOWNORMAL);
}

// ---------------------------------
void WSys::exit()
{
    if (mainWindow)
        PostMessage(mainWindow,WM_CLOSE,0,0);
    else
        ::exit(0);
}

// --------------------------------------------------
void WSys::executeFile(const char *file)
{
    ShellExecuteA(NULL,"open",file,NULL,NULL,SW_SHOWNORMAL);
}

// --------------------------------------------------
std::string WSys::getHostname()
{
    char buf[256];
 
    if (gethostname(buf, 256) == SOCKET_ERROR) {
        throw GeneralException("gethostname", WSAGetLastError());
    } else {
        return buf;
    }
}

// --------------------------------------------------
std::vector<std::string> WSys::getIPAddresses(const std::string& name)
{
    std::vector<std::string> addrs;
    struct addrinfo hints = {}, *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(name.c_str(), NULL, &hints, &result);
    if (err) {
        throw GeneralException("getaddrinfo", err);
    }

    for (auto p = result; p; p = p->ai_next) {
        switch (p->ai_family) {
        case AF_INET:
            addrs.push_back(inet_ntoa(((sockaddr_in *) p->ai_addr)->sin_addr));
            break;
        case AF_INET6:
            {
                char buf[46];
                inet_ntop(AF_INET6, &((sockaddr_in6 *) p->ai_addr)->sin6_addr, buf, 46);
                addrs.push_back(buf);
            }
            break;
        default:
            break;
        }
    }
    return addrs;
}

// --------------------------------------------------
std::vector<std::string> WSys::getAllIPAddresses()
{
    return getIPAddresses(getHostname());
}
