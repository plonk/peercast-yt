#include <gtest/gtest.h>
#include "uptest.h"

class UptestInfoFixture : public ::testing::Test {
public:
    UptestInfoFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~UptestInfoFixture()
    {
    }

    UptestInfo i;
};


TEST_F(UptestInfoFixture, operator_eq)
{
    ASSERT_TRUE(UptestInfo() == i);

    UptestInfo j;
    j.name = "hoge";
    ASSERT_FALSE(j == i);
}

TEST_F(UptestInfoFixture, postURL)
{
    i.addr = "example.com";
    i.port = "443";
    i.object = "/yp/uptest.cgi";

    ASSERT_EQ("http://example.com:443/yp/uptest.cgi", i.postURL());
}
