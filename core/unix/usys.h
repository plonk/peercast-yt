// ------------------------------------------------
// File : usys.h
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

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------



#ifndef _USYS_H
#define _USYS_H
// ------------------------------------
#include "socket.h"
#include "sys.h"

// ------------------------------------
class USys : public Sys
{
public:
    USys();

    ClientSocket    *createSocket() override;
    bool            startThread(ThreadInfo *) override;
    void            sleep(int ) override;
    void            appMsg(long, long) override;
    unsigned int    getTime() override;
    double          getDTime() override;
    unsigned int    rnd() override { return rndGen.next(); }
    void            getURL(const char *) override;
    void            exit() override;
    bool            hasGUI() override { return false; }
    void            callLocalURL(const char *, int) override;
    void            executeFile(const char *) override;
    void            endThread(ThreadInfo *) override;
    void            waitThread(ThreadInfo *, int timeout = 30000) override;
    void            setThreadName(ThreadInfo *, const char* name) override;

    peercast::Random rndGen;
private:

#ifdef __APPLE__
    void openURL( const char* url );
#endif
};


// ------------------------------------
#endif
