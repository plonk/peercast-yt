#include <gtest/gtest.h>

#include "dmstream.h"

class DynamicMemoryStreamFixture : public ::testing::Test {
public:
    DynamicMemoryStreamFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~DynamicMemoryStreamFixture()
    {
    }
};

TEST_F(DynamicMemoryStreamFixture, initialState)
{
    DynamicMemoryStream s;

    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(0, s.getLength());

    ASSERT_TRUE(s.eof());
    ASSERT_THROW(s.read(NULL, 0), StreamException);
}

TEST_F(DynamicMemoryStreamFixture, writeAdvancesPositionAndLength)
{
    DynamicMemoryStream s;

    s.write("hoge", 4);
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());
}

TEST_F(DynamicMemoryStreamFixture, writeAdvancesPositionAndLengthNullCase)
{
    DynamicMemoryStream s;

    s.write("", 0);
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(0, s.getLength());
}

TEST_F(DynamicMemoryStreamFixture, rewindResetsPosition)
{
    DynamicMemoryStream s;

    s.write("hoge", 4);
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(true, s.eof());
    s.rewind();
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(false, s.eof());
}

TEST_F(DynamicMemoryStreamFixture, whatIsWrittenCanBeRead)
{
    DynamicMemoryStream s;

    char buf[5] = "";

    s.write("hoge", 4);
    ASSERT_THROW(s.read(buf, 4), StreamException);
    s.rewind();
    ASSERT_EQ(4, s.read(buf, 100));
    ASSERT_STREQ("hoge", buf);
}

TEST_F(DynamicMemoryStreamFixture, seekToChangesPosition)
{
    DynamicMemoryStream s;

    s.write("hoge", 4);
    s.seekTo(1);
    ASSERT_EQ(1, s.getPosition());

    char buf[4] = "";
    s.read(buf, 3);
    ASSERT_STREQ("oge", buf);
}

TEST_F(DynamicMemoryStreamFixture, seekToChangesLength)
{
    DynamicMemoryStream s;

    s.seekTo(1000);
    ASSERT_EQ(1000, s.getLength());

    char buf[1000];
    buf[0] = 0xff;
    buf[999] = 0xff;

    s.rewind();
    s.read(buf, 1000);
    ASSERT_EQ(0, buf[0]);
    ASSERT_EQ(0, buf[999]);
}

TEST_F(DynamicMemoryStreamFixture, strGet)
{
    DynamicMemoryStream s;

    s.writeString("hoge");
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());

    ASSERT_STREQ("hoge", s.str().c_str());
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());
}

TEST_F(DynamicMemoryStreamFixture, strSet)
{
    DynamicMemoryStream s;

    s.writeString("hoge");
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());

    s.str("foo");
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(3, s.getLength());

    ASSERT_STREQ("foo", s.str().c_str());
}

