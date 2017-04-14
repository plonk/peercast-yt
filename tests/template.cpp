#include <gtest/gtest.h>

class __Fixture : public ::testing::Test {
public:
    __Fixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~__Fixture()
    {
    }
};

TEST_F(__Fixture, )
{
}
