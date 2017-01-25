// ------------------------------------------------
// File : main.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		see .cpp for details
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

// ----------------------------------
String iniFileName;
bool quit=false;
FILE *logfile=NULL;
String htmlPath;
String pidFileName;
String logFileName;
bool forkDaemon = false;
bool setPidFile = false;
bool logToFile = false;


WLock loglock;

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
		loglock.on();
		if (t != LogBuffer::T_NONE) {
			printf("[%s] ",LogBuffer::getTypeStr(t));
			if (logfile != NULL) {
				fprintf(logfile, "[%s] ",LogBuffer::getTypeStr(t)); }
			}
		printf("%s\n",str);
		if (logfile != NULL) {
			fprintf(logfile, "%s\n",str);
		}
		loglock.off();
	}

};

// ----------------------------------
void setSettingsUI(){}
// ----------------------------------
void showConnections(){}
// ----------------------------------
void PRINTLOG(LogBuffer::TYPE type, const char *fmt,va_list ap)
{
}

// ----------------------------------
void sigProc(int sig)
{
	switch (sig)
	{
		case SIGTERM:
			if (!quit)
				LOG_DEBUG("Received TERM signal");
			quit=true;
			break;
		case SIGHUP:
				LOG_DEBUG("Received HUP signal, reloading a new logfile");
				// The aim of this call is to completly reload a new log file.
				// It can be used in conjonction with logrotate, 
				// to remove the logfile after it has been copied.
				// some data can still be lost, but this way it is reduced to minimun at lost costs..
				if (logToFile) {
					loglock.on();
					if (logfile != NULL) {
						fclose(logfile);
					}
					unlink(logFileName);
					logfile = fopen(logFileName,"a");
					loglock.off();
				}
			break;
	}

	/* This may be nescessary for some systems... */
	signal(SIGTERM, sigProc); 
	signal(SIGHUP, sigProc); 
}

// ----------------------------------
int main(int argc, char* argv[])
{
			
	iniFileName.set("peercast.ini");
	htmlPath.set("./");
	pidFileName.set("peercast.pid");
	logFileName.set("peercast.log");

	for (int i = 1; i < argc; i++)
		{
			if (!strcmp(argv[i],"--inifile") || !strcmp(argv[i],"-i")) {
				if (++i < argc) {
					iniFileName.setFromString(argv[i]);
				}
			} else if (!strcmp(argv[i],"--logfile") || !strcmp(argv[i],"-l") ) {
				if (++i < argc) {
					logToFile = true;
					logFileName.setFromString(argv[i]);
				}
			} else if (!strcmp(argv[i],"--path") || !strcmp(argv[i],"-P")) {
				if (++i < argc) {
					htmlPath.setFromString(argv[i]);
					// Add a "/" in order to make this parameter more natural:
					htmlPath.append("/");
				}
			} else if (!strcmp(argv[i],"--help") || !strcmp(argv[i],"-h")) {
				printf("peercast - P2P Streaming Server, version %s\n",PCX_VERSTRING);
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
			} else if (!strcmp(argv[i],"--daemon") || !strcmp(argv[i],"-d")) {
					forkDaemon = true;
			} else if (!strcmp(argv[i],"--pidfile") || !strcmp(argv[i],"-p")) {
				if (++i < argc) 
				{
					setPidFile = true;
					pidFileName.setFromString(argv[i]);
				}
			}
		}


	if (logToFile) logfile = fopen(logFileName,"a");

	if (forkDaemon) {
		LOG_DEBUG("Forking to the background");
		daemon(1,0);
	}

	// PID file must be written after the daemon() call so that we get the daemonized PID.
	if (setPidFile) {
		LOG_DEBUG("Peercast PID is: %i", (int) getpid() );
		FILE *pidfileopened = fopen(pidFileName, "w");
		if (pidfileopened != NULL) 
			{
				fprintf(pidfileopened, "%i\n", (int) getpid() );
				fclose(pidfileopened);
			}
	}


	peercastInst = new MyPeercastInst();
	peercastApp = new MyPeercastApp();

	peercastInst->init();

	signal(SIGTERM, sigProc); 
	signal(SIGHUP, sigProc); 

	while (!quit) {
		sys->sleep(1000);
		if (logfile != NULL)
			loglock.on();
			fflush(logfile);
			loglock.off();
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
