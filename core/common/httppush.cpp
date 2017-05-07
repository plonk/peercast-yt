#include <memory>

#include "socket.h"
#include "httppush.h"
#include "dechunker.h"

// ------------------------------------------------
void HTTPPushSource::stream(Channel *ch)
{
    try
    {
        if (!ch->sock)
            throw StreamException("HTTP Push channel has no socket");
        m_sock = ch->sock;

        ch->resetPlayTime();

        ch->setStatus(Channel::S_BROADCASTING);

        std::unique_ptr<ChannelStream> source(ch->createSource());

        if (m_isChunked)
        {
            Dechunker dechunker(*(Stream*)ch->sock);
            ch->readStream(dechunker, source.get());
        }
        else
        {
            ch->readStream(*ch->sock, source.get());
        }
    }catch (StreamException &e)
    {
        LOG_ERROR("Channel aborted: %s", e.msg);
    }

    ch->setStatus(Channel::S_CLOSING);

    if (ch->sock)
    {
        m_sock = NULL;
        ch->sock->close();
        delete ch->sock;
        ch->sock = NULL;
    }
}

int HTTPPushSource::getSourceRate()
{
    if (m_sock != nullptr)
        return m_sock->bytesInPerSec();
    else
        return 0;
}

int HTTPPushSource::getSourceRateAvg()
{
    if (m_sock != nullptr)
        return m_sock->stat.bytesInPerSecAvg();
    else
        return 0;
}

