#include <gtest/gtest.h>
#include "common.h"

TEST(HostTest, initialState)
{
    Host h;

    ASSERT_EQ(0, h.ip);
    ASSERT_EQ(0, h.port);
}

TEST(HostTest, loopbackIP)
{
    Host host;

    host.fromStrIP("127.0.0.1", 0);
    ASSERT_TRUE( host.loopbackIP() );

    // 127 で始まるクラスAのネットワーク全部がループバックとして機能す
    // るが、loopbackIP は 127.0.0.1 以外には FALSE を返す。
    host.fromStrIP("127.99.99.99", 0);
    ASSERT_FALSE( host.loopbackIP() );
}

TEST(HostTest, isMemberOf)
{
    Host host, pattern;

    host.fromStrIP("192.168.0.1", -1);
    pattern.fromStrIP("192.168.0.1", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.0.2", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.0.255", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.255.255", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("192.168.255.1", -1);
    ASSERT_TRUE(host.isMemberOf(pattern));

    pattern.fromStrIP("0.0.0.0", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));

    host.fromStrIP("0.0.0.0", -1);
    pattern.fromStrIP("0.0.0.0", -1);
    ASSERT_FALSE(host.isMemberOf(pattern));
}

TEST(HostTest, str)
{
    Host host;
    ASSERT_STREQ("0.0.0.0:0", host.str().c_str());
    ASSERT_STREQ("0.0.0.0:0", host.str(true).c_str());
    ASSERT_STREQ("0.0.0.0", host.str(false).c_str());
}

TEST(HostTest, strUlimit)
{
    Host host;
    host.fromStrIP("255.255.255.255", 65535);
    ASSERT_STREQ("255.255.255.255:65535", host.str().c_str());
    ASSERT_STREQ("255.255.255.255:65535", host.str(true).c_str());
    ASSERT_STREQ("255.255.255.255", host.str(false).c_str());
}
