#include <gtest/gtest.h>

#include "chandir.h"

class ChannelEntryFixture : public ::testing::Test {
public:
    ChannelEntryFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChannelEntryFixture()
    {
    }
};

TEST_F(ChannelEntryFixture, textToChannelEntries)
{
    auto vec = ChannelEntry::textToChannelEntries("予定地<>97968780D09CC97BB98D4A2BF221EDE7<>127.0.0.1:7144<>http://www.example.com/<>プログラミング<>peercastをいじる - &lt;Free&gt;<>-1<>-1<>428<>FLV<><><><><>%E4%BA%88%E5%AE%9A%E5%9C%B0<>1:14<>click<><>1\n");

    ASSERT_EQ(1, vec.size());

    ASSERT_STREQ("予定地", vec[0].name.c_str());
    ASSERT_EQ(428, vec[0].bitrate);
    ASSERT_STREQ("FLV", vec[0].contentTypeStr.c_str());
}
