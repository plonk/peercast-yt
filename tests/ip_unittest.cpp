#include <gtest/gtest.h>
#include "ip.h"

class IPFixture : public ::testing::Test {
public:
    IPFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~IPFixture()
    {
    }
};

TEST_F(IPFixture, isIPv4Mapped)
{
    IP ip(127<<24|1);
    ASSERT_TRUE(ip.isIPv4Mapped());
}

TEST_F(IPFixture, tryParse)
{
    IP ip;

    ASSERT_TRUE(IP::tryParse("127.0.0.1", ip));
    ASSERT_TRUE(ip.isIPv4Mapped());
    ASSERT_TRUE(ip.isIPv4Loopback());
    ASSERT_TRUE(ip);

    ASSERT_TRUE(IP::tryParse("::1", ip));
    ASSERT_FALSE(ip.isIPv4Mapped());
    ASSERT_TRUE(ip.isIPv6Loopback());
    ASSERT_TRUE(ip);

    ASSERT_FALSE(IP::tryParse("foo", ip));
    ASSERT_FALSE(ip);
}

TEST_F(IPFixture, parse)
{
    auto ip = IP::parse("::1");
    ASSERT_FALSE(ip.isIPv4Mapped());
    ASSERT_TRUE(ip.isIPv6Loopback());

}
