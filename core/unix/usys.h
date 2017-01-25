// ------------------------------------------------
// File : usys.h
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		WSys derives from Sys to provide basic win32 functions such as starting threads.
//
// (c) 2002 peercast.org
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



#ifndef _USYS_H
#define _USYS_H
// ------------------------------------
#include "socket.h"
#include "sys.h"

// ------------------------------------
class USys : public Sys
{
public:
	USys();

	virtual	ClientSocket	*createSocket();
	virtual bool			startThread(ThreadInfo *);
	virtual void			sleep(int );
	virtual void			appMsg(long,long);
	virtual unsigned int	getTime();
	virtual double			getDTime();
	virtual unsigned int	rnd() {return rndGen.next();}
	virtual void			getURL(const char *);
	virtual void			exit();
	virtual bool			hasGUI() {return false;}
	virtual void			callLocalURL(const char *,int);
	virtual void			executeFile(const char *);
	virtual void			endThread(ThreadInfo *);
	virtual void			waitThread(ThreadInfo *, int timeout = 30000);


	peercast::Random rndGen;
private:

#ifdef __APPLE__
	void openURL( const char* url );
#endif
};                               


// ------------------------------------
#endif
