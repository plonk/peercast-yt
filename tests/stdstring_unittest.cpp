#include <gtest/gtest.h>

#include <string>

using namespace std;

class stdStringFixture : public ::testing::Test {
public:
    stdStringFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~stdStringFixture()
    {
    }
};

TEST(stdStringFixture, size)
{
    const char *p = "a\0b";
    ASSERT_EQ(3, string(p, p + 3).size());

    // char* から初期化する場合は \0 で切れる。
    ASSERT_EQ(1, string(p).size());
}

TEST(stdStringFixture, equals)
{
    ASSERT_TRUE(string("a") != string("b"));
    ASSERT_TRUE(string("a") != string("ab"));
    ASSERT_TRUE(string("a\0b") == string("a"));
    ASSERT_TRUE(string("abc") == string("abc\0"));
}

TEST(stdStringFixture, substr)
{
    string s = "0123456789";

    // 2つ目の引数は位置ではなく長さ。
    ASSERT_STREQ("23", s.substr(2, 2).c_str());
}

TEST(stdStringFixture, find)
{
    string s = "0123456789";

    ASSERT_EQ(string::npos, s.find('A'));
    ASSERT_EQ(-1, string::npos);
}

TEST(stdStringFixture, initializerList)
{
    string s = { 0, 1 };

    ASSERT_EQ(2, s.size());
    ASSERT_EQ(0, s[0]);
    ASSERT_EQ(1, s[1]);
}

