#include <gtest/gtest.h>

#include "regexp.h"

class RegexpFixture : public ::testing::Test {
public:
    RegexpFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~RegexpFixture()
    {
    }
};

TEST_F(RegexpFixture, nomatch)
{
    Regexp pat("abc");
    ASSERT_EQ(0, pat.exec("hoge").size());
}

TEST_F(RegexpFixture, match)
{
    Regexp pat("abc");
    ASSERT_EQ(1, pat.exec("abc").size());
    ASSERT_EQ("abc", pat.exec("abc")[0]);
}

TEST_F(RegexpFixture, captures)
{
    Regexp pat("(.)(.)(.)");
    auto captures = pat.exec("abc");
    ASSERT_EQ(4, captures.size());
    ASSERT_EQ("abc", captures[0]);
    ASSERT_EQ("a", captures[1]);
    ASSERT_EQ("b", captures[2]);
    ASSERT_EQ("c", captures[3]);
}

TEST_F(RegexpFixture, caretMatchesLineBeginning)
{
    Regexp pat("^a");
    auto captures = pat.exec("b\na");
    ASSERT_EQ(1, captures.size());
    ASSERT_EQ("a", captures[0]);
}

TEST_F(RegexpFixture, backslashCapitalADoesntMatchLineBeginning)
{
    Regexp pat("\\Aa");
    auto captures = pat.exec("b\na");
    ASSERT_EQ(0, captures.size());
}

TEST_F(RegexpFixture, matches)
{
    ASSERT_TRUE(Regexp("").matches("abc"));
    ASSERT_TRUE(Regexp("abc").matches("abc"));
    ASSERT_FALSE(Regexp("def").matches("abc"));
    ASSERT_FALSE(Regexp("def").matches(""));
}

TEST_F(RegexpFixture, copy)
{
    Regexp r1("^$");
    Regexp r2(r1);
    ASSERT_TRUE(r1.matches(""));
    ASSERT_TRUE(r1.matches("\n"));
    ASSERT_TRUE(r2.matches(""));
    ASSERT_TRUE(r2.matches("\n"));
}

