/*
 *  ctimer.h
 *  PeerCast
 *
 *  Created by mode7 on Mon Apr 12 2004.
 *  Copyright (c) 2002-2004 peercast.org. All rights reserved.
 *
 */
 
#ifndef _CTIMER_H
#define _CTIMER_H

#include <Carbon/Carbon.h>

class Timer
{
public:
	explicit Timer( EventLoopRef mainLoop, 
					EventTimerInterval inFireDelay,
					EventTimerInterval inInterval,
					void*			   inTimerData,
					EventLoopTimerProcPtr userRoutine );
					
	~Timer() { RemoveEventLoopTimer( mTimer ); DisposeEventLoopTimerUPP(mTimerUPP); }

private:
	EventLoopTimerUPP  mTimerUPP;
	EventLoopTimerRef  mTimer;
};

#endif // _CTIMER_H