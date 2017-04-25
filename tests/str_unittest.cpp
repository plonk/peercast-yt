#include <gtest/gtest.h>

#include "str.h"
using namespace str;

class strFixture : public ::testing::Test {
public:
    strFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~strFixture()
    {
    }
};

TEST_F(strFixture, group_digits)
{
    ASSERT_STREQ("0", group_digits("0").c_str());
    ASSERT_STREQ("1", group_digits("1").c_str());
    ASSERT_STREQ("1.0", group_digits("1.0").c_str());
    ASSERT_STREQ("1.1", group_digits("1.1").c_str());
    ASSERT_STREQ("1.1234", group_digits("1.1234").c_str());
    ASSERT_STREQ("123,456", group_digits("123456").c_str());
    ASSERT_STREQ("123,456.789123", group_digits("123456.789123").c_str());

    ASSERT_STREQ("1",             group_digits("1").c_str());
    ASSERT_STREQ("11",            group_digits("11").c_str());
    ASSERT_STREQ("111",           group_digits("111").c_str());
    ASSERT_STREQ("1,111",         group_digits("1111").c_str());
    ASSERT_STREQ("11,111",        group_digits("11111").c_str());
    ASSERT_STREQ("111,111",       group_digits("111111").c_str());
    ASSERT_STREQ("1,111,111",     group_digits("1111111").c_str());
    ASSERT_STREQ("11,111,111",    group_digits("11111111").c_str());
    ASSERT_STREQ("111,111,111",   group_digits("111111111").c_str());
    ASSERT_STREQ("1,111,111,111", group_digits("1111111111").c_str());
}

TEST_F(strFixture, split)
{
    auto vec = split("a b c", " ");

    ASSERT_EQ(3, vec.size());
    ASSERT_STREQ("a", vec[0].c_str());
    ASSERT_STREQ("b", vec[1].c_str());
    ASSERT_STREQ("c", vec[2].c_str());
}

TEST_F(strFixture, codepoint_to_utf8)
{
    ASSERT_STREQ(" ", codepoint_to_utf8(0x20).c_str());
    ASSERT_STREQ("あ", codepoint_to_utf8(12354).c_str());
    ASSERT_STREQ("\xf0\x9f\x92\xa9", codepoint_to_utf8(0x1f4a9).c_str()); // PILE OF POO
}

TEST_F(strFixture, contains)
{
    ASSERT_TRUE(str::contains("abc", "bc"));
    ASSERT_FALSE(str::contains("abc", "d"));
    ASSERT_TRUE(str::contains("abc", ""));
    ASSERT_TRUE(str::contains("", ""));
    ASSERT_TRUE(str::contains("abc", "abc"));
    ASSERT_FALSE(str::contains("", "abc"));
}
