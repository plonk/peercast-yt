#include <gtest/gtest.h>

#include "asf.h"

class ASFStreamFixture : public ::testing::Test {
public:
    ASFStreamFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ASFStreamFixture()
    {
    }

    ASFStream s;
};

TEST_F(ASFStreamFixture, initialState)
{
    // the fields are not initialized!
}

TEST_F(ASFStreamFixture, reset)
{
    s.reset();
    ASSERT_EQ(0, s.id);
    ASSERT_EQ(0, s.bitrate);
    ASSERT_EQ(ASFStream::T_UNKNOWN, s.type);
}

TEST_F(ASFStreamFixture, getTypeName)
{
    s.reset();
    ASSERT_STREQ("Unknown", s.getTypeName());

    s.type = ASFStream::T_VIDEO;
    ASSERT_STREQ("Video", s.getTypeName());

    s.type = ASFStream::T_AUDIO;
    ASSERT_STREQ("Audio", s.getTypeName());
}
