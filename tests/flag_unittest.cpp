#include <gtest/gtest.h>
#include "flag.h"

class FlagFixture : public ::testing::Test {
public:
    FlagFixture()
        : flag("name", "desc", true)
        , flag2("NAME", "DESC", false)
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~FlagFixture()
    {
    }

    Flag flag;
    Flag flag2;
};

TEST_F(FlagFixture, initialState)
{
    ASSERT_EQ("name", flag.name);
    ASSERT_EQ("desc", flag.desc);
    ASSERT_EQ(true, flag.defaultValue);
    ASSERT_EQ(true, flag.currentValue);
}

TEST_F(FlagFixture, initialState2)
{
    ASSERT_EQ("NAME", flag2.name);
    ASSERT_EQ("DESC", flag2.desc);
    ASSERT_EQ(false, flag2.defaultValue);
    ASSERT_EQ(false, flag2.currentValue);
}

TEST_F(FlagFixture, toBool)
{
    ASSERT_TRUE((bool) flag);
    ASSERT_FALSE((bool) flag2);
}

TEST_F(FlagFixture, assignBool)
{
    ASSERT_TRUE(flag);
    ASSERT_TRUE(flag.defaultValue);
    flag = false;
    ASSERT_FALSE(flag);
    ASSERT_TRUE(flag.defaultValue);
}




