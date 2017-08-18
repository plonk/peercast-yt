#include <gtest/gtest.h>
#include "logbuf.h"
#include "sstream.h"

class LogBufferFixture : public ::testing::Test {
public:
    LogBufferFixture()
        : lb(1000, 100) // nlines, lineLength
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~LogBufferFixture()
    {
    }

    LogBuffer lb;
};

TEST_F(LogBufferFixture, initialState)
{
    ASSERT_NE(nullptr, lb.buf);
    ASSERT_NE(nullptr, lb.times);
    ASSERT_NE(nullptr, lb.types);

    ASSERT_EQ(0, lb.currLine);
    ASSERT_EQ(1000, lb.maxLines);
    ASSERT_EQ(100, lb.lineLen);
}

TEST_F(LogBufferFixture, dumpHTML_nullCase)
{
    StringStream mem;
    lb.dumpHTML(mem);
    ASSERT_EQ("", mem.str());
}

TEST_F(LogBufferFixture, dumpHTML_simple)
{
    StringStream mem;

    lb.write("<>&ほげ", LogBuffer::T_ERROR);
    lb.dumpHTML(mem);

    ASSERT_EQ("Thu Jan  1 09:00:00 1970\n <b>[EROR]</b> &lt;&gt;&amp;ほげ<br>", mem.str());
}

TEST_F(LogBufferFixture, write)
{
    ASSERT_EQ(0, lb.currLine);
    lb.write("hoge", LogBuffer::T_ERROR);
    ASSERT_EQ(1, lb.currLine);

    ASSERT_STREQ("hoge", &lb.buf[0]);
    ASSERT_EQ(LogBuffer::T_ERROR, lb.types[0]);
}

TEST_F(LogBufferFixture, getTypeStr)
{
    ASSERT_STREQ("", LogBuffer::getTypeStr(LogBuffer::T_NONE));
    ASSERT_STREQ("TRAC", LogBuffer::getTypeStr(LogBuffer::T_TRACE));
    ASSERT_STREQ("DBUG", LogBuffer::getTypeStr(LogBuffer::T_DEBUG));
    ASSERT_STREQ("INFO", LogBuffer::getTypeStr(LogBuffer::T_INFO));
    ASSERT_STREQ("WARN", LogBuffer::getTypeStr(LogBuffer::T_WARN));
    ASSERT_STREQ("EROR", LogBuffer::getTypeStr(LogBuffer::T_ERROR));
    ASSERT_STREQ("FATL", LogBuffer::getTypeStr(LogBuffer::T_FATAL));
    ASSERT_STREQ(" OFF", LogBuffer::getTypeStr(LogBuffer::T_OFF));
}
