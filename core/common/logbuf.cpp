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
void LogBuffer::dumpHTML(Stream &out)
{
    CriticalSection cs(lock);

    unsigned int nl = currLine;
    unsigned int sp = 0;
    if (nl > maxLines)
    {
        nl = maxLines-1;
        sp = (currLine+1)%maxLines;
    }

    String tim;
    if (nl)
    {
        for (unsigned int i=0; i<nl; i++)
        {
            unsigned int bp = sp*lineLen;

            if (types[sp])
            {
                tim.setFromTime(times[sp]);

                out.writeString(tim.cstr());
                out.writeString(" <b>[");
                out.writeString(getTypeStr(types[sp]));
                out.writeString("]</b> ");
            }

            out.writeString(cgi::escape_html(&buf[bp]).c_str());
            out.writeString("<br>");

            sp++;
            sp %= maxLines;
        }
    }
}

// ---------------------------
void    LogBuffer::clear()
{
    CriticalSection cs(lock);
    currLine = 0;
}
