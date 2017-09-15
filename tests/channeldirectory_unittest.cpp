#include <gtest/gtest.h>

#include "chandir.h"

class ChannelDirectoryFixture : public ::testing::Test {
public:
    ChannelDirectory dir;
};

// TEST_F(ChannelDirectoryFixture, update)
// {
//     bool res = dir.update();

//     ASSERT_EQ(false, res);
// }

TEST_F(ChannelDirectoryFixture, findTracker)
{
    ChannelEntry entry({ "", "01234567890123456789012345678901", "127.0.0.1:7144", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, "http://example.com/index.txt");

    dir.m_channels.push_back(entry);
    GnuID id("01234567890123456789012345678901");
    ASSERT_STREQ("127.0.0.1:7144", dir.findTracker(id).c_str());

    GnuID id2("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF");
    ASSERT_STREQ("", dir.findTracker(id2).c_str());
}

#include "sstream.h"

TEST_F(ChannelDirectoryFixture, writeChannelVariable)
{
    StringStream mem;
    ChannelEntry entry({ "NAME", "01234567890123456789012345678901", "127.0.0.1:7144", "URL", "GENRE", "DESC", "100", "200", "1000", "WMV", "ARTIST", "ALBUM", "TRACK_NAME", "TRACK_CONTACT", "%20", "0:10", "click", "COMMENT", "1" }, "http://example.com/index.txt");

    ASSERT_FALSE(dir.writeChannelVariable(mem, "name", 0));

    dir.m_channels.push_back(entry);

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "name", 0));
    EXPECT_STREQ("NAME", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "id", 0));
    EXPECT_STREQ("01234567890123456789012345678901", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "bitrate", 0));
    EXPECT_STREQ("1000", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "contentTypeStr", 0));
    EXPECT_STREQ("WMV", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "desc", 0));
    EXPECT_STREQ("DESC", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "genre", 0));
    EXPECT_STREQ("GENRE", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "url", 0));
    EXPECT_STREQ("URL", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "tip", 0));
    EXPECT_STREQ("127.0.0.1:7144", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "encodedName", 0));
    EXPECT_STREQ("%20", mem.str().c_str()); // undecoded

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "uptime", 0));
    EXPECT_STREQ("0:10", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "numDirects", 0));
    EXPECT_STREQ("100", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "numRelays", 0));
    EXPECT_STREQ("200", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "chatUrl", 0));
    EXPECT_STREQ("http://example.com/chat.php?cn=%20", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "statsUrl", 0));
    EXPECT_STREQ("http://example.com/getgmt.php?cn=%20", mem.str().c_str());

    mem.str("");
    EXPECT_TRUE(dir.writeChannelVariable(mem, "isPlayable", 0));
    EXPECT_STREQ("1", mem.str().c_str());

}

TEST_F(ChannelDirectoryFixture, numChannels)
{
    ASSERT_EQ(0, dir.numChannels());
}

TEST_F(ChannelDirectoryFixture, numFeeds)
{
    ASSERT_EQ(0, dir.numFeeds());
}

TEST_F(ChannelDirectoryFixture, feeds)
{
    ASSERT_TRUE(dir.feeds().empty());
}

TEST_F(ChannelDirectoryFixture, totalListeners)
{
    ASSERT_EQ(0, dir.totalListeners());
}

TEST_F(ChannelDirectoryFixture, totalRelays)
{
    ASSERT_EQ(0, dir.totalRelays());
}

TEST_F(ChannelDirectoryFixture, channels)
{
    ASSERT_TRUE(dir.channels().empty());
}
