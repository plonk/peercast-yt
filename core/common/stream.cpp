// ------------------------------------------------
// File : stream.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//		Basic stream handling functions. 
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


#include "stream.h"
#include "common.h"
#include "sys.h"


// --------------------------------------------------
void MemoryStream::convertFromBase64()
{       
	char *out = buf;
	char *in = buf;
	
	int rl=len;
    while(rl >= 4) 
	{
		out += String::base64WordToChars(out,in);
		in += 4;
        rl -= 4;
    }
    *out = 0;
	len = out-buf;
}
// -------------------------------------
void FileStream::openReadOnly(const char *fn)
{
	file = fopen(fn,"rb");

    if (!file)
    	throw StreamException("Unable to open file");
}
// -------------------------------------
void FileStream::openWriteReplace(const char *fn)
{
	file = fopen(fn,"wb");

	if (!file)
    	throw StreamException("Unable to open file");
}
// -------------------------------------
void FileStream::openWriteAppend(const char *fn)
{
        file = fopen(fn,"ab");

        if (!file)
        	throw StreamException("Unable to open file");
}

// -------------------------------------
void FileStream::close()
{
	if (file)
	{
		fclose(file);
		file = NULL;
	}
}
// -------------------------------------
void FileStream::rewind()
{
	if (file)
		fseek(file,0,SEEK_SET);
}
// -------------------------------------
int FileStream::length()
{
	int len = 0;
	if (file)
	{
		int old = ftell(file);
		fseek(file,0,SEEK_END);
		len = ftell(file);
		fseek(file,old,SEEK_SET);

	}
	return len;
}

// -------------------------------------
bool FileStream::eof()
{
	if (file)
		return (feof(file)!=0);
	else
		return true;
}
// -------------------------------------
int FileStream::read(void *ptr, int len)
{
	if (!file)
		return 0;
	if (feof(file))
    	throw StreamException("End of file");

	int r = (int)fread(ptr,1,len,file);

	updateTotals(r, 0);
    return r;
}

// -------------------------------------
void FileStream::write(const void *ptr, int len)
{
	if (!file)
		return;
    fwrite(ptr,1,len,file);
	updateTotals(0, len);

}
// -------------------------------------
void FileStream::flush()
{
	if (!file)
		return;
    fflush(file);
}
// -------------------------------------
int FileStream::pos()
{
	if (!file)
		return 0;
    return ftell(file);
}

// -------------------------------------
void FileStream::seekTo(int pos)
{
	if (!file)
		return;
	fseek(file,pos,SEEK_SET);
}

// -------------------------------------
void Stream::writeTo(Stream &out, int len)
{
	char tmp[4096];
	while (len)
	{
		int rlen = sizeof(tmp);
		if (rlen > len)
			rlen = len;

		read(tmp,rlen);
		out.write(tmp,rlen);

		len-=rlen;
	}
}
// -------------------------------------
int	Stream::writeUTF8(unsigned int code)
{
	if (code < 0x80)
	{
		writeChar(code);
		return 1;
	}else 
	if (code < 0x0800)
	{
		writeChar(code>>6 | 0xC0);
		writeChar(code & 0x3F | 0x80);
		return 2;
	}else if (code < 0x10000)
	{
		writeChar(code>>12 | 0xE0);
		writeChar(code>>6 & 0x3F | 0x80);
		writeChar(code & 0x3F | 0x80);
		return 3;
	}else 
	{
		writeChar(code>>18 | 0xF0);
		writeChar(code>>12 & 0x3F | 0x80);
		writeChar(code>>6 & 0x3F | 0x80);
		writeChar(code & 0x3F | 0x80);
		return 4;
	}
		
}

// -------------------------------------
void Stream::skip(int len)
{
	char tmp[4096];
	while (len)
	{
		int rlen = sizeof(tmp);
		if (rlen > len)
			rlen = len;
		read(tmp,rlen);
		len-=rlen;
	}

}


// -------------------------------------
void Stream::updateTotals(unsigned int in, unsigned int out)
{
	totalBytesIn += in;
	totalBytesOut += out;

	unsigned int tdiff = sys->getTime()-lastUpdate;
	if (tdiff >= 5)
	{
		bytesInPerSec = (totalBytesIn-lastBytesIn)/tdiff;
		bytesOutPerSec = (totalBytesOut-lastBytesOut)/tdiff;
		lastBytesIn = totalBytesIn;
		lastBytesOut = totalBytesOut;
		lastUpdate = sys->getTime();
	}
}
// -------------------------------------
int	Stream::readLine(char *in, int max)
{
    int i=0;
	max -= 2;

	while(max--)
    {                
    	char c;         
    	read(&c,1);
		if (c == '\n')
			break;
		if (c == '\r')
			continue;
        in[i++] = c;
    }
    in[i] = 0;
	return i;
}
// -------------------------------------
void Stream::write(const char *fmt,va_list ap)
{
	char tmp[4096];
	vsprintf(tmp,fmt,ap);
    write(tmp,strlen(tmp));
}
// -------------------------------------
void Stream::writeStringF(const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);
	write(fmt,ap);
   	va_end(ap);	
}
// -------------------------------------
void Stream::writeString(const char *str)
{
	write(str,strlen(str));
}
// -------------------------------------
void Stream::writeLineF(const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);

	write(fmt,ap);

	if (writeCRLF)
	    write("\r\n",2);
	else
		write("\n",1);

   	va_end(ap);	
}

// -------------------------------------
void Stream::writeLine(const char *str)
{
	writeString(str);

	if (writeCRLF)
	    write("\r\n",2);
	else
		write("\n",1);
}

// -------------------------------------
int	Stream::readWord(char *in, int max)
{
	int i=0;
    while (!eof())
    {
        char c = readChar();

        if ((c == ' ') || (c == '\t') || (c == '\r') || (c == '\n'))
        {
        	if (i)
            	break;		// stop reading
            else
	        	continue;	// skip whitespace
        }

    	if (i >= (max-1))
        	break;

        in[i++] = c;
    }

	in[i]=0;
    return i;
}


// --------------------------------------------------
int Stream::readBase64(char *p, int max)
{       
	char vals[4];

	int cnt=0;
	while (cnt < (max-4))
	{
		read(vals,4);
		int rl = String::base64WordToChars(p,vals);
		if (!rl)
			break;

		p+=rl;
		cnt+=rl;
	}
	*p = 0;
	return cnt;
}


// -------------------------------------
int Stream::readBits(int cnt)
{
	int v = 0;

	while (cnt)
	{
		if (!bitsPos)
			bitsBuffer = readChar();

		cnt--;

		v |= (bitsBuffer&(1<<(7-bitsPos)))?(1<<cnt):0;

		bitsPos = (bitsPos+1)&7;
	}

	return v;
}
