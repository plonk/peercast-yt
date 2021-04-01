#include <gtest/gtest.h>
#include "ip.h"

class IPFixture : public ::testing::Test {
public:
    IPFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~IPFixture()
    {
    }
};

TEST_F(IPFixture, isIPv4Mapped)
{
    IP ip(127<<24|1);
    ASSERT_TRUE(ip.isIPv4Mapped());
}
