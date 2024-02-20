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
#include "template.h"

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
    void    startNode(const char *, const char * = nullptr);
    void    addLink(const char *, const char *, bool = false);
    void    startTag(const char *, const char * = nullptr, ...) __attribute__ ((format (printf, 3, 4)));
    void    startTagEnd(const char *, const char * = nullptr, ...) __attribute__ ((format (printf, 3, 4)));
    void    startSingleTagEnd(const char *, ...) __attribute__ ((format (printf, 2, 3)));
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
    void    writeTemplate(const char *fileName, const char *args, const std::vector<Template::Scope*>& scopes);


    String  title, refreshURL;
    char    currTag[MAX_TAGLEVEL][MAX_TAGLEN];
    int     tagLevel;
    int     refresh;
    Stream  *out;
};

#endif
