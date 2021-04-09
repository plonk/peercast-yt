#include <gtest/gtest.h>

#include "cookie.h"

class CookieListFixture : public ::testing::Test {
public:
    CookieList ls;
};

TEST_F(CookieListFixture, initialState)
{
    ASSERT_FALSE(ls.neverExpire);
}

TEST_F(CookieListFixture, containsDifferentIPs)
{
    Cookie c;
    c.set("12345678901234567890123456789012", IP::parse("192.168.0.1"));
    Cookie d;
    d.set("12345678901234567890123456789012", IP::parse("192.168.0.2"));

    ASSERT_FALSE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ASSERT_TRUE(ls.add(c));
    ASSERT_TRUE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ASSERT_TRUE(ls.add(d));
    ASSERT_TRUE(ls.contains(c));
    ASSERT_TRUE(ls.contains(d));
}

TEST_F(CookieListFixture, containsIPv6DifferentIPs)
{
    Cookie c;
    c.set("12345678901234567890123456789012", IP::parse("fd44:7144:7144::1"));
    Cookie d;
    d.set("12345678901234567890123456789012", IP::parse("fd44:7144:7144::2"));

    ASSERT_FALSE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ls.add(c);
    ASSERT_TRUE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ls.add(d);
    ASSERT_TRUE(ls.contains(c));
    ASSERT_TRUE(ls.contains(d));
}

TEST_F(CookieListFixture, containsDifferentIDs)
{
    Cookie c;
    c.set("12345678901234567890123456789012", IP::parse("192.168.0.1"));
    Cookie d;
    d.set("02345678901234567890123456789012", IP::parse("192.168.0.1"));

    ASSERT_FALSE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ASSERT_TRUE(ls.add(c));
    ASSERT_TRUE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ASSERT_TRUE(ls.add(d));
    ASSERT_TRUE(ls.contains(c));
    ASSERT_TRUE(ls.contains(d));
}

TEST_F(CookieListFixture, containsSameIDs)
{
    Cookie c;
    c.set("12345678901234567890123456789012", IP::parse("192.168.0.1"));
    Cookie d;
    d.set("12345678901234567890123456789012; hoge=fuga", IP::parse("192.168.0.1"));

    ASSERT_FALSE(ls.contains(c));
    ASSERT_FALSE(ls.contains(d));

    ASSERT_TRUE(ls.add(c));
    ASSERT_TRUE(ls.contains(c));
    ASSERT_TRUE(ls.contains(d));
}
