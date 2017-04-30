#include <gtest/gtest.h>
#include "jrpc.h"

using json = nlohmann::json;

class JrpcApiFixture : public ::testing::Test {
public:
    JrpcApi api;
};

TEST_F(JrpcApiFixture, methodListIsInitialized)
{
    ASSERT_NE(0, api.m_methods.size());
}

TEST_F(JrpcApiFixture, toPositionalArguments)
{
    json named_params = {
        {"a", 1},
        {"b", 2}
    };
    json names = { "a", "b" };

    json result = api.toPositionalArguments(named_params,
                                            names);

    ASSERT_TRUE(json::array({ 1, 2 }) == result);
}

TEST_F(JrpcApiFixture, getNewVersions)
{
    json result = api.getNewVersions(json::array());

    ASSERT_TRUE(json::array() == result);
}

TEST_F(JrpcApiFixture, getNotificationMessages)
{
    json result = api.getNotificationMessages(json::array());

    ASSERT_TRUE(json::array() == result);
}

TEST_F(JrpcApiFixture, getStatus)
{
    json result = api.getStatus(json::array());

    std::string localIP = Host(ClientSocket::getIP(NULL), 7144).IPtoStr();

    json expected = {
        { "globalDirectEndPoint", { "127.0.0.1", 7144 } },
        { "globalRelayEndPoint", { "127.0.0.1", 7144 } },
        { "isFirewalled", nullptr },
        { "localDirectEndPoint", { localIP, 7144 } },
        { "localRelayEndPoint", { localIP, 7144 } },
        { "uptime", 0 },
    };

    ASSERT_TRUE(expected == result);
}

TEST_F(JrpcApiFixture, getChannelRelayTree)
{
    ASSERT_THROW(api.getChannelRelayTree({"hoge"}), JrpcApi::application_error);
}
