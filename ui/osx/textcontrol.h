/*
 *  textcontrol.h
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
#ifndef _TEXTCONTROL_H
#define _TEXTCONTROL_H

#include <Carbon/Carbon.h>

class TextControl
{
	static const int skMaxBufSize = 256; 
public:
	explicit TextControl( const int a, const int id )
	: mID( )
	 ,mStringRef( NULL )
	{
		mID.signature = a;
		mID.id		  = id;
	}
	
	void setText( WindowRef window, const char* text );
	void setText( WindowRef window, CFStringRef text );
	
	const char* getString( WindowRef window, CFStringEncoding encoding )
	{
		if( CFStringRef stringRef = getStringRef(window) )
		{
			if( CFStringGetCString( stringRef, mStrBuffer, skMaxBufSize, encoding ) )
			{
				return mStrBuffer;
			}
		}
		
		return skNullString; 
	}

protected:	
	CFStringRef getStringRef( WindowRef window );
	
private:
	ControlID   mID;
	CFStringRef mStringRef;
	
	char mStrBuffer[skMaxBufSize];
	
	static const char* const skNullString;
	
};

#endif // _TEXTCONTROL_H
