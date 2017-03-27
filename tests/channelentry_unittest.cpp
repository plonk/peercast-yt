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

TEST_F(ChannelEntryFixture, constructor)
{
    ASSERT_THROW(ChannelEntry({}), std::runtime_error);
}

TEST_F(ChannelEntryFixture, textToChannelEntries)
{
    auto vec = ChannelEntry::textToChannelEntries("予定地<>97968780D09CC97BB98D4A2BF221EDE7<>127.0.0.1:7144<>http://www.example.com/<>プログラミング<>peercastをいじる - &lt;Free&gt;<>-1<>-1<>428<>FLV<><><><><>%E4%BA%88%E5%AE%9A%E5%9C%B0<>1:14<>click<><>1\n");

    ASSERT_EQ(1, vec.size());

    auto& entry = vec[0];

    ASSERT_STREQ("予定地", entry.name.c_str());
    ASSERT_STREQ("97968780D09CC97BB98D4A2BF221EDE7", ((std::string) entry.id).c_str());
    ASSERT_EQ(428, entry.bitrate);
    ASSERT_STREQ("FLV", entry.contentTypeStr.c_str());
    ASSERT_STREQ("peercastをいじる - &lt;Free&gt;", entry.desc.c_str());
    ASSERT_STREQ("プログラミング", entry.genre.c_str());
    ASSERT_STREQ("http://www.example.com/", entry.url.c_str());
    ASSERT_STREQ("127.0.0.1:7144", entry.tip.c_str());
    ASSERT_STREQ("1:14", entry.uptime.c_str());
    ASSERT_EQ(-1, entry.numDirects);
    ASSERT_EQ(-1, entry.numRelays);
}
