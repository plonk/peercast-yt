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
#include <memory>
#include "subprog.h"

// ------------------------------------------------
class URLSource : public ChannelSource
{
public:
    URLSource(const char *url)
    : inputStream(nullptr)
    {
        baseurl.set(url);
    }

    ::String        streamURL(std::shared_ptr<Channel>, const char *);
    void            stream(std::shared_ptr<Channel>) override;
    int             getSourceRate() override;
    int             getSourceRateAvg() override;

    static ChanInfo::PROTOCOL getSourceProtocol(char*& fileName);

    std::shared_ptr<Subprogram> m_subprogram;

    std::shared_ptr<Stream> inputStream;
    ::String        baseurl;
};

#endif

