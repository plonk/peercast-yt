#ifndef _WMHTTP_H
#define _WMHTTP_H

#include <stdint.h>

#include "asf.h"
#include "mms.h"
#include "stream.h"
#include "channel.h"

class WMHTTPChunk
{
public:

    void read(Stream &in)
    {
        uint16_t t = in.readShort();;
        type = { (char) (t & 0xff), (char) (t >> 8) };
        len  = in.readShort();

        if (len > sizeof(data))
            throw StreamException("WMHTTP chunk too big");
        in.read(data, len);
    }

    ASFChunk getASFChunk(uint32_t seqno)
    {
        ASFChunk chunk;

        chunk.type = type[0] | (type[1] << 8);
        chunk.len = len + 8;
        chunk.seq = seqno;
        if (type == "$H")
            chunk.v1 = 0x0c00;
        else
            chunk.v1 = 0x0000;
        chunk.v2 = len + 8;
        memcpy(chunk.data, data, len);
        chunk.dataLen = len;

        return chunk;
    }

    std::string type;
    uint16_t len;
    unsigned char   data[8192];
};

class WMHTTPStream : public ChannelStream
{
public:
    WMHTTPStream()
        : m_seqno(0)
    {}

    void    readHeader(Stream &, std::shared_ptr<Channel>) override;
    int     readPacket(Stream &, std::shared_ptr<Channel>) override;
    void    readEnd(Stream &, std::shared_ptr<Channel>) override;

    uint32_t m_seqno;
};

#endif
