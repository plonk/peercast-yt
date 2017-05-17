// ------------------------------------------------
// File : http.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      HTTP protocol handling
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

#include <stdlib.h>
#include <cctype>

#include "http.h"
#include "sys.h"
#include "common.h"
#include "str.h"

//-----------------------------------------
bool HTTP::checkResponse(int r)
{
    if (readResponse()!=r)
    {
        LOG_ERROR("Unexpected HTTP: %s", cmdLine);
        throw StreamException("Unexpected HTTP response");
        return false;
    }

    return true;
}

//-----------------------------------------
void HTTP::readRequest()
{
    readLine(cmdLine, sizeof(cmdLine));
    parseRequestLine();
}

//-----------------------------------------
void HTTP::initRequest(const char *r)
{
    strcpy(cmdLine, r);
    parseRequestLine();
}

//-----------------------------------------
void HTTP::parseRequestLine()
{
    auto vec = str::split(cmdLine, " ");
    if (vec.size() > 0) method          = vec[0];
    if (vec.size() > 1) requestUrl      = vec[1];
    if (vec.size() > 2) protocolVersion = vec[2];
}

//-----------------------------------------
int HTTP::readResponse()
{
    readLine(cmdLine, sizeof(cmdLine));

    char *cp = cmdLine;

    while (*cp) if (*++cp == ' ') break;
    while (*cp) if (*++cp != ' ') break;

    char *scp = cp;

    while (*cp) if (*++cp == ' ') break;
    *cp = 0;

    return atoi(scp);
}

//-----------------------------------------
bool    HTTP::nextHeader()
{
    using namespace std;

    if (readLine(cmdLine, sizeof(cmdLine)))
    {
        char *ap = strstr(cmdLine, ":");
        if (ap)
            while (*++ap)
                if (*ap!=' ')
                    break;
        arg = ap;

        if (ap)
        {
            string name(cmdLine, strchr(cmdLine, ':'));
            string value;
            char *end;
            if (!(end = strchr(ap, '\r')))
                if (!(end = strchr(ap, '\n')))
                    end = ap + strlen(ap);
            value = string(ap, end);
            for (int i = 0; i < name.size(); ++i)
                name[i] = toupper(name[i]);
            headers.set(name, value);
        }
        return true;
    }else
    {
        arg = NULL;
        return false;
    }
}

//-----------------------------------------
bool    HTTP::isHeader(const char *hs)
{
    return stristr(cmdLine, hs) != NULL;
}

//-----------------------------------------
bool    HTTP::isRequest(const char *rq)
{
    return strncmp(cmdLine, rq, strlen(rq)) == 0;
}

//-----------------------------------------
char *HTTP::getArgStr()
{
    return arg;
}

//-----------------------------------------
int HTTP::getArgInt()
{
    if (arg)
        return atoi(arg);
    else
        return 0;
}

//-----------------------------------------
void HTTP::getAuthUserPass(char *user, char *pass, size_t ulen, size_t plen)
{
    if (arg)
    {
        char *s = stristr(arg, "Basic");
        if (s)
        {
            while (*s)
                if (*s++ == ' ')
                    break;
            String str;
            str.set(s, String::T_BASE64);
            str.convertTo(String::T_ASCII);
            s = strstr(str.cstr(), ":");
            if (s)
            {
                *s = 0;
                if (user) {
                    strncpy(user, str.cstr(), ulen);
                    user[ulen - 1] = 0;
                }
                if (pass) {
                    strncpy(pass, s+1, plen);
                    pass[plen - 1] = 0;
                }
            }
        }
    }
}

#include <functional>
class Defer
{
public:
    Defer(std::function<void()> aCallback)
        : callback(aCallback)
    {}

    ~Defer()
    {
        callback();
    }

    std::function<void()> callback;
};

static const char* statusMessage(int statusCode)
{
    switch (statusCode)
    {
    case 101: return "Switch protocols";
    case 200: return "OK";
    case 302: return "Found";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 403: return "Forbidden";
    case 404: return "Not Found";
    case 500: return "Internal Server Error";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    default: return "Unknown";
    }
}

// -----------------------------------
#include "cgi.h"
#include "version2.h" // PCX_AGENT
void HTTP::send(const HTTPResponse& response)
{
    bool crlf = writeCRLF;
    Defer cb([=]() { writeCRLF = crlf; });

    writeCRLF = true;

    writeLineF("HTTP/1.0 %d %s", response.statusCode, statusMessage(response.statusCode));

    std::map<std::string,std::string> headers = {
        {"Server", PCX_AGENT},
        {"Connection", "close"},
        {"Date", cgi::rfc1123Time(sys->getTime())}
    };

    for (const auto& pair : response.headers)
        headers[pair.first] = pair.second;

    for (const auto& pair : headers)
        writeLineF("%s: %s", pair.first.c_str(), pair.second.c_str());

    writeLine("");

    if (response.body.size())
        write(response.body.data(), response.body.size());
}

// -----------------------------------
void    CookieList::init()
{
    for (int i=0; i<MAX_COOKIES; i++)
        list[i].clear();

    neverExpire = false;
}

// -----------------------------------
bool    CookieList::contains(Cookie &c)
{
    if ((c.id[0]) && (c.ip))
        for (int i=0; i<MAX_COOKIES; i++)
            if (list[i].compare(c))
                return true;

    return false;
}

// -----------------------------------
void    Cookie::logDebug(const char *str, int ind)
{
    char ipstr[64];
    Host h;
    h.ip = ip;
    h.IPtoStr(ipstr);

    LOG_DEBUG("%s %d: %s - %s", str, ind, ipstr, id);
}

// -----------------------------------
bool    CookieList::add(Cookie &c)
{
    if (contains(c))
        return false;

    unsigned int oldestTime=(unsigned int)-1;
    int oldestIndex=0;

    for (int i=0; i<MAX_COOKIES; i++)
        if (list[i].time <= oldestTime)
        {
            oldestIndex = i;
            oldestTime = list[i].time;
        }

    c.logDebug("Added cookie", oldestIndex);
    c.time = sys->getTime();
    list[oldestIndex]=c;
    return true;
}

// -----------------------------------
void    CookieList::remove(Cookie &c)
{
    for (int i=0; i<MAX_COOKIES; i++)
        if (list[i].compare(c))
            list[i].clear();
}
