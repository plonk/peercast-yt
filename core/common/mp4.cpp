// ------------------------------------------------
// File : mp4.cpp
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
#include "mp4.h"
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

static std::shared_ptr<MP4Box> readBox(Stream &in)
{
    LOG_DEBUG("readBox");
    uint8_t s[4];
    in.read(s, 4);
    size_t size = s[0] << 24 | s[1] << 16 | s[2] << 8 | s[3];
    LOG_DEBUG("size: %d", (int) size);
    uint8_t *data = new uint8_t[size];
    memcpy(data, s, 4);
    in.read(data + 4, size - 4);
    auto box = std::make_shared<MP4Box>();
    box->data(data); // move
    data = nullptr;
    LOG_TRACE("Box type %s", box->type().c_str());
    LOG_DEBUG("readBox End");
    return box;
}

// ------------------------------------------
void MP4Stream::readEnd(Stream &, std::shared_ptr<Channel>)
{
    LOG_DEBUG("MP4Stream::readEnd");
}

// ------------------------------------------
void MP4Stream::readHeader(Stream &in, std::shared_ptr<Channel> ch)
{
    LOG_DEBUG("MP4Stream::readHeader");
    using namespace std;

    // read ftyp box
    shared_ptr<MP4Box> ftyp = readBox(in);
    if (ftyp->type() != "ftyp") {
        throw StreamException("ftyp expected");
    }
    LOG_DEBUG("Got ftyp: %d bytes", (int) ftyp->size());

    // read moov box
    shared_ptr<MP4Box> moov = readBox(in);
    if (moov->type() != "moov") {
        throw StreamException("moov expected");
    }
    LOG_DEBUG("Got moov: %d bytes", (int) moov->size());

    if (ftyp->size() + moov->size() > ChanPacket::MAX_DATALEN) {
        throw StreamException("head packet too large");
    }

    MemoryStream mem(ch->headPack.data, ftyp->size() + moov->size());
    mem.write(ftyp->data(), ftyp->size());
    mem.write(moov->data(), moov->size());

    ch->rawData.init();
    ch->streamIndex++;

    ch->headPack.type = ChanPacket::T_HEAD;
    ch->headPack.len = mem.pos;
    ch->headPack.pos = 0;
    ch->newPacket(ch->headPack);

    ch->streamPos = ch->headPack.len;
}

// ------------------------------------------
int MP4Stream::readPacket(Stream &in, std::shared_ptr<Channel> ch)
{
    LOG_DEBUG("MP4Stream::readPacket");

    using namespace std;
    shared_ptr<MP4Box> moof = readBox(in);

    if (moof->type() == "moof") {
        shared_ptr<MP4Box> mdat = readBox(in);
        if (mdat->type() != "mdat") {
            throw StreamException("mdat expected");
        }

        memcpy(m_buffer, moof->data(), moof->size());
        memcpy(m_buffer + moof->size(), mdat->data(), mdat->size());

        MemoryStream mem(m_buffer, moof->size() + mdat->size());
        ChanPacket pack;
        int rlen = moof->size() + mdat->size();
        bool firstTime = true;

        while (rlen)
        {
             int rl = (rlen > MAX_OUTGOING_PACKET_SIZE) ?
                  MAX_OUTGOING_PACKET_SIZE :
                  rlen;
             pack.type = ChanPacket::T_DATA;
             pack.pos = ch->streamPos;
             pack.len = rl;
             pack.cont = !firstTime;
             mem.read(pack.data, rl);

             // copy to channel buffer
             LOG_TRACE("newPacket");
             ch->newPacket(pack);
             ch->streamPos += pack.len;

             rlen -= rl;
             firstTime = false;
        }
    } else {
         throw StreamException("moof expected");
    }

    // ストリームからの実測値が現在の公称値を超えていれば公称値を更新する。
    int newBitrate = in.stat.bytesInPerSecAvg() / 1000 * 8;
    if (newBitrate > ch->info.bitrate) {
        ChanInfo info = ch->info;
        info.bitrate = newBitrate;
        ch->updateInfo(info);
    }

    return 0;
}
