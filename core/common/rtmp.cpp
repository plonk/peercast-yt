#ifdef WITH_RTMP

#include <iterator>

#include "librtmp/rtmp.h"

#include "rtmp.h"

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

    int rem = len;
    while (rem > 0)
    {
        int nsize = RTMP_Read(&m_rtmp, (char*) buffer, rem);
        if (nsize == 0)
        {
            m_eof = true;
            throw StreamException("End of stream");
        }

        updateTotals(nsize, 0);
        rem -= nsize;
        buffer = (char*) buffer + nsize;
    }
    return len;
}

#endif
