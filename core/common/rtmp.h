#ifndef _RTMP_H
#define _RTMP_H

#include <deque>
#include "stream.h"
#include "librtmp/rtmp.h"

class RTMPClientStream : public Stream
{
public:
    RTMPClientStream()
        : m_packetData(new char[kMaxPacketSize])
        , m_eof(false)
    {
        RTMP_Init(&m_rtmp);
    }

    ~RTMPClientStream()
    {
        RTMP_Close(&m_rtmp);
        delete[] m_packetData;
    }

    void open(const std::string&);

    int  read(void *, int) override;

    void write(const void *, int) override
    {
        throw StreamException("Stream can`t write");
    }

    bool eof() override
    {
        return m_eof;
    }

    void getNextPacket();

    std::deque<char> m_buffer;
    char *m_packetData;
    RTMP m_rtmp;
    bool m_eof;
    static const size_t kMaxPacketSize = 1024*1024;
};

#endif
