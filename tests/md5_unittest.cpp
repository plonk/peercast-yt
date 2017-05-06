#include <gtest/gtest.h>

#include "md5.h"

class md5Fixture : public ::testing::Test {
public:
    md5Fixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~md5Fixture()
    {
    }
};

TEST_F(md5Fixture, test)
{
    auto out = md5::hexdigest("hello");
    ASSERT_STREQ("5d41402abc4b2a76b9719d911017c592", out.c_str());
}
