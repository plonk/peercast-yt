/*
 *  numericcontrol.cpp
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 05 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */

#include "numericcontrol.h"

void NumericControl::setIntValue( WindowRef window, const int value )
{
	CFStringRef controlText = CFStringCreateWithFormat( NULL, NULL, CFSTR("%d"), value );
	setText( window, controlText );
	CFRelease( controlText );
}

void NumericControl::setFloatValue( WindowRef window, const float value )
{
	CFStringRef controlText = CFStringCreateWithFormat( NULL, NULL, CFSTR("%f"), value );
	setText( window, controlText );
	CFRelease( controlText );
}

int NumericControl::getIntValue( WindowRef window )
{
	return static_cast<int>( getFloatValue( window ) );
}

float NumericControl::getFloatValue( WindowRef window )
{
	CFStringRef stringRef = getStringRef( window );
	
	if( stringRef )
	{
		mValue = CFStringGetDoubleValue( stringRef );
		return mValue;
	}
	
	return 0.0f;
}

