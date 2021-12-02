#include <gtest/gtest.h>
#include "common.h"

class GeneralExceptionFixture : public ::testing::Test {
public:
    GeneralExceptionFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~GeneralExceptionFixture()
    {
    }
};

TEST_F(GeneralExceptionFixture, messageOnly)
{
    GeneralException e("foobar");

    ASSERT_STREQ(e.msg, "foobar");
    ASSERT_EQ(e.err, 0);
}

TEST_F(GeneralExceptionFixture, withErrorCode)
{
    GeneralException e("hundred", 100);

    ASSERT_STREQ(e.msg, "hundred");
    ASSERT_EQ(e.err, 100);
}

TEST_F(GeneralExceptionFixture, what)
{
    GeneralException e("foobar");

    ASSERT_STREQ(e.what(), "foobar");
}
