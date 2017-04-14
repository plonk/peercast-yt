#ifndef _HTTPPUSH_H
#define _HTTPPUSH_H

#include "channel.h"

// ------------------------------------------------
class HTTPPushSource : public ChannelSource
{
public:
    void stream(Channel *) override;
};

#endif
