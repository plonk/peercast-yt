#include <gtest/gtest.h>

#include "sys.h"

class SysFixture : public ::testing::Test {
public:
    SysFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~SysFixture()
    {
    }
};

TEST_F(SysFixture, strdup)
{
    const char *orig = "abc";
    char *copy = Sys::strdup(orig);
    ASSERT_STREQ("abc", copy);
    ASSERT_NE(orig, copy);
    free(copy);
}

TEST_F(SysFixture, stricmp)
{
    ASSERT_EQ(0, Sys::stricmp("", ""));
    ASSERT_EQ(0, Sys::stricmp("a", "a"));
    ASSERT_EQ(0, Sys::stricmp("a", "A"));
    ASSERT_EQ(-1, Sys::stricmp("a", "b"));
    ASSERT_TRUE(Sys::stricmp("a", "b") < 0);
    ASSERT_TRUE(Sys::stricmp("a", "B") < 0);
    ASSERT_TRUE(Sys::stricmp("b", "a") > 0);
    ASSERT_TRUE(Sys::stricmp("b", "A") > 0);
}

// #include <strings.h>
TEST_F(SysFixture, strnicmp)
{
    // ASSERT_EQ(0, strncasecmp("", "", 0));
    // ASSERT_EQ(0, strncasecmp("", "", 1));
    // ASSERT_EQ(0, strncasecmp("", "", 100));

    // ASSERT_EQ(0, strncasecmp("a", "a", 0));
    // ASSERT_EQ(0, strncasecmp("a", "a", 1));
    // ASSERT_EQ(0, strncasecmp("a", "a", 100));

    // ASSERT_EQ(0, strncasecmp("a", "A", 0));
    // ASSERT_EQ(0, strncasecmp("a", "A", 1));
    // ASSERT_EQ(0, strncasecmp("a", "A", 100));

    // ASSERT_EQ(0, strncasecmp("a", "b", 0));
    // ASSERT_NE(0, strncasecmp("a", "b", 1));
    // ASSERT_NE(0, strncasecmp("a", "b", 100));

    ASSERT_EQ(0, Sys::strnicmp("", "", 0));
    ASSERT_EQ(0, Sys::strnicmp("", "", 1));
    ASSERT_EQ(0, Sys::strnicmp("", "", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "a", 0));
    ASSERT_EQ(0, Sys::strnicmp("a", "a", 1));
    ASSERT_EQ(0, Sys::strnicmp("a", "a", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "A", 0));
    ASSERT_EQ(0, Sys::strnicmp("a", "A", 1));
    ASSERT_EQ(0, Sys::strnicmp("a", "A", 100));

    ASSERT_EQ(0, Sys::strnicmp("a", "b", 0));
    ASSERT_NE(0, Sys::strnicmp("a", "b", 1));
    ASSERT_NE(0, Sys::strnicmp("a", "b", 100));
}
