// ------------------------------------------------
// File : logbuf.h
// Date: 16-aug-2017
// Desc:
//      Extracted from sys.h. Ring buffer for logging.
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

#ifndef _LOGBUF_H
#define _LOGBUF_H

#include "threading.h"
#include <vector>

class Stream;

// ------------------------------------
class LogBuffer
{
public:
    enum TYPE
    {
        T_NONE  = 0, // Denotes that the line is a continuation of the
                     // previous one.
        T_TRACE = 1,
        T_DEBUG = 2,
        T_INFO  = 3,
        T_WARN  = 4,
        T_ERROR = 5,
        T_FATAL = 6,
        T_OFF   = 7,
    };

    LogBuffer(int aMaxLines, int aLineLen)
        : lineLen(aLineLen)
        , maxLines(aMaxLines)
    {
        currLine = 0;
        buf = new char[lineLen*maxLines];
        times = new unsigned int [maxLines];
        types = new TYPE [maxLines];
    }

    ~LogBuffer()
    {
        delete[] buf;
        delete[] times;
        delete[] types;
    }

    void    clear();

    void                write(const char *, TYPE);
    static const char   *getTypeStr(TYPE t) { return logTypes[t]; }
    void                eachLine(std::function<void(unsigned int, TYPE, const char*)> block);
    std::vector<std::string> toLines(std::function<std::string(unsigned int, TYPE, const char*)> renderer);
    void                dumpHTML(Stream &);

    static std::string  lineRendererHTML(unsigned int time, TYPE type, const char* line);
    static void         escapeHTML(char* dest, char* src);

    char                *buf;
    unsigned int        *times;
    unsigned int        currLine;
    const unsigned int  maxLines;
    const unsigned int  lineLen;
    TYPE                *types;
    WLock               lock;
    static const char   *logTypes[];
};

#endif
