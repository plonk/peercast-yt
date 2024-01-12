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

#include <stdexcept>
#include <stdint.h>
#include <string.h> // memcpy

#include "channel.h"

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

    int32_t getTimestamp() const
    {
        if (packetSize < 8)
            throw std::runtime_error("no timestamp data");

        return (packet[7] << 24) | (packet[4] << 16) | (packet[5] << 8) | (packet[6]);
    }

    void setTimestamp(int32_t timestamp)
    {
        if (packetSize < 8)
            throw std::runtime_error("no timestamp data");

        packet[7] = (timestamp >> 24) & 0xff;
        packet[4] = (timestamp >> 16) & 0xff;
        packet[5] = (timestamp >> 8) & 0xff;
        packet[6] = (timestamp >> 0) & 0xff;
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
        default:
            return "Unknown";
        }
    }

    bool isKeyFrame()
    {
        if (!data) return false;
        if (type != T_VIDEO) return false;

        uint8_t frameType = data[0] >> 4;
        return frameType == 1 || frameType == 4;  // key frame or generated key frame
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
    static const int MAX_OUTGOING_PACKET_SIZE = 15 * 1024;
    static const int FLUSH_THRESHOLD          =  4 * 1024;

    FLVTagBuffer()
        : m_mem(ChanPacket::MAX_DATALEN)
        , m_streamHasKeyFrames(false)
        , startTime(0)
    {}

    ~FLVTagBuffer()
    {
    }

    bool put(FLVTag& tag, std::shared_ptr<Channel> ch);
    void flush(std::shared_ptr<Channel> ch);
    void rateLimit(uint32_t timestamp);

    MemoryStream m_mem;
    bool m_streamHasKeyFrames;
    double startTime;

private:
    void sendImmediately(FLVTag& tag, std::shared_ptr<Channel> ch);
};

// ----------------------------------------------
class FLVStream : public ChannelStream
{
public:
    int metaBitrate;
    FLVFileHeader fileHeader;
    FLVTag metaData;
    FLVTag aacHeader;
    FLVTag avcHeader;
    FLVStream() : metaBitrate(0)
    {
    }
    void readHeader(Stream &, std::shared_ptr<Channel>) override;
    int  readPacket(Stream &, std::shared_ptr<Channel>) override;
    void readEnd(Stream &, std::shared_ptr<Channel>) override;

    static std::pair<bool,int> readMetaData(void* data, int size);

    FLVTagBuffer m_buffer;
};

#endif
