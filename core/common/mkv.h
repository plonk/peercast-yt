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
        , m_hasKeyFrame(false)
    {
    }
    void readHeader(Stream &, Channel *) override;
    int  readPacket(Stream &, Channel *) override;
    void readEnd(Stream &, Channel *) override;

    void sendPacket(ChanPacket::TYPE, const matroska::byte_string& data, bool continuation, Channel*);
    bool hasKeyFrame(const matroska::byte_string& cluster);
    void sendCluster(const matroska::byte_string& cluster, Channel* ch);
    void checkBitrate(Stream &in, Channel *ch);
    void readTracks(const std::string& data);

    int m_videoTrackNumber;
    bool m_hasKeyFrame;
};

#endif
