/*
 *  carbonwraps.h
 *  PeerCast
 *
 *  Created by mode7 on Mon Jun 14 2004.
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
#ifndef _CARBONWRAPS_H
#define _CARBONWRAPS_H

#include <Carbon/Carbon.h>

class CEventHandlerUPP
{
public:
	explicit CEventHandlerUPP( EventHandlerProcPtr userRoutine )
	: mHandler( NewEventHandlerUPP( userRoutine ) )
	{
	}
	
	EventHandlerUPP& eventHandler() { return mHandler; }
	
	~CEventHandlerUPP()
	{
		if( mHandler != NULL )
		{
			DisposeEventHandlerUPP( mHandler );
		}
	}
private:
	// disable copy and assignment
	CEventHandlerUPP           ( const CEventHandlerUPP& );
	CEventHandlerUPP& operator=( const CEventHandlerUPP& );
	
	EventHandlerUPP mHandler;
};

class CEvent
{
public:
	explicit CEvent( const UInt32 eventClass, const UInt32 eventKind )
	:mEventRef( NULL )
	{
		CreateEvent( NULL, eventClass, eventKind, GetCurrentEventTime(), kEventAttributeNone, &mEventRef );
	}
	
	~CEvent()
	{
		if( mEventRef != NULL )
		{
			ReleaseEvent( mEventRef );
		}
	}
	
	OSStatus setParameter( EventParamName inName, EventParamType inType, UInt32 inSize, const void * inDataPtr )
	{
		if( mEventRef != NULL )
		{
			return SetEventParameter( mEventRef, inName, inType, inSize, inDataPtr );
		}
		
		return eventInternalErr;
	}
	
	OSStatus postToQueue( EventQueueRef inQueue, EventPriority inPriority )
	{
		if( mEventRef != NULL )
		{
			return PostEventToQueue( inQueue, mEventRef, inPriority );
		}
		
		return eventInternalErr;
	}
	
private:
	// disable copy and assignment
	CEvent           ( const CEvent& );
	CEvent& operator=( const CEvent& );

	EventRef mEventRef;
};

#endif // _CARBONWRAPS_H

