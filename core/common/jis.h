// ------------------------------------------------
// File : jis.h
// Date: 1-april-2004
// Author: giles
//
// (c) 2002-4 peercast.org
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

#ifndef _JIS_H
#define _JIS_H

// ----------------------------------
class JISConverter
{
public:

	static unsigned short sjisToUnicode(unsigned short);
	static unsigned short eucToUnicode(unsigned short);
};



#endif


