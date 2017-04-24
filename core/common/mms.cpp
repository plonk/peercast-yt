// ------------------------------------------------
// File : mms.cpp
// Date: 28-may-2003
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

#include "channel.h"
#include "mms.h"
#include "asf.h"

#include "str.h"

// ------------------------------------------
ASFInfo parseASFHeader(Stream &in);


// ------------------------------------------
void MMSStream::readEnd(Stream &, Channel *)
{
}

// ------------------------------------------
void MMSStream::readHeader(Stream &, Channel *)
{
}

static int toLength(char bits)
{
    if (bits == 0) return 0;
    if (bits == 1) return 1;
    if (bits == 2) return 2;
    if (bits == 3) return 4;
    else throw StreamException("error");
}

// ------------------------------------------
void MMSStream::processChunk(Stream &in, Channel *ch, ASFChunk& chunk)
{
    switch (chunk.type)
    {
        case 0x4824:        // asf header
        {
            m_inCleanPointStretch = false;

            MemoryStream mem(ch->headPack.data, sizeof(ch->headPack.data));

            chunk.write(mem);

            MemoryStream asfm(chunk.data, chunk.dataLen);
            ASFObject asfHead;
            asfHead.readHead(asfm);

            ASFInfo asf = parseASFHeader(asfm);
            LOG_DEBUG("ASF Info: pnum=%d, psize=%d, br=%d", asf.numPackets, asf.packetSize, asf.bitrate);
            for (int i=0; i<ASFInfo::MAX_STREAMS; i++)
            {
                ASFStream *s = &asf.streams[i];
                if (s->id)
                    LOG_DEBUG("ASF Stream %d : %s, br=%d", s->id, s->getTypeName(), s->bitrate);

                if (s->type == ASFStream::T_VIDEO)
                    m_videoStreamNumber = s->id;
            }

            ch->info.bitrate = asf.bitrate/1000;

            ch->headPack.type = ChanPacket::T_HEAD;
            ch->headPack.len = mem.pos;
            ch->headPack.pos = ch->streamPos;
            ch->newPacket(ch->headPack);

            ch->streamPos += ch->headPack.len;

            break;
        }
        case 0x4424:        // asf data
        {
            ChanPacket pack;

            MemoryStream mem(pack.data, sizeof(pack.data));

            chunk.write(mem);

            // LOG_DEBUG("data: %s", str::hexdump(std::string(pack.data, pack.data + std::min(25, mem.getPosition()))).c_str());
            //LOG_DEBUG("Stream number= %d", pack.data[15]);

            int streamNumberOffset = 12 + 9;

            char errorCorrectionFlags = pack.data[12];
            // LOG_DEBUG("error correction flags = %02hhX", errorCorrectionFlags);

            bool isMulti = false;

            if (errorCorrectionFlags & 0x80)
            {
                streamNumberOffset += (errorCorrectionFlags & 0xf);

                char lengthTypeFlags =
                    pack.data[12 + 1 + (errorCorrectionFlags & 0xf)];

                if (lengthTypeFlags & 1)
                {
                    isMulti = true;
                    streamNumberOffset += 1;
                }

                streamNumberOffset += toLength((lengthTypeFlags >> 1) & 0x3);
                streamNumberOffset += toLength((lengthTypeFlags >> 3) & 0x3);
                streamNumberOffset += toLength((lengthTypeFlags >> 5) & 0x3);
            }
            int streamNumber = (uint8_t) pack.data[streamNumberOffset];

            bool cleanPoint = ((streamNumber & 0x80) != 0);
            bool videoStream = ((streamNumber & 0x7f) == m_videoStreamNumber);

            pack.type = ChanPacket::T_DATA;
            pack.len = mem.pos;
            pack.pos = ch->streamPos;
            if (videoStream)
            {
                if (m_inCleanPointStretch)
                {
                    pack.cont = true;
                    if (!cleanPoint)
                        m_inCleanPointStretch = false;
                }else
                {
                    if (cleanPoint)
                    {
                        pack.cont = false;
                        m_inCleanPointStretch = true;
                    }else
                        pack.cont = true;
                }
            }else
            {
                pack.cont = true;
            }

            LOG_DEBUG("STREAM NUMBER = %02hhX\tCONT = %d\t%s", streamNumber, pack.cont, isMulti? "MULTI": "SINGLE");

            ch->newPacket(pack);
            ch->streamPos += pack.len;
            break;
        }
        default:
            throw StreamException("Unknown ASF chunk");
    }
}

// ------------------------------------------
int MMSStream::readPacket(Stream &in, Channel *ch)
{
    ASFChunk chunk;

    chunk.read(in);

    processChunk(in, ch, chunk);
    return 0;
}

// -----------------------------------
ASFInfo parseASFHeader(Stream &in)
{
    ASFInfo asf;

    try
    {
        int numHeaders = in.readLong();

        in.readChar();
        in.readChar();

        LOG_CHANNEL("ASF Headers: %d", numHeaders);
        for (int i=0; i<numHeaders; i++)
        {

            ASFObject obj;

            unsigned int l = obj.readHead(in);
            obj.readData(in, l);


            MemoryStream data(obj.data, obj.lenLo);


            switch (obj.type)
            {
                case ASFObject::T_FILE_PROP:
                {
                    data.skip(32);

                    unsigned int dpLo = data.readLong();
                    unsigned int dpHi = data.readLong();

                    data.skip(24);

                    data.readLong();
                    //data.writeLong(1);    // flags = broadcast, not seekable

                    int min = data.readLong();
                    int max = data.readLong();
                    int br = data.readLong();

                    if (min != max)
                        throw StreamException("ASF packetsizes (min/max) must match");

                    asf.packetSize = max;
                    asf.bitrate = br;
                    asf.numPackets = dpLo;
                    break;
                }
                case ASFObject::T_STREAM_BITRATE:
                {
                    int cnt = data.readShort();
                    for (int i=0; i<cnt; i++)
                    {
                        unsigned int id = data.readShort();
                        int bitrate = data.readLong();
                        if (id < ASFInfo::MAX_STREAMS)
                            asf.streams[id].bitrate = bitrate;
                    }
                    break;
                }
                case ASFObject::T_STREAM_PROP:
                {
                    ASFStream s;
                    s.read(data);
                    asf.streams[s.id].id = s.id;
                    asf.streams[s.id].type = s.type;
                    break;
                }
            }

        }
    }catch (StreamException &e)
    {
        LOG_ERROR("ASF: %s", e.msg);
    }

    return asf;
}


