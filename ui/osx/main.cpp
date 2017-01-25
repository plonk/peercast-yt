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
#include "stdio.h"
#include "channel.h"
#include "servent.h"
#include "servmgr.h"
#include "unix/usys.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "peercast.h"


// ----------------------------------
String iniFileName;
bool quit=false;


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

	virtual const char *APICALL getClientTypeOS() 
	{
		return PCX_OS_MACOSX;
	}

	virtual void APICALL printLog(LogBuffer::TYPE t, const char *str)
	{
		if (t != LogBuffer::T_NONE)
			printf("[%s] ",LogBuffer::getTypeStr(t));
		printf("%s\n",str);
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
		case 2:
			if (!quit)
				LOG_DEBUG("Received QUIT signal");
			quit=true;
			break;
	}
}

// ----------------------------------
int main(int argc, char* argv[])
{

	iniFileName.set("peercast.ini");

	if (argc > 2)
		if (strcmp(argv[1],"-inifile")==0)
			iniFileName.setFromString(argv[2]);

	peercastInst = new MyPeercastInst();
	peercastApp = new MyPeercastApp();

	peercastInst->init();


	signal(SIGINT, sigProc); 

	while (!quit)
		sys->sleep(1000);

	peercastInst->saveSettings();

	peercastInst->quit();


	return 0;
}
