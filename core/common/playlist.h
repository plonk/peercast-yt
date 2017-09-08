// ------------------------------------------------
// File : playlist.h
// Date: 4-apr-2002
// Author: giles
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

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include "channel.h"
#include "stream.h"

class ChanInfo;

// ----------------------------------
class PlayList
{
public:

    enum TYPE
    {
        T_NONE,
        T_SCPLS, // SHOUTcast playlist
        T_PLS,
        T_ASX,
        T_RAM,
    };

    PlayList(TYPE t, int max)
    {
        maxURLs = max;
        numURLs = 0;
        type = t;
        urls = new ::String[max];
        titles = new ::String[max];
        wmvProtocol = "http";
    }

    ~PlayList()
    {
        delete [] urls;
        delete [] titles;
    }

    void    addURL(const char *url, const char *tit)
    {
        if (numURLs < maxURLs)
        {
            urls[numURLs].set(url);
            titles[numURLs].set(tit);
            numURLs++;
        }
    }
    void    addChannels(const char *, Channel **, int);
    void    addChannel(const char *, ChanInfo &);

    void    writeSCPLS(Stream &);
    void    writePLS(Stream &);
    void    writeASX(Stream &);
    void    writeRAM(Stream &);

    void    readSCPLS(Stream &);
    void    readPLS(Stream &);
    void    readASX(Stream &);

    void    read(Stream &s)
    {
        try
        {
            switch (type)
            {
                case T_SCPLS: readSCPLS(s); break;
                case T_PLS: readPLS(s); break;
                case T_ASX: readASX(s); break;
                default: throw std::runtime_error("unsupported playlist type for reading");
            }
        }catch (StreamException &) {}    // keep pls regardless of errors (eof isn`t handled properly in sockets)
    }

    void    write(Stream &s)
    {
        switch (type)
        {
            case T_SCPLS: writeSCPLS(s); break;
            case T_PLS: writePLS(s); break;
            case T_ASX: writeASX(s); break;
            case T_RAM: writeRAM(s); break;
            default: throw std::runtime_error("unsupported playlist type for writing");
        }
    }

    static PlayList::TYPE getPlayListType(ChanInfo::TYPE chanType);

    TYPE        type;
    int         numURLs, maxURLs;
    ::String    *urls, *titles;
    std::string wmvProtocol;
};

#endif
