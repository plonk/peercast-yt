#include <gtest/gtest.h>

#include "cookie.h"

class CookieFixture : public ::testing::Test {
public:
    Cookie c;
};

TEST_F(CookieFixture, initialState)
{
    ASSERT_EQ(0, c.ip);
    ASSERT_STREQ("", c.id);
}

TEST_F(CookieFixture, set)
{
    c.set("hoge", 0xffffffff);
    ASSERT_EQ(IP(0xffffffff), c.ip);
    ASSERT_NE(0xffffffff, c.ip);
    ASSERT_STREQ("hoge", c.id);
}

TEST_F(CookieFixture, compare)
{
    Cookie d, e, f, g;

    c.set("hoge", 0xffffffff);

    d.set("hoge", 0xffffffff);
    e.set("hoge", 0xfffffffe);
    f.set("fuga", 0xffffffff);
    g.set("fuga", 0xfffffffe);

    ASSERT_TRUE(c == d);
    ASSERT_TRUE(d == c);

    ASSERT_FALSE(c == e);
    ASSERT_FALSE(e == c);

    ASSERT_FALSE(c == f);
    ASSERT_FALSE(f == c);

    ASSERT_FALSE(c == g);
    ASSERT_FALSE(g == c);
}
