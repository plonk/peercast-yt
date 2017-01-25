// ------------------------------------------------
// File : main.cpp
// Date: ??-apr-2004
// Author: giles & mode7
// Desc: 
//		see .cpp for details
//		
// (c) 2002-2004 peercast.org
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
#include <Carbon/Carbon.h>

#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include "channel.h"
#include "servent.h"
#include "servmgr.h"
#include "textcontrol.h"
#include "numericcontrol.h"
#include "databrowser.h"
#include "osxapp.h"
#include "ctimer.h"
#include "xcode_main.h"
//---------------------------------------------------------------------------------------------
//#define DISABLE_PEERCAST
//---------------------------------------------------------------------------------------------
const ControlID dbControlID			= { 'PCRE', 1000 };
//---------------------------------------------------------------------------------------------
bool quit=false;
//---------------------------------------------------------------------------------------------
void relayBrowserUpdate(EventLoopTimerRef theTimer, void* userData)
{
	OSXPeercastApp *pOsxPeerCast = static_cast<OSXPeercastApp *>(userData);
	
	if( pOsxPeerCast )
	{
		pOsxPeerCast->redisplayInfo();
	}
}
//-------------------------------------------
void handleGetURLEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
    AEDesc		directParam={'NULL', NULL};
	WindowPtr	targetWindow = NULL;
	OSErr		err;
	
	// extract the direct parameter (an object specifier)
	err = AEGetKeyDesc(appleEvent, keyDirectObject, typeWildCard, &directParam);
//!	ThrowIfOSErr(err);

	// we need to look for other parameters, to do with destination etc.
	long		dataSize = AEGetDescDataSize( &directParam );
	char*	urlString = new char[dataSize + 1];
	
	AEGetDescData( &directParam, urlString, dataSize );
	urlString[dataSize] = 0;

	LOG_DEBUG("URL request: %s",urlString);
	ChanInfo info;
	servMgr->procConnectArgs(&urlString[15],info);
	chanMgr->findAndPlayChannel(info,false);

	delete [] urlString;
	AEDisposeDesc( &directParam );
}
//-------------------------------------------
OSErr gurlHan( const AppleEvent* theAppleEvent, AppleEvent* reply, SInt32 handlerRefcon )
{
	OSErr		err = noErr;
	
	AEEventID	eventID;
	OSType		typeCode;
	Size		actualSize 	= 0L;
	
	// Get the event ID
	err = AEGetAttributePtr( theAppleEvent
							,keyEventIDAttr
							,typeType
							,&typeCode
							,(Ptr)&eventID
							,sizeof(eventID)
							,&actualSize);
							
	if( eventID == 'GURL' )
	{
		handleGetURLEvent( theAppleEvent, reply );
	}
	
	return err;
}

void InstallAppleEventHandlers(void)
{
	AEInstallEventHandler('GURL', typeWildCard, NewAEEventHandlerUPP( gurlHan ), 0, false);
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
OSStatus runApp( int argc, char* argv[], WindowRef window )
{
	CAppPathInfo appPathInfo;

	std::string filePath( appPathInfo.getPath() );
	filePath += "peercast.ini";	
	
	InstallAppleEventHandlers();
#ifndef DISABLE_PEERCAST
	peercastInst = new OSXPeercastInst();
	peercastApp  = new OSXPeercastApp( appPathInfo.getPath(), filePath.c_str(), argc, argv, window, dbControlID );
	peercastInst->init();
	
	signal(SIGINT, sigProc); 
#endif
	OSXPeercastApp *pOSXApp = static_cast<OSXPeercastApp *>(peercastApp);
	pOSXApp->initAPP();
	
	//////////////////////////
	
	Timer relayBrowserTimer(  GetMainEventLoop()
							 ,3*kEventDurationSecond
							 ,kEventDurationSecond
							 ,pOSXApp
							 ,relayBrowserUpdate );
	//////////////////////////
   
	// The window was created hidden so show it.
	ShowWindow( window );

	// Call the event loop
	RunApplicationEventLoop();

#ifndef DISABLE_PEERCAST
	peercastInst->saveSettings();
	peercastInst->quit();
#endif

	return noErr;
}
// ----------------------------------
int main(int argc, char* argv[])
{
    IBNibRef 		nibRef;
    WindowRef 		window;
    
    OSStatus		err;

    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );
    
    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
    require_noerr( err, CantSetMenuBar );
    
    // Then create a window. "MainWindow" is the name of the window object. This name is set in 
    // InterfaceBuilder when the nib is created.
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &window);
    require_noerr( err, CantCreateWindow );

    // We don't need the nib reference anymore.
    DisposeNibReference(nibRef);
	
	try
	{
		runApp( argc, argv, window );
	}
	
	catch( const CAppPathInfo::EPathErr pathError )
	{
		printf("[APP PATH ERROR]: # %d´n", pathError );
	}
	
CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
	return err;
}

