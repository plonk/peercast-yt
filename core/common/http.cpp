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

#include <cctype>

#include "http.h"
#include "sys.h"
#include "common.h"
#include "str.h"

#include "cgi.h"
#include "version2.h" // PCX_AGENT
#include "defer.h"
#include "dechunker.h"

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
    Sys::strcpy_truncate(cmdLine, sizeof(cmdLine), r);
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

    if (m_headersRead)
        return false;

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
            for (size_t i = 0; i < name.size(); ++i)
                name[i] = toupper(name[i]);
            headers.set(name, value);
        }
        return true;
    }else
    {
        arg = NULL;
        m_headersRead = true;
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
    parseAuthorizationHeader(arg, user, pass, ulen, plen);
}

//-----------------------------------------
void HTTP::parseAuthorizationHeader(const char* arg, char* user, char* pass, size_t ulen, size_t plen)
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

// -----------------------------------
void HTTP::parseAuthorizationHeader(const std::string& arg, std::string& sUser, std::string& sPass)
{
    char user[64] = "", pass[64] = "";
    parseAuthorizationHeader(arg.c_str(), user, pass, sizeof(user), sizeof(pass));

    sUser = user;
    sPass = pass;
}

// -----------------------------------
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
    case 501: return "Not Implemented";
    case 502: return "Bad Gateway";
    case 503: return "Service Unavailable";
    default: return "Unknown";
    }
}

// -----------------------------------
HTTPRequest HTTP::getRequest()
{
    if (method.size() > 0 &&
        requestUrl.size() > 0 &&
        protocolVersion.size() > 0 &&
        strlen(cmdLine) == 0)
    {
        if (method == "POST") {
            if (headers.get("Content-Length") == "") {
                throw GeneralException("POST without Content-Length");
            } else {
                HTTPRequest req(method, requestUrl, protocolVersion, headers);
                int size = atoi(headers.get("Content-Length").c_str());

                if (m_body == nullptr) {
                    m_body = std::make_shared<std::string>();
                    *m_body = stream->read(size);
                }
                req.body = *m_body;

                return req;
            }
        } else {
            return HTTPRequest(method, requestUrl, protocolVersion, headers);
        }
    }else
    {
        throw GeneralException("Request not ready");
    }
}

// -----------------------------------
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
        writeLineF("%s: %s", str::capitalize(pair.first).c_str(), pair.second.c_str());

    writeLine("");

    if (response.stream != nullptr)
    {
        try
        {
            while (true)
            {
                char buf[4096];
                int r = response.stream->read(buf, 4096);
                write(buf, r);
            }
        } catch (StreamException&) {}
    }
    else
    {
        write(response.body.data(), response.body.size());
    }
}

// -----------------------------------
HTTPResponse HTTP::send(const HTTPRequest& request)
{
    // send request
    stream->writeLineF("%s %s %s",
                       request.method.c_str(),
                       request.path.c_str(),
                       request.protocolVersion.c_str());
    for (auto it = request.headers.begin(); it != request.headers.end(); ++it)
    {
        stream->writeLineF("%s: %s",
                           str::capitalize(it->first).c_str(),
                           it->second.c_str());
    }
    stream->writeLine("");

    if (request.method == "POST" || request.method == "PUT")
    {
        if (request.headers.get("Content-Length") != "" &&
            std::atoi(request.headers.get("Content-Length").c_str()) != request.body.size())
            throw StreamException("body size mismatch");
        stream->writeString(request.body);
    }

    // receive response
    int status = readResponse();
    readHeaders();
    HTTPResponse response(status, headers);

    if (headers.get("Transfer-Encoding") == "chunked") {
        Dechunker stream1(*stream);

        try {
            while (true)
                response.body += stream1.readChar();
        }catch(StreamException& e)
        {
        }
    }else
    {
        std::string contentLengthStr = headers.get("Content-Length");
        if (contentLengthStr.empty())
            throw StreamException("Content-Length missing");
        int length = atoi(contentLengthStr.c_str());
        if (length < 0)
            throw StreamException("invalid Content-Length value");

        response.body = stream->read(length);
    }
    return response;
}
