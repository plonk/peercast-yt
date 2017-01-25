// ------------------------------------------------
// File : usys.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		LSys derives from Sys to provide basic Linux functions such as starting threads.
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


#include <sys/time.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include "usys.h"
#include "usocket.h"
#include "stats.h"

// ---------------------------------
USys::USys()
{
	stats.clear();

	rndGen.setSeed(rnd()+getpid());	
	signal(SIGPIPE, SIG_IGN); 
	signal(SIGABRT, SIG_IGN); 

	rndSeed = rnd();

}
// ---------------------------------
double USys::getDTime()
{
  struct timeval tv;

  gettimeofday(&tv,0);
  return (double)tv.tv_sec + ((double)tv.tv_usec)/1000000;
}

// ---------------------------------
unsigned int USys::getTime()
{
	time_t ltime;
	time( &ltime );
	return ltime;
}

// ---------------------------------
ClientSocket *USys::createSocket()
{
    return new UClientSocket();
}
               

// ---------------------------------
void USys::endThread(ThreadInfo *info)
{
	numThreads--;

	LOG_DEBUG("End thread: %d",numThreads);

	//pthread_exit(NULL);
}

// ---------------------------------
void USys::waitThread(ThreadInfo *info, int timeout)
{
	//pthread_join(info->handle,NULL);
}



// ---------------------------------
typedef void *(*THREAD_PTR)(void *);
bool	USys::startThread(ThreadInfo *info)
{
	info->active = true;


	LOG_DEBUG("New thread: %d",numThreads);

	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	int r = pthread_create(&info->handle,&attr,(THREAD_PTR)info->func,info);

	pthread_attr_destroy(&attr);

	if (r)
	{
		LOG_ERROR("Error creating thread %d: %d",numThreads,r);
		return false;
	}else
	{
		numThreads++;
		return true;
	}
}
// ---------------------------------
void	USys::sleep(int ms)
{
	::usleep(ms*1000);
}

// ---------------------------------
void USys::appMsg(long msg, long arg)
{
	//SendMessage(mainWindow,WM_USER,(WPARAM)msg,(LPARAM)arg);
}
// ---------------------------------
#ifndef __APPLE__
void USys::getURL(const char *url)
{
}
// ---------------------------------
void USys::callLocalURL(const char *str,int port)
{
} 
// ---------------------------------
void USys::executeFile( const char *file )
{
}
void USys::exit()
{
	::exit(0);
}
#else
// ---------------------------------
void USys::openURL( const char* url )
{
	CFStringRef urlString = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), url );
	
	if( urlString )
	{
		CFURLRef pathRef = CFURLCreateWithString( NULL, urlString, NULL );
		if( pathRef )
		{
			OSStatus err = LSOpenCFURLRef( pathRef, NULL );
			CFRelease(pathRef);
		}
		CFRelease( urlString );
	}
}
// ---------------------------------
void USys::callLocalURL(const char *str,int port)
{
	char cmd[512];
	sprintf(cmd,"http://localhost:%d/%s",port,str);
	openURL( cmd );
} 
// --------------------------------- 
void USys::getURL(const char *url) 
{
	if (strnicmp(url,"http://",7) || strnicmp(url,"mailto:",7))
	{
		openURL( url );
	}
}
// ---------------------------------
void USys::executeFile( const char *file )
{
	CFStringRef fileString = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), file );
	
	if( fileString )
	{
		CFURLRef pathRef = CFURLCreateWithString( NULL, fileString, NULL );
		if( pathRef )
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
