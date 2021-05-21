#include <gtest/gtest.h>
#include "jrpc.h"

using json = nlohmann::json;

class HostGraphFixture : public ::testing::Test {
public:
};

TEST_F(HostGraphFixture, constructorNullChannel)
{
    ASSERT_THROW(JrpcApi::HostGraph(nullptr, nullptr, 4), std::invalid_argument);
}

TEST_F(HostGraphFixture, simplestCase)
{
    auto ch = std::make_shared<Channel>();
    auto hitList = std::make_shared<ChanHitList>();

    JrpcApi::HostGraph graph(ch, hitList.get(), 4);

    ASSERT_STREQ("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218}]", ((json) graph.getRelayTree()).dump().c_str());
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

    JrpcApi::HostGraph graph(ch, hitList.get(), 4);

    ASSERT_STREQ("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218},{\"address\":\"8.8.8.8\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":false,\"isReceiving\":true,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":7144,\"sessionId\":\"00000000000000000000000000000000\",\"version\":0}]", ((json) graph.getRelayTree()).dump().c_str());
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

    JrpcApi::HostGraph graph(ch, hitList.get(), 4);

    auto relayTree = graph.getRelayTree();

    ASSERT_EQ(relayTree.size(), 1);
    ASSERT_EQ(relayTree[0]["address"], "127.0.0.1");
    ASSERT_EQ(relayTree[0]["children"].size(), 1);
    ASSERT_EQ(relayTree[0]["children"][0]["address"], "8.8.8.8");
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

    JrpcApi::HostGraph graph(ch, hitList.get(), 4);

    auto relayTree = graph.getRelayTree();

    ASSERT_EQ(relayTree.size(), 1);
    ASSERT_EQ(relayTree[0]["address"], "127.0.0.1");
    ASSERT_EQ(relayTree[0]["children"].size(), 1);
    ASSERT_EQ(relayTree[0]["children"][0]["address"], "8.8.8.8");
}
