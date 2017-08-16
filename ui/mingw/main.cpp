// ------------------------------------------------
// File : main.cpp
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

#include <stdarg.h>
#include <stdio.h>
#include "channel.h"
#include "servent.h"
#include "servmgr.h"
#include "win32/wsys.h"
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include "peercast.h"
#include "version2.h"
#include <sys/time.h>
#include <libgen.h> // dirname
#include <windows.h> // GetModuleFileName
#include <locale.h>

// ----------------------------------
static std::string utf8ToAnsi(const char *utf8);

extern "C" {
    char *_fullpath(char*, const char*, size_t);
}
#ifndef PATH_MAX
#  define PATH_MAX 4096
#endif
#define realpath(N,R) _fullpath((R),(N),PATH_MAX)

// ----------------------------------
String iniFileName;
bool quit = false;
FILE *logfile = NULL;
String htmlPath;
String pidFileName;
String logFileName;
bool setPidFile = false;
bool logToFile = false;
WLock loglock;

// ---------------------------------
class MyPeercastInst : public PeercastInstance
{
public:
    virtual Sys * APICALL createSys()
    {
        // We don't have a window handle!
        return new WSys(0);
    }
};

// ---------------------------------
class MyPeercastApp : public PeercastApplication
{
public:
    virtual const char * APICALL getIniFilename()
    {
        return iniFileName;
    }

    virtual const char * APICALL getPath()
    {
        return htmlPath;
    }

    virtual const char *APICALL getClientTypeOS()
    {
        return PCX_OS_WIN32;
    }

    virtual void APICALL printLog(LogBuffer::TYPE t, const char *str)
    {
        loglock.on();

        if (t != LogBuffer::T_NONE) {
            printf("[%s] ", LogBuffer::getTypeStr(t));
            if (logfile != NULL) {
                fprintf(logfile, "[%s] ", LogBuffer::getTypeStr(t));
            }
        }
        printf("%s\n", utf8ToAnsi(str).c_str());
        if (logfile != NULL) {
            fprintf(logfile, "%s\n", str);
        }
        loglock.off();
    }
};

// ----------------------------------
static std::string utf8ToAnsi(const char *utf8)
{
    int nchars = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);

    LPWSTR wstring = new wchar_t[nchars];
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, wstring, nchars);

    int nbytes = WideCharToMultiByte(CP_ACP, 0, wstring, nchars, NULL, 0, NULL, NULL);

    LPSTR ansi = new char[nbytes];
    WideCharToMultiByte(CP_ACP, 0, wstring, nchars, ansi, nbytes, NULL, NULL);

    std::string res(ansi);
    delete[] wstring;
    delete[] ansi;
    return res;
}

// ----------------------------------
void setSettingsUI() {}
// ----------------------------------
void showConnections() {}
// ----------------------------------
void PRINTLOG(LogBuffer::TYPE type, const char *fmt, va_list ap)
{
}

// ----------------------------------
static void init()
{
    char path[PATH_MAX];
    GetModuleFileName(NULL, path, PATH_MAX);
    auto dir = dirname(path);

    iniFileName.set(dir);
    iniFileName.append("/peercast.ini");
    htmlPath.set(dir);
    htmlPath.append("/");
    pidFileName.set(dir);
    pidFileName.append("/peercast.pid");
    logFileName.set(dir);
    logFileName.append("/peercast.log");
}

// ----------------------------------
int main(int argc, char* argv[])
{
    init();

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--inifile") || !strcmp(argv[i], "-i")) {
            if (++i < argc) {
                iniFileName.setFromString(argv[i]);
            }
        } else if (!strcmp(argv[i], "--logfile") || !strcmp(argv[i], "-l") ) {
            if (++i < argc) {
                logToFile = true;
                logFileName.setFromString(argv[i]);
            }
        } else if (!strcmp(argv[i], "--path") || !strcmp(argv[i], "-P")) {
            if (++i < argc) {
                htmlPath.setFromString(argv[i]);
                // Add a "/" in order to make this parameter more natural:
                htmlPath.append("/");
            }
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("peercast - P2P Streaming Server, version %s\n", PCX_VERSTRING);
            printf("\nCopyright (c) 2002-2006 PeerCast.org <code@peercast.org>\n");
            printf("This is free software; see the source for copying conditions.\n");
            printf("Usage: peercast [options]\n");
            printf("-i, --inifile <inifile>      specify ini file\n");
            printf("-l, --logfile <logfile>      specify log file\n");
            printf("-P, --path <path>            set path to html files\n");
            printf("-p, --pidfile <pidfile>      specify pid file\n");
            printf("-h, --help                   show this help\n");
            return 0;
        } else if (!strcmp(argv[i], "--pidfile") || !strcmp(argv[i], "-p")) {
            if (++i < argc) {
                setPidFile = true;
                pidFileName.setFromString(argv[i]);
            }
        } else {
            printf("Invalid argument %s\n", argv[i]);
            return 1;
        }
    }

    if (logToFile) logfile = fopen(logFileName, "a");

    // PID file must be written after the daemon() call so that we get the daemonized PID.
    if (setPidFile) {
        LOG_DEBUG("Peercast PID is: %i", (int) getpid());
        FILE *pidfileopened = fopen(pidFileName, "w");
        if (pidfileopened != NULL) {
            fprintf(pidfileopened, "%i\n", (int) getpid());
            fclose(pidfileopened);
        }
    }

    peercastInst = new MyPeercastInst();
    peercastApp = new MyPeercastApp();

    peercastInst->init();

    while (!quit) {
        sys->sleep(1000);
        if (logfile != NULL)
        {
            loglock.on();
            fflush(logfile);
            loglock.off();
        }
    }

    peercastInst->saveSettings();

    peercastInst->quit();

    if (logfile != NULL) {
        loglock.on();
        fflush(logfile);
        fclose(logfile);
        // Log might continue but will only be written to stdout.
        loglock.off();
    }
    if (setPidFile) unlink(pidFileName);

    return 0;
}
