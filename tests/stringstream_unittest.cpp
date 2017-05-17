#include <gtest/gtest.h>

#include "sstream.h"

class StringStreamFixture : public ::testing::Test {
public:
};

TEST_F(StringStreamFixture, initialState)
{
    StringStream s;

    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(0, s.getLength());

    ASSERT_TRUE(s.eof());
    ASSERT_THROW(s.read(NULL, 0), StreamException);
}

TEST_F(StringStreamFixture, writeAdvancesPositionAndLength)
{
    StringStream s;

    s.write("hoge", 4);
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());
}

TEST_F(StringStreamFixture, writeAdvancesPositionAndLengthNullCase)
{
    StringStream s;

    s.write("", 0);
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(0, s.getLength());
}

TEST_F(StringStreamFixture, rewindResetsPosition)
{
    StringStream s;

    s.write("hoge", 4);
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(true, s.eof());
    s.rewind();
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(false, s.eof());
}

TEST_F(StringStreamFixture, whatIsWrittenCanBeRead)
{
    StringStream s;

    char buf[5] = "";

    s.write("hoge", 4);
    ASSERT_THROW(s.read(buf, 4), StreamException);
    s.rewind();
    ASSERT_EQ(4, s.read(buf, 100));
    ASSERT_STREQ("hoge", buf);
}

TEST_F(StringStreamFixture, seekToChangesPosition)
{
    StringStream s;

    s.write("hoge", 4);
    s.seekTo(1);
    ASSERT_EQ(1, s.getPosition());

    char buf[4] = "";
    s.read(buf, 3);
    ASSERT_STREQ("oge", buf);
}

TEST_F(StringStreamFixture, seekToChangesLength)
{
    StringStream s;

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

TEST_F(StringStreamFixture, strGet)
{
    StringStream s;

    s.writeString("hoge");
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());

    ASSERT_STREQ("hoge", s.str().c_str());
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());
}

TEST_F(StringStreamFixture, strSet)
{
    StringStream s;

    s.writeString("hoge");
    ASSERT_EQ(4, s.getPosition());
    ASSERT_EQ(4, s.getLength());

    s.str("foo");
    ASSERT_EQ(0, s.getPosition());
    ASSERT_EQ(3, s.getLength());

    ASSERT_STREQ("foo", s.str().c_str());
}

