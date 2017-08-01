#ifndef _SPLITTER_H
#define _SPLITTER_H

#include "stream.h"

// データストリームを複数のストリームに分岐する。close 時、あるいはデ
// コンストラクト時に出力先のストリームも close される。
struct StreamSplitter : public Stream
{
    StreamSplitter(const std::vector<Stream*>& streams = {})
        : Stream()
        , m_streams(streams) {}

    ~StreamSplitter()
    {
        close();
    }

    void addStream(Stream* stream)
    {
        m_streams.push_back(stream);
    }

    const std::vector<Stream*>&
    streams()
    {
        return m_streams;
    }

    int read(void *buf, int len) override
    {
        throw StreamException("Stream can`t read");
    }

    void write(const void *data, int len) override
    {
        for (auto& stream : m_streams)
            stream->write(data, len);
    }

    void close() override
    {
        for (auto& stream : m_streams)
            stream->close();
    }

    std::vector<Stream*> m_streams;
};

#endif
