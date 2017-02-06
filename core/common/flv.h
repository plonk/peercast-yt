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

// -----------------------------------
class FLVFileHeader
{
public:
    FLVFileHeader() : size(0) {}

    void read(Stream &in)
    {
        size = 13;
        in.read(data, 13);

        if (data[0] == 0x46 && //F
            data[1] == 0x4c && //L
            data[2] == 0x56) { //V
            version = data[3];
        }
    }
    int size;
    int version;
    unsigned char data[13];
};


// -----------------------------------
class FLVTag
{
public:
    enum TYPE
    {
        T_UNKNOWN,
        T_SCRIPT    = 18,
        T_AUDIO     = 8,
        T_VIDEO     = 9
    };

    FLVTag()
    {
        size = 0;
        packetSize = 0;
        type = T_UNKNOWN;
        data = NULL;
        packet = NULL;
    }

    ~FLVTag()
    {
        if (packet)
            delete [] packet;
    }

    FLVTag& operator=(const FLVTag& other)
    {
        size = other.size;
        packetSize = other.packetSize;
        type = other.type;

        if (packet)
            delete [] packet;
        if (other.packet) {
            packet = new unsigned char[other.packetSize];
            memcpy(packet, other.packet, other.packetSize);
        } else
            packet = NULL;

        if (packet)
            data = packet + 11;

        return *this;
    }

    FLVTag(const FLVTag& other)
    {
        size = other.size;
        packetSize = other.packetSize;
        type = other.type;

        if (other.packet) {
            packet = new unsigned char[other.packetSize];
            memcpy(packet, other.packet, other.packetSize);
        } else
            packet = NULL;

        if (packet)
            data = packet + 11;
    }

    void read(Stream &in)
    {
        if (packet != NULL)
            delete [] packet;

        unsigned char binary[11];
        in.read(binary, 11);

        type = static_cast<TYPE>(binary[0]);
        size = (binary[1] << 16) | (binary[2] << 8) | (binary[3]);
        //int timestamp = (binary[7] << 24) | (binary[4] << 16) | (binary[5] << 8) | (binary[6]);
        //int streamID = (binary[8] << 16) | (binary[9] << 8) | (binary[10]);

        packet = new unsigned char[11 + size + 4];
        memcpy(packet, binary, 11);
        in.read(packet + 11, size + 4);

        data = packet + 11;
        packetSize = 11 + size + 4;
    }

    const char *getTagType()
    {
        switch (type)
        {
        case T_SCRIPT:
            return "Script";
        case T_VIDEO:
            return "Video";
        case T_AUDIO:
            return "Audio";
        }
        return "Unknown";
    }

    int size;
    int packetSize;
    TYPE type;
    unsigned char *data;
    unsigned char *packet;
};

// ----------------------------------------------
class FLVTagBuffer
{
public:
    static const int MAX_DATALEN = 15 * 1024;

    FLVTagBuffer() : m_mem(MAX_DATALEN) {}

    bool put(FLVTag& tag, Channel* ch);
    void flush(Channel* ch);

    MemoryStream m_mem;

private:
    void sendImmediately(FLVTag& tag, Channel* ch);
};

// ----------------------------------------------
class FLVStream : public ChannelStream
{
public:
    int bitrate;
    FLVFileHeader fileHeader;
    FLVTag metaData;
    FLVTag aacHeader;
    FLVTag avcHeader;
    FLVStream() : bitrate(0)
    {
    }
    void readHeader(Stream &, Channel *) override;
    int  readPacket(Stream &, Channel *) override;
    void readEnd(Stream &, Channel *) override;

    FLVTagBuffer m_buffer;
};

class AMFObject
{
public:
    static const int AMF_NUMBER      = 0x00;
    static const int AMF_BOOL        = 0x01;
    static const int AMF_STRING      = 0x02;
    static const int AMF_OBJECT      = 0x03;
    static const int AMF_MOVIECLIP   = 0x04;
    static const int AMF_NULL        = 0x05;
    static const int AMF_UNDEFINED   = 0x06;
    static const int AMF_REFERENCE   = 0x07;
    static const int AMF_ARRAY       = 0x08;
    static const int AMF_OBJECT_END  = 0x09;
    static const int AMF_STRICTARRAY = 0x0a;
    static const int AMF_DATE        = 0x0b;
    static const int AMF_LONG_STRING = 0x0c;

    AMFObject() : bitrate(0) {}

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
            char* data = new char[len+1];
            *(data+len) = '\0';
            in.read(data, len);
            return data;
        }
    };

    double readDouble(Stream &in)
    {
        double number;
        char* data = reinterpret_cast<char*>(&number);
        for (int i = 8; i > 0; i--) {
            char c = in.readChar();
            *(data + i - 1) = c;
        }
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
            delete [] key;
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
            delete [] name;
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
            delete [] readString(in);
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
    int bitrate;
};

#endif
