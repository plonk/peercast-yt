// ------------------------------------------------
// File : http.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		HTTP protocol handling
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


#include <stdlib.h>
#include "http.h"
#include "sys.h"
#include "common.h"

//-----------------------------------------
bool HTTP::checkResponse(int r)
{
	if (readResponse()!=r)
	{
		LOG_ERROR("Unexpected HTTP: %s",cmdLine);
		throw StreamException("Unexpected HTTP response");
		return false;
	}
	
	return true;
}
//-----------------------------------------
void HTTP::readRequest()
{
	readLine(cmdLine,sizeof(cmdLine));
}
//-----------------------------------------
int HTTP::readResponse()
{
	readLine(cmdLine,sizeof(cmdLine));

	char *cp = cmdLine;

	while (*cp)	if (*++cp == ' ') break;
	while (*cp) if (*++cp != ' ') break;

	char *scp = cp;

	while (*cp)	if (*++cp == ' ') break;
	*cp = 0;

	return atoi(scp);
}
	
//-----------------------------------------
bool	HTTP::nextHeader()
{
	if (readLine(cmdLine,sizeof(cmdLine)))
	{
		char *ap = strstr(cmdLine,":");
		if (ap)
			while (*++ap)
				if (*ap!=' ')
					break;
		arg = ap;
		return true;
	}else
	{
		arg = NULL;
		return false;
	}

}
//-----------------------------------------
bool	HTTP::isHeader(const char *hs)
{
	return stristr(cmdLine,hs) != NULL;
}
//-----------------------------------------
bool	HTTP::isRequest(const char *rq)
{
	return strncmp(cmdLine,rq,strlen(rq)) == 0;
}
//-----------------------------------------
char *HTTP::getArgStr()
{
	return arg;
}
//-----------------------------------------
int	HTTP::getArgInt()
{
	if (arg) 
		return atoi(arg);
	else 
		return 0;
}
//-----------------------------------------
void HTTP::getAuthUserPass(char *user, char *pass)
{
	if (arg)
	{
		char *s = stristr(arg,"Basic");
		if (s)
		{
			while (*s)
				if (*s++ == ' ')
					break;
			String str;
			str.set(s,String::T_BASE64);
			str.convertTo(String::T_ASCII);
			s = strstr(str.cstr(),":");
			if (s)
			{
				*s = 0;
				if (user)
					strcpy(user,str.cstr());
				if (pass)
					strcpy(pass,s+1);
			}			
		}
	}
}
// -----------------------------------
void	CookieList::init()
{
	for(int i=0; i<MAX_COOKIES; i++)
		list[i].clear();

	neverExpire = false;
}

// -----------------------------------
bool	CookieList::contains(Cookie &c)
{
	if ((c.id[0]) && (c.ip))
		for(int i=0; i<MAX_COOKIES; i++)
			if (list[i].compare(c))
				return true;

	return false;
}
// -----------------------------------
void	Cookie::logDebug(const char *str, int ind)
{
	char ipstr[64];
	Host h;
	h.ip = ip;
	h.IPtoStr(ipstr);

	LOG_DEBUG("%s %d: %s - %s",str,ind,ipstr,id);
}

// -----------------------------------
bool	CookieList::add(Cookie &c)
{
	if (contains(c))
		return false;

	unsigned int oldestTime=(unsigned int)-1; 
	int oldestIndex=0;

	for(int i=0; i<MAX_COOKIES; i++)
		if (list[i].time <= oldestTime)
		{
			oldestIndex = i;
			oldestTime = list[i].time;
		}

	c.logDebug("Added cookie",oldestIndex);
	c.time = sys->getTime();
	list[oldestIndex]=c;
	return true;
}
// -----------------------------------
void	CookieList::remove(Cookie &c)
{
	for(int i=0; i<MAX_COOKIES; i++)
		if (list[i].compare(c))
			list[i].clear();
}
