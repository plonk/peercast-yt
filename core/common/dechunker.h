#ifndef _DECHUNKER_H
#define _DECHUNKER_H

#include <deque>
#include "stream.h"

// HTTP chunked encoding のストリームをデコードするアダプタクラス。
class Dechunker : public Stream
{
public:
    Dechunker(Stream& aStream)
        : m_eof(false)
        , m_stream(aStream)
    {
    }

    ~Dechunker()
    {
    }

    int  read(void *buf, int size) override;

    void write(const void *buf, int size) override
    {
        throw StreamException("Stream can`t write");
    }

    bool eof() override
    {
        return m_eof;
    }

    static int hexValue(char c);
    void       getNextChunk();

    bool             m_eof;
    std::deque<char> m_buffer; // 後ろから入れて前から出す
    Stream&          m_stream;
};

#endif
