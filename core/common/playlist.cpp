// ------------------------------------------------
// File : playlist.cpp
// Date: 4-apr-2002
// Author: giles
//
// (c) 2002 peercast.org
//
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

#include "playlist.h"
#include "chaninfo.h"
#include "chanmgr.h"

// -----------------------------------
void PlayList::readASX(Stream &in)
{
    LOG_DEBUG("Reading ASX");
    XML xml;

    try
    {
        xml.read(in);
    }catch (StreamException &) {} // TODO: eof is NOT handled properly in sockets - always get error at end

    if (xml.root)
    {
        XML::Node *n = xml.root->child;
        while (n)
        {
            if (Sys::stricmp("entry", n->getName())==0)
            {
                XML::Node *rf = n->findNode("ref");
                if (rf)
                {
                    char *hr = rf->findAttr("href");
                    if (hr)
                    {
                        addURL(hr, "");
                        //LOG("asx url %s", hr);
                    }
                }
            }
            n=n->sibling;
        }
    }
}

// -----------------------------------
void PlayList::readSCPLS(Stream &in)
{
    char tmp[256];
    while (in.readLine(tmp, sizeof(tmp)))
    {
        if (Sys::strnicmp(tmp, "file", 4)==0)
        {
            char *p = strstr(tmp, "=");
            if (p)
                addURL(p+1, "");
        }
    }
}

// -----------------------------------
void PlayList::readPLS(Stream &in)
{
    char tmp[256];
    while (in.readLine(tmp, sizeof(tmp)))
    {
        if (tmp[0] != '#')
            addURL(tmp, "");
    }
}

// -----------------------------------
void PlayList::writeSCPLS(Stream &out)
{
    out.writeLine("[playlist]");
    out.writeLine("");
    out.writeLineF("NumberOfEntries=%d", numURLs);

    for (int i=0; i<numURLs; i++)
    {
        out.writeLineF("File%d=%s", i+1, urls[i].cstr());
        out.writeLineF("Title%d=%s", i+1, titles[i].cstr());
        out.writeLineF("Length%d=-1", i+1);
    }
    out.writeLine("Version=2");
}

// -----------------------------------
void PlayList::writePLS(Stream &out)
{
    for (int i=0; i<numURLs; i++)
        out.writeLineF("%s", urls[i].cstr());
}

// -----------------------------------
void PlayList::writeRAM(Stream &out)
{
    for (int i=0; i<numURLs; i++)
        out.writeLineF("%s", urls[i].cstr());
}

// -----------------------------------
void PlayList::writeASX(Stream &out)
{
    out.writeLine("<ASX Version=\"3.0\">");
    for (int i=0; i<numURLs; i++)
    {
        auto url = str::replace_prefix(urls[i].cstr(), "http", wmvProtocol);
        out.writeLine("<ENTRY>");
        out.writeLineF("<REF href=\"%s\" />", url.c_str());
        out.writeLine("</ENTRY>");
    }
    out.writeLine("</ASX>");
}

// -----------------------------------
void PlayList::addChannel(const char *path, ChanInfo &info)
{
    std::string url;
    std::string nid = info.id.isSet() ? info.id.str() : info.name;

    url = str::format("%s/stream/%s%s?auth=%s",
                      path,
                      nid.c_str(),
                      info.getTypeExt(),
                      chanMgr->authToken(info.id).c_str());
    addURL(url.c_str(), info.name);
}

// -----------------------------------
PlayList::TYPE PlayList::getPlayListType(ChanInfo::TYPE chanType)
{
    if ((chanType == ChanInfo::T_WMA) || (chanType == ChanInfo::T_WMV))
        return PlayList::T_ASX;
    else if (chanType == ChanInfo::T_OGM)
        return PlayList::T_RAM;
    else
        return PlayList::T_PLS;
}
