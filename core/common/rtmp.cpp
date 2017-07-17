#ifdef WITH_RTMP

#include <iterator>

#include "librtmp/rtmp.h"

#include "rtmp.h"

#define MAX_PACKET_SIZE (1024*1024)

void RTMPClientStream::open(const std::string& url)
{
    if (RTMP_SetupURL(&m_rtmp, (char*)url.c_str()) == false)
        throw StreamException("RTMP_SetupURL");

    if (RTMP_Connect(&m_rtmp, NULL) == false)
        throw StreamException("RTMP_Connect");
}

int  RTMPClientStream::read(void *buffer, int len)
{
    if (len == 0) return 0;

    char *p = (char*)buffer;
    if (len <= static_cast<int>(m_buffer.size()))
    {
        std::copy(m_buffer.begin(), m_buffer.begin() + len, stdext::checked_array_iterator<char *>(p, len));
        m_buffer.erase(m_buffer.begin(), m_buffer.begin() + len);
        return len;
    }else
    {
        getNextPacket();
        return read(buffer, len);
    }
}

void RTMPClientStream::getNextPacket()
{
    char *buffer = new char[MAX_PACKET_SIZE];
    int nsize = RTMP_Read(&m_rtmp, buffer, MAX_PACKET_SIZE);

    if (nsize == 0)
    {
        m_eof = true;
        throw StreamException("End of stream");
    }

    std::copy(buffer, buffer + nsize, std::back_inserter(m_buffer));
    updateTotals(nsize, 0);
    delete[] buffer;
}

#endif
