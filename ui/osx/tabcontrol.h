// ------------------------------------------------
//  tabcontrol.h
//  PeerCast
//
//  Created by mode7 on Fri Apr 02 2004.
//  Copyright (c) 2002-2004 peercast.org. All rights reserved.
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

#ifndef _TABCONTROL_H
#define _TABCONTROL_H

#include <Carbon/Carbon.h>
#include "carbonwraps.h"
#include "textcontrol.h"
#include "numericcontrol.h"
#include "sys.h"

class TabControl
{
public:
	explicit TabControl( const int signature, const int id, WindowRef windowRef );
	const ControlID& getID    () const { return mControlID; }
	WindowRef        getWindow() const { return mWindowRef; }
	
	OSStatus installHandler();	
	void updateSettings    ();
	void saveSettings();
	
	static OSStatus handler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );

private:

	void setInitialTab(WindowRef window);

	static ControlRef getControlRef( WindowRef window, const ControlID& controlID );
	static void       switchItem   ( WindowRef window, ControlRef tabControl, const short currentTabIndex );

	ControlID		 mControlID;
	WindowRef		 mWindowRef;
	CEventHandlerUPP mEventHandler;
	TextControl		 mYPAddress;
	TextControl		 mDJMessage;
	TextControl		 mPassword;
	NumericControl   mMaxRelays;
	WLock			 mLock;
	short			 mLastTabIndex;
};

#endif // _TABCONTROL_H