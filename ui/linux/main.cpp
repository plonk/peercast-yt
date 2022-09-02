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
#include "unix/usys.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "peercast.h"
#include "version2.h"
#include <sys/time.h>
#include <limits.h> // PATH_MAX
#include <libgen.h> // dirname
#include "gnutella.h"

// ----------------------------------
String iniFileName;
bool quit = false;
FILE *logfile = NULL;
String htmlPath;
String pidFileName;
String logFileName;
bool forkDaemon = false;
bool setPidFile = false;
bool logToFile = false;
std::recursive_mutex loglock;

// ---------------------------------
class MyPeercastInst : public PeercastInstance
{
public:
    virtual Sys * APICALL createSys()
    {
        return new USys();
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
        return PCX_OS_LINUX;
    }

    virtual void APICALL printLog(LogBuffer::TYPE t, const char *str)
    {
	std::lock_guard<std::recursive_mutex> cs(loglock);

        char buf[20];
        struct timeval tv;
        struct tm tm;

        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        strftime(buf, sizeof(buf), "%Y/%m/%d %T", &tm);

        FILE *out = logfile ? logfile : stdout;
        fprintf(out, "%s.%03d ", buf, (int) tv.tv_usec/1000);
        if (t != LogBuffer::T_NONE)
            fprintf(out, "[%s] ", LogBuffer::getTypeStr(t));
        fprintf(out, "%s\n", str);
    }
};

// ----------------------------------
void setSettingsUI() {}
// ----------------------------------
void showConnections() {}
// ----------------------------------
void PRINTLOG(LogBuffer::TYPE type, const char *fmt, va_list ap)
{
}

// ----------------------------------
void sigProc(int sig)
{
    switch (sig)
    {
    case SIGINT:
        if (!quit)
            LOG_DEBUG("Received INT signal");
        quit = true;
        signal(SIGINT, SIG_DFL);
        break;
    case SIGTERM:
        if (!quit)
            LOG_DEBUG("Received TERM signal");
        quit = true;
        signal(SIGTERM, SIG_DFL);
        break;
    case SIGHUP:
        LOG_DEBUG("Received HUP signal, reloading a new logfile");
        // The aim of this call is to completly reload a new log file.
        // It can be used in conjunction with logrotate,
        // to remove the logfile after it has been copied.
        // some data can still be lost, but this way it is reduced to minimun at lost costs..
        if (logToFile) {
	    std::lock_guard<std::recursive_mutex> cs(loglock);

            if (logfile != NULL) {
                fclose(logfile);
            }
            unlink(logFileName);
            logfile = fopen(logFileName, "a");
        }
        /* This may be nescessary for some systems... */
        signal(SIGHUP, sigProc);
        break;
    }
}

#include <pwd.h>
#include <sys/stat.h>

static bool mkdir_p(const char* dir)
{
    if (strcmp(dir, "/") == 0) {
        return true; // the root dir is assumed to exist.
    } else {
        struct stat sb;

        if (stat(dir, &sb) == -1) {
            if (errno == ENOENT) {
                char *dircopy = strdup(dir);
                if (mkdir_p(dirname(dircopy))) {
                    free(dircopy);
                    if (mkdir(dir, 0777) == -1) {
                        perror("mkdir");
                        return false;
                    } else {
                        return true;
                    }
                }
            } else {
                perror("stat");
                return false;
            }
        }

        if ((sb.st_mode & S_IFMT) == S_IFDIR) {
            return true;
        } else {
            fprintf(stderr, "mkdir_p: Not a directory `%s`", dir);
            return false;
        }
    }
}

static const char* getConfDir()
{
    static ::String confdir;

    if (confdir.c_str()[0] == '\0')
    {
        char* dir;
        dir = getenv("XDG_CONFIG_HOME");
        if (dir) {
            confdir.set(dir);
        } else {
            dir = getenv("HOME");
            if (!dir)
                dir = getpwuid(getuid())->pw_dir;
            confdir.set(dir);
            confdir.append("/.config");
        }
        confdir.append("/peercast");

        if (confdir.c_str()[0] == '/') {
            mkdir_p(confdir.c_str());
        }
    }
    return confdir.c_str();
}

// ----------------------------------
static void init()
{
    // readlink does not append a null byte to the buffer, so we zero it out beforehand.
    char path[PATH_MAX + 1] = "";
    if (readlink("/proc/self/exe", path, PATH_MAX) == -1) {
        perror("/proc/self/exe");
        exit(1);
    }
    auto bindir = dirname(path);
    htmlPath.set(bindir);
    htmlPath.append("/../share/peercast/");

    const char* confdir = getConfDir();
    iniFileName.set(confdir);
    iniFileName.append("/peercast.ini");
    pidFileName.set(confdir);
    pidFileName.append("/peercast.pid");
    logFileName.set(confdir);
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
                char path[PATH_MAX];
                if (realpath(argv[i], path)) {
                    htmlPath.setFromString(path);
                    // Add a "/" in order to make this parameter more natural:
                    htmlPath.append("/");
                } else {
                    perror(argv[i]);
                }
            }
        } else if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("peercast - P2P Streaming Server, version %s\n", PCX_VERSTRING);
            printf("\nCopyright (c) 2002-2006 PeerCast.org <code@peercast.org>\n");
            printf("This is free software; see the source for copying conditions.\n");
            printf("Usage: peercast [options]\n");
            printf("-i, --inifile <inifile>      specify ini file\n");
            printf("-l, --logfile <logfile>      specify log file\n");
            printf("-P, --path <path>            set path to html files\n");
            printf("-d, --daemon                 fork in background\n");
            printf("-p, --pidfile <pidfile>      specify pid file\n");
            printf("-h, --help                   show this help\n");
            return 0;
        } else if (!strcmp(argv[i], "--daemon") || !strcmp(argv[i], "-d")) {
            forkDaemon = true;
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

    if (forkDaemon) {
        LOG_DEBUG("Forking to the background");
        daemon(1, 0);
    }

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

    LOG_INFO("Config file: %s", iniFileName.c_str());
    if (logToFile)
        LOG_INFO("Log file: %s", logFileName.c_str());
    if (setPidFile)
        LOG_INFO("PID file: %s", pidFileName.c_str());

    signal(SIGINT, sigProc);
    signal(SIGTERM, sigProc);
    signal(SIGHUP, sigProc);

    while (!quit) {
        sys->sleep(1000);
        if (logfile != NULL)
        {
            std::lock_guard<std::recursive_mutex> cs(loglock);
            fflush(logfile);
        }
    }

    peercastInst->saveSettings();

    peercastInst->quit();

    if (logfile != NULL) {
        std::lock_guard<std::recursive_mutex> cs(loglock);

        fflush(logfile);
        fclose(logfile);
        // Log might continue but will only be written to stdout.
    }
    if (setPidFile) unlink(pidFileName);

    return 0;
}
