/*
 *  textcontrol.cpp
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 05 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */
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

#include "textcontrol.h"

const char* const TextControl::skNullString = "";

void TextControl::setText( WindowRef window, const char* text )
{
	CFStringRef controlText = CFStringCreateWithFormat( NULL, NULL, CFSTR("%s"), text );
	setText( window, controlText );
	CFRelease( controlText );
}

void TextControl::setText( WindowRef window, CFStringRef text )
{
	ControlRef controlRef = NULL;
	
	OSStatus err = GetControlByID( window, &mID, &controlRef );
	
	err = SetControlData(   controlRef
						   ,0
						   ,kControlEditTextCFStringTag
						   ,sizeof(CFStringRef)
						   ,&text );
								   
	DrawOneControl( controlRef );
}

CFStringRef TextControl::getStringRef( WindowRef window )
{
	if( mStringRef == NULL )
	{
		ControlRef controlRef=NULL;
		
		OSStatus err = GetControlByID( window, &mID, &controlRef );
		err = GetControlData( controlRef
							 ,0
							 ,kControlEditTextCFStringTag
							 ,sizeof(CFStringRef)
							 ,&mStringRef
							 ,NULL );
	}
	
	return mStringRef;	
}