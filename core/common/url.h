// ------------------------------------------------
// File : url.h
// Date: 20-feb-2004
// Author: giles
//
// (c) 2002-4 peercast.org
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

#ifndef _URL_H
#define _URL_H

#include "channel.h"

// ------------------------------------------------
class URLSource : public ChannelSource
{
public:
	URLSource(const char *url) 
	:inputStream(NULL)
	{
		baseurl.set(url);
	}

	::String streamURL(Channel *, const char *);	

	virtual void stream(Channel *);

	virtual int getSourceRate();


	Stream		*inputStream;
	::String	baseurl;
};




#endif

