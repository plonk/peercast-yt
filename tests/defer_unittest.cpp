#include <gtest/gtest.h>

#include "defer.h"

class DeferFixture : public ::testing::Test {
public:
    DeferFixture()
    {
    }

    void SetUp()
    {
        y = false;
    }

    void TearDown()
    {
    }

    void makeTrueAlways()
    {
        Defer makeTrue([=]() { y = true; });
        throw std::runtime_error("Oops!");
    }


    ~DeferFixture()
    {
    }
    bool y;
};

TEST_F(DeferFixture, makeTrue)
{
    bool x = false;
    {
        Defer makeTrue([&]() { x = true; });
        ASSERT_FALSE(x);
    }
    ASSERT_TRUE(x);
}

TEST_F(DeferFixture, makeTrueAlways)
{
    ASSERT_FALSE(y);

    ASSERT_THROW(makeTrueAlways(), std::runtime_error);
    ASSERT_TRUE(y);
}
