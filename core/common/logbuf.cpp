// ------------------------------------------------
// File : logbuf.cpp
// Date: 16-aug-2017
// Desc:
//      Extracted from sys.cpp. Ring buffer for logging.
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

#include "logbuf.h"
#include "stream.h"
#include "critsec.h"
#include "cgi.h"

// -----------------------------------
const char *LogBuffer::logTypes[]=
{
    "",
    "TRAC",
    "DBUG",
    "INFO",
    "WARN",
    "EROR",
    "FATL",
    " OFF",
};

// -----------------------------------
void LogBuffer::write(const char *str, TYPE t)
{
    CriticalSection cs(lock);

    unsigned int len = strlen(str);
    int cnt=0;
    while (len)
    {
        unsigned int rlen = len;
        if (rlen > (lineLen-1))
            rlen = lineLen-1;

        int i = currLine % maxLines;
        int bp = i*lineLen;
        strncpy(&buf[bp], str, rlen);
        buf[bp+rlen] = 0;
        if (cnt==0)
        {
            times[i] = sys->getTime();
            types[i] = t;
        }else
        {
            times[i] = 0;
            types[i] = T_NONE;
        }
        currLine++;

        str += rlen;
        len -= rlen;
        cnt++;
    }
}

// ---------------------------
std::string LogBuffer::lineRendererHTML(unsigned int time, TYPE type, const char* line)
{
    std::string buf;

    if (type != LogBuffer::T_NONE)
    {
        String tim;
        tim.setFromTime(time);

        buf += tim.cstr();
        buf += " <b>[";
        buf += getTypeStr(type);
        buf += "]</b> ";
    }

    buf += cgi::escape_html(line).c_str();
    buf += "<br>";

    return buf;
}

// ---------------------------
void LogBuffer::eachLine(std::function<void(unsigned int, TYPE, const char*)> block)
{
    CriticalSection cs(lock);

    unsigned int nlines;
    unsigned int sp;
    if (currLine < maxLines)
    {
        nlines = currLine;
        sp = 0;
    }else
    {
        nlines = maxLines;
        sp = currLine % maxLines;
    }

    for (unsigned int i = 0; i < nlines; i++)
    {
        block(times[sp], types[sp], &buf[sp*lineLen]);

        sp = (sp+1) % maxLines;
    }
}

// ---------------------------
void LogBuffer::dumpHTML(Stream &out)
{
    eachLine([&out](unsigned int time, TYPE type, const char* line)
             {
                 out.writeString(lineRendererHTML(time, type, line));
             });
}

// ---------------------------
std::vector<std::string> LogBuffer::toLines(std::function<std::string(unsigned int, TYPE, const char*)> renderer)
{
    std::vector<std::string> res;

    eachLine([&res, renderer](unsigned int time, TYPE type, const char* line)
             {
                 res.push_back(renderer(time, type, line));
             });

    return res;
}

// ---------------------------
void    LogBuffer::clear()
{
    CriticalSection cs(lock);
    currLine = 0;
}
