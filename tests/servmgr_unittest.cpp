#include <gtest/gtest.h>

#include "servmgr.h"
#include "sstream.h"

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
    //ASSERT_EQ(0, m.relayBroadcast); // 不定
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

    // String              serverName;
    ASSERT_TRUE(m.serverName == "");

    // std::string         genrePrefix;
    ASSERT_EQ("", m.genrePrefix);

    // bool                transcodingEnabled;
    ASSERT_FALSE(m.transcodingEnabled);

    // std::string         preset;
    ASSERT_EQ("veryfast", m.preset);

    // std::string         audioCodec;
    ASSERT_EQ("mp3", m.audioCodec);

    // std::string         wmvProtocol;
    ASSERT_EQ("http", m.wmvProtocol);

    // RTMPServerMonitor   rtmpServerMonitor;
    ASSERT_FALSE(m.rtmpServerMonitor.isEnabled());

    // uint16_t            rtmpPort;
    ASSERT_EQ(1935, m.rtmpPort);

    // ChanInfo            defaultChannelInfo;
    ASSERT_FALSE(m.defaultChannelInfo.id.isSet());
}

TEST_F(ServMgrFixture, writeVariable)
{
    StringStream mem;

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "version"));
    ASSERT_EQ(12, mem.str().size());
    ASSERT_STREQ("v0.1218", mem.str().substr(0,7).c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "uptime"));
    ASSERT_STREQ("-", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numRelays"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numDirect"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "totalConnected"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numServHosts"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numServents"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverName"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverPort"));
    ASSERT_STREQ("7144", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverIP"));
    ASSERT_STREQ("127.0.0.1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "ypAddress"));
    ASSERT_STREQ("bayonet.ddo.jp:7146", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "password"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "isFirewalled"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "rootMsg"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "isRoot"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "isPrivate"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "forceYP"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "refreshHTML"));
    ASSERT_STREQ("5", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxRelays"));
    ASSERT_STREQ("2", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxDirect"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxBitrateOut"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxControlsIn"));
    ASSERT_STREQ("3", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxServIn"));
    ASSERT_STREQ("50", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numFilters"));
    ASSERT_STREQ("2", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "maxPGNUIn"));
    ASSERT_STREQ("20", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "minPGNUIn"));
    ASSERT_STREQ("10", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numActive1"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numActive2"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numPGNU"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numCIN"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numCOUT"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numIncoming"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numValidBCID"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "disabled"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverPort1"));
    ASSERT_STREQ("7144", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverLocalIP"));
    ASSERT_STRNE("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "upgradeURL"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "serverPort2"));
    ASSERT_STREQ("7145", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.HTML1"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.broadcasting1"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.broadcasting2"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.network1"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.direct1"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "auth.useCookies"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "auth.useHTTP"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "auth.useSessionCookies"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "log.debug"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "log.errors"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "log.gnet"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "log.channel"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "lang.en"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numExternalChannels"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numChannelFeedsPlusOne"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numChannelFeeds"));
    ASSERT_STREQ("0", mem.str().c_str());

    // channelDirectory.*

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "publicDirectoryEnabled"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "genrePrefix"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "test"));
    ASSERT_STREQ("かきくけこABCDabcd", mem.str().c_str());
}

TEST_F(ServMgrFixture, isFiltered)
{
    Host h;
    h.fromStrIP("192.168.0.1", 0);

    ASSERT_FALSE(m.isFiltered(ServFilter::F_PRIVATE, h));
    ASSERT_FALSE(m.isFiltered(ServFilter::F_BAN, h));
    ASSERT_TRUE(m.isFiltered(ServFilter::F_NETWORK, h));
    ASSERT_TRUE(m.isFiltered(ServFilter::F_DIRECT, h));
}
