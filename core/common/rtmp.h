#ifndef _RTMP_H
#define _RTMP_H

#include <deque>
#include "stream.h"
#include "librtmp/rtmp.h"

class RTMPClientStream : public Stream
{
public:
    RTMPClientStream()
        : m_eof(false)
    {
        RTMP_Init(&m_rtmp);
    }

    ~RTMPClientStream()
    {
        RTMP_Close(&m_rtmp);
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
    RTMP m_rtmp;
    bool m_eof;
};

#endif
