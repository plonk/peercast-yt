#include <gtest/gtest.h>

#include "cgi.h"

using namespace cgi;

class QueryFixture : public ::testing::Test {
public:
    QueryFixture()
        : query("a=1&b=2&b=3&c")
    {
    }

    Query query;
};

TEST_F(QueryFixture, singleValuedKey)
{
    ASSERT_EQ(true, query.hasKey("a"));
    ASSERT_STREQ("1", query.get("a").c_str());
    ASSERT_EQ(1, query.getAll("a").size());
    ASSERT_STREQ("1", query.getAll("a")[0].c_str());
}

TEST_F(QueryFixture, multiValuedKey)
{
    ASSERT_EQ(true, query.hasKey("b"));
    ASSERT_STREQ("2", query.get("b").c_str());
    ASSERT_EQ(2, query.getAll("b").size());
    ASSERT_STREQ("2", query.getAll("b")[0].c_str());
    ASSERT_STREQ("3", query.getAll("b")[1].c_str());
}

TEST_F(QueryFixture, keyWithNoValue)
{
    ASSERT_EQ(true, query.hasKey("c"));
    ASSERT_STREQ("", query.get("c").c_str());
    ASSERT_EQ(0, query.getAll("c").size());
}

TEST_F(QueryFixture, nonexistentKey)
{
    ASSERT_EQ(false, query.hasKey("d"));
    ASSERT_STREQ("", query.get("d").c_str());
    ASSERT_EQ(0, query.getAll("d").size());
    ASSERT_EQ(false, query.hasKey("d")); // still doesn't have the key
}
