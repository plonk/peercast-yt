// ------------------------------------------------
// File : wsocket.h
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		see .cpp for details
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

#ifndef _WSOCKET_H
#define _WSOCKET_H

#include <windows.h>
#include "socket.h"


// --------------------------------------------------
class WSAClientSocket : public ClientSocket
{
public:
	static void	init();

    WSAClientSocket()
	:sockNum(0)
	,writeCnt(0)
    {
    }


	virtual void	open(Host &);
	virtual int		read(void *, int);
	virtual int		readUpto(void *, int);
	virtual void	write(const void *, int);
	virtual void	bind(Host &);
	virtual void	connect();
	virtual void	close();
	virtual ClientSocket * accept();
	virtual bool	active() {return sockNum != 0;}
	virtual bool	readReady();
	virtual Host 	getLocalHost();
	virtual void	setBlocking(bool);
	void	setReuse(bool);
	void	setNagle(bool);
	void	setLinger(int);

	static	HOSTENT		*resolveHost(const char *);

	void	checkTimeout(bool,bool);
	void	checkTimeout2(bool,bool);

	unsigned int writeCnt;
	unsigned int sockNum;
	struct sockaddr_in remoteAddr;


};




#endif
 