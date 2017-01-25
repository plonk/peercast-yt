/*
 *  ctimer.cpp
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 12 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */

#include "ctimer.h"

Timer::Timer( EventLoopRef		 mainLoop,
			  EventTimerInterval inFireDelay,
			  EventTimerInterval inInterval,
			  void*				 inTimerData,
			  EventLoopTimerProcPtr userRoutine )
: mTimerUPP( NewEventLoopTimerUPP(userRoutine) )
{
	InstallEventLoopTimer( mainLoop
						  ,inFireDelay
						  ,inInterval
						  ,mTimerUPP
						  ,inTimerData
						  ,&mTimer);
}