// ------------------------------------------------
// File : _string.h
// Author: giles
// Desc:
//
// (c) peercast.org
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

#ifndef __STRING_H
#define __STRING_H

#include <string>
#include <string.h>
#include "sys.h"

// ------------------------------------
class String
{
public:
    enum {
        MAX_LEN = 256
    };

    enum TYPE
    {
        T_UNKNOWN,
        T_ASCII,
        T_ESC,
        T_ESCSAFE,
        T_META,
        T_METASAFE,
        T_BASE64,
        T_UNICODE,
        T_UNICODESAFE
    };

    String()
    {
        clear();
    }

    String(const char *p, TYPE t=T_ASCII)
    {
        set(p, t);
    }

    // set from straight null terminated string
    String& set(const char *p, TYPE t=T_ASCII);

    // set from quoted or unquoted null terminated string
    String& setFromString(const char *str, TYPE t=T_ASCII);

    // set from stopwatch
    String& setFromStopwatch(unsigned int t);

    // set from time
    String& setFromTime(unsigned int t);

    // set from null terminated string, remove first/last chars
    String& setUnquote(const char *p, TYPE t=T_ASCII);

    void clear();

    void ASCII2ESC(const char *, bool);
    void ASCII2HTML(const char *);
    void ASCII2META(const char *, bool);
    void ESC2ASCII(const char *);
    void HTML2ASCII(const char *);
    void BASE642ASCII(const char *);
    void UNKNOWN2UNICODE(const char *, bool);

    static  int base64WordToChars(char *, const char *);

    bool startsWith(const char *s) const { return strncmp(data, s, strlen(s))==0; }
    bool isEmpty() const { return data[0]==0; }
    bool isSame(::String &s) const { return strcmp(data, s.data)==0; }
    bool isSame(const char *s) const { return strcmp(data, s)==0; }
    bool contains(const ::String &s) const { return stristr(data, s.data)!=NULL; }
    bool contains(const char *s) const { return stristr(data, s)!=NULL; }
    void append(const char *s);
    void append(char c);
    void prepend(const char *s);

    void sprintf(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)));

    static ::String format(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

    operator std::string () const { return data; }

    bool operator == (const char *s) const { return isSame(s); }
    bool operator != (const char *s) const { return !isSame(s); }

    String& operator = (const String& other);
    String& operator = (const char* cstr);
    String& operator = (const std::string& rhs);

    operator const char *() const { return data; }

    String& convertTo(TYPE t);

    char    *cstr() { return data; }
    const char* c_str() const { return data; }
    std::string str() const { return data; }

    static bool isWhitespace(char c) { return c==' ' || c=='\t'; }

    size_t size() const { return strlen(data); }

    TYPE    type;
    char    data[MAX_LEN];
};

#endif
