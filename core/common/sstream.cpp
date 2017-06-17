#include "sstream.h"
#include <algorithm>
#include <string>

StringStream::StringStream()
    : m_pos(0)
{
}

StringStream::StringStream(const std::string& content)
    : StringStream()
{
    str(content);
}

void  StringStream::checkSize(size_t size)
{
    if (size > m_buffer.size())
        while (m_buffer.size() < size)
            m_buffer.push_back(0);
}

int  StringStream::read(void *buf, int count)
{
    if (m_pos == m_buffer.size())
        throw StreamException("End of stream");

    int bytesRead = (std::min)(count, static_cast<int>(m_buffer.size() - m_pos));
    memcpy(buf, m_buffer.c_str() + m_pos, bytesRead);
    m_pos += bytesRead;
    return bytesRead;
}

void StringStream::write(const void *buf, int count)
{
    if (count < 0)
        throw GeneralException("Bad argument");

    checkSize(m_pos + count);
    std::copy(static_cast<const char*>(buf),
              static_cast<const char*>(buf) + count,
              m_buffer.begin() + m_pos);
    m_pos += count;
}

bool StringStream::eof()
{
    return m_pos == m_buffer.size();
}

void StringStream::rewind()
{
    m_pos = 0;
}

void StringStream::seekTo(int newPos)
{
    checkSize(newPos);
    m_pos = newPos;
}

int  StringStream::getPosition()
{
    return static_cast<int>(m_pos);
}

int  StringStream::getLength()
{
    return static_cast<int>(m_buffer.size());
}

std::string StringStream::str()
{
    return std::string(m_buffer.begin(), m_buffer.end());
}

void StringStream::str(const std::string& data)
{
    m_buffer = data;
    m_pos = 0;
}
