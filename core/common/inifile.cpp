// ------------------------------------------------
// File : inifile.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		.INI file reading/writing class
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


	void	openReadOnly(const char *);
	void	openWriteReplace(const char *);
// -----------------------------------------
bool IniFile::openReadOnly(const char *fn)
{
	try 
	{
		fStream.openReadOnly(fn);
	}catch(StreamException &)
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

	}catch(StreamException &)
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
bool	IniFile::readNext()
{
	if (fStream.eof())
		return false;

	try
	{
		fStream.readLine(currLine,256);
	}catch(StreamException &)
	{
		return false;
	}


	// find end of value name and null terminate
	char *nend = strstr(currLine,"=");

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
bool IniFile::isName(const char *str)
{
	return stricmp(getName(),str)==0;
}

// -----------------------------------------
char *	IniFile::getName()
{
	return nameStr;
}
// -----------------------------------------
int		IniFile::getIntValue()
{
	if (valueStr)
		return atoi(valueStr);
	else
		return 0;
}
// -----------------------------------------
char *	IniFile::getStrValue()
{
	if (valueStr)
		return valueStr;
	else
		return "";
}
// -----------------------------------------
bool	IniFile::getBoolValue()
{
	if (!valueStr)
		return false;


	if ( (stricmp(valueStr,"yes")==0) ||
		 (stricmp(valueStr,"y")==0) ||
		 (stricmp(valueStr,"1")==0) )
		return true;

	return false;
}

// -----------------------------------------
void	IniFile::writeIntValue(const char *name, int iv)
{
	sprintf(currLine,"%s = %d",name,iv);
	fStream.writeLine(currLine);
}
// -----------------------------------------
void	IniFile::writeStrValue(const char *name, const char *sv)
{
	sprintf(currLine,"%s = %s",name,sv);
	fStream.writeLine(currLine);
}
// -----------------------------------------
void	IniFile::writeSection(const char *name)
{
	fStream.writeLine("");
	sprintf(currLine,"[%s]",name);
	fStream.writeLine(currLine);
}
// -----------------------------------------
void	IniFile::writeBoolValue(const char *name, int v)
{
	sprintf(currLine,"%s = %s",name,(v!=0)?"Yes":"No");
	fStream.writeLine(currLine);
}
// -----------------------------------------
void	IniFile::writeLine(const char *str)
{
	fStream.writeLine(str);
}
