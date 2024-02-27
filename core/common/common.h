// ------------------------------------------------
// File : common.h
// Date: 4-apr-2002
// Author: giles
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

#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h> // snprintf
#include <string>
#include <assert.h>

#include <vector>

#ifndef __GNUC__
#define __attribute__(x)
#endif

// ----------------------------------
class GeneralException : public std::exception
{
public:
    GeneralException(const std::string& m, int e = 0);

    const char* what() const throw() override;
    
    std::string msgbuf;
    const char* msg;
    std::vector<std::string> backtrace;
    int  err;
};

// -------------------------------------
class NotImplementedException : public GeneralException
{
public:
    NotImplementedException(const std::string& m) : GeneralException(m) {}
    NotImplementedException(const std::string& m, int e) : GeneralException(m, e) {}
};

// -------------------------------------
class LogicError : public GeneralException
{
public:
    LogicError(const std::string& m) : GeneralException(m) {}
    LogicError(const std::string& m, int e) : GeneralException(m, e) {}
};

// -------------------------------------
class ArgumentException : public GeneralException
{
public:
    ArgumentException(const std::string& m) : GeneralException(m) {}
    ArgumentException(const std::string& m, int e) : GeneralException(m, e) {}
};

// -------------------------------------
class FormatException : public GeneralException
{
public:
    FormatException(const std::string& m) : GeneralException(m) {}
    FormatException(const std::string& m, int e) : GeneralException(m, e) {}
};

// -------------------------------------
class StreamException : public GeneralException
{
public:
    StreamException(const std::string& m) : GeneralException(m) {}
    StreamException(const std::string& m, int e) : GeneralException(m, e) {}
};

// ----------------------------------
class SockException : public StreamException
{
public:
    SockException(const std::string& m = "Socket") : StreamException(m) {}
    SockException(const std::string& m, int e) : StreamException(m, e) {}
};

// ----------------------------------
class EOFException : public StreamException
{
public:
    EOFException(const std::string& m = "EOF") : StreamException(m) {}
    EOFException(const std::string& m, int e) : StreamException(m, e) {}
};

// ----------------------------------
// unused
class CryptException : public StreamException
{
public:
    CryptException(const std::string& m = "Crypt") : StreamException(m) {}
    CryptException(const std::string& m, int e) : StreamException(m, e) {}
};

// ----------------------------------
class TimeoutException : public StreamException
{
public:
    TimeoutException(const std::string& m = "Timeout") : StreamException(m) {}
};

// ----------------------------------
#define SWAP2(v) ( ((v&0xff)<<8) | ((v&0xff00)>>8) )
#define SWAP3(v) (((v&0xff)<<16) | ((v&0xff00)) | ((v&0xff0000)>>16) )
#define SWAP4(v) (((v&0xff)<<24) | ((v&0xff00)<<8) | ((v&0xff0000)>>8) | ((v&0xff000000)>>24))
#define TOUPPER(c) ((((c) >= 'a') && ((c) <= 'z')) ? (c)+'A'-'a' : (c))
#define TONIBBLE(c) ((((c) >= 'A')&&((c) <= 'F')) ? (((c)-'A')+10) : ((c)-'0'))
#define BYTES_TO_KBPS(n) (float)(((((float)(n))*8.0f)/1024.0f))

// -----------------------------------
const char  *getCGIarg(const char *str, const char *arg);
bool        hasCGIarg(const char *str, const char *arg);

// --------------------------------------------------
#include "logbuf.h"
namespace peercast {
    void addlog(LogBuffer::TYPE type, const char* file, int line, const char* func, const char* fmt, ...)  __attribute__ ((format (printf, 5, 6)));
}
#define LOG(...)        peercast::addlog(LogBuffer::T_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_TRACE(...)  peercast::addlog(LogBuffer::T_TRACE, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...)  peercast::addlog(LogBuffer::T_DEBUG, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)   peercast::addlog(LogBuffer::T_INFO,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)   peercast::addlog(LogBuffer::T_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...)  peercast::addlog(LogBuffer::T_ERROR, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)   peercast::addlog(LogBuffer::T_WARN,  __FILE__, __LINE__, __func__, __VA_ARGS__)
#define LOG_FATAL(...)  peercast::addlog(LogBuffer::T_FATAL, __FILE__, __LINE__, __func__, __VA_ARGS__)

// ----------------------------------
#define ASSERT(expr) do{if(!(expr))LOG_WARN("Assertion failed: " #expr " at " __FILE__ ":%d", __LINE__);}while(0)

#endif

