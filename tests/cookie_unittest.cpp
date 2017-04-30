#include <gtest/gtest.h>

#include "http.h"

class CookieFixture : public ::testing::Test {
public:
    Cookie c;
};

TEST_F(CookieFixture, initialState)
{
    ASSERT_EQ(0, c.ip);
    ASSERT_STREQ("", c.id);
    ASSERT_EQ(0, c.time);
}

TEST_F(CookieFixture, set)
{
    c.set("hoge", 0xffffffff);
    ASSERT_EQ(0xffffffff, c.ip);
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

    ASSERT_TRUE(c.compare(d));
    ASSERT_TRUE(d.compare(c));

    ASSERT_FALSE(c.compare(e));
    ASSERT_FALSE(e.compare(c));

    ASSERT_FALSE(c.compare(f));
    ASSERT_FALSE(f.compare(c));

    ASSERT_FALSE(c.compare(g));
    ASSERT_FALSE(g.compare(c));
}
