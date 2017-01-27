#include <gtest/gtest.h>
//#include "channel.h"
#include "servmgr.h"
#include "mockpeercast.h"

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);

    // sys = new MockSys();
    // servMgr = new ServMgr();
    // chanMgr = new ChanMgr();

    peercastApp = new MockPeercastApplication();
    peercastInst = new MockPeercastInstance();
    peercastInst->init();

    int status = RUN_ALL_TESTS();

    // delete chanMgr;
    // delete servMgr;
    // delete sys;

    return status;
}
