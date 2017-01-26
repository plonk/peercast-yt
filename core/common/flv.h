// ------------------------------------------------
// File : flv.h
// Date: 14-jan-2017
// Author: niwakazoider
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

#ifndef _FLV_H
#define _FLV_H

#include "channel.h"
#include "stdio.h"


const int TAG_SCRIPTDATA = 18;
const int TAG_AUDIO = 8;
const int TAG_VIDEO = 9;

// -----------------------------------
class FLVFileHeader
{
public:
	void read(Stream &in)
	{
		size = 13;
		data = (char *)malloc(size);
		in.read(data, size);

		if (data[0] == 0x46 && //F
			data[1] == 0x4c && //L
			data[2] == 0x56) { //V
			version = data[3];
		}
	}
	int version;
	int size = 0;
	char *data = NULL;
};


// -----------------------------------
class FLVTag
{
public:
	enum TYPE
	{
		T_UNKNOWN,
		T_SCRIPT,
		T_AUDIO,
		T_VIDEO
	};

	FLVTag()
	{
		size = 0;
		type = T_UNKNOWN;
		data = NULL;
		packet = NULL;
	}

	void read(Stream &in)
	{
		if (data != NULL) free(data);
		if (packet != NULL) free(packet);

		unsigned char binary[11];
		in.read(binary, 11);

		type = binary[0];
		size = (binary[1] << 16) | (binary[2] << 8) | (binary[3]);
		//int timestamp = (binary[7] << 24) | (binary[4] << 16) | (binary[5] << 8) | (binary[6]);
		//int streamID = (binary[8] << 16) | (binary[9] << 8) | (binary[10]);
		data = (char *)malloc(size);
		in.read(data, size);

		unsigned char prevsize[4];
		in.read(prevsize, 4);

		packet = (char *)malloc(11+size+4);
		memcpy(packet, binary, 11);
		memcpy(packet+11, data, size);
		memcpy(packet+11+size, prevsize, 4);
		packetSize = 11 + size + 4;
	}

	const char *getTagType()
	{
		switch (type)
		{
		case TAG_SCRIPTDATA:
			return "Script";
		case TAG_VIDEO:
			return "Video";
		case TAG_AUDIO:
			return "Audio";
		}
		return "Unknown";
	}

	int size = 0;
	int packetSize = 0;
	char type = T_UNKNOWN;
	char *data = NULL;
	char *packet = NULL;
};



// ----------------------------------------------
class FLVStream : public ChannelStream
{
public:
	int bitrate = 0;
	FLVFileHeader fileHeader;
	FLVTag metaData;
	FLVTag aacHeader;
	FLVTag avcHeader;
	FLVStream()
	{
		fileHeader = FLVFileHeader();
		metaData = FLVTag();
		aacHeader = FLVTag();
		avcHeader = FLVTag();
	}
	virtual void readHeader(Stream &, Channel *);
	virtual int	 readPacket(Stream &, Channel *);
	virtual void readEnd(Stream &, Channel *);
};

class AMFObject
{
public:
	const int AMF_NUMBER      = 0x00;
	const int AMF_BOOL        = 0x01;
	const int AMF_STRING      = 0x02;
	const int AMF_OBJECT      = 0x03;
	const int AMF_MOVIECLIP   = 0x04;
	const int AMF_NULL        = 0x05;
	const int AMF_UNDEFINED   = 0x06;
	const int AMF_REFERENCE   = 0x07;
	const int AMF_ARRAY       = 0x08;
	const int AMF_OBJECT_END  = 0x09;
	const int AMF_STRICTARRAY = 0x0a;
	const int AMF_DATE        = 0x0b;
	const int AMF_LONG_STRING = 0x0c;

	bool readBool(Stream &in)
	{
		return in.readChar() != 0;
	}

	int readInt(Stream &in)
	{
		return (in.readChar() << 24) | (in.readChar() << 16) | (in.readChar() << 8) | (in.readChar());
	}

	char* readString(Stream &in)
	{
		int len = (in.readChar() << 8) | (in.readChar());
		if (len == 0) {
			return NULL;
		}
		else {
			char* data = (char *)malloc(len+1);
			*(data+len) = '\0';
			in.read(data, len);
			return data;
		}
	};

	double readDouble(Stream &in)
	{
		char* data = (char *)malloc(sizeof(double));
		for (int i = 8; i > 0; i--) {
			char c = in.readChar();
			*(data + i - 1) = c;
		}
		double number = *reinterpret_cast<double*>(data);
		return number;
	};

	void readObject(Stream &in)
	{
		while (true) {
			char* key = readString(in);
			if (key == NULL) {
				break;
			}
			else {
				if (strcmp(key, "audiodatarate") == 0) {
					in.readChar();
					bitrate += readDouble(in);
				}
				else if (strcmp(key, "videodatarate") == 0) {
					in.readChar();
					bitrate += readDouble(in);
				}
				else {
					read(in);
				}
			}
		}
		in.readChar();
	}

	bool readMetaData(Stream &in)
	{
		char type = in.readChar();
		if (type == AMF_STRING) {
			char* name = readString(in);
			if (strcmp(name, "onMetaData") == 0) {
				bitrate = 0;
				read(in);
			}
		}
		return bitrate > 0;
	}

	void read(Stream &in)
	{
		char type = in.readChar();
		if (type == AMF_NUMBER) {
			readDouble(in);
		}
		else if (type == AMF_BOOL) {
			readBool(in);
		}
		else if (type == AMF_STRING) {
			readString(in);
		}
		else if (type == AMF_OBJECT) {
			readObject(in);
		}
		else if (type == AMF_OBJECT_END) {
		}
		else if (type == AMF_ARRAY) {
			int len = readInt(in);
			readObject(in);
		}
		else if (type == AMF_STRICTARRAY) {
			int len = readInt(in);
			for (int i = 0; i < len; i++) {
				read(in);
			}
		}
		else if (type == AMF_DATE) {
			in.skip(10);
		}
	}
	int bitrate = 0;
};


#endif 
