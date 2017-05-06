// ------------------------------------------------
// File : html.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      HTML protocol handling
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

#include <stdarg.h>
#include <stdlib.h>
#include "cgi.h"
#include "channel.h"
#include "gnutella.h"
#include "html.h"
#include "http.h"
#include "stream.h"
#include "version2.h"
#include "template.h"
#include "dmstream.h"

// --------------------------------------
HTML::HTML(const char *t, Stream &o)
{
    out = &o;
    out->writeCRLF = false;

    title.set(t);
    tagLevel = 0;
    refresh = 0;
}

// --------------------------------------
void HTML::writeOK(const char *content, const std::map<std::string,std::string>& additionalHeaders)
{
    bool crlf = out->writeCRLF;

    out->writeCRLF = true;
    out->writeLine(HTTP_SC_OK);
    out->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    //out->writeLine("%s %s", HTTP_HS_CACHE, "no-cache");
    out->writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
    out->writeLineF("%s %s", HTTP_HS_CONTENT, content);
    out->writeLineF("Date: %s", cgi::rfc1123Time(sys->getTime()).c_str());

    for (auto& pair : additionalHeaders)
        out->writeLineF("%s: %s", pair.first.c_str(), pair.second.c_str());

    out->writeLine("");
    out->writeCRLF = crlf;
}

// --------------------------------------
void HTML::writeTemplate(const char *fileName, const char *args)
{
    FileStream file;
    try
    {
        DynamicMemoryStream mem;
        file.openReadOnly(fileName);
        file.writeTo(mem, file.length());
        mem.rewind();

        WriteBufferedStream bufferedOut(out);
        Template temp(args);
        if (args)
        {
            cgi::Query query(args);
            temp.selectedFragment = query.get("fragment");
        }
        temp.readTemplate(mem, &bufferedOut, 0);
    }catch (StreamException &e)
    {
        out->writeString(e.msg);
        out->writeString(" : ");
        out->writeString(fileName);
    }
    file.close();
}

// --------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static time_t mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        return -1;
    else
        return st.st_mtime;
}

// --------------------------------------
void HTML::writeRawFile(const char *fileName, const char *mimeType)
{
    FileStream file;
    std::map<std::string,std::string> additionalHeaders;

    try
    {
        file.openReadOnly(fileName);
        time_t t = mtime(fileName);

        if (t != -1)
            additionalHeaders["Last-Modified"] = cgi::rfc1123Time(t);

        writeOK(mimeType, additionalHeaders);
        file.writeTo(*out, file.length());
    }catch (StreamException &)
    {
    }

    file.close();
}

// --------------------------------------
void HTML::locateTo(const char *url)
{
    bool prev = out->writeCRLF;
    out->writeCRLF = true;
    out->writeLine(HTTP_SC_FOUND);
    out->writeLineF("Location: %s", url);
    out->writeLine("");
    out->writeCRLF = prev;
}

// --------------------------------------
void HTML::startHTML()
{
    startNode("html");
}

// --------------------------------------
void HTML::startBody()
{
    startNode("body");
}

// --------------------------------------
void HTML::addHead()
{
    char buf[512];
    startNode("head");
        startTagEnd("title", "%s", title.cstr());
        startTagEnd("meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"");
        if (!refreshURL.isEmpty())
        {
            sprintf(buf, "meta http-equiv=\"refresh\" content=\"%d;URL=%s\"", refresh, refreshURL.cstr());
            startTagEnd(buf);
        }else if (refresh)
        {
            sprintf(buf, "meta http-equiv=\"refresh\" content=\"%d\"", refresh);
            startTagEnd(buf);
        }
    end();
}

// --------------------------------------
void HTML::startNode(const char *tag, const char *data)
{
    const char *p = tag;
    char *o = &currTag[tagLevel][0];

    for (int i=0; i<MAX_TAGLEN-1; i++)
    {
        char c = *p++;
        if ((c==0) || (c==' '))
            break;
        else
            *o++ = c;
    }
    *o = 0;

    out->writeString("<");
    out->writeString(tag);
    out->writeString(">");
    if (data)
        out->writeString(data);

    tagLevel++;
    if (tagLevel >= MAX_TAGLEVEL)
        throw StreamException("HTML too deep!");
}

// --------------------------------------
void HTML::end()
{
    tagLevel--;
    if (tagLevel < 0)
        throw StreamException("HTML premature end!");

    out->writeString("</");
    out->writeString(&currTag[tagLevel][0]);
    out->writeString(">");
}

// --------------------------------------
void HTML::addLink(const char *url, const char *text, bool toblank)
{
    char buf[1024];

    sprintf(buf, "a href=\"%s\" %s", url, toblank?"target=\"_blank\"":"");
    startNode(buf, text);
    end();
}

// --------------------------------------
void HTML::startTag(const char *tag, const char *fmt, ...)
{
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);

        char tmp[512];
        vsprintf(tmp, fmt, ap);
        startNode(tag, tmp);

        va_end(ap);
    }else{
        startNode(tag, NULL);
    }
}

// --------------------------------------
void HTML::startTagEnd(const char *tag, const char *fmt, ...)
{
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);

        char tmp[512];
        vsprintf(tmp, fmt, ap);
        startNode(tag, tmp);

        va_end(ap);
    }else{
        startNode(tag, NULL);
    }
    end();
}

// --------------------------------------
void HTML::startSingleTagEnd(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char tmp[512];
    vsprintf(tmp, fmt, ap);
    startNode(tmp);

    va_end(ap);
    end();
}

// --------------------------------------
void HTML::startTableRow(int i)
{
    if (i & 1)
        startTag("tr bgcolor=\"#dddddd\" align=\"left\"");
    else
        startTag("tr bgcolor=\"#eeeeee\" align=\"left\"");
}
