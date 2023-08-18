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
#include "amf0.h"
#include <math.h> // ceil

// static String timestampToString(uint32_t timestamp)
// {
//     return String::format("%d:%02d:%02d.%03d",
//                           timestamp / 1000 / 3600,
//                           timestamp / 1000 / 60 % 60,
//                           timestamp / 1000 % 60,
//                           timestamp % 1000);
// }

// ------------------------------------------
void FLVStream::readEnd(Stream &, std::shared_ptr<Channel>)
{
}

// ------------------------------------------
void FLVStream::readHeader(Stream &in, std::shared_ptr<Channel> ch)
{
    metaBitrate = 0;
    fileHeader.read(in);
    m_buffer.startTime = sys->getDTime();
}

// ----------------------------------------------------------
std::pair<bool,int> FLVStream::readMetaData(void* data, int size)
{
    // スクリプトタグのペイロード data が onMetaData ならば、
    // videodatarate / audiodatarate プロパティから総ビットレートを計
    // 算して (true, bitrate) を返す。onMetaData でなかった場合や、ど
    // ちらのプロパティも存在しなかった場合は、(false, _) を返す。

    try {
        MemoryStream flvmem(data, size);
        amf0::Deserializer deserializer;
        double bitrate = 0;
        auto cmd = deserializer.readValue(flvmem);
        if (cmd.string() == "onMetaData")
        {
            auto amfValue = deserializer.readValue(flvmem);
            auto object = amfValue.object();

            LOG_DEBUG("onMetaData %s", amfValue.inspect().c_str());
            if (object.count("videodatarate"))
            {
                bitrate += object["videodatarate"].number();
            }
            if (object.count("audiodatarate"))
            {
                bitrate += object["audiodatarate"].number();
            }

            if (bitrate > 0)
                return std::make_pair(true, ceil(bitrate));
        }
        return std::make_pair(false, 0);
    } catch (std::runtime_error& e) {
        LOG_ERROR("readMetaData: %s", e.what());
        return std::make_pair(false, 0);
    }
}

// ------------------------------------------
int FLVStream::readPacket(Stream &in, std::shared_ptr<Channel> ch)
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
            bool success;
            int bitrate;

            std::tie(success, bitrate) = readMetaData(flvTag.data, flvTag.size);
            if (success)
            {
                metaBitrate = bitrate;
                metaData = flvTag;
                headerUpdate = true;
            }
        }
        break;
    case FLVTag::T_VIDEO:
        // AVC Header
        if (avcHeader.type == FLVTag::T_UNKNOWN) {
            LOG_DEBUG("Got AVC header");
            avcHeader = flvTag;
            if (avcHeader.getTimestamp() != 0)
            {
                LOG_INFO("AVC header has non-zero timestamp. Cleared to zero.");
                avcHeader.setTimestamp(0);
            }
            headerUpdate = true;
        }
        break;
    case FLVTag::T_AUDIO:
        // AAC Header
        if (aacHeader.type == FLVTag::T_UNKNOWN) {
            LOG_DEBUG("Got AAC header");
            aacHeader = flvTag;
            if (aacHeader.getTimestamp() != 0)
            {
                LOG_INFO("AAC header has non-zero timestamp. Cleared to zero.");
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

        ch->rawData.init();
        ch->streamIndex++;

        ch->headPack.type = ChanPacket::T_HEAD;
        ch->headPack.len = mem.pos;
        ch->headPack.pos = 0;
        ch->newPacket(ch->headPack);

        ch->streamPos = 0 + ch->headPack.len;
    }
    else {
        bool packet_sent;

        packet_sent = m_buffer.put(flvTag, ch);

        if (!packet_sent && in.readReady())
            return readPacket(in, ch);
    }

    return 0;
}

bool FLVTagBuffer::put(FLVTag& tag, std::shared_ptr<Channel> ch)
{
    if (tag.isKeyFrame())
    {
        m_streamHasKeyFrames = true;

        if (m_mem.pos > 0)
        {
            flush(ch);
        }
        sendImmediately(tag, ch);
        return true;
    } else if (m_mem.pos + tag.packetSize > MAX_OUTGOING_PACKET_SIZE)
    {
        flush(ch);
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

void FLVTagBuffer::rateLimit(uint32_t timestamp)
{
    double diff = (startTime + timestamp/1000.0) - sys->getDTime();
    if (diff > 10)
    {
        // 10秒は長すぎるので、タイムスタンプがジャンプしてるっぽい。
        // 基準時刻をリセット。
        LOG_DEBUG("Timestamp way into the future. Resetting referece point.");
        startTime = sys->getDTime();
    }else if (diff < -10)
    {
        LOG_DEBUG("Timestamp way back in the past. Resetting referece point.");
        startTime = sys->getDTime();
    }else if (diff > 0)
    {
        LOG_TRACE("Sleeping %.2f s", diff);
        sys->sleep(diff * 1000);
    }
}

void FLVTagBuffer::sendImmediately(FLVTag& tag, std::shared_ptr<Channel> ch)
{
    if (ch->readDelay)
        rateLimit(tag.getTimestamp());

    ChanPacket pack;
    MemoryStream mem(tag.packet, tag.packetSize);

    int rlen = tag.packetSize;
    bool firstTimeRound = true;
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
        if (m_streamHasKeyFrames)
        {
            if (firstTimeRound && tag.isKeyFrame())
                pack.cont = false;
            else
                pack.cont = true;
        }
        mem.read(pack.data, pack.len);

        ch->newPacket(pack);
        //ch->checkReadDelay(pack.len);
        ch->streamPos += pack.len;

        rlen -= rl;
        firstTimeRound = false;
    }
}

void FLVTagBuffer::flush(std::shared_ptr<Channel> ch)
{
    if (m_mem.pos == 0)
        return;

    int length = m_mem.pos;

    m_mem.rewind();
    {
        FLVTag flvTag;
        flvTag.read(m_mem);
        if (ch->readDelay)
            rateLimit(flvTag.getTimestamp());
        m_mem.rewind();
    }

    ChanPacket pack;

    pack.type = ChanPacket::T_DATA;
    pack.pos = ch->streamPos;
    pack.len = length;
    // キーフレームでないタグだけがバッファリングされる。
    if (m_streamHasKeyFrames)
        pack.cont = true;
    m_mem.read(pack.data, length);

    ch->newPacket(pack);
    //ch->checkReadDelay(pack.len);
    ch->streamPos += pack.len;

    m_mem.rewind();
}
