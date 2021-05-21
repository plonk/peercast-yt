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
    //ASSERT_EQ(10, m.minGnuIncoming);
    //ASSERT_EQ(20, m.maxGnuIncoming);
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
    ASSERT_STREQ("yp.pcgw.pgw.jp:7146", m.rootHost.cstr());
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
    //ASSERT_EQ(7, m.queryTTL);
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
    //ASSERT_EQ(nullptr, m.validBCID);
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
    // bool                publicDirectoryEnabled;
    ASSERT_FALSE(m.publicDirectoryEnabled);
    // FW_STATE            firewalled;
    ASSERT_EQ(ServMgr::FW_UNKNOWN, m.firewalled);

    // String              serverName;
    ASSERT_TRUE(m.serverName == "");

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
    ASSERT_STREQ("yp.pcgw.pgw.jp:7146", mem.str().c_str());

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
    ASSERT_TRUE(m.writeVariable(mem, "numActive1"));
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

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numExternalChannels"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numChannelFeedsPlusOne"));
    ASSERT_STREQ("2", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "numChannelFeeds"));
    ASSERT_STREQ("1", mem.str().c_str());

    // channelDirectory.*

    mem.str("");
    ASSERT_TRUE(m.writeVariable(mem, "publicDirectoryEnabled"));
    ASSERT_STREQ("0", mem.str().c_str());

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
    //ASSERT_EQ(0, m.numStreams(Servent::T_PGNU, false));

    ASSERT_EQ(0, m.numStreams(Servent::T_NONE, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_INCOMING, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_SERVER, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_RELAY, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_DIRECT, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_COUT, true));
    ASSERT_EQ(0, m.numStreams(Servent::T_CIN, true));
    //ASSERT_EQ(0, m.numStreams(Servent::T_PGNU, true));
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

#include <stdio.h>
TEST_F(ServMgrFixture, getSettings)
{
    auto doc = m.getSettings();

    /* セクション名とキーの名前だけテストし、セクションの数とキーの値はテストしない。 */
    for (auto& r : std::vector<std::pair<std::string, std::vector<std::pair<std::string,ini::Value>>>>({
        {{"Server",
          {{"serverName", ""},
           {"serverPort", "7144"},
           {"autoServe", "Yes"},
           {"forceIP", ""},
           {"isRoot", "No"},
           {"maxBitrateOut", "0"},
           {"maxRelays", "2"},
           {"maxDirect", "0"},
           {"maxRelaysPerChannel", "0"},
           {"firewallTimeout", "30"},
           {"forceNormal", "No"},
           {"rootMsg", ""},
           {"authType", "cookie"},
           {"cookiesExpire", "session"},
           {"htmlPath", "html/en"},
           {"maxServIn", "50"},
           {"chanLog", ""},
           {"publicDirectory", "No"},
           {"networkID", "00000000000000000000000000000000"}}},
         {"Broadcast",
          {{"broadcastMsgInterval", "10"},
           {"broadcastMsg", ""},
           {"icyMetaInterval", "8192"},
           {"broadcastID", "00151515151515151515151515151515"},
           {"hostUpdateInterval", "120"},
           {"maxControlConnections", "3"},
           {"rootHost", "yp.pcgw.pgw.jp:7146"}}},
         {"Client",
          {{"refreshHTML", "5"},
           {"chat", "Yes"},
           {"relayBroadcast", "30"},
           {"minBroadcastTTL", "1"},
           {"maxBroadcastTTL", "7"},
           {"pushTries", "5"},
           {"pushTimeout", "60"},
           {"maxPushHops", "8"},
           {"transcodingEnabled", "No"},
           {"preset", "veryfast"},
           {"audioCodec", "mp3"},
           {"wmvProtocol", "http"}}},
         {"Privacy", {{"password", ""}, {"maxUptime", "0"}}},
         {"Filter",
          {{"ip", "255.255.255.255"},
           {"private", "No"},
           {"ban", "No"},
           {"network", "Yes"},
           {"direct", "Yes"}}},
         {"Feed", {{"url", "http://yp.pcgw.pgw.jp/index.txt"}}},
         {"Uptest", {{"url", "http://bayonet.ddo.jp/sp/yp4g.xml"}}},
         {"Notify",
          {{"PeerCast", "Yes"}, {"Broadcasters", "Yes"}, {"TrackInfo", "Yes"}}},
         {"Server1",
          {{"allowHTML", "Yes"},
           {"allowBroadcast", "Yes"},
           {"allowNetwork", "Yes"},
           {"allowDirect", "Yes"}}},
         {"Debug", {{"logLevel", "3"}, {"pauseLog", "No"}, {"idleSleepTime", "10"}}}}
         }))
    {
        auto it = std::find_if(doc.begin(), doc.end(), [&](ini::Section& s){ return s.name == r.first; });
        ASSERT_EQ(r.first, it != doc.end() ? r.first : std::string("SECTION NOT FOUND"));
        //ASSERT_TRUE(it != doc.end());
        for (auto& p : r.second) {
            auto it2 = std::find_if(it->keys.begin(),
                                    it->keys.end(),
                                    [&](std::pair<std::string, ini::Value>& q) { return p.first == q.first; });
            ASSERT_EQ(p.first, it2 != it->keys.end() ? p.first : std::string("KEY NOT FOUND"));
            //ASSERT_TRUE(it2 != it->keys.end());
        }
    }
}

TEST_F(ServMgrFixture, hasUnsafeFilterSettings)
{
    ASSERT_EQ(1, m.numFilters);
    ASSERT_EQ("255.255.255.255", m.filters[0].getPattern());
    ASSERT_FALSE(m.hasUnsafeFilterSettings());

    m.filters[0].flags |= ServFilter::F_PRIVATE;
    ASSERT_TRUE(m.hasUnsafeFilterSettings());
}
