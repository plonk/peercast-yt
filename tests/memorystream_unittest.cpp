#include <gtest/gtest.h>
#include "stream.h"

class MemoryStreamFixture : public ::testing::Test {
public:
    MemoryStreamFixture()
        : one_byte_mm(1),
          hoge_mm(data, 4)
    {
        strcpy(data, "hoge");
    }

    void SetUp()
    {
        one_byte_mm.write("A", 1);
        one_byte_mm.rewind();
    }

    MemoryStream one_byte_mm;
    char data[5];
    MemoryStream hoge_mm;
};

// readUpto は実装されていないので何もせず 0 を返す。
TEST_F(MemoryStreamFixture, readUpto)
{
    char buf[1024] = "X";
    int result;

    result = one_byte_mm.readUpto(buf, 1);
    ASSERT_EQ(0, result);
    ASSERT_EQ(buf[0], 'X');
}

TEST_F(MemoryStreamFixture, PositionAdvancesOnWrite)
{
    ASSERT_EQ(1, one_byte_mm.len);
    ASSERT_EQ(0, one_byte_mm.getPosition());

    one_byte_mm.write("X", 1);

    ASSERT_EQ(1, one_byte_mm.len);
    ASSERT_EQ(1, one_byte_mm.getPosition());
}

TEST_F(MemoryStreamFixture, ThrowsExceptionIfCannotWrite)
{
    char buf[1024];

    // メモリーに収まらない write は StreamException を上げる。
    ASSERT_THROW(one_byte_mm.write("XXX", 3), StreamException);

    one_byte_mm.rewind();

    // エラーになった場合は1文字も書き込まれていない。
    one_byte_mm.read(buf, 1);
    ASSERT_EQ('A', buf[0]);
}


TEST_F(MemoryStreamFixture, ExternalMemory)
{
    // ASSERT_FALSE(hoge_mm.own);
    ASSERT_EQ(4, hoge_mm.len);
    ASSERT_EQ(0, hoge_mm.getPosition());
}


TEST_F(MemoryStreamFixture, read)
{
    char buf[1024];
    ASSERT_EQ(4, hoge_mm.read(buf, 4));
    ASSERT_EQ(0, strncmp(buf, "hoge", 4));
}

TEST_F(MemoryStreamFixture, seekAndRewind)
{
    ASSERT_EQ(0, hoge_mm.getPosition());
    hoge_mm.seekTo(4);
    ASSERT_EQ(4, hoge_mm.getPosition());
    hoge_mm.rewind();
    ASSERT_EQ(0, hoge_mm.getPosition());
}

TEST_F(MemoryStreamFixture, write)
{
    char buf[1024];

    hoge_mm.write("fuga", 4);
    hoge_mm.rewind();
    hoge_mm.read(buf, 4);
    ASSERT_EQ(0, strncmp(buf, "fuga", 4));
}
