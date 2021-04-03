#include <gtest/gtest.h>
#include "common.h"
#include "host.h"

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

TEST(HostTest, strIPv6)
{
    IP ip;
    ASSERT_TRUE(IP::tryParse("::1", ip));
    Host host(ip, 7144);
    ASSERT_STREQ("[::1]:7144", host.str().c_str());
    ASSERT_STREQ("::1", host.str(false).c_str());
}

// 128 バイトのホスト名で内部バッファーが NUL終端されなくなるバグを発
// 現させるテスト。valgrind などで検知せよ。
TEST(HostTest, fromStrName_128bytes)
{
    Host host;
    char hostname[129] = "";

    memset(hostname, 'A', 128);
    host.fromStrName(hostname, 0);

    ASSERT_EQ(0, host.ip);
    ASSERT_EQ(0, host.port);
}

TEST(HostTest, fromStrName_localhost)
{
    Host host;

    host.fromStrName("localhost", 0);

    ASSERT_EQ(127<<24 | 1, host.ip.ipv4());
    ASSERT_EQ(0, host.port);

    host.fromStrName("localhost", 7144);

    ASSERT_EQ(127<<24 | 1, host.ip.ipv4());
    ASSERT_EQ(7144, host.port);

    host.fromStrName("localhost:8144", 7144);

    ASSERT_EQ(127<<24 | 1, host.ip.ipv4());
    ASSERT_EQ(8144, host.port);
}

TEST(HostTest, fromString)
{
    Host host;

    host = Host::fromString("localhost");
    ASSERT_EQ(IP::parse("127.0.0.1"), host.ip);
    ASSERT_EQ(0, host.port);
    host = Host::fromString("localhost:7144");
    ASSERT_EQ(IP::parse("127.0.0.1"), host.ip);
    ASSERT_EQ(7144, host.port);

    host = Host::fromString("192.168.0.1");
    ASSERT_EQ(IP::parse("192.168.0.1"), host.ip);
    ASSERT_EQ(0, host.port);
    host = Host::fromString("192.168.0.1:7144");
    ASSERT_EQ(IP::parse("192.168.0.1"), host.ip);
    ASSERT_EQ(7144, host.port);

    host = Host::fromString("[::1]");
    ASSERT_EQ(IP::parse("::1"), host.ip);
    ASSERT_EQ(0, host.port);
    host = Host::fromString("[::1]:7144");
    ASSERT_EQ(IP::parse("::1"), host.ip);
    ASSERT_EQ(7144, host.port);

    // "::1" は解釈できないことにしよう。
    host = Host::fromString("::1");
    ASSERT_EQ(0, host.ip);
    ASSERT_EQ(0, host.port);
}
