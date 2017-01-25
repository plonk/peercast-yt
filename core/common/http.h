// ------------------------------------------------
// File : http.h
// Date: 4-apr-2002
// Author: giles
// Desc: 
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

#ifndef _HTTP_H
#define _HTTP_H

#include "stream.h"

// -------------------------------------
class HTTPException : public StreamException
{
public:
	HTTPException(const char *m, int c) : StreamException(m) {code=c;}
	int code;
};

// --------------------------------------------
static const char *HTTP_SC_OK			= "HTTP/1.0 200 OK";
static const char *HTTP_SC_NOTFOUND		= "HTTP/1.0 404 Not Found";
static const char *HTTP_SC_UNAVAILABLE	= "HTTP/1.0 503 Service Unavailable";
static const char *HTTP_SC_UNAUTHORIZED	= "HTTP/1.0 401 Unauthorized";
static const char *HTTP_SC_FOUND		= "HTTP/1.0 302 Found";
static const char *HTTP_SC_BADREQUEST	= "HTTP/1.0 400 Bad Request";
static const char *HTTP_SC_FORBIDDEN	= "HTTP/1.0 403 Forbidden";
static const char *HTTP_SC_SWITCH		= "HTTP/1.0 101 Switch protocols";

static const char *RTSP_SC_OK			= "RTSP/1.0 200 OK";


static const char *HTTP_PROTO1		= "HTTP/1.";
static const char *RTSP_PROTO1		= "RTSP/1.";

static const char *HTTP_HS_SERVER		= "Server:";
static const char *HTTP_HS_AGENT		= "User-Agent:"; 
static const char *HTTP_HS_CONTENT		= "Content-Type:"; 
static const char *HTTP_HS_CACHE		= "Cache-Control:"; 
static const char *HTTP_HS_CONNECTION	= "Connection:"; 
static const char *HTTP_HS_SETCOOKIE	= "Set-Cookie:";
static const char *HTTP_HS_COOKIE		= "Cookie:";
static const char *HTTP_HS_HOST			= "Host:";
static const char *HTTP_HS_ACCEPT		= "Accept:";
static const char *HTTP_HS_LENGTH		= "Content-Length:";

static const char *MIME_MP3			= "audio/mpeg";
static const char *MIME_XMP3		= "audio/x-mpeg";
static const char *MIME_OGG			= "application/ogg";
static const char *MIME_XOGG		= "application/x-ogg";
static const char *MIME_MOV			= "video/quicktime";
static const char *MIME_MPG			= "video/mpeg";
static const char *MIME_NSV			= "video/nsv";
static const char *MIME_ASF			= "video/x-ms-asf";
static const char *MIME_ASX			= "video/x-ms-asf";	// same as ASF
static const char *MIME_MMS			= "application/x-mms-framed";

static const char *MIME_RAM			= "audio/x-pn-realaudio";


static const char *MIME_WMA			= "audio/x-ms-wma";
static const char *MIME_WMV			= "video/x-ms-wmv";

static const char *MIME_HTML		= "text/html";
static const char *MIME_XML			= "text/xml";
static const char *MIME_CSS			= "text/css";
static const char *MIME_TEXT		= "text/plain";
static const char *MIME_PLS			= "audio/mpegurl";
static const char *MIME_XPLS		= "audio/x-mpegurl";
static const char *MIME_XSCPLS		= "audio/x-scpls";
static const char *MIME_SDP			= "application/sdp";
static const char *MIME_M3U			= "audio/m3u";
static const char *MIME_MPEGURL		= "audio/mpegurl";
static const char *MIME_XM3U		= "audio/x-mpegurl";
static const char *MIME_XPEERCAST	= "application/x-peercast";
static const char *MIME_XPCP		= "application/x-peercast-pcp";
static const char *MIME_RAW			= "application/binary";
static const char *MIME_JPEG		= "image/jpeg";
static const char *MIME_GIF			= "image/gif";
static const char *MIME_PNG			= "image/png";


// --------------------------------------------
class Cookie
{
public:
	Cookie()
	{
		clear();
	}

	void	clear()
	{
		time = 0;
		ip = 0;
		id[0]=0;
	}

	void	set(const char *i, unsigned int nip)
	{
		strncpy(id,i,sizeof(id)-1);
		id[sizeof(id)-1]=0;
		ip = nip;
	}
	bool	compare(Cookie &c)
	{
		if (c.ip == ip)
			if (strcmp(c.id,id)==0)
				return true;

		return false;
	}

	void	logDebug(const char *,int);

	unsigned int ip;
	char	id[64];
	unsigned int time;
};

// --------------------------------------------
class CookieList
{
public:
	enum {
		MAX_COOKIES = 32
	};



	void	init();
	bool	add(Cookie &);
	void	remove(Cookie &);
	bool	contains(Cookie &);


	Cookie list[MAX_COOKIES];
	bool	neverExpire;

};

// --------------------------------------------
class HTTP : public IndirectStream
{
public:
	HTTP(Stream &s)
	{
		init(&s);
	}

	void	initRequest(const char *r)
	{
		strcpy(cmdLine,r);
	}
	void	readRequest();
	bool	isRequest(const char *);

	int		readResponse();
	bool	checkResponse(int);

	bool	nextHeader();
	bool	isHeader(const char *);
	char	*getArgStr();
	int		getArgInt();

	void	getAuthUserPass(char *, char *);

	char	cmdLine[8192],*arg;

};

#endif
