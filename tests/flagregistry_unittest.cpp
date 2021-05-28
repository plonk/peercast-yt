#include <gtest/gtest.h>

#include "flag.h"

class FlagRegistoryFixture : public ::testing::Test {
public:
    FlagRegistoryFixture()
        : reg({{"flag1", "desc1", true},
               {"flag2", "desc2", false}})
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~FlagRegistoryFixture()
    {
    }

    FlagRegistory reg;
};

TEST_F(FlagRegistoryFixture, get)
{
    ASSERT_TRUE(reg.get("flag1"));
    ASSERT_FALSE(reg.get("flag2"));

    ASSERT_THROW(reg.get("flag3"), std::out_of_range);
}

TEST_F(FlagRegistoryFixture, forEachFlag)
{
    std::string str;
    int count = 0;

    reg.forEachFlag([&](Flag& f){ str += f.name; count++; });

    ASSERT_EQ(2, count);
    ASSERT_TRUE(str == "flag1flag2" || str == "flag2flag1");
}
