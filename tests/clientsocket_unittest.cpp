#include <gtest/gtest.h>
#include "socket.h"

class ClientSocketFixture : public ::testing::Test {
public:
};

TEST_F(ClientSocketFixture, getIP)
{
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("localhost"));
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("127.0.0.1"));
}

TEST_F(ClientSocketFixture, getHostname)
{
    char name[256] = "";

    EXPECT_TRUE(ClientSocket::getHostname(name, ClientSocket::getIP("8.8.8.8")));
    EXPECT_STREQ("dns.google", name);
}

TEST_F(ClientSocketFixture, getHostnameByAddress)
{
    {
        std::string str;
        EXPECT_TRUE(ClientSocket::getHostnameByAddress(IP::parse("127.0.0.1"), str));
        EXPECT_STREQ("localhost", str.c_str());
    }

    {
        std::string str;
        EXPECT_TRUE(ClientSocket::getHostnameByAddress(IP::parse("::1"), str));
        EXPECT_STREQ("localhost", str.c_str());
    }
}
