// ------------------------------------------------
// File : inifile.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      .INI file reading/writing class
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
#include "inifile.h"
#include "sys.h"

// -----------------------------------------
bool IniFile::openReadOnly(const char *fn)
{
    try
    {
        fStream.openReadOnly(fn);
    }catch (StreamException &)
    {
        return false;
    }
    return true;
}

// -----------------------------------------
bool IniFile::openWriteReplace(const char *fn)
{
    try
    {
        fStream.openWriteReplace(fn);
#if defined(_LINUX) || defined(__APPLE__)
        fStream.writeCRLF = false;
#endif
    }catch (StreamException &)
    {
        return false;
    }
    return true;
}

// -----------------------------------------
void IniFile::close()
{
    fStream.close();
}

// -----------------------------------------
bool    IniFileBase::readNext()
{
    if (stream.eof())
        return false;

    try
    {
        stream.readLine(currLine, 256);
    }catch (StreamException &)
    {
        return false;
    }

    // find end of value name and null terminate
    char *nend = strstr(currLine, "=");

    if (nend)
    {
        *nend = 0;
        valueStr = trimstr(nend+1);
    }else
        valueStr = NULL;

    nameStr = trimstr(currLine);

    return true;
}

// -----------------------------------------
bool IniFileBase::isName(const char *str)
{
    return stricmp(getName(), str)==0;
}

// -----------------------------------------
char *  IniFileBase::getName()
{
    return nameStr;
}

// -----------------------------------------
int     IniFileBase::getIntValue()
{
    if (valueStr)
        return atoi(valueStr);
    else
        return 0;
}

// -----------------------------------------
const char *    IniFileBase::getStrValue()
{
    if (valueStr)
        return valueStr;
    else
        return "";
}

// -----------------------------------------
bool    IniFileBase::getBoolValue()
{
    if (!valueStr)
        return false;

    if ( (stricmp(valueStr, "yes")==0) ||
         (stricmp(valueStr, "y")==0) ||
         (stricmp(valueStr, "1")==0) )
        return true;

    return false;
}

// -----------------------------------------
void    IniFileBase::writeIntValue(const char *name, int iv)
{
    snprintf(currLine, sizeof(currLine), "%s = %d", name, iv);
    stream.writeLine(currLine);
}

// -----------------------------------------
void    IniFileBase::writeStrValue(const char *name, const char *sv)
{
    snprintf(currLine, sizeof(currLine), "%s = %s", name, sv);
    stream.writeLine(currLine);
}

// -----------------------------------------
void    IniFileBase::writeSection(const char *name)
{
    stream.writeLine("");
    snprintf(currLine, sizeof(currLine), "[%s]", name);
    stream.writeLine(currLine);
}

// -----------------------------------------
void    IniFileBase::writeBoolValue(const char *name, int v)
{
    snprintf(currLine, sizeof(currLine), "%s = %s", name, (v!=0)?"Yes":"No");
    stream.writeLine(currLine);
}

// -----------------------------------------
void    IniFileBase::writeLine(const char *str)
{
    stream.writeLine(str);
}
