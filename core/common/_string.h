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
        T_HTML,
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
    void set(const char *p, TYPE t=T_ASCII)
    {
        strncpy(data, p, MAX_LEN-1);
        data[MAX_LEN-1] = 0;
        type = t;
    }

    // set from quoted or unquoted null terminated string
    void setFromString(const char *str, TYPE t=T_ASCII);

    // set from stopwatch
    void setFromStopwatch(unsigned int t);

    // set from time
    void setFromTime(unsigned int t);


    // from single word (end at whitespace)
    void setFromWord(const char *str)
    {
        int i;
        for (i=0; i<MAX_LEN-1; i++)
        {
            data[i] = *str++;
            if ((data[i]==0) || (data[i]==' '))
                break;
        }
        data[i]=0;
    }


    // set from null terminated string, remove first/last chars
    void setUnquote(const char *p, TYPE t=T_ASCII)
    {
        int slen = strlen(p);
        if (slen > 2)
        {
            if (slen >= MAX_LEN) slen = MAX_LEN;
            strncpy(data, p+1, slen-2);
            data[slen-2]=0;
        }else
            clear();
        type = t;
    }

    void clear()
    {
        data[0]=0;
        type = T_UNKNOWN;
    }
    void ASCII2ESC(const char *, bool);
    void ASCII2HTML(const char *);
    void ASCII2META(const char *, bool);
    void ESC2ASCII(const char *);
    void HTML2ASCII(const char *);
    void HTML2UNICODE(const char *);
    void BASE642ASCII(const char *);
    void UNKNOWN2UNICODE(const char *, bool);

    static  int base64WordToChars(char *, const char *);

    static bool isSame(const char *s1, const char *s2) { return strcmp(s1, s2)==0; }

    bool startsWith(const char *s) const { return strncmp(data, s, strlen(s))==0; }
    bool isValidURL();
    bool isEmpty() const { return data[0]==0; }
    bool isSame(::String &s) const { return strcmp(data, s.data)==0; }
    bool isSame(const char *s) const { return strcmp(data, s)==0; }
    bool contains(::String &s) { return stristr(data, s.data)!=NULL; }
    bool contains(const char *s) { return stristr(data, s)!=NULL; }
    void append(const char *s)
    {
        if ((strlen(s)+strlen(data) < (MAX_LEN-1)))
            strcat(data, s);
    }
    void append(char c)
    {
        char tmp[2];
        tmp[0]=c;
        tmp[1]=0;
        append(tmp);
    }

    void prepend(const char *s)
    {
        ::String tmp;
        tmp.set(s);
        tmp.append(data);
        tmp.type = type;
        *this = tmp;
    }

    void sprintf(const char* fmt, ...) __attribute__ ((format (printf, 2, 3)))
    {
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(this->data, ::String::MAX_LEN - 1, fmt, ap);
        va_end(ap);
    }

    static ::String format(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)))
    {
        va_list ap;
        ::String result;

        va_start(ap, fmt);
        vsnprintf(result.data, ::String::MAX_LEN - 1, fmt, ap);
        va_end(ap);

        return result;
    }

    operator std::string () const
    {
        return data;
    }

    bool operator == (const char *s) const { return isSame(s); }
    bool operator != (const char *s) const { return !isSame(s); }

    String& operator = (const String& other)
    {
        strcpy(this->data, other.data);
        this->type = other.type;

        return *this;
    }

    String& operator = (const char* cstr)
    {
        // FIXME: possibility of overflow
        strcpy(this->data, cstr);
        this->type = T_ASCII;

        return *this;
    }

    String& operator = (const std::string& rhs)
    {
        strcpy(data, rhs.substr(0, MAX_LEN - 1).c_str());
        this->type = T_ASCII;

        return *this;
    }

    operator const char *() const { return data; }

    void convertTo(TYPE t);

    char    *cstr() { return data; }
    std::string str() const { return data; }

    static bool isWhitespace(char c) { return c==' ' || c=='\t'; }

    TYPE    type;
    char    data[MAX_LEN];
};

#endif
