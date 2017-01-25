// ------------------------------------------------
// File : asf.h
// Date: 10-apr-2003
// Author: giles
//
// (c) 2002-3 peercast.org
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

#ifndef _ASF_H
#define _ASF_H


#include "stream.h"

// -----------------------------------
class MSID
{
public:

	void read(Stream &in)
	{
		data1 = in.readLong();
		data2 = in.readShort();
		data3 = in.readShort();
		in.read(data4,8);
	}

	void write(Stream &out)
	{
		out.writeLong(data1);
		out.writeShort(data2);
		out.writeShort(data3);
		out.write(data4,8);
	}
	
	
	void toString(String &s)
	{
		sprintf(s.data,"%X-%X-%X-%02X%02X%02X%02X%02X%02X%02X%02X",
			data1,data2,data3,
			data4[0],data4[1],data4[2],data4[3],
			data4[4],data4[5],data4[6],data4[7]);
	}

    int operator==(const MSID& msid) const{return !memcmp(this, &msid, sizeof(MSID));}

	unsigned int data1;
	unsigned short data2,data3;
	unsigned char data4[8];

};




// -----------------------------------
const MSID headObjID=
	{0x75B22630, 0x668E, 0x11CF, 0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C};
const MSID dataObjID=
	{0x75B22636, 0x668E, 0x11CF, 0xA6,0xD9,0x00,0xAA,0x00,0x62,0xCE,0x6C};
const MSID filePropObjID=
	{0x8CABDCA1, 0xA947, 0x11CF, 0x8E,0xE4,0x00,0xC0,0x0C,0x20,0x53,0x65};
const MSID streamPropObjID=
	{0xB7DC0791, 0xA9B7, 0x11CF, 0x8E,0xE6,0x00,0xC0,0x0C,0x20,0x53,0x65};

const MSID audioStreamObjID=
	{0xF8699E40, 0x5B4D, 0x11CF, 0xA8,0xFD,0x00,0x80,0x5F,0x5C,0x44,0x2B};
const MSID videoStreamObjID=
	{0xBC19EFC0, 0x5B4D, 0x11CF, 0xA8,0xFD,0x00,0x80,0x5F,0x5C,0x44,0x2B};

const MSID streamBitrateObjID=
	{0x7BF875CE, 0x468D, 0x11D1, 0x8D,0x82,0x00,0x60,0x97,0xC9,0xA2,0xB2};


// -----------------------------------
class ASFObject
{
public:

	enum TYPE
	{
		T_UNKNOWN,
		T_HEAD_OBJECT,
		T_DATA_OBJECT,
		T_FILE_PROP,
		T_STREAM_PROP,
		T_STREAM_BITRATE
	};

	int getTotalLen()
	{
		return 24+dataLen;
	}

	unsigned int readHead(Stream &in)
	{
		id.read(in);

		lenLo = in.readLong();
		lenHi = in.readLong();

		type = T_UNKNOWN;
		if (id == headObjID)
			type = T_HEAD_OBJECT;
		else if (id == dataObjID)
			type = T_DATA_OBJECT;
		else if (id == filePropObjID)
			type = T_FILE_PROP;
		else if (id == streamPropObjID)
			type = T_STREAM_PROP;
		else if (id == streamBitrateObjID)
			type = T_STREAM_BITRATE;

		String str;
		id.toString(str);
		LOG_DEBUG("ASF: %s (%s)= %d : %d\n",str.data,getTypeName(),lenLo,lenHi);


		dataLen = 0;

		return lenLo-24;
	}

	void readData(Stream &in,int len)
	{
		dataLen = len;

		if ((dataLen > sizeof(data)) || (lenHi)) 
			throw StreamException("ASF object too big");

		in.read(data,dataLen);
	}


	void write(Stream &out)
	{
		id.write(out);
		out.writeLong(lenLo);
		out.writeLong(lenHi);
		if (dataLen)
			out.write(data,dataLen);
	}

	const char *getTypeName()
	{
		switch(type)
		{
			case T_HEAD_OBJECT:
				return "ASF_Header_Object";
			case T_DATA_OBJECT:
				return "ASF_Data_Object";
			case T_FILE_PROP:
				return "ASF_File_Properties_Object";
			case T_STREAM_PROP:
				return "ASF_Stream_Properties_Object";
			case T_STREAM_BITRATE:
				return "ASF_Stream_Bitrate_Properties_Object";
			default:
				return "Unknown_Object";
		}
	}

	char data[8192];
	MSID	id;
	unsigned int lenLo,lenHi,dataLen;
	TYPE type;
};
// -----------------------------------
class ASFStream
{
public:
	enum TYPE
	{
		T_UNKNOWN,
		T_AUDIO,
		T_VIDEO
	};

	void read(Stream &in)
	{
		MSID sid;
		sid.read(in);

		if (sid == videoStreamObjID)
			type = T_VIDEO;
		else if (sid == audioStreamObjID)
			type = T_AUDIO;
		else 
			type = T_UNKNOWN;

		in.skip(32);
		id = in.readShort()&0x7f;
	}


	const char *getTypeName()
	{
		switch(type)
		{
			case T_VIDEO:
				return "Video";
			case T_AUDIO:
				return "Audio";
		}
		return "Unknown";
	}

	void reset()
	{
		id = 0;
		bitrate = 0;
		type = T_UNKNOWN;
	}

	unsigned int id;
	int bitrate;
	TYPE type;
};

// -----------------------------------
class ASFInfo
{
public:
	enum
	{
		MAX_STREAMS = 128
	};

	ASFInfo()
	{
		numPackets = 0;
		packetSize = 0;
		flags = 0;
		bitrate=0;
		for(int i=0; i<MAX_STREAMS; i++)
			streams[i].reset();
	}

	unsigned int packetSize,numPackets,flags,bitrate;

	ASFStream streams[MAX_STREAMS];
};

// -----------------------------------
class ASFChunk
{
public:

	void read(Stream &in)
	{
		type = in.readShort();
		len = in.readShort();
		seq = in.readLong();
		v1 = in.readShort();
		v2 = in.readShort();

		dataLen = len-8;
		if (dataLen > sizeof(data)) 
			throw StreamException("ASF chunk too big");
		in.read(data,dataLen);
	}

	void write(Stream &out)
	{
		out.writeShort(type);
		out.writeShort(len);
		out.writeLong(seq);
		out.writeShort(v1);
		out.writeShort(v2);
		out.write(data,dataLen);
	}

	unsigned int seq,dataLen;
	unsigned short type,len,v1,v2;
	unsigned char data[8192];
};


#endif

