#include <gtest/gtest.h>

#include "public.h"

class PublicControllerFixture : public ::testing::Test {
public:
    PublicControllerFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~PublicControllerFixture()
    {
    }
};

TEST_F(PublicControllerFixture, formatUptime)
{
    ASSERT_STREQ("00:00", PublicController::formatUptime(0).c_str());
    ASSERT_STREQ("00:00", PublicController::formatUptime(1).c_str());
    ASSERT_STREQ("00:00", PublicController::formatUptime(59).c_str());
    ASSERT_STREQ("00:01", PublicController::formatUptime(60).c_str());
    ASSERT_STREQ("00:59", PublicController::formatUptime(3600 - 1).c_str());
    ASSERT_STREQ("01:00", PublicController::formatUptime(3600).c_str());
    ASSERT_STREQ("99:59", PublicController::formatUptime(100 * 3600 - 1).c_str());
    ASSERT_STREQ("100:00", PublicController::formatUptime(100 * 3600).c_str());
}
