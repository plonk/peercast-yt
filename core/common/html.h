// ------------------------------------------------
// File : html.h
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

#ifndef _HTML_H
#define _HTML_H

// ---------------------------------------
#include <map>
#include "xml.h"
#include "sys.h"

class FileStream;

// ---------------------------------------
class HTML
{
public:
    enum
    {
        MAX_TAGLEVEL = 64,
        MAX_TAGLEN = 64
    };

    HTML(const char *, Stream &);

    // HTML ヘルパー
    void    startNode(const char *, const char * = NULL);
    void    addLink(const char *, const char *, bool = false);
    void    startTag(const char *, const char * = NULL, ...);
    void    startTagEnd(const char *, const char * = NULL, ...);
    void    startSingleTagEnd(const char *, ...);
    void    startTableRow(int);
    void    end();
    void    setRefresh(int sec) { refresh = sec; }
    void    setRefreshURL(const char *u) { refreshURL.set(u); }
    void    addHead();
    void    startHTML();
    void    startBody();

    // HTTP レスポンス
    void    writeOK(const char *content,
                    const std::map<std::string,std::string>& = {});
    void    locateTo(const char *);
    void    writeRawFile(const char *, const char *);

    void    writeTemplate(const char *, const char *);

    String  title, refreshURL;
    char    currTag[MAX_TAGLEVEL][MAX_TAGLEN];
    int     tagLevel;
    int     refresh;
    Stream  *out;
};

#endif
