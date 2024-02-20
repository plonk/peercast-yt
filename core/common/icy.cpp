#include "icy.h"
#include "socket.h"

// ------------------------------------------------
void ICYSource::stream(std::shared_ptr<Channel> ch)
{
    std::shared_ptr<ChannelStream> source;
    try
    {
        if (!ch->sock)
            throw StreamException("ICY channel has no socket");

        ch->resetPlayTime();

        ch->setStatus(Channel::S_BROADCASTING);
        source = ch->createSource();
        ch->readStream(*ch->sock, source);
    }catch (StreamException &e)
    {
        LOG_ERROR("Channel aborted: %s", e.msg);
    }

    ch->setStatus(Channel::S_CLOSING);

    if (ch->sock)
    {
        ch->sock->close();
        ch->sock = nullptr;
    }
}
