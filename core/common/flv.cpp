// ------------------------------------------------
// File : flv.cpp
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

#include "channel.h"
#include "flv.h"
#include "string.h"
#include "stdio.h"
#ifdef _DEBUG
#include "chkMemoryLeak.h"
#define DEBUG_NEW new(__FILE__, __LINE__)
#define new DEBUG_NEW
#endif

static String timestampToString(uint32_t timestamp)
{
    return String::format("%d:%02d:%02d.%03d",
                          timestamp / 1000 / 3600,
                          timestamp / 1000 / 60 % 60,
                          timestamp / 1000 % 60,
                          timestamp % 1000);
}

// ------------------------------------------
void FLVStream::readEnd(Stream &, Channel *)
{
}

// ------------------------------------------
void FLVStream::readHeader(Stream &in, Channel *ch)
{
    metaBitrate = 0;
    fileHeader.read(in);
}

// ------------------------------------------
int FLVStream::readPacket(Stream &in, Channel *ch)
{
    bool headerUpdate = false;

    FLVTag flvTag;
    flvTag.read(in);

    // LOG_DEBUG("%s: %s: %d byte %s tag",
    //           static_cast<std::string>(ch->info.id).c_str(),
    //           timestampToString(flvTag.getTimestamp()).cstr(),
    //           flvTag.packetSize,
    //           flvTag.getTagType());

    switch (flvTag.type)
    {
    case FLVTag::T_SCRIPT:
        {
            AMFObject amf;
            MemoryStream flvmem(flvTag.data, flvTag.size);
            if (amf.readMetaData(flvmem)) {
                flvmem.close();
                metaBitrate = amf.bitrate;
                metaData = flvTag;
                headerUpdate = true;
            }
        }
        break;
    case FLVTag::T_VIDEO:
        // AVC Header
        if (flvTag.data[0] == 0x17 && flvTag.data[1] == 0x00 &&
            flvTag.data[2] == 0x00 && flvTag.data[3] == 0x00) {
            avcHeader = flvTag;
            if (avcHeader.getTimestamp() != 0)
            {
                LOG_CHANNEL("AVC header has non-zero timestamp. Cleared to zero.");
                avcHeader.setTimestamp(0);
            }
            headerUpdate = true;
        }
        break;
    case FLVTag::T_AUDIO:
        // AAC Header
        if (flvTag.data[0] == 0xaf && flvTag.data[1] == 0x00) {
            aacHeader = flvTag;
            if (aacHeader.getTimestamp() != 0)
            {
                LOG_CHANNEL("AAC header has non-zero timestamp. Cleared to zero.");
                aacHeader.setTimestamp(0);
            }
            headerUpdate = true;
        }
        break;
    default:
        LOG_ERROR("Invalid FLV tag!");
    }

    // メタ情報からのビットレートが無い場合、ストリームからの実測値が
    // 現在の公称値を超えていれば公称値を更新する。
    if (metaBitrate == 0) {
        ChanInfo info = ch->info;

        int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
        if (newBitrate > info.bitrate) {
            info.bitrate = newBitrate;
            ch->updateInfo(info);
        }
    }

    if (headerUpdate && fileHeader.size>0) {
        int len = fileHeader.size;
        if (metaData.type == FLVTag::T_SCRIPT) len += metaData.packetSize;
        if (avcHeader.type == FLVTag::T_VIDEO) len += avcHeader.packetSize;
        if (aacHeader.type == FLVTag::T_AUDIO) len += aacHeader.packetSize;
        MemoryStream mem(ch->headPack.data, len);
        mem.write(fileHeader.data, fileHeader.size);
        if (metaData.type == FLVTag::T_SCRIPT) mem.write(metaData.packet, metaData.packetSize);
        if (avcHeader.type == FLVTag::T_VIDEO) mem.write(avcHeader.packet, avcHeader.packetSize);
        if (aacHeader.type == FLVTag::T_AUDIO) mem.write(aacHeader.packet, aacHeader.packetSize);

        // メタ情報からのビットレートがあればその値を設定。無ければ、
        // 前回のエンコードセッションからの値をクリアするために 0 を設
        // 定する。
        ChanInfo info = ch->info;
        info.bitrate = metaBitrate;
        ch->updateInfo(info);

        m_buffer.flush(ch);

        ch->headPack.type = ChanPacket::T_HEAD;
        ch->headPack.len = mem.pos;
        ch->headPack.pos = ch->streamPos;
        ch->newPacket(ch->headPack);

        ch->streamPos += ch->headPack.len;
    }
    else {
        bool packet_sent;

        packet_sent = m_buffer.put(flvTag, ch);

        if (!packet_sent && in.readReady())
            return readPacket(in, ch);
    }

    return 0;
}

bool FLVTagBuffer::put(FLVTag& tag, Channel* ch)
{
    if (tag.isKeyFrame())
    {
        if (m_mem.pos > 0)
        {
            flush(ch);
        }
        sendImmediately(tag, ch);
        return true;
    } else if (m_mem.pos + tag.packetSize > MAX_OUTGOING_PACKET_SIZE)
    {
        if (m_mem.pos > 0)
        {
            flush(ch);
        }
        sendImmediately(tag, ch);
        return true;
    } else if (m_mem.pos + tag.packetSize > FLUSH_THRESHOLD)
    {
        flush(ch);

        m_mem.write(tag.packet, tag.packetSize);

        if (m_mem.pos > FLUSH_THRESHOLD)
            flush(ch);
        return true;
    } else
    {
        m_mem.write(tag.packet, tag.packetSize);
        return false;
    }
}

void FLVTagBuffer::sendImmediately(FLVTag& tag, Channel* ch)
{
    ChanPacket pack;
    MemoryStream mem(tag.packet, tag.packetSize);

    int rlen = tag.packetSize;
    bool firstTime = true;
    while (rlen)
    {
        int rl = rlen;
        if (rl > MAX_OUTGOING_PACKET_SIZE)
        {
            rl = MAX_OUTGOING_PACKET_SIZE;
        }

        pack.type = ChanPacket::T_DATA;
        pack.pos = ch->streamPos;
        pack.len = rl;
        if (firstTime && tag.isKeyFrame())
            pack.cont = false;
        else
            pack.cont = true;

        mem.read(pack.data, pack.len);

        ch->newPacket(pack);
        ch->checkReadDelay(pack.len);
        ch->streamPos += pack.len;

        rlen -= rl;
        firstTime = false;
    }
}

void FLVTagBuffer::flush(Channel* ch)
{
    if (m_mem.pos == 0)
        return;

    int length = m_mem.pos;

    m_mem.rewind();

    ChanPacket pack;

    pack.type = ChanPacket::T_DATA;
    pack.pos = ch->streamPos;
    pack.len = length;
    pack.cont = true;
    m_mem.read(pack.data, length);

    ch->newPacket(pack);
    ch->checkReadDelay(pack.len);
    ch->streamPos += pack.len;

    m_mem.rewind();
}
