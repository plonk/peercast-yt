#include <gtest/gtest.h>

#include "sstream.h"
#include "servfilter.h"

class ServFilterFixture : public ::testing::Test {
public:
    ServFilter filter;
    StringStream mem;
};

TEST_F(ServFilterFixture, initialState)
{
    ASSERT_EQ(0, filter.flags);
    ASSERT_EQ(ServFilter::T_IP, filter.type);
    ASSERT_FALSE(filter.isSet());
}

TEST_F(ServFilterFixture, writeVariable)
{
    mem.str("");
    filter.writeVariable(mem, "network");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "private");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "direct");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "banned");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "ip");
    ASSERT_STREQ("0.0.0.0", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableNetwork)
{
    filter.flags |= ServFilter::F_NETWORK;
    filter.writeVariable(mem, "network");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariablePrivate)
{
    filter.flags |= ServFilter::F_PRIVATE;
    filter.writeVariable(mem, "private");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableDirect)
{
    filter.flags |= ServFilter::F_DIRECT;
    filter.writeVariable(mem, "direct");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableBanned)
{
    filter.flags |= ServFilter::F_BAN;
    filter.writeVariable(mem, "banned");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableIP)
{
    filter.setPattern("127.0.0.1");
    filter.writeVariable(mem, "ip");
    ASSERT_STREQ("127.0.0.1", mem.str().c_str());
}

TEST_F(ServFilterFixture, matches)
{
    filter.flags = ServFilter::F_PRIVATE;
    filter.setPattern("192.168.0.255"); // 192.168.0.*

    ASSERT_TRUE( filter.matches(ServFilter::F_PRIVATE, Host(192<<24 | 168<<16 | 1, 0)) );
    ASSERT_FALSE( filter.matches(ServFilter::F_DIRECT, Host(192<<24 | 168<<16 | 1, 0)) );
    ASSERT_FALSE( filter.matches(ServFilter::F_PRIVATE, Host(192<<24 | 168<<16 | 1<<8 | 1, 0)) );
}

TEST_F(ServFilterFixture, emptyStringResets)
{
    filter.setPattern("127.0.0.1");
    ASSERT_TRUE(filter.isSet());

    filter.setPattern("");
    ASSERT_FALSE(filter.isSet());
    ASSERT_EQ("0.0.0.0", filter.getPattern());
}

// パターン 0.0.0.0 は 0.0.0.0 にマッチしない。
TEST_F(ServFilterFixture, nullNeqNull)
{
    filter.setPattern("0.0.0.0");
    filter.flags = ServFilter::F_DIRECT;
    ASSERT_FALSE( filter.matches(ServFilter::F_DIRECT, Host(0, 0)) );
}

TEST_F(ServFilterFixture, setHostnamePattern)
{
    filter.setPattern("localhost");
    ASSERT_EQ(ServFilter::T_HOSTNAME, filter.type);
    ASSERT_EQ("localhost", filter.getPattern());
}

TEST_F(ServFilterFixture, setSuffixPattern)
{
    filter.setPattern(".com");
    ASSERT_EQ(ServFilter::T_SUFFIX, filter.type);
    ASSERT_EQ(".com", filter.getPattern());
}

TEST_F(ServFilterFixture, matchHostnamePattern)
{
    filter.setPattern("localhost");
    filter.flags = ServFilter::F_DIRECT;
    EXPECT_TRUE(filter.matches(ServFilter::F_DIRECT, Host("localhost", 0)));
}

TEST_F(ServFilterFixture, matchSuffixPattern)
{
    filter.setPattern(".google");
    filter.flags = ServFilter::F_DIRECT;
    EXPECT_TRUE(filter.matches(ServFilter::F_DIRECT, Host("dns.google", 0)));
}

TEST_F(ServFilterFixture, setIPv6)
{
    filter.setPattern("::1");
    ASSERT_EQ(ServFilter::T_IPV6, filter.type);
    ASSERT_EQ("::1", filter.getPattern());

    filter.setPattern("fd44:7144:7144::1");
    ASSERT_EQ(ServFilter::T_IPV6, filter.type);
    ASSERT_EQ("fd44:7144:7144::1", filter.getPattern());
}

TEST_F(ServFilterFixture, matchIPv6)
{
    filter.flags = ServFilter::F_DIRECT;
    filter.setPattern("::1");
    ASSERT_TRUE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("::1"), 0)));
    ASSERT_FALSE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("fd44:7144:7144::1"), 0)));
}

TEST_F(ServFilterFixture, matchIPv4MappedIPv6)
{
    filter.flags = ServFilter::F_DIRECT;
    filter.setPattern("::ffff:192.168.0.1");
    ASSERT_EQ(ServFilter::T_IPV6, filter.type);
    ASSERT_TRUE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("192.168.0.1"), 0)));
    ASSERT_FALSE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("192.168.0.2"), 0)));
}

TEST_F(ServFilterFixture, matchIPv4MappedIPv6UpperCase)
{
    filter.flags = ServFilter::F_DIRECT;
    filter.setPattern("::FFFF:192.168.0.1");
    ASSERT_EQ(ServFilter::T_IPV6, filter.type);
    ASSERT_TRUE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("192.168.0.1"), 0)));
    ASSERT_FALSE(filter.matches(ServFilter::F_DIRECT, Host(IP::parse("192.168.0.2"), 0)));
}
