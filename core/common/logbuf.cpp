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
size_t LogBuffer::copy_utf8(char* dest, const char* src, size_t buflen)
{
    auto origlen = buflen;

    while (buflen && *src != '\0') {
        int charlen;

        if ((*src & 0x80) == 0) // 0xxx xxxx
            charlen = 1;
        else if ((*src & 0xE0) == 0xC0) // 110x xxxx
            charlen = 2;
        else if ((*src & 0xF0) == 0xE0) // 1110 xxxx
            charlen = 3;
        else if ((*src & 0xF8) == 0xF0) // 1111 0xxx
            charlen = 4;
        else
            charlen = 1; // malformed

        if (buflen < charlen)
            break;

        // copy char
        for (int i = 0; i < charlen; i++)
            *dest++ = *src++;
        buflen -= charlen;
    }

    for (int i = 0; i < buflen; i++)
        *dest++ = '\0';
    return origlen - buflen;
}

// -----------------------------------
void LogBuffer::write(const char *str, TYPE t)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    for (auto& pair : m_listeners) {
        pair.second(str, t);
    }

    size_t len = strlen(str);
    int cnt=0;
    while (len)
    {
        size_t rlen = len;
        if (rlen > (lineLen-1))
            rlen = lineLen-1;

        int i = currLine % maxLines;
        int bp = i*lineLen;

        //strncpy(&buf[bp], str, rlen);
        rlen = copy_utf8(&buf[bp], str, rlen);
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
    std::lock_guard<std::recursive_mutex> cs(lock);

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
    std::lock_guard<std::recursive_mutex> cs(lock);
    currLine = 0;
}

#include <assert.h>
unsigned int LogBuffer::addListener(std::function<void(const char*,TYPE)> f)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    assert( m_listeners.count(seq) == 0 );
    m_listeners[seq] = f;
    return seq++;
}

bool LogBuffer::removeListener(unsigned int i)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    return !!m_listeners.erase(i);
}
