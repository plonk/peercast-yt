#include <gtest/gtest.h>
#include "jrpc.h"

using json = nlohmann::json;

class HostGraphFixture : public ::testing::Test {
public:
    HostGraphFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~HostGraphFixture()
    {
    }
};

TEST_F(HostGraphFixture, constructorNullChannel)
{
    ASSERT_THROW(JrpcApi::HostGraph(nullptr, nullptr), std::invalid_argument);
}

TEST_F(HostGraphFixture, simplestCase)
{
    auto ch = new Channel();
    auto hitList = new ChanHitList();

    JrpcApi::HostGraph graph(ch, hitList);

    ASSERT_STREQ("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218}]", ((json) graph.getRelayTree()).dump().c_str());
}

TEST_F(HostGraphFixture, withHitList)
{
    auto ch = new Channel();
    auto hitList = new ChanHitList();
    ChanHit hit;

    hit.init();
    hit.rhost[0].fromStrIP("8.8.8.8", 7144);
    hit.rhost[1].fromStrIP("192.168.0.1", 7144);

    hitList->addHit(hit);

    JrpcApi::HostGraph graph(ch, hitList);

    ASSERT_STREQ("[{\"address\":\"127.0.0.1\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":true,\"isReceiving\":false,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":0,\"sessionId\":\"00151515151515151515151515151515\",\"version\":1218},{\"address\":\"8.8.8.8\",\"children\":[],\"isControlFull\":false,\"isDirectFull\":true,\"isFirewalled\":false,\"isReceiving\":true,\"isRelayFull\":false,\"isTracker\":false,\"localDirects\":0,\"localRelays\":0,\"port\":7144,\"sessionId\":\"00000000000000000000000000000000\",\"version\":0}]", ((json) graph.getRelayTree()).dump().c_str());
}
