#ifndef _CHUNKER_H
#define _CHUNKER_H

#include "stream.h"

// HTTP chunked transfer encoding にエンコードするクラス。
class Chunker : public Stream
{
public:
    Chunker(Stream& aStream)
        : m_stream(aStream)
    {
    }

    ~Chunker()
    {
    }

    int read(void *buf, int size) override
    {
        throw StreamException("Stream can`t read");
    }

    void write(const void *buf, int size) override
    {
        if (size == 0) // Cannot send zero sized data.
            return;
        m_stream.writeStringF("%X\r\n", size);
        m_stream.write(buf, size);
        m_stream.writeStringF("\r\n");
    }

    void close() override
    {
        m_stream.writeStringF("0\r\n\r\n");
    }

    Stream&          m_stream;
};

#endif
