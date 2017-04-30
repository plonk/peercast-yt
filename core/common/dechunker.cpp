#include <algorithm>
#include <iterator>
#include <memory>

#include "dechunker.h"

using namespace std;

int Dechunker::hexValue(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    else if (c >= 'a' && c <= 'f')
        return c - 'a' + 0xa;
    else if (c >= 'A' && c <= 'F')
        return c - 'A' + 0xA;
    else
        return -1;
}

// 読み込めた分だけ返してもいいのかなぁ…？read の仕様がわからない。
// MemoryStream だと要求されただけのデータがなかったら 0 を返して
// いるが、FileStream だと読めた分だけ読んでいる。
//
// きっちり size バイト読めるまでブロックして欲しい。UClientSocket
// はそういう作りになってるな。上流はClientSocketが本番環境だから、
// それでいこう。
int  Dechunker::read(void *buf, int size)
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

void Dechunker::getNextChunk()
{
    size_t size = 0;

    // チャンクサイズを読み込む。
    while (true)
    {
        char c = m_stream.readChar();
        if (c == '\r')
        {
            c = m_stream.readChar();
            if (c != '\n')
                throw StreamException("Protocol error");
            break;
        }
        int v = hexValue(c);
        if (v < 0)
            throw StreamException("Protocol error");
        size *= 0x10;
        size += v;
    }

    if (size == 0)
    {
        if (m_stream.readChar() != '\r' || m_stream.readChar() != '\n')
            throw StreamException("Protocol error");
        m_eof = true;
        throw StreamException("Closed on read");
    }

    unique_ptr<char> buf(new char[size]);
    int r = m_stream.read(buf.get(), size);
    if (r != size)
    {
        throw StreamException("Premature end");
    }
    copy(buf.get(), buf.get() + size, back_inserter(m_buffer));

    if (m_stream.readChar() != '\r' || m_stream.readChar() != '\n')
        throw StreamException("Protocol error");
}
