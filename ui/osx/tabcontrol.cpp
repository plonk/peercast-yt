// ------------------------------------------------
//  tabcontrol.cpp
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

#include "tabcontrol.h"
#include "osxapp.h"

static int tabList[] = { 2, 1001, 1002 };
static ControlID skTabItemID = { 'PCTB', 0 };

TabControl::TabControl( const int signature, const int id, WindowRef windowRef )
:mControlID		  ( )
,mWindowRef		  ( windowRef )
,mEventHandler    ( TabControl::handler )
,mYPAddress		  ('PCCG', 1000)
,mDJMessage		  ('PCCG', 1001)
,mPassword		  ('PCCG', 1002)
,mMaxRelays		  ('PCCG', 1003)
,mLastTabIndex    ( 1 )
{
	mControlID.signature = signature;
	mControlID.id        = id;
	
	setInitialTab( windowRef );
}

OSStatus TabControl::installHandler()
{
	ControlRef tabControlRef;
	
	static const EventTypeSpec tabControlEvents[] =
	{
		 { kEventClassControl, kEventControlHit }
		,{ kEventClassWindow,  kEventWindowActivated }
	};
	
	GetControlByID( getWindow(), &getID(), &tabControlRef );	

	return InstallControlEventHandler(  tabControlRef
								       ,mEventHandler.eventHandler()
									   ,GetEventTypeCount(tabControlEvents)
									   ,tabControlEvents
									   ,this
									   ,NULL );
}
//------------------------------------------------------------------------
void TabControl::updateSettings()
{
	if( !servMgr || !chanMgr )
		return;
		
	mLock.on();
	mYPAddress.setText( mWindowRef, servMgr->rootHost );
	mDJMessage.setText( mWindowRef, chanMgr->broadcastMsg );
	mPassword.setText( mWindowRef, servMgr->password );
	mMaxRelays.setIntValue( mWindowRef, servMgr->maxRelays );
	mLock.off();
}
//------------------------------------------------------------------------
void TabControl::saveSettings()
{
	if( !servMgr || !chanMgr )
		return;

	mLock.on();
	chanMgr->broadcastMsg.set(mDJMessage.getString( mWindowRef, kCFStringEncodingUnicode ),String::T_ASCII);
	servMgr->rootHost   = mYPAddress.getString( mWindowRef, kCFStringEncodingASCII );
	strncpy( servMgr->password, mPassword.getString( mWindowRef, kCFStringEncodingASCII ), 64 );
	servMgr->setMaxRelays( mMaxRelays.getIntValue( mWindowRef ) );
	mLock.off();
}
//------------------------------------------------------------------------
void TabControl::setInitialTab(WindowRef window)
{
	ControlID controlID = skTabItemID;
	
	for(short i=1; i<tabList[0]+1; ++i)
	{
		controlID.id = tabList[i];	
		SetControlVisibility( getControlRef( window, controlID ), false, true );
	}

	// set
	SetControlValue( getControlRef(window, mControlID), mLastTabIndex );

	controlID.id = tabList[mLastTabIndex];
	SetControlVisibility( getControlRef(window, controlID), true, true );
}

ControlRef TabControl::getControlRef( WindowRef window, const ControlID& controlID )
{
	ControlRef controlRef = NULL;
	GetControlByID( window, &controlID, &controlRef );
	
	return controlRef;
}

OSStatus TabControl::handler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
	switch( GetEventKind( inEvent ) )
	{
		case kEventControlHit:
			{
				TabControl& tControl = *static_cast<TabControl *>(inUserData);
				WindowRef   window   = static_cast<WindowRef>(tControl.getWindow());

				ControlRef  tabControl = getControlRef( window, tControl.getID() );

				const short controlValue = GetControlValue( tabControl );
	
				if( controlValue != tControl.mLastTabIndex )
				{
					tControl.updateSettings();
					
					switchItem( window, tabControl, controlValue );
					tControl.mLastTabIndex = controlValue;
					return noErr;
				}
			}
			break;
	}
	
	return eventNotHandledErr;
}

void TabControl::switchItem( WindowRef window, ControlRef tabControl, const short currentTabIndex )
{
	ControlRef selectedPaneControl = NULL;
	ControlID  controlID           = skTabItemID;
	
	for(short i=1; i<tabList[0]+1; ++i)
	{
		controlID.id = tabList[i];
		ControlRef userPaneControl = getControlRef( window, controlID );
		
		if( i == currentTabIndex )
		{
			selectedPaneControl = userPaneControl;
		}
		else
		{
			SetControlVisibility( userPaneControl, false, true );
		}
	}
	
	if( selectedPaneControl != NULL )
	{
		(void)ClearKeyboardFocus( window );
		SetControlVisibility( selectedPaneControl, true, true );
	}
	
	Draw1Control( tabControl );
}