#include <gtest/gtest.h>

#include "logbuf.h"
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
    // ASSERT_FALSE(m.allowGnutella);
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
    // atomic<int>         m_logLevel;
    ASSERT_EQ(LogBuffer::T_INFO, m.m_logLevel);
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
    //ASSERT_EQ(Servent::ALLOW_BROADCAST, m.allowServer2);
    // unsigned int        startTime;
    ASSERT_EQ(0, m.startTime);
    // unsigned int        tryoutDelay;
    ASSERT_EQ(10, m.tryoutDelay);
    // unsigned int        refreshHTML;
    ASSERT_EQ(5, m.refreshHTML);
    // unsigned int        relayBroadcast;
    ASSERT_EQ(30, m.relayBroadcast); // オリジナルでは不定。
    // unsigned int        notifyMask;
    ASSERT_EQ(0xffff, m.notifyMask);
    // BCID                *validBCID;
    ASSERT_EQ(nullptr, m.validBCID);
    // GnuID               sessionID;
    ASSERT_STREQ("00151515151515151515151515151515", m.sessionID.str().c_str());
    // ServFilter          filters[MAX_FILTERS];
    ASSERT_EQ("255.255.255.255", m.filters[0].getPattern());
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
    ASSERT_TRUE(m.writeVariable(mem, "allow.HTML1"));
    ASSERT_STREQ("1", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "allow.broadcasting1"));
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
    ASSERT_TRUE(m.writeVariable(mem, "log.level"));
    ASSERT_STREQ("3", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "lang.en"));
    ASSERT_STREQ("1", mem.str().c_str());

    // channelDirectory.*

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "genrePrefix"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "test"));
    ASSERT_STREQ("かきくけこABCDabcd", mem.str().c_str());
}

// serventNum がインクリメントされ、サーバントの serventIndex にセットされる。
TEST_F(ServMgrFixture, allocServent)
{
    Servent *s;

    ASSERT_EQ(0, m.serventNum);
    s = m.allocServent();
    ASSERT_NE(nullptr, s);
    ASSERT_EQ(s, m.servents);
    ASSERT_EQ(1, s->serventIndex);
    ASSERT_EQ(1, m.serventNum);
}

TEST_F(ServMgrFixture, numStreams_nullcase)
{
    ASSERT_EQ(nullptr, m.servents);

    ASSERT_EQ(0, m.numStreams(Servent::T_NONE, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_INCOMING, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_SERVER, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_RELAY, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_DIRECT, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_COUT, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_CIN, false));
    ASSERT_EQ(0, m.numStreams(Servent::T_PGNU, false));

    ASSERT_EQ(0, m.numStreams(Servent::T_NONE, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_INCOMING, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_SERVER, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_RELAY, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_DIRECT, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_COUT, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_CIN, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_PGNU, true));
}

TEST_F(ServMgrFixture, numStreams_connectedRelaysAreCounted)
{
    Servent *s;

    s = m.allocServent();
    ASSERT_EQ(s, m.servents);
    s->type = Servent::T_RELAY;
    ASSERT_EQ(0, m.numStreams(Servent::T_RELAY, false));
    s->status = Servent::S_CONNECTED;
    ASSERT_EQ(1, m.numStreams(Servent::T_RELAY, false));
}

TEST_F(ServMgrFixture, numStreams_connectedDirectsAreCounted)
{
    Servent *s;

    s = m.allocServent();
    ASSERT_EQ(s, m.servents);
    s->type = Servent::T_DIRECT;
    ASSERT_EQ(0, m.numStreams(Servent::T_DIRECT, false));
    s->status = Servent::S_CONNECTED;
    ASSERT_EQ(1, m.numStreams(Servent::T_DIRECT, false));
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

TEST_F(ServMgrFixture, doSaveSettings)
{
    StringStream mem;
    IniFileBase ini(mem);
    m.doSaveSettings(ini);
    ASSERT_EQ("\r\n"
              "[Server]\r\n"
              "serverName = \r\n"
              "serverPort = 7144\r\n"
              "autoServe = Yes\r\n"
              "forceIP = \r\n"
              "isRoot = No\r\n"
              "maxBitrateOut = 0\r\n"
              "maxRelays = 2\r\n"
              "maxDirect = 0\r\n"
              "maxRelaysPerChannel = 0\r\n"
              "firewallTimeout = 30\r\n"
              "forceNormal = No\r\n"
              "rootMsg = \r\n"
              "authType = cookie\r\n"
              "cookiesExpire = session\r\n"
              "htmlPath = html/en\r\n"
              "minPGNUIncoming = 10\r\n"
              "maxPGNUIncoming = 20\r\n"
              "maxServIn = 50\r\n"
              "chanLog = \r\n"
              "genrePrefix = \r\n"
              "networkID = 00000000000000000000000000000000\r\n"
              "\r\n"
              "[Broadcast]\r\n"
              "broadcastMsgInterval = 10\r\n"
              "broadcastMsg = \r\n"
              "icyMetaInterval = 8192\r\n"
              "broadcastID = 00151515151515151515151515151515\r\n"
              "hostUpdateInterval = 120\r\n"
              "maxControlConnections = 3\r\n"
              "rootHost = bayonet.ddo.jp:7146\r\n"
              "\r\n"
              "[Client]\r\n"
              "refreshHTML = 5\r\n"
              "relayBroadcast = 30\r\n"
              "minBroadcastTTL = 1\r\n"
              "maxBroadcastTTL = 7\r\n"
              "pushTries = 5\r\n"
              "pushTimeout = 60\r\n"
              "maxPushHops = 8\r\n"
              "autoQuery = 0\r\n"
              "queryTTL = 7\r\n"
              "transcodingEnabled = No\r\n"
              "preset = veryfast\r\n"
              "audioCodec = mp3\r\n"
              "wmvProtocol = http\r\n"
              "\r\n"
              "[Privacy]\r\n"
              "password = \r\n"
              "maxUptime = 0\r\n"
              "\r\n"
              "[Filter]\r\n"
              "ip = 255.255.255.255\r\n"
              "private = No\r\n"
              "ban = No\r\n"
              "network = Yes\r\n"
              "direct = Yes\r\n"
              "[End]\r\n"
              "\r\n"
              "[Notify]\r\n"
              "PeerCast = Yes\r\n"
              "Broadcasters = Yes\r\n"
              "TrackInfo = Yes\r\n"
              "[End]\r\n"
              "\r\n"
              "[Server1]\r\n"
              "allowHTML = Yes\r\n"
              "allowBroadcast = Yes\r\n"
              "allowNetwork = Yes\r\n"
              "allowDirect = Yes\r\n"
              "[End]\r\n"
              "\r\n"
              "[Debug]\r\n"
              "logLevel = 3\r\n"
              "pauseLog = No\r\n"
              "idleSleepTime = 10\r\n", mem.str());
}

TEST_F(ServMgrFixture, hasUnsafeFilterSettings)
{
    ASSERT_EQ(1, m.numFilters);
    ASSERT_EQ("255.255.255.255", m.filters[0].getPattern());
    ASSERT_FALSE(m.hasUnsafeFilterSettings());

    m.filters[0].flags |= ServFilter::F_PRIVATE;
    ASSERT_TRUE(m.hasUnsafeFilterSettings());
}
