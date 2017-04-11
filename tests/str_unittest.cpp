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
}
