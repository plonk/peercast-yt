/*
 *  numericcontrol.h
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 05 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */

#ifndef _NUMERICCONTROL_H
#define _NUMERICCONTROL_H

#include "textcontrol.h"

class NumericControl: public TextControl
{
public:
	explicit NumericControl( const int a, const int id )
	:TextControl( a, id )
	{
	}
	
	void setIntValue  ( WindowRef window, const int   value );
	void setFloatValue( WindowRef window, const float value );
	
	int   getIntValue  ( WindowRef window );
	float getFloatValue( WindowRef window );
	
private:
	float mValue;
};

#endif // _NUMERICCONTROL_H