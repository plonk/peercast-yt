#include <gtest/gtest.h>

#include <map>
#include <string>

using namespace std;

class mapFixture : public ::testing::Test {
public:
    mapFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~mapFixture()
    {
    }
    map<string,string> dic;
};

TEST_F(mapFixture, bracketsDoesNotThrow)
{
    ASSERT_EQ(0, dic.size());
    ASSERT_NO_THROW(dic["hoge"]);
    ASSERT_STREQ("", dic["hoge"].c_str());
    ASSERT_EQ(1, dic.size());
}

TEST_F(mapFixture, atThrows)
{
    ASSERT_THROW(dic.at("hoge"), std::out_of_range);
}
