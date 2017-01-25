// ------------------------------------------------
// File : wsocket.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Windows version of ClientSocket. Handles the nitty gritty of actually
//		reading and writing TCP
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

// TODO: fix socket closing



#include "winsock2.h"
#include <windows.h>
#include <stdio.h>
#include "wsocket.h"
#include "stats.h"


// --------------------------------------------------
void WSAClientSocket::init()
{
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
    
	wVersionRequested = MAKEWORD( 2, 0 );
	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 )
		throw SockException("Unable to init sockets");

    //LOG4("WSAStartup:  OK");

}
// --------------------------------------------------
bool ClientSocket::getHostname(char *str,unsigned int ip)
{
	HOSTENT *he;

	ip = htonl(ip);

	he = gethostbyaddr((char *)&ip,sizeof(ip),AF_INET);

	if (he)
	{
		strcpy(str,he->h_name);
		return true;
	}else
		return false;
}

// --------------------------------------------------
unsigned int ClientSocket::getIP(char *name)
{

	char szHostName[256];

	if (!name)
	{
		if (gethostname(szHostName, sizeof(szHostName))==0)
			name = szHostName;
		else
			return 0;
	}

	HOSTENT *he = WSAClientSocket::resolveHost(name);

	if (!he)
		return 0;


	LPSTR lpAddr = he->h_addr_list[0];
	if (lpAddr)
	{
		struct in_addr  inAddr;
		memmove (&inAddr, lpAddr, 4);

		return inAddr.S_un.S_un_b.s_b1<<24 |
			   inAddr.S_un.S_un_b.s_b2<<16 |
			   inAddr.S_un.S_un_b.s_b3<<8 |
			   inAddr.S_un.S_un_b.s_b4;
	}
	return 0;
}
// --------------------------------------------------
void WSAClientSocket::setLinger(int sec)
{
	linger linger;
	linger.l_onoff = (sec>0)?1:0;
    linger.l_linger = sec;

	if (setsockopt(sockNum, SOL_SOCKET, SO_LINGER, (const char *)&linger, sizeof (linger)) == -1) 
		throw SockException("Unable to set LINGER");
}

// --------------------------------------------------
void WSAClientSocket::setBlocking(bool yes)
{
	unsigned long op = yes ? 0 : 1;
	if (ioctlsocket(sockNum, FIONBIO, &op) == SOCKET_ERROR)
		throw SockException("Can`t set blocking");
}
// --------------------------------------------------
void WSAClientSocket::setNagle(bool on)
{
    int nodelay = (on==false);
	if (setsockopt(sockNum, SOL_SOCKET, TCP_NODELAY, (char *)&nodelay, sizeof nodelay) == -1) 
		throw SockException("Unable to set NODELAY");

}

// --------------------------------------------------
void WSAClientSocket::setReuse(bool yes)
{
	unsigned long op = yes ? 1 : 0;
	if (setsockopt(sockNum,SOL_SOCKET,SO_REUSEADDR,(char *)&op,sizeof(unsigned long)) == -1) 
		throw SockException("Unable to set REUSE");
}

// --------------------------------------------------
HOSTENT *WSAClientSocket::resolveHost(const char *hostName)
{
	HOSTENT *he;

	if ((he = gethostbyname(hostName)) == NULL)
	{
		// if failed, try using gethostbyaddr instead

		unsigned long ip = inet_addr(hostName);
		
		if (ip == INADDR_NONE)
			return NULL;

		if ((he = gethostbyaddr((char *)&ip,sizeof(ip),AF_INET)) == NULL)
			return NULL;	
	}
	return he;
}

// --------------------------------------------------
void WSAClientSocket::open(Host &rh)
{
	sockNum = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sockNum == INVALID_SOCKET)
		throw SockException("Can`t open socket");

	setBlocking(false);
#ifdef DISABLE_NAGLE
	setNagle(false);
#endif

	host = rh;

	memset(&remoteAddr,0,sizeof(remoteAddr));

	remoteAddr.sin_family = AF_INET;
	remoteAddr.sin_port = htons(host.port);
	remoteAddr.sin_addr.S_un.S_addr = htonl(host.ip);

}
// --------------------------------------------------
void WSAClientSocket::checkTimeout(bool r, bool w)
{
    int err = WSAGetLastError();
    if (err == WSAEWOULDBLOCK)
    {

		timeval timeout;
		fd_set read_fds;
		fd_set write_fds;

		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

        FD_ZERO (&write_fds);
		if (w)
		{
			timeout.tv_sec = (int)this->writeTimeout/1000;
			FD_SET (sockNum, &write_fds);
		}

        FD_ZERO (&read_fds);
		if (r)
		{
			timeout.tv_sec = (int)this->readTimeout/1000;
	        FD_SET (sockNum, &read_fds);
		}

		timeval *tp;
		if (timeout.tv_sec)
			tp = &timeout;
		else
			tp = NULL;


		int r=select (NULL, &read_fds, &write_fds, NULL, tp);

        if (r == 0)
			throw TimeoutException();
		else if (r == SOCKET_ERROR)
			throw SockException("select failed.");

	}else{
		char str[32];
		sprintf(str,"%d",err);
		throw SockException(str);
	}
}

// --------------------------------------------------
void WSAClientSocket::checkTimeout2(bool r, bool w)
{
    {

		timeval timeout;
		fd_set read_fds;
		fd_set write_fds;

		timeout.tv_sec = 0;
		timeout.tv_usec = 0;

        FD_ZERO (&write_fds);
		if (w)
		{
			timeout.tv_sec = (int)this->writeTimeout/1000;
			FD_SET (sockNum, &write_fds);
		}

        FD_ZERO (&read_fds);
		if (r)
		{
			timeout.tv_sec = (int)this->readTimeout/1000;
	        FD_SET (sockNum, &read_fds);
		}

		timeval *tp;
		if (timeout.tv_sec)
			tp = &timeout;
		else
			tp = NULL;


		int r=select (NULL, &read_fds, &write_fds, NULL, tp);

        if (r == 0)
			throw TimeoutException();
		else if (r == SOCKET_ERROR)
			throw SockException("select failed.");
	}
}

// --------------------------------------------------
Host WSAClientSocket::getLocalHost()
{
	struct sockaddr_in localAddr;

	int len = sizeof(localAddr);
    if (getsockname(sockNum, (sockaddr *)&localAddr, &len) == 0)
		return Host(SWAP4(localAddr.sin_addr.s_addr),0);
	else
		return Host(0,0);
}

// --------------------------------------------------
void WSAClientSocket::connect()
{
	if (::connect(sockNum,(struct sockaddr *)&remoteAddr,sizeof(remoteAddr)) == SOCKET_ERROR)
		checkTimeout(false,true);

}
// --------------------------------------------------
int WSAClientSocket::read(void *p, int l)
{
	int bytesRead=0;
	while (l)
	{
		int r = recv(sockNum, (char *)p, l, 0);
		if (r == SOCKET_ERROR)
		{
			// non-blocking sockets always fall through to here
			checkTimeout(true,false);

		}else if (r == 0)
		{
			throw EOFException("Closed on read");

		}else
		{
			stats.add(Stats::BYTESIN,r);
			if (host.localIP())
				stats.add(Stats::LOCALBYTESIN,r);
			updateTotals(r,0);
			bytesRead += r;
			l -= r;
			p = (char *)p+r;
		}
	}
	return bytesRead;
}
// --------------------------------------------------
int WSAClientSocket::readUpto(void *p, int l)
{
	int bytesRead=0;
	while (l)
	{
		int r = recv(sockNum, (char *)p, l, 0);
		if (r == SOCKET_ERROR)
		{
			// non-blocking sockets always fall through to here
			checkTimeout(true,false);

		}else if (r == 0)
		{
			break;
		}else
		{
			stats.add(Stats::BYTESIN,r);
			if (host.localIP())
				stats.add(Stats::LOCALBYTESIN,r);
			updateTotals(r,0);
			bytesRead += r;
			l -= r;
			p = (char *)p+r;
		}
	}
	return bytesRead;
}


// --------------------------------------------------
void WSAClientSocket::write(const void *p, int l)
{
	while (l)
	{
		int r = send(sockNum, (char *)p, l, 0);
		if (r == SOCKET_ERROR)
		{
			checkTimeout(false,true);	
		}
		else if (r == 0)
		{
			throw SockException("Closed on write");
		}
		else
		if (r > 0)
		{
			stats.add(Stats::BYTESOUT,r);
			if (host.localIP())
				stats.add(Stats::LOCALBYTESOUT,r);

			updateTotals(0,r);
			l -= r;
			p = (char *)p+r;
		}
	}
}


// --------------------------------------------------
void WSAClientSocket::bind(Host &h)
{
	struct sockaddr_in localAddr;

	if ((sockNum = socket (PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
		throw SockException("Can`t open socket");

	setBlocking(false);
	setReuse(true);

	memset(&localAddr,0,sizeof(localAddr));
	localAddr.sin_family = AF_INET;
	localAddr.sin_port = htons(h.port);
	localAddr.sin_addr.s_addr = INADDR_ANY;

	if( ::bind (sockNum, (sockaddr *)&localAddr, sizeof(localAddr)) == -1)
		throw SockException("Can`t bind socket");

	if (::listen(sockNum,SOMAXCONN))
		throw SockException("Can`t listen",WSAGetLastError());

	host = h;
}
// --------------------------------------------------
ClientSocket *WSAClientSocket::accept()
{

	int fromSize = sizeof(sockaddr_in);
	sockaddr_in from;

	int conSock = ::accept(sockNum,(sockaddr *)&from,&fromSize);


	if (conSock ==  INVALID_SOCKET)
		return NULL;

	
    WSAClientSocket *cs = new WSAClientSocket();
	cs->sockNum = conSock;

	cs->host.port = from.sin_port;
	cs->host.ip = from.sin_addr.S_un.S_un_b.s_b1<<24 |
				  from.sin_addr.S_un.S_un_b.s_b2<<16 |
				  from.sin_addr.S_un.S_un_b.s_b3<<8 |
				  from.sin_addr.S_un.S_un_b.s_b4;


	cs->setBlocking(false);
#ifdef DISABLE_NAGLE
	cs->setNagle(false);
#endif

	return cs;
}

// --------------------------------------------------
void WSAClientSocket::close()
{
	if (sockNum)
	{
		shutdown(sockNum,SD_SEND);

		setReadTimeout(2000);
		try
		{
			//char c;
			//while (readUpto(&c,1)!=0);
			//readUpto(&c,1);
		}catch(StreamException &) {}

		if (closesocket(sockNum))
			LOG_ERROR("closesocket() error");


		sockNum=0;
	}
}

// --------------------------------------------------
bool	WSAClientSocket::readReady()
{
	timeval timeout;
	fd_set read_fds;

	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

    FD_ZERO (&read_fds);
    FD_SET (sockNum, &read_fds);

	return select (sockNum+1, &read_fds, NULL, NULL, &timeout) == 1;
}

