#include <gtest/gtest.h>

#include "chandir.h"

class ChannelDirectoryFixture : public ::testing::Test {
public:
    ChannelDirectoryFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChannelDirectoryFixture()
    {
    }

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
