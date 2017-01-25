#include "icy.h"
#include "socket.h"

// ------------------------------------------------
void ICYSource::stream(Channel *ch)
{
	ChannelStream *source=NULL;
	try 
	{

		if (!ch->sock)
			throw StreamException("ICY channel has no socket");

		ch->resetPlayTime();

		ch->setStatus(Channel::S_BROADCASTING);
		source = ch->createSource();
		ch->readStream(*ch->sock,source);

	}catch(StreamException &e)
	{
		LOG_ERROR("Channel aborted: %s",e.msg);
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
