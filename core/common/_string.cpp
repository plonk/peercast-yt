// ------------------------------------------------
// File : _string.cpp
// Date: 4-apr-2002
// Author: giles
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

#include <ctime> // for ctime

#include "_string.h"
#include "jis.h"
#include "stream.h"

// -----------------------------------
#define isSJIS(a, b) (((a >= 0x81 && a <= 0x9f) || (a >= 0xe0 && a <= 0xfc)) && ((b >= 0x40 && b <= 0x7e) || (b >= 0x80 && b <= 0xfc)))
#define isEUC(a) (a >= 0xa1 && a <= 0xfe)
#define isASCII(a) (a <= 0x7f)
#define isPLAINASCII(a) (((a >= '0') && (a <= '9')) || ((a >= 'a') && (a <= 'z')) || ((a >= 'A') && (a <= 'Z')))
#define isUTF8(a, b) ((a & 0xc0) == 0xc0 && (b & 0x80) == 0x80 )
#define isESCAPE(a, b) ((a == '&') && (b == '#'))
#define isHTMLSPECIAL(a) ((a == '&') || (a == '\"') || (a == '\'') || (a == '<') || (a == '>'))

// -----------------------------------
static int base64chartoval(char input)
{
    if (input >= 'A' && input <= 'Z')
        return input - 'A';
    else if (input >= 'a' && input <= 'z')
        return input - 'a' + 26;
    else if (input >= '0' && input <= '9')
        return input - '0' + 52;
    else if (input == '+')
        return 62;
    else if (input == '/')
        return 63;
    else if (input == '=')
        return -1;
    else
        return -2;
}

// -----------------------------------
String& String::setFromTime(unsigned int t)
{
    time_t t2 = t;
    char *p = ctime_r(&t2, data);
    if (!p)
        strcpy(data, "-");
    type = T_ASCII;

    return *this;
}

// -----------------------------------
String& String::setFromStopwatch(unsigned int t)
{
    unsigned int sec, min, hour, day;

    sec = t%60;
    min = (t/60)%60;
    hour = (t/3600)%24;
    day = (t/86400);

    if (day)
        std::sprintf(data, "%d day, %d hour", day, hour);
    else if (hour)
        std::sprintf(data, "%d hour, %d min", hour, min);
    else if (min)
        std::sprintf(data, "%d min, %d sec", min, sec);
    else if (sec)
        std::sprintf(data, "%d sec", sec);
    else
        std::sprintf(data, "-");

    type = T_ASCII;

    return *this;
}

// -----------------------------------
String& String::setFromString(const char *str, TYPE t)
{
    int cnt = 0;
    bool quote = false;
    while (*str)
    {
        bool add = true;
        if (*str == '\"')
        {
            if (quote)
                break;
            else
                quote = true;
            add = false;
        }else if (*str == ' ')
        {
            if (!quote)
            {
                if (cnt)
                    break;
                else
                    add = false;
            }
        }

        if (add)
        {
            data[cnt++] = *str++;
            if (cnt >= (MAX_LEN - 1))
                break;
        }else
            str++;
    }
    data[cnt] = 0;
    type = t;

    return *this;
}

// -----------------------------------
int String::base64WordToChars(char *out, const char *input)
{
    char *start = out;
    signed char vals[4];

    vals[0] = base64chartoval(*input++);
    vals[1] = base64chartoval(*input++);
    vals[2] = base64chartoval(*input++);
    vals[3] = base64chartoval(*input++);

    if (vals[0] < 0 || vals[1] < 0 || vals[2] < -1 || vals[3] < -1)
        return 0;

    *out++ = vals[0]<<2 | vals[1]>>4;
    if (vals[2] >= 0)
        *out++ = ((vals[1]&0x0F)<<4) | (vals[2]>>2);
    else
        *out++ = 0;

    if (vals[3] >= 0)
        *out++ = ((vals[2]&0x03)<<6) | (vals[3]);
    else
        *out++ = 0;

    return out-start;
}

// -----------------------------------
void String::BASE642ASCII(const char *input)
{
    char *out = data;
    int len = strlen(input);

    while (len >= 4)
    {
        out += base64WordToChars(out, input);
        input += 4;
        len -= 4;
    }
    *out = 0;
}

// -----------------------------------
void String::UNKNOWN2UNICODE(const char *in, bool safe)
{
    MemoryStream utf8(data, MAX_LEN - 1);

    unsigned char c;
    unsigned char d;

    while ((c = *in++) != '\0')
    {
        d = *in;

        if (isUTF8(c, d))       // utf8 encoded
        {
            int numChars = 0;

            for (int i = 0; i < 6; i++)
            {
                if (c & (0x80>>i))
                    numChars++;
                else
                    break;
            }

            utf8.writeChar(c);
            for (int i = 0; i < numChars - 1; i++)
                utf8.writeChar(*in++);
        }
        else if (isSJIS(c, d))           // shift_jis
        {
            utf8.writeUTF8(JISConverter::sjisToUnicode((c<<8 | d)));
            in++;
        }
        else if (isEUC(c) && isEUC(d))       // euc-jp
        {
            utf8.writeUTF8(JISConverter::eucToUnicode((c<<8 | d)));
            in++;
        }
        else if (isESCAPE(c, d))        // html escape tags &#xx;
        {
            in++;
            char code[16];
            char *cp = code;
            while ((c = *in++) != '\0')
            {
                if (c != ';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;

            utf8.writeUTF8(strtoul(code, NULL, 10));
        }
        else if (isPLAINASCII(c))   // plain ascii : a-z 0-9 etc..
        {
            utf8.writeUTF8(c);
        }
        else if (isHTMLSPECIAL(c) && safe)
        {
            const char *str = NULL;
            if (c == '&') str = "&amp;";
            else if (c == '\"') str = "&quot;";
            else if (c == '\'') str = "&#039;";
            else if (c == '<') str = "&lt;";
            else if (c == '>') str = "&gt;";
            else str = "?";

            utf8.writeString(str);
        }
        else
        {
            utf8.writeUTF8(c);
        }

        if (utf8.pos >= (MAX_LEN - 10))
            break;
    }

    utf8.writeChar(0);  // null terminate
}

// -----------------------------------
void String::ASCII2HTML(const char *in)
{
    char *op = data;
    char *oe = data+MAX_LEN - 10;
    unsigned char c;
    const char *p = in;
    while ((c = *p++) != '\0')
    {
        if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
        {
            *op++ = c;
        }else
        {
            std::sprintf(op, "&#x%02X;", (int)c);
            op += 6;
        }
        if (op >= oe)
            break;
    }
    *op = 0;
}

// -----------------------------------
void String::ASCII2ESC(const char *in, bool safe)
{
    char *op = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    unsigned char c;
    while ((c = *p++) != '\0')
    {
        if (((c >= '0') && (c <= '9')) || ((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')))
            *op++ = c;
        else
        {
            *op++ = '%';
            if (safe)
                *op++ = '%';
            *op = 0;
            std::sprintf(op, "%02X", (int)c);
            op += 2;
        }
        if (op >= oe)
            break;
    }
    *op = 0;
}

// -----------------------------------
void String::HTML2ASCII(const char *in)
{
    unsigned char c;
    char *o = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    while ((c = *p++) != '\0')
    {
        if ((c == '&') && (p[0] == '#'))
        {
            p++;
            char code[8];
            char *cp = code;
            char ec = *p++;     // hex/dec
            while ((c = *p++) != '\0')
            {
                if (c != ';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;
            c = (unsigned char)strtoul(code, NULL, ec=='x'?16:10);
        }
        *o++ = c;
        if (o >= oe)
            break;
    }

    *o = 0;
}

// -----------------------------------
void String::HTML2UNICODE(const char *in)
{
    MemoryStream utf8(data, MAX_LEN-1);

    unsigned char c;
    while ((c = *in++) != '\0')
    {
        if ((c == '&') && (*in == '#'))
        {
            in++;
            char code[16];
            char *cp = code;
            char ec = *in++;        // hex/dec
            while ((c = *in++) != '\0')
            {
                if (c != ';')
                    *cp++ = c;
                else
                    break;
            }
            *cp = 0;
            utf8.writeUTF8(strtoul(code, NULL, ec=='x'?16:10));
        }else
            utf8.writeUTF8(c);

        if (utf8.pos >= (MAX_LEN-10))
            break;
    }

    utf8.writeUTF8(0);
}

// -----------------------------------
void String::ESC2ASCII(const char *in)
{
    unsigned char c;
    char *o = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    while ((c = *p++) != '\0')
    {
        if (c == '+')
            c = ' ';
        else if (c == '%')
        {
            if (p[0] == '%')
                p++;

            char hi = TOUPPER(p[0]);
            char lo = TOUPPER(p[1]);
            c = (TONIBBLE(hi)<<4) | TONIBBLE(lo);
            p += 2;
        }
        *o++ = c;
        if (o >= oe)
            break;
    }

    *o = 0;
}

// -----------------------------------
void String::ASCII2META(const char *in, bool safe)
{
    char *op = data;
    char *oe = data+MAX_LEN-10;
    const char *p = in;
    unsigned char c;
    while ((c = *p++) != '\0')
    {
        switch (c)
        {
            case '%':
                if (safe)
                    *op++ = '%';
                break;
            case ';':
                c = ':';
                break;
        }

        *op++ = c;
        if (op >= oe)
            break;
    }
    *op = 0;
}

// -----------------------------------
void String::convertTo(TYPE t)
{
    if (t != type)
    {
        String tmp = *this;

        // convert to ASCII
        switch (type)
        {
            case T_UNKNOWN:
            case T_ASCII:
                break;
            case T_HTML:
                tmp.HTML2ASCII(data);
                break;
            case T_ESC:
            case T_ESCSAFE:
                tmp.ESC2ASCII(data);
                break;
            case T_META:
            case T_METASAFE:
                break;
            case T_BASE64:
                tmp.BASE642ASCII(data);
                break;
            default:
                break;
        }

        // convert to new format
        switch (t)
        {
            case T_UNKNOWN:
            case T_ASCII:
                strcpy(data, tmp.data);
                break;
            case T_UNICODE:
                UNKNOWN2UNICODE(tmp.data, false);
                break;
            case T_UNICODESAFE:
                UNKNOWN2UNICODE(tmp.data, true);
                break;
            case T_HTML:
                ASCII2HTML(tmp.data);
                break;
            case T_ESC:
                ASCII2ESC(tmp.data, false);
                break;
            case T_ESCSAFE:
                ASCII2ESC(tmp.data, true);
                break;
            case T_META:
                ASCII2META(tmp.data, false);
                break;
            case T_METASAFE:
                ASCII2META(tmp.data, true);
                break;
            default:
                break;
        }

        type = t;
    }
}

// -----------------------------------
String& String::setUnquote(const char *p, TYPE t)
{
    int slen = strlen(p);
    if (slen > 2)
    {
        if (slen >= MAX_LEN) slen = MAX_LEN;
        strncpy(data, p+1, slen-2);
        data[slen-2] = 0;
    }else
        clear();
    type = t;

    return *this;
}

// -----------------------------------
void String::clear()
{
    data[0] = 0;
    type = T_UNKNOWN;
}

// -----------------------------------
void String::append(const char *s)
{
    if ((strlen(s)+strlen(data)) < (MAX_LEN-1))
        strcat(data, s);
}

// -----------------------------------
void String::append(char c)
{
    char tmp[2];
    tmp[0] = c;
    tmp[1] = 0;
    append(tmp);
}

// -----------------------------------
void String::prepend(const char *s)
{
    ::String tmp;
    tmp.set(s);
    tmp.append(data);
    tmp.type = type;
    *this = tmp;
}

// -----------------------------------
String& String::operator = (const String& other)
{
    strcpy(this->data, other.data);
    this->type = other.type;

    return *this;
}

// -----------------------------------
String& String::operator = (const char* cstr)
{
    strncpy(this->data, cstr, MAX_LEN - 1);
    this->data[MAX_LEN - 1] = '\0';
    this->type = T_ASCII;

    return *this;
}

// -----------------------------------
String& String::operator = (const std::string& rhs)
{
    strcpy(data, rhs.substr(0, MAX_LEN - 1).c_str());
    this->type = T_ASCII;

    return *this;
}

// -----------------------------------
String& String::set(const char *p, TYPE t)
{
    strncpy(data, p, MAX_LEN-1);
    data[MAX_LEN-1] = 0;
    type = t;

    return *this;
}

// -----------------------------------
void String::sprintf(const char* fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(this->data, ::String::MAX_LEN - 1, fmt, ap);
    va_end(ap);
}

// -----------------------------------
String String::format(const char* fmt, ...)
{
    va_list ap;
    String result;

    va_start(ap, fmt);
    vsnprintf(result.data, ::String::MAX_LEN - 1, fmt, ap);
    va_end(ap);

    return result;
}
