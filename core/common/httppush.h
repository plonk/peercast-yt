#ifndef _HTTPPUSH_H
#define _HTTPPUSH_H

#include "channel.h"

// ------------------------------------------------
class HTTPPushSource : public ChannelSource
{
public:
    HTTPPushSource()
        : m_sock(nullptr) {}

    void stream(Channel *) override;
    int getSourceRate() override;
    int getSourceRateAvg() override;

    ClientSocket* m_sock;
};

#endif
