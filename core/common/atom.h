// ------------------------------------------------
// File : atom.h
// Date: 1-mar-2004
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

#ifndef _ATOM_H
#define _ATOM_H

#include "stream.h"

#include "id.h"

// ------------------------------------------------
class AtomStream
{
public:
	AtomStream(Stream &s) : io(s),numChildren(0),numData(0)
	{}

	void checkData(int d)
	{
		if (numData != d)
			throw StreamException("Bad atom data");
	}

	void writeParent(ID4 id,int nc)
	{
		io.writeID4(id);
		io.writeInt(nc|0x80000000);
	}

	void writeInt(ID4 id,int d)
	{
		io.writeID4(id);
		io.writeInt(4);
		io.writeInt(d);
	}
	
	void writeID4(ID4 id,ID4 d)
	{
		io.writeID4(id);
		io.writeInt(4);
		io.writeID4(d);
	}

	void writeShort(ID4 id,short d)
	{
		io.writeID4(id);
		io.writeInt(2);
		io.writeShort(d);
	}

	void writeChar(ID4 id,char d)
	{
		io.writeID4(id);
		io.writeInt(1);
		io.writeChar(d);
	}

	void writeBytes(ID4 id,const void *p,int l)
	{
		io.writeID4(id);
		io.writeInt(l);
		io.write(p,l);
	}


	int writeStream(ID4 id,Stream &in,int l)
	{
		io.writeID4(id);
		io.writeInt(l);
		in.writeTo(io,l);
		return (sizeof(int)*2)+l;

	}

	void writeString(ID4 id,const char *p)
	{
		writeBytes(id,p,strlen(p)+1);
	}

	ID4	read(int &numc,int &dlen)
	{
		ID4 id = io.readID4();

		unsigned int v = io.readInt();
		if (v & 0x80000000)
		{
			numc = v&0x7fffffff;
			dlen = 0;
		}else
		{
			numc = 0;
			dlen = v;
		}

		numChildren = numc;
		numData = dlen;

		return id;
	}

	void skip(int c, int d)
	{
		if (d)
			io.skip(d);

		for(int i=0; i<c; i++)
		{
			int numc,data;
			read(numc,data);
			skip(numc,data);
		}

	}

	int		readInt() 
	{
		checkData(4);
		return io.readInt();
	}
	int		readID4() 
	{
		checkData(4);
		return io.readID4();
	}

	int		readShort() 
	{
		checkData(2);
		return io.readShort();
	}
	int		readChar() 
	{
		checkData(1);
		return io.readChar();
	}
	int		readBytes(void *p,int l) 
	{
		checkData(l);
		return io.read(p,l);
	}

	void	readString(char *s,int max,int dlen)
	{
		checkData(dlen);
		readBytes(s,max,dlen);
		s[max-1] = 0;

	}

	void	readBytes(void *s,int max,int dlen)
	{
		checkData(dlen);
		if (max > dlen)
			readBytes(s,dlen);
		else
		{
			readBytes(s,max);
			io.skip(dlen-max);
		}
	}

	int		writeAtoms(ID4 id, Stream &in, int cnt, int data)
	{
		int total=0;

		if (cnt)
		{
			writeParent(id,cnt);
			total+=sizeof(int)*2;

			for(int i=0; i<cnt; i++)
			{
				AtomStream ain(in);
				int c,d;
				ID4 cid = ain.read(c,d);
				total += writeAtoms(cid,in,c,d);
			}
		}else
		{
			total += writeStream(id,in,data);
		}

		return total;
	}

	bool	eof() {return io.eof();}	


	int	numChildren,numData;
	Stream &io;
};

#endif
