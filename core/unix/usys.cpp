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

#include "stats.h"
#include "str.h"
#include "usocket.h"
#include "usys.h"

// ---------------------------------
USys::USys()
{
    stats.clear();

    rndGen.setSeed(rnd() + getpid());
    signal(SIGPIPE, SIG_IGN);
    signal(SIGABRT, SIG_IGN);

    rndSeed = rnd();
}

// ---------------------------------
double USys::getDTime()
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    return (double)tv.tv_sec + ((double)tv.tv_usec)/1000000;
}

// ---------------------------------
ClientSocket *USys::createSocket()
{
    return new UClientSocket();
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
void USys::appMsg(long msg, long arg)
{
    //SendMessage(mainWindow, WM_USER, (WPARAM)msg, (LPARAM)arg);
}

#ifndef __APPLE__

// ---------------------------------
void USys::getURL(const char *url)
{
}

// ---------------------------------
void USys::callLocalURL(const char *str, int port)
{
    int retval;
    std::string cmdLine = str::format("xdg-open http://localhost:%d/%s", port, str);
    retval = system(cmdLine.c_str()); // XXX: should shell-escape cmdLine
    if (retval == -1)
    {
        LOG_ERROR("USys::callLocalURL: system(%s) returned -1", cmdLine.c_str());
    }else if (WIFEXITED(retval))
    {
        LOG_ERROR("Usys::callLocalURL: Shell terminated abnormally");
    }else if (WEXITSTATUS(retval) != 0)
    {
        LOG_ERROR("Usys::callLocalURL: Shell exited with error status (%d)", WEXITSTATUS(retval));
    }else
        ; // Shell exited normally.
}

// ---------------------------------
void USys::executeFile( const char *file )
{
}

// ---------------------------------
void USys::exit()
{
    ::exit(0);
}

#else

// ---------------------------------
void USys::openURL( const char* url )
{
    CFStringRef urlString = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), url );

    if ( urlString )
    {
        CFURLRef pathRef = CFURLCreateWithString( NULL, urlString, NULL );
        if ( pathRef )
        {
            OSStatus err = LSOpenCFURLRef( pathRef, NULL );
            CFRelease(pathRef);
        }
        CFRelease( urlString );
    }
}

// ---------------------------------
void USys::callLocalURL(const char *str, int port)
{
    char cmd[512];
    sprintf(cmd, "http://localhost:%d/%s", port, str);
    openURL( cmd );
}

// ---------------------------------
void USys::getURL(const char *url)
{
    if (Sys::strnicmp(url, "http://", 7) || Sys::strnicmp(url, "mailto:", 7)) // XXX: ==0 が抜けてる？
    {
        openURL( url );
    }
}

// ---------------------------------
void USys::executeFile( const char *file )
{
    CFStringRef fileString = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), file );

    if ( fileString )
    {
        CFURLRef pathRef = CFURLCreateWithString( NULL, fileString, NULL );
        if ( pathRef )
        {
            FSRef fsRef;
            CFURLGetFSRef( pathRef, &fsRef );
            OSStatus err = LSOpenFSRef( &fsRef, NULL );
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

#endif
