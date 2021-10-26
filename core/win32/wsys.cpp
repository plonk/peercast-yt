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
std::shared_ptr<ClientSocket> WSys::createSocket()
{
    return std::make_shared<WSAClientSocket>();
}

// --------------------------------------------------
void WSys::callLocalURL(const char *str,int port)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "http://localhost:%d/%s", port, str);
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
    freeaddrinfo(result);
    return addrs;
}

// --------------------------------------------------
std::vector<std::string> WSys::getAllIPAddresses()
{
    return getIPAddresses(getHostname());
}

// --------------------------------------------------
bool WSys::getHostnameByAddress(const IP& ip, std::string& out)
{
    char hbuf[NI_MAXHOST];

    if (ip.isIPv4Mapped()) {
        sockaddr_in addr = { AF_INET };
        unsigned int i = htonl(ip.ipv4());
        memcpy(&addr.sin_addr, &i, 4);

        if (int errcode = getnameinfo((struct sockaddr *) &addr,
                        sizeof(addr),
                        hbuf,
                        sizeof(hbuf),
                        NULL,
                        0,
                        NI_NAMEREQD)) {
            LOG_ERROR("getnameinfo: error code = %d", errcode);
            out = "";
            return false;
        } else {
            out = hbuf;
            return true;
        }
    } else {
        sockaddr_in6 addr = { AF_INET6 };
        addr.sin6_addr = ip.serialize();

        if (int errcode = getnameinfo((struct sockaddr *) &addr,
                        sizeof(addr),
                        hbuf,
                        sizeof(hbuf),
                        NULL,
                        0,
                        NI_NAMEREQD)) {
            LOG_ERROR("getnameinfo: error code = %d", errcode);
            out = "";
            return false;
        } else {
            out = hbuf;
            return true;
        }
    }
}

// ---------------------------------
std::string WSys::getExecutablePath()
{
    char path[1024];
    if (GetModuleFileNameA(NULL, path, sizeof(path)) == 0) {
        throw GeneralException(str::format("%s: %lu", __func__, GetLastError()).c_str());
    }
    return path;
}

// ---------------------------------
#include "Shlwapi.h"
std::string WSys::realPath(const std::string& path)
{
    char resolvedPath[4096];
    char* ret = _fullpath(resolvedPath, path.c_str(), 4096);
    if (ret == NULL)
    {
        throw GeneralException(str::format("_fullpath: Failed to resolve `%s`", path.c_str()).c_str());
    }

    if (!PathFileExistsA(resolvedPath)) {
        throw GeneralException(str::format("realPath: File not found: %s", resolvedPath).c_str());
    }

    /* 多分フォワードスラッシュは入ってこないから、バックスラッシュだ
       け見る。 \\ が空文字列になるのはまずいかもしれないので 2 文字未
       満には切り詰めない。 */
    for (int len = strlen(resolvedPath); len >= 2 && resolvedPath[len - 1] == '\\'; --len) {
        resolvedPath[len - 1] = '\0';
    }
    return resolvedPath;
}

// ---------------------------------
std::string WSys::getDirectorySeparator()
{
    return "\\";
}

// ---------------------------------
void WSys::rename(const std::string& oldpath, const std::string& newpath)
{
    if (MoveFileExA(oldpath.c_str(), newpath.c_str(), MOVEFILE_REPLACE_EXISTING) == 0) {
        throw GeneralException(str::format("rename: MoveFileExA failed: error = %lu", GetLastError()).c_str());
    }
}
