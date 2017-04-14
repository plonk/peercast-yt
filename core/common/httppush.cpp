#include "socket.h"
#include "httppush.h"
#include "dechunker.h"

// ------------------------------------------------
void HTTPPushSource::stream(Channel *ch)
{
    ChannelStream *source=NULL;

    try
    {
        if (!ch->sock)
            throw StreamException("HTTP Push channel has no socket");

        ch->resetPlayTime();

        ch->setStatus(Channel::S_BROADCASTING);
        source = ch->createSource();

        Dechunker dechunker(*(Stream*)ch->sock);
        ch->readStream(dechunker, source);
    }catch (StreamException &e)
    {
        LOG_ERROR("Channel aborted: %s", e.msg);
    }

    ch->setStatus(Channel::S_CLOSING);

    if (ch->sock)
    {
        ch->sock->close();
        delete ch->sock;
        ch->sock = NULL;
    }

    if (source)
        delete source;
}
