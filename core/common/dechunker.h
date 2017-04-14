#ifndef _DECHUNKER_H
#define _DECHUNKER_H

#include <deque>
#include "stream.h"

// HTTP chunked encoding のストリームをデコードするアダプタクラス。
class Dechunker : public Stream
{
public:
    Dechunker(Stream& aStream)
        : m_stream(aStream)
        , m_eof(false)
    {
    }

    ~Dechunker()
    {
    }

    static int hexValue(char c);

    // 読み込めた分だけ返してもいいのかなぁ…？read の仕様がわからない。
    // MemoryStream だと要求されただけのデータがなかったら 0 を返して
    // いるが、FileStream だと読めた分だけ読んでいる。
    //
    // きっちり size バイト読めるまでブロックして欲しい。UClientSocket
    // はそういう作りになってるな。上流はClientSocketが本番環境だから、
    // それでいこう。
    int  read(void *buf, int size) override
    {
        if (m_eof)
            throw StreamException("Closed on read");

        char *p = (char*) buf;

        while (true)
        {
            if (m_buffer.size() >= size)
            {
                while (size > 0)
                {
                    *p++ = m_buffer.front();
                    m_buffer.pop_front();
                    size--;
                }
                int r = p - (char*)buf;
                updateTotals(r, 0);
                return r;
            } else {
                getNextChunk();
                continue;
            }
        }
    }

    // チャンク全部読むまで read が返らなくていいのかな…
    void getNextChunk();

    void write(const void *buf, int size) override
    {
        throw StreamException("Stream can`t write");
    }

    bool eof() override
    {
        return m_eof;
    }

    bool m_eof;
    std::deque<char> m_buffer; // 後ろから入れて前から出す
    Stream& m_stream;
};

#endif
