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
class IniFile 
{
public:
	bool	openReadOnly(const char *);
	bool	openWriteReplace(const char *);
	void	close();

	bool	readNext();

	bool	isName(const char *);
	char *	getName();
	int		getIntValue();
	char *	getStrValue();
	bool	getBoolValue();

	void	writeSection(const char *);
	void	writeIntValue(const char *, int);
	void	writeStrValue(const char *, const char *);
	void	writeBoolValue(const char *, int);
	void	writeLine(const char *);


	FileStream	fStream;
	char	currLine[256];
	char	*nameStr,*valueStr;
};

#endif
