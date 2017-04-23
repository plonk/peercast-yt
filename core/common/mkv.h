#ifndef _MKV_H
#define _MKV_H

#include "stream.h"
#include "channel.h"
#include "matroska.h"

// ----------------------------------------------
class MKVStream : public ChannelStream
{
public:
    MKVStream()
        : m_videoTrackNumber(1)
    {
    }
    void readHeader(Stream &, Channel *) override;
    int  readPacket(Stream &, Channel *) override;
    void readEnd(Stream &, Channel *) override;

    void sendPacket(ChanPacket::TYPE, const matroska::byte_string& data, bool continuation, Channel*);
    bool hasKeyFrame(const matroska::byte_string& cluster);
    void sendCluster(const matroska::byte_string& cluster, Channel* ch);
    void checkBitrate(Stream &in, Channel *ch);

    int m_videoTrackNumber;
};

#endif
