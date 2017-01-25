// ------------------------------------------------
// File : stream.h
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

#ifndef _STREAM_H
#define _STREAM_H

// -------------------------------------

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "common.h"
#include "sys.h"
#include "id.h"

// -------------------------------------
class Stream
{
public:
	Stream()
	:writeCRLF(true)
	,totalBytesIn(0)
	,totalBytesOut(0)
	,lastBytesIn(0)
	,lastBytesOut(0)
	,bytesInPerSec(0)
	,bytesOutPerSec(0)
	,lastUpdate(0)
	,bitsBuffer(0)
	,bitsPos(0)
	{
	}

	virtual int readUpto(void *,int) {return 0;}
	virtual int read(void *,int)=0;
	virtual void write(const void *,int) = 0;
    virtual bool eof()
    {
    	throw StreamException("Stream can`t eof");
		return false;
    }

	virtual void rewind()
	{
    	throw StreamException("Stream can`t rewind");
	}

	virtual void seekTo(int)
	{
    	throw StreamException("Stream can`t seek");
	}

	void writeTo(Stream &out, int len);
	virtual void skip(int i);

	virtual void close()
	{
	}

	virtual void	setReadTimeout(unsigned int ) 
	{
	}
	virtual void	setWriteTimeout(unsigned int )
	{
	}
	virtual void	setPollRead(bool)
	{
	}

	virtual int		getPosition() {return 0;}


	// binary
    char	readChar()
    {
    	char v;
        read(&v,1);
        return v;
    }
    short	readShort()
    {
    	short v;
        read(&v,2);
		CHECK_ENDIAN2(v);
        return v;
    }
    long	readLong()
    {
    	long v;
        read(&v,4);
		CHECK_ENDIAN4(v);
        return v;
    }
	int readInt()
	{
		return readLong();
	}
	ID4 readID4()
	{
		ID4 id;
		read(id.getData(),4);
		return id;
	}
	int	readInt24()
	{
		int v=0;
        read(&v,3);
		CHECK_ENDIAN3(v);
	}



	long readTag()
	{
		long v = readLong();
		return SWAP4(v);
	}

	int	readString(char *s, int max)
	{
		int cnt=0;
		while (max)
		{
			char c = readChar();
			*s++ = c;
			cnt++;
			max--;
			if (!c)
				break;
		}
		return cnt;
	}

	virtual bool	readReady() {return true;}
	virtual int numPending() {return 0;}


	void writeID4(ID4 id)
	{
		write(id.getData(),4);
	}

	void	writeChar(char v)
	{
		write(&v,1);
	}
	void	writeShort(short v)
	{
		CHECK_ENDIAN2(v);
		write(&v,2);
	}
	void	writeLong(long v)
	{
		CHECK_ENDIAN4(v);
		write(&v,4);
	}
	void writeInt(int v) {writeLong(v);}

	void	writeTag(long v)
	{
		//CHECK_ENDIAN4(v);
		writeLong(SWAP4(v));
	}

	void	writeTag(char id[4])
	{
		write(id,4);
	}

	int	writeUTF8(unsigned int);

	// text
    int	readLine(char *in, int max);

    int		readWord(char *, int);
	int		readBase64(char *, int);

	void	write(const char *,va_list);
	void	writeLine(const char *);
	void	writeLineF(const char *,...);
	void	writeString(const char *);
	void	writeStringF(const char *,...);

	bool	writeCRLF;

	int		readBits(int);

	void	updateTotals(unsigned int,unsigned int);


	unsigned char bitsBuffer;
	unsigned int bitsPos;

	unsigned int totalBytesIn,totalBytesOut;
	unsigned int lastBytesIn,lastBytesOut;
	unsigned int bytesInPerSec,bytesOutPerSec;
	unsigned int lastUpdate;

};


// -------------------------------------
class FileStream : public Stream
{
public:
	FileStream() {file=NULL;}

    void	openReadOnly(const char *);
    void	openWriteReplace(const char *);
    void	openWriteAppend(const char *);
	bool	isOpen(){return file!=NULL;}
	int		length();
	int		pos();

	virtual void	seekTo(int);
	virtual int		getPosition() {return pos();}
	virtual void	flush();
    virtual int		read(void *,int);
    virtual void	write(const void *,int);
    virtual bool	eof();
    virtual void	rewind();
    virtual void	close();


	FILE *file;
};
// -------------------------------------
class MemoryStream : public Stream
{
public:
	MemoryStream()
	:buf(NULL)
	,len(0)
	,pos(0)
	{
	}

	MemoryStream(void *p, int l)
	:buf((char *)p)
	,len(l)
	,pos(0)
	{
	}

	MemoryStream(int l)
	:buf(new char[l])
	,len(l)
	,pos(0)
	{
	}

	void readFromFile(FileStream &file)
	{
		len = file.length();
		buf = new char[len];
		pos = 0;
		file.read(buf,len);
	}

	void free()
	{
		if (buf)
		{
			delete buf;
			buf = NULL;
		}

	}

	virtual int read(void *p,int l)
    {
		if (pos+l <= len)
		{
			memcpy(p,&buf[pos],l);
			pos += l;
			return l;
		}else
		{
			memset(p,0,l);
			return 0;
		}
    }

	virtual void write(const void *p,int l)
    {
		if ((pos+l) > len)
			throw StreamException("Stream - premature end of write()");
		memcpy(&buf[pos],p,l);
		pos += l;
    }

    virtual bool eof()
    {
        return pos >= len;
    }

	virtual void rewind()
	{
		pos = 0;
	}

	virtual void seekTo(int p)
	{
		pos = p;
	}

	virtual int getPosition()
	{
		return pos;
	}

	void	convertFromBase64();


	char *buf;
	int len,pos;
};
// --------------------------------------------------
class IndirectStream : public Stream
{
public:

	void init(Stream *s)
	{
		stream = s;
	}

	virtual int read(void *p,int l)
    {
		return stream->read(p,l);
    }

	virtual void write(const void *p,int l)
    {
		stream->write(p,l);
    }

    virtual bool eof()
    {
        return stream->eof();
    }

    virtual void close()
    {
        stream->close();
    }

	Stream *stream;
};


#endif

