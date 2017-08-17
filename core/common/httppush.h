#ifndef _HTTPPUSH_H
#define _HTTPPUSH_H

#include "channel.h"

// ------------------------------------------------
class HTTPPushSource : public ChannelSource
{
public:
    HTTPPushSource(bool isChunked)
        : m_sock(nullptr)
        , m_isChunked(isChunked)
    {
    }

    void stream(std::shared_ptr<Channel>) override;
    int getSourceRate() override;
    int getSourceRateAvg() override;

    ClientSocket* m_sock;
    bool m_isChunked;
};

#endif
