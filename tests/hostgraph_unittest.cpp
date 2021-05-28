#include <gtest/gtest.h>
#include "hostgraph.h"

using json = nlohmann::json;

class HostGraphFixture : public ::testing::Test {
public:
};

TEST_F(HostGraphFixture, constructorNullChannel)
{
    ASSERT_THROW(HostGraph(nullptr, nullptr, 4), std::invalid_argument);
}

TEST_F(HostGraphFixture, simplestCase)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();

    HostGraph graph(ch, hitList.get(), 4);

    json j = graph.getRelayTree();

    ASSERT_EQ(1, j.size());
    json::object_t h = j[0];
    ASSERT_TRUE(h["address"].is_string());
    ASSERT_TRUE(h["children"].is_array());
    ASSERT_TRUE(h["isControlFull"].is_boolean());
    ASSERT_TRUE(h["isDirectFull"].is_boolean());
    ASSERT_TRUE(h["isFirewalled"].is_boolean());
    ASSERT_TRUE(h["isReceiving"].is_boolean());
    ASSERT_TRUE(h["isRelayFull"].is_boolean());
    ASSERT_TRUE(h["isTracker"].is_boolean());

    ASSERT_TRUE(h["localDirects"].is_number());
    ASSERT_TRUE(h["localRelays"].is_number());
    ASSERT_TRUE(h["port"].is_number());
    ASSERT_TRUE(h["sessionId"].is_string());
    ASSERT_TRUE(h["version"].is_number());
}

TEST_F(HostGraphFixture, noUphost)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();
    ChanHit hit;

    hit.init();
    hit.rhost[0].fromStrIP("8.8.8.8", 7144);
    hit.rhost[1].fromStrIP("192.168.0.1", 7144);

    hitList->addHit(hit);

    HostGraph graph(ch, hitList.get(), 4);
    json j = graph.getRelayTree();

    ASSERT_TRUE(j.is_array());
    ASSERT_EQ(2, j.size());

    // 順番はわからない。
    ASSERT_TRUE(j[0]["address"] == "127.0.0.1" || j[0]["address"] == "8.8.8.8");
}

TEST_F(HostGraphFixture, withUphost)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();

    {
        ChanHit hit;

        hit.rhost[0].fromStrIP("8.8.8.8", 7144);
        hit.rhost[1].fromStrIP("192.168.0.1", 7144);
        hit.uphost = Host("127.0.0.1", 0);

        hitList->addHit(hit);
    }

    HostGraph graph(ch, hitList.get(), 4);

    auto relayTree = graph.getRelayTree();

    ASSERT_EQ(relayTree.size(), 1);
    // std::string に変換しておかないと失敗時に json を文字列化しようとしてSEGVる。
    ASSERT_EQ(relayTree[0]["address"].get<std::string>(), "127.0.0.1");
    ASSERT_EQ(relayTree[0]["children"].size(), 1);
    ASSERT_EQ(relayTree[0]["children"][0]["address"].get<std::string>(), "8.8.8.8");
}

TEST_F(HostGraphFixture, withUphostPush)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();

    {
        ChanHit hit;

        hit.rhost[0].fromStrIP("8.8.8.8", 0);
        hit.rhost[1].fromStrIP("192.168.0.1", 7144);
        hit.firewalled = true;
        hit.uphost = Host("127.0.0.1", 49152 /* ephemeral port */);

        hitList->addHit(hit);
    }

    HostGraph graph(ch, hitList.get(), 4);

    auto relayTree = graph.getRelayTree();

    ASSERT_EQ(relayTree.size(), 1);
    ASSERT_EQ(relayTree[0]["address"].get<std::string>(), "127.0.0.1");
    ASSERT_EQ(relayTree[0]["children"].size(), 1);
    ASSERT_EQ(relayTree[0]["children"][0]["address"].get<std::string>(), "8.8.8.8");
}

#include "servmgr.h"

TEST_F(HostGraphFixture, localRelay)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();

    // このシナリオではホストA 127.0.0.1:0/192.168.0.2:7144 へホストB
    // 127.0.0.1:8144/192.168.0.3:8144 がLAN接続している。

    ASSERT_EQ(IP::parse("127.0.0.1"), servMgr->serverHost.ip);
    ASSERT_EQ(IP::parse("192.168.0.2"), sys->getInterfaceIPv4Address());

    {
        ChanHit hit;

        hit.rhost[0].fromStrIP("127.0.0.1", 8144);
        hit.rhost[1].fromStrIP("192.168.0.3", 8144);
        hit.firewalled = false;
        hit.uphost = Host("192.168.0.2", 7144);

        hitList->addHit(hit);
    }

    HostGraph graph(ch, hitList.get(), 4);

    auto relayTree = graph.getRelayTree();

    ASSERT_EQ(relayTree.size(), 1);
    ASSERT_EQ(relayTree[0]["address"].get<std::string>(), "127.0.0.1");
    ASSERT_EQ(relayTree[0]["isFirewalled"].get<bool>(), true);
    ASSERT_EQ(relayTree[0]["children"].size(), 1);
    ASSERT_EQ(relayTree[0]["children"][0]["address"].get<std::string>(), "127.0.0.1");
    ASSERT_EQ(relayTree[0]["children"][0]["port"].get<int>(), 8144);
    ASSERT_EQ(relayTree[0]["children"][0]["isFirewalled"].get<bool>(), false);
}
