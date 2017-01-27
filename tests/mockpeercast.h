#ifndef MOCKPEERCAST_H
#define MOCKPEERCAST_H

#include "peercast.h"
#include "mocksys.h"

class MockPeercastInstance : public PeercastInstance
{
public:
    virtual Sys* createSys()
    {
        return new MockSys();
    }
};

class MockPeercastApplication : public PeercastApplication
{
public:
    virtual const char* APICALL getIniFilename()
    {
        return "";
    }

    virtual const char* APICALL getPath()
    {
        return "";
    }

    virtual const char* APICALL getClientTypeOS()
    {
        return PCX_OS_LINUX;
    }

    virtual void APICALL printLog(LogBuffer::TYPE t, const char* str)
    {
    }
};

#endif
