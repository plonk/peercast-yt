// ------------------------------------------------
// File : usys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      LSys derives from Sys to provide basic Linux functions such as starting threads.
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
#ifdef __APPLE__
#ifdef __USE_CARBON__
#include <Carbon/Carbon.h>
#endif
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h> // WIFEXITED, WEXITSTATUS
#include <thread>
#include <stdio.h>
#include <limits.h>

#include "stats.h"
#include "str.h"
#include "usocket.h"
#include "usys.h"
#include "subprog.h"
#include "strerror.h"

// ---------------------------------
USys::USys()
{
    stats.clear();

    // 乱数生成器を初期化。urandom の無い環境では PID から一意に決まる。
    {
        FILE *fp = fopen("/dev/urandom", "rb");
        if (fp)
        {
            int seed;
            char *p = (char*) &seed;
            for (size_t i = 0; i < sizeof(int); i++)
            {
                p[i] = fgetc(fp);
            }
            rndGen.setSeed(seed);
            fclose(fp);
        }else
        {
            rndGen.setSeed(rnd() + getpid());
        }
    }
    signal(SIGPIPE, SIG_IGN);
    signal(SIGABRT, SIG_IGN);
}

// ---------------------------------
double USys::getDTime()
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + ((double)tv.tv_usec)/1000000;
}

// ---------------------------------
std::shared_ptr<ClientSocket> USys::createSocket()
{
    return std::make_shared<UClientSocket>();
}

// ---------------------------------
void    USys::setThreadName(const char* name)
{
#ifdef _GNU_SOURCE
    char buf[16];
    snprintf(buf, 16, "%s", name);
    pthread_setname_np(pthread_self(), buf);
#endif
}

// ---------------------------------
bool USys::hasGUI()
{
    return false;
}

// --------------------------------------------------
std::string USys::getHostname()
{
    char buf[256];
 
    if (gethostname(buf, 256) == -1) {
        throw GeneralException(str::strerror(errno).c_str());
    } else {
        return buf;
    }
}

// --------------------------------------------------
std::vector<std::string> USys::getIPAddresses(const std::string& name)
{
#ifndef NO_SERIALIZE_GETADDRINFO
    static std::mutex getaddrinfo_lock;
    std::lock_guard<std::mutex> cs(getaddrinfo_lock);
#endif

    std::vector<std::string> addrs;
    struct addrinfo hints = {}, *result;

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int err = getaddrinfo(name.c_str(), nullptr, &hints, &result);
    if (err) {
        throw GeneralException(str::format("getaddrinfo err = %d", err), err);
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
std::vector<std::string> USys::getAllIPAddresses()
{
    Subprogram hostname("hostname", true, false);
    Environment env;
    if (!hostname.start({"-I"}, env)) {
        return {};
    }
    auto pipe = hostname.inputStream();
    std::string buf;
    try {
        while (!pipe->eof())
            buf += pipe->readChar();
    } catch (StreamException& e) {
    }
    while (isspace(buf[buf.size()-1]))
        buf.resize(buf.size()-1);

    int statusCode;
    if (hostname.wait(&statusCode)) {
        if (statusCode != 0)
            LOG_ERROR("hostname exited with status code %d", statusCode);
    } else {
        LOG_ERROR("hostname didn't exit normally");
    }

    std::vector<std::string> ips = str::split(buf, " ");
    return ips;
}

// --------------------------------------------------
#include <arpa/inet.h>
bool USys::getHostnameByAddress(const IP& ip, std::string& out)
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
                        nullptr,
                        0,
                        NI_NAMEREQD)) {
            LOG_TRACE("getnameinfo: %s (%s)", gai_strerror(errcode), ip.str().c_str());
            out = "";
            return false;
        } else {
            LOG_TRACE("getnameinfo: %s (%s)", hbuf, ip.str().c_str());
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
                        nullptr,
                        0,
                        NI_NAMEREQD)) {
            LOG_TRACE("getnameinfo: %s (%s)", gai_strerror(errcode), ip.str().c_str());
            out = "";
            return false;
        } else {
            LOG_TRACE("getnameinfo: %s (%s)", hbuf, ip.str().c_str());
            out = hbuf;
            return true;
        }
    }
}

// ---------------------------------
std::string USys::getExecutablePath()
{
    // readlink does not append a null byte to the buffer, so we zero it out beforehand.
    char path[PATH_MAX + 1] = "";
    if (readlink("/proc/self/exe", path, PATH_MAX) == -1) {
        throw GeneralException(str::format("%s: %s", __func__, str::strerror(errno).c_str()).c_str());
    }
    return path;
}

// ---------------------------------
std::string USys::dirname(const std::string& path)
{
    std::string normal;

    // 連続するスラッシュを１つにする。
    for (size_t i = 0; i < path.size(); i++) {
        if (path[i] == '/') {
            if (path[i+1] != '/')
                normal.push_back(path[i]);
        } else {
            normal.push_back(path[i]);
        }
    }

    // ディレクトリ名末尾のスラッシュを削除する。
    if (normal.size() > 1 && normal[normal.size() - 1] == '/')
        normal.pop_back();

    while (normal.size() && normal[normal.size() - 1] != '/')
        normal.pop_back();

    if (normal.empty()) {
        return ".";
    } else if (normal != "/") {
        // ディレクトリ名末尾のスラッシュを削除する。
        if (normal[normal.size() - 1] == '/')
            normal.pop_back();
    }

    return normal;
}

// ---------------------------------
std::string USys::joinPath(const std::vector<std::string>& vec)
{
    std::string result;

    for (size_t i = 0; i < vec.size(); i++) {
        std::string frag = vec[i];
        if (frag.empty()) {
            // 空文字列の要素はスキップする。
        } else {
            if (i == 0) {
            } else {
                while (frag[0] == '/') {
                    frag.erase(0, 1);
                }
                if (frag.empty())
                    continue;
                result += "/";
            }
            while (frag.back() == '/') {
                frag.pop_back();
            }
            result += frag;
        }
    }
    return result;
}

#ifndef __APPLE__
// Defining callLocalURL, executeFile, exit, getURL and openURL for non-Apple systems.

// ---------------------------------
void USys::callLocalURL(const char *str, int port)
{
    auto localURL = str::format("http://localhost:%d/%s", port, str);
    openURL(localURL.c_str());
}

// ---------------------------------
void USys::executeFile( const char *file )
{
    openURL(file);
}

// ---------------------------------
void USys::exit()
{
    ::exit(0);
}

// ---------------------------------
void USys::getURL(const char *url)
{
}

// ---------------------------------
void USys::openURL( const char* url )
{
    char* disp = getenv("DISPLAY");

    if (disp == nullptr || disp[0] == '\0')
    {
        LOG_WARN("%s: Ignoring request (no DISPLAY environment variable): %s", __func__, url);
        return;
    }

    int retval;
    std::string cmdLine = str::format("xdg-open %s", str::escapeshellarg_unix(url).c_str());
    LOG_DEBUG("%s: Calling system(%s)", __func__, str::inspect(cmdLine).c_str());
    retval = system(cmdLine.c_str());
    if (retval == -1)
    {
        LOG_ERROR("%s: system(3) returned -1", __func__);
    }else if (!WIFEXITED(retval))
    {
        LOG_ERROR("%s: Shell terminated abnormally", __func__);
    }else if (WEXITSTATUS(retval) != 0)
    {
        LOG_ERROR("%s: Shell exited with error status (%d)", __func__, WEXITSTATUS(retval));
    }else
        ; // Shell exited normally.
}

#else
// Defining callLocalURL, executeFile, exit, getURL and openURL for Apple systems.

// ---------------------------------
void USys::callLocalURL(const char *str, int port)
{
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "http://localhost:%d/%s", port, str);
    openURL( cmd );
}

// ---------------------------------
void USys::executeFile( const char *file )
{
    CFStringRef fileString = CFStringCreateWithFormat( nullptr, nullptr, CFSTR("%s"), file );

    if ( fileString )
    {
        CFURLRef pathRef = CFURLCreateWithString( nullptr, fileString, nullptr );
        if ( pathRef )
        {
            FSRef fsRef;
            CFURLGetFSRef( pathRef, &fsRef );
            OSStatus err = LSOpenFSRef( &fsRef, nullptr );
            CFRelease(pathRef);
        }
        CFRelease( fileString );
    }
}

// ---------------------------------
void USys::exit()
{
#ifdef __USE_CARBON__
    QuitApplicationEventLoop();
#else
    ::exit(0);
#endif
}

// ---------------------------------
void USys::getURL(const char *url)
{
    if (Sys::strnicmp(url, "http://", 7) == 0 ||
        Sys::strnicmp(url, "https://", 8) == 0 ||
        Sys::strnicmp(url, "mailto:", 7) == 0)
    {
        openURL( url );
    }
}

// ---------------------------------
void USys::openURL( const char* url )
{
    CFStringRef urlString = CFStringCreateWithFormat( nullptr, nullptr, CFSTR("%s"), url );

    if ( urlString )
    {
        CFURLRef pathRef = CFURLCreateWithString( nullptr, urlString, nullptr );
        if ( pathRef )
        {
            OSStatus err = LSOpenCFURLRef( pathRef, nullptr );
            CFRelease(pathRef);
        }
        CFRelease( urlString );
    }
}

#endif

// ---------------------------------
std::string USys::realPath(const std::string& path)
{
    char resolvedPath[PATH_MAX];
    char *p = realpath(path.c_str(), resolvedPath);

    if (!p)
        throw GeneralException((std::string("realPath: ") + str::strerror(errno).c_str()).c_str());
    else
        return resolvedPath;
}

// ---------------------------------
void USys::rename(const std::string& oldpath, const std::string& newpath)
{
    if (::rename(oldpath.c_str(), newpath.c_str()) < 0) {
        throw GeneralException( str::format("rename: %s", str::strerror(errno).c_str()).c_str() );
    }
}
