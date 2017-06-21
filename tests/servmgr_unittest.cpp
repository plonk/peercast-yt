#include <gtest/gtest.h>

#include "servmgr.h"

class ServMgrFixture : public ::testing::Test {
public:
    ServMgr m;
};

TEST_F(ServMgrFixture, initialState)
{
    // Servent             *servents;
    ASSERT_EQ(nullptr, m.servents);
    // WLock               lock;
    // ServHost            hostCache[MAX_HOSTCACHE];
    // char                password[64];
    ASSERT_STREQ("", m.password);
    // bool                allowGnutella;
    ASSERT_FALSE(m.allowGnutella);
    // unsigned int        maxBitrateOut, maxControl, maxRelays, maxDirect;
    ASSERT_EQ(0, m.maxBitrateOut);
    ASSERT_EQ(3, m.maxControl);
    ASSERT_EQ(2, m.maxRelays);
    ASSERT_EQ(0, m.maxDirect);
    // unsigned int        minGnuIncoming, maxGnuIncoming;
    ASSERT_EQ(10, m.minGnuIncoming);
    ASSERT_EQ(20, m.maxGnuIncoming);
    // unsigned int        maxServIn;
    ASSERT_EQ(50, m.maxServIn);
    // bool                isDisabled;
    ASSERT_FALSE(m.isDisabled);
    // bool                isRoot;
    ASSERT_FALSE(m.isRoot);
    // int                 totalStreams;
    ASSERT_EQ(0, m.totalStreams);
    // Host                serverHost;
    ASSERT_STREQ("127.0.0.1:7144", m.serverHost.str().c_str());
    // String              rootHost;
    ASSERT_STREQ("bayonet.ddo.jp:7146", m.rootHost.cstr());
    // char                downloadURL[128];
    ASSERT_STREQ("", m.downloadURL);
    // String              rootMsg;
    ASSERT_STREQ("", m.rootMsg.cstr());
    // String              forceIP;
    ASSERT_STREQ("", m.forceIP.cstr());
    // char                connectHost[128];
    ASSERT_STREQ("connect1.peercast.org", m.connectHost);
    // GnuID               networkID;
    ASSERT_STREQ("00000000000000000000000000000000", m.networkID.str().c_str());
    // unsigned int        firewallTimeout;
    ASSERT_EQ(30, m.firewallTimeout);
    // int                 showLog;
    ASSERT_EQ(4, m.showLog); // ERROR
    // int                 shutdownTimer;
    ASSERT_EQ(0, m.shutdownTimer);
    // bool                pauseLog;
    ASSERT_FALSE(m.pauseLog);
    // bool                forceNormal;
    ASSERT_FALSE(m.forceNormal);
    // bool                useFlowControl;
    ASSERT_TRUE(m.useFlowControl);
    // unsigned int        lastIncoming;
    ASSERT_EQ(0, m.lastIncoming);
    // bool                restartServer;
    ASSERT_FALSE(m.restartServer);
    // bool                allowDirect;
    ASSERT_TRUE(m.allowDirect);
    // bool                autoConnect, autoServe, forceLookup;
    ASSERT_TRUE(m.autoConnect);
    ASSERT_TRUE(m.autoServe);
    ASSERT_TRUE(m.forceLookup);
    // int                 queryTTL;
    ASSERT_EQ(7, m.queryTTL);
    // unsigned int        allowServer1, allowServer2;
    ASSERT_EQ(Servent::ALLOW_ALL, m.allowServer1);
    ASSERT_EQ(Servent::ALLOW_BROADCAST, m.allowServer2);
    // unsigned int        startTime;
    ASSERT_EQ(0, m.startTime);
    // unsigned int        tryoutDelay;
    ASSERT_EQ(10, m.tryoutDelay);
    // unsigned int        refreshHTML;
    ASSERT_EQ(5, m.refreshHTML);
    // unsigned int        relayBroadcast;
    //ASSERT_EQ(0, m.relayBroadcast); // •s’è
    // unsigned int        notifyMask;
    ASSERT_EQ(0xffff, m.notifyMask);
    // BCID                *validBCID;
    ASSERT_EQ(nullptr, m.validBCID);
    // GnuID               sessionID;
    ASSERT_STREQ("00151515151515151515151515151515", m.sessionID.str().c_str());
    // ServFilter          filters[MAX_FILTERS];
    ASSERT_EQ(0xffffffff, m.filters[0].host.ip);
    ASSERT_EQ(ServFilter::F_NETWORK|ServFilter::F_DIRECT, m.filters[0].flags);
    // int                 numFilters;
    ASSERT_EQ(1, m.numFilters);
    // CookieList          cookieList;
    // AUTH_TYPE           authType;
    ASSERT_EQ(ServMgr::AUTH_COOKIE, m.authType);
    // char                htmlPath[128];
    ASSERT_STREQ("html/en", m.htmlPath);
    // int                 serventNum;
    ASSERT_EQ(0, m.serventNum);
    // String              chanLog;
    ASSERT_STREQ("", m.chanLog.cstr());
    // ChannelDirectory    channelDirectory;
    // bool                publicDirectoryEnabled;
    ASSERT_FALSE(m.publicDirectoryEnabled);
    // FW_STATE            firewalled;
    ASSERT_EQ(ServMgr::FW_UNKNOWN, m.firewalled);
}
