#include "wmhttp.h"

extern ASFInfo parseASFHeader(Stream &in); // from mms.cpp

void WMHTTPStream::readEnd(Stream &, Channel *)
{
}

// ------------------------------------------
void WMHTTPStream::readHeader(Stream &, Channel *)
{
}

// ------------------------------------------
int WMHTTPStream::readPacket(Stream &in, Channel *ch)
{
    WMHTTPChunk c;

    c.read(in);

    if (c.type == "$E") // End of stream
    {
        return 1;
    }
    if (c.type != "$H" && c.type != "$D")
        return 0;

    ASFChunk chunk = c.getASFChunk(m_seqno++);

    switch (chunk.type)
    {
        case 0x4824:        // asf header
        {
            LOG_DEBUG("ASF header %d bytes", chunk.len);
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
            LOG_DEBUG("ASF data %d bytes", chunk.len);
            ChanPacket pack;

            MemoryStream mem(pack.data, sizeof(pack.data));

            chunk.write(mem);

            pack.type = ChanPacket::T_DATA;
            pack.len = mem.pos;
            pack.pos = ch->streamPos;

            ch->newPacket(pack);
            ch->streamPos += pack.len;

            break;
        }
        default:
            throw StreamException("Unknown ASF chunk");
    }
    return 0;
}
