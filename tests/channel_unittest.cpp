#include <gtest/gtest.h>

#include "channel.h"

class ChannelFixture : public ::testing::Test {
public:
    ChannelFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChannelFixture()
    {
    }
};

TEST_F(ChannelFixture, renderHexDump)
{
    ASSERT_STREQ("",
                 Channel::renderHexDump("").c_str());
    ASSERT_STREQ("41                                               A\n",
                 Channel::renderHexDump("A").c_str());
    ASSERT_STREQ("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n",
                 Channel::renderHexDump("AAAAAAAAAAAAAAAA").c_str());
    ASSERT_STREQ("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n"
                 "41                                               A\n",
                 Channel::renderHexDump("AAAAAAAAAAAAAAAAA").c_str());
}
