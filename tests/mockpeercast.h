#ifndef MOCKPEERCAST_H
#define MOCKPEERCAST_H

#include "peercast.h"
#include "mocksys.h"
#include "gnutella.h"

class MockPeercastInstance : public PeercastInstance
{
public:
    Sys* APICALL createSys() override
    {
        return new MockSys();
    }
};

class MockPeercastApplication : public PeercastApplication
{
public:
    const char* APICALL getIniFilename() override
    {
        return "";
    }

    const char* APICALL getPath() override
    {
        return "";
    }

    const char* APICALL getClientTypeOS() override
    {
        return PCX_OS_LINUX;
    }

    void APICALL printLog(LogBuffer::TYPE t, const char* str) override
    {
    }
};

#endif
