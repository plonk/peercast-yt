// ------------------------------------------------
// File : inifile.h
// Date: 4-apr-2002
// Author: giles
// Desc:
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

#ifndef _INIFILE
#define _INIFILE

#include "stream.h"

// -----------------------------------------
class IniFileBase
{
public:
    IniFileBase(Stream& aStream)
        : stream(aStream)
        , nameStr(nullptr)
        , valueStr(nullptr)
    {
        currLine[0] = '\0';
    }

    bool        readNext();

    bool        isName(const char *);
    char        *getName();
    int         getIntValue();
    const char  *getStrValue();
    bool        getBoolValue();

    void        writeSection(const char *);
    void        writeIntValue(const char *, int);
    void        writeStrValue(const char *, const char *);
    void        writeStrValue(const char *key, const String& value) { writeStrValue(key, value.c_str()); }
    void        writeStrValue(const char *key, const std::string& value) { writeStrValue(key, value.c_str()); }
    void        writeBoolValue(const char *, int);
    void        writeLine(const char *);

    Stream&     stream;
    char        currLine[256];
    char        *nameStr, *valueStr;
};

// -----------------------------------------
class IniFile : public IniFileBase
{
public:
    IniFile()
        : IniFileBase(fStream)
    {
    }
    ~IniFile() { close(); }

    bool        openReadOnly(const char *);
    bool        openWriteReplace(const char *);
    void        close();

    FileStream  fStream;
};

#endif
