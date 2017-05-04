#include <gtest/gtest.h>

#include "stream.h"
#include "dmstream.h"

class MockStream : public Stream
{
public:
    MockStream()
    {
        readCount = 0;
        writeCount = 0;
        totalReadBytes = 0;
        totalWrittenBytes = 0;
    }

    int read(void *p, int len) override
    {
        readCount++;
        for (int i = 0; i < len; i++)
            ((char*)p)[i] = (i & 1) ? 'B' : 'A';
        totalReadBytes += len;
        return len;
    }

    void write(const void* p, int len) override
    {
        writeCount++;
        lastWriteData = std::string((char*)p, (char*)p+len);
        totalWrittenBytes += len;
    }

    int totalReadBytes;
    int totalWrittenBytes;
    int readCount;
    int writeCount;
    std::string lastWriteData;
};

class StreamFixture : public ::testing::Test {
public:
    StreamFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~StreamFixture()
    {
    }

    static void forward_write_vargs(Stream& stream, const char* fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        stream.write(fmt, ap);
        va_end(ap);
    }

    MockStream s;
    DynamicMemoryStream mem;
};

TEST_F(StreamFixture, readUpto)
{
    ASSERT_EQ(0, s.readUpto(NULL, 0));
}

TEST_F(StreamFixture, eof)
{
    ASSERT_THROW(s.eof(), StreamException);
}

TEST_F(StreamFixture, rewind)
{
    ASSERT_THROW(s.rewind(), StreamException);
}

TEST_F(StreamFixture, seekTo)
{
    ASSERT_THROW(s.seekTo(0), StreamException);
}

TEST_F(StreamFixture, writeTo)
{
    DynamicMemoryStream mem;
    ASSERT_EQ(0, s.readCount);
    s.writeTo(mem, 1);
    ASSERT_EQ(1, s.readCount);
    ASSERT_STREQ("A", mem.str().c_str());
}

TEST_F(StreamFixture, skip)
{
    ASSERT_EQ(0, s.readCount);

    ASSERT_NO_THROW(s.skip(0));
    ASSERT_EQ(0, s.readCount);

    ASSERT_NO_THROW(s.skip(1));
    ASSERT_EQ(1, s.readCount);
}

TEST_F(StreamFixture, close)
{
    ASSERT_NO_THROW(s.close());
}

TEST_F(StreamFixture, setReadTimeout)
{
    ASSERT_NO_THROW(s.setReadTimeout(0));
}

TEST_F(StreamFixture, setWriteTimeout)
{
    ASSERT_NO_THROW(s.setWriteTimeout(0));
}

TEST_F(StreamFixture, setPollRead)
{
    ASSERT_NO_THROW(s.setPollRead(false));
    ASSERT_NO_THROW(s.setPollRead(true));
}

TEST_F(StreamFixture, getPosition)
{
    ASSERT_EQ(0, s.getPosition());
}

TEST_F(StreamFixture, readChar)
{
    ASSERT_EQ('A', s.readChar());
}

TEST_F(StreamFixture, readShort)
{
    ASSERT_EQ(0x4241, s.readShort());
}

TEST_F(StreamFixture, readLong)
{
    ASSERT_EQ(0x42414241, s.readLong());
}

TEST_F(StreamFixture, readInt)
{
    ASSERT_EQ(0x42414241, s.readInt());
}

TEST_F(StreamFixture, readID4)
{
    ASSERT_STREQ("ABAB", s.readID4().getString().str());
}

TEST_F(StreamFixture, readInt24)
{
    ASSERT_EQ(0x00414241, s.readInt24());
}

TEST_F(StreamFixture, readTag)
{
    ASSERT_EQ('ABAB', s.readTag());
}

TEST_F(StreamFixture, readString)
{
    char buf[6] = "";
    ASSERT_EQ(5, s.readString(buf, 5));
    ASSERT_EQ(5, s.readCount);
    ASSERT_STREQ("AAAAA", buf);
}

TEST_F(StreamFixture, readReady)
{
    ASSERT_TRUE(s.readReady(0));
}

TEST_F(StreamFixture, numPending)
{
    ASSERT_EQ(0, s.numPending());
}

TEST_F(StreamFixture, writeID4)
{
    ID4 id("ruby");
    s.writeID4(id);
    ASSERT_STREQ("ruby", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeChar)
{
    s.writeChar('Z');
    ASSERT_STREQ("Z", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeShort)
{
    s.writeShort(0x4142);
    ASSERT_STREQ("BA", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeLong)
{
    s.writeLong(0x41424344);
    ASSERT_STREQ("DCBA", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeInt)
{
    s.writeInt(0x41424344);
    ASSERT_STREQ("DCBA", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeTag)
{
    s.writeTag("ABCD");
    ASSERT_STREQ("ABCD", s.lastWriteData.c_str());
}

TEST_F(StreamFixture, writeUTF8_null)
{
    mem.writeUTF8(0x00);
    ASSERT_EQ(1, mem.getLength());
    ASSERT_STREQ("", mem.str().c_str());
}

TEST_F(StreamFixture, writeUTF8_1)
{
    mem.writeUTF8(0x41);
    ASSERT_EQ(1, mem.getLength());
    ASSERT_STREQ("A", mem.str().c_str());
}

TEST_F(StreamFixture, writeUTF8_2)
{
    mem.writeUTF8(0x3b1);
    ASSERT_EQ(2, mem.getLength());
    ASSERT_STREQ("α", mem.str().c_str());
}

TEST_F(StreamFixture, writeUTF8_3)
{
    mem.writeUTF8(0x3042);
    ASSERT_EQ(3, mem.getLength());
    ASSERT_STREQ("あ", mem.str().c_str());
}

TEST_F(StreamFixture, writeUTF8_4)
{
    mem.writeUTF8(0x1f4a9);
    ASSERT_EQ(4, mem.getLength());
    ASSERT_STREQ("💩", mem.str().c_str());
}

TEST_F(StreamFixture, readLine)
{
    char buf[1024];

    memset(buf, 0, 1024);
    mem.str("abc");
    ASSERT_THROW(mem.readLine(buf, 1024), StreamException);

    memset(buf, 0, 1024);
    mem.str("abc\ndef");
    ASSERT_EQ(3, mem.readLine(buf, 1024));
    ASSERT_STREQ("abc", buf);

    memset(buf, 0, 1024);
    mem.str("abc\r\ndef");
    ASSERT_EQ(3, mem.readLine(buf, 1024));
    ASSERT_STREQ("abc", buf);

    // CR では停止しない
    memset(buf, 0, 1024);
    mem.str("abc\rdef");
    ASSERT_THROW(mem.readLine(buf, 1024), StreamException);

    // 行中の CR は削除される
    memset(buf, 0, 1024);
    mem.str("abc\rdef\r\n");
    ASSERT_EQ(6, mem.readLine(buf, 1024));
    ASSERT_STREQ("abcdef", buf);
}

TEST_F(StreamFixture, readWord_nullcase)
{
    char buf[1024] = "ABC";

    mem.str("");
    ASSERT_EQ(0, mem.readWord(buf, 1024));
    ASSERT_STREQ("", buf);
}

TEST_F(StreamFixture, readWord)
{
    char buf[1024];

    memset(buf, 0, 1024);
    mem.str("abc def");
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("abc", buf);
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("def", buf);

    memset(buf, 0, 1024);
    mem.str("abc\tdef");
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("abc", buf);
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("def", buf);

    memset(buf, 0, 1024);
    mem.str("abc\rdef");
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("abc", buf);
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("def", buf);

    memset(buf, 0, 1024);
    mem.str("abc\ndef");
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("abc", buf);
    ASSERT_EQ(3, mem.readWord(buf, 1024));
    ASSERT_STREQ("def", buf);
}

TEST_F(StreamFixture, readWord_bufferSize)
{
    char buf[1024] = "ABCD";

    // バッファーサイズが足りないと記録できなかった1文字は消えてしまう
    mem.str("abc");
    ASSERT_EQ(1, mem.readWord(buf, 2));
    ASSERT_STREQ("a", buf);
    ASSERT_EQ(1, mem.readWord(buf, 1024));
    ASSERT_STREQ("c", buf);
}

// このメソッド使い方わからないし、使われてないから消したいな。
TEST_F(StreamFixture, readBase64)
{
    // Base64.strict_encode64("foo")
    // => "Zm9v"

    mem.str("Zm9v");
    char buf[1024] = "AAAAAAAAA";
    ASSERT_EQ(3, mem.readBase64(buf, 5));
    ASSERT_STREQ("foo", buf);
}

TEST_F(StreamFixture, write_vargs)
{
    StreamFixture::forward_write_vargs(mem, "hello %s", "world");
    ASSERT_STREQ("hello world", mem.str().c_str());
    mem.str("");
    StreamFixture::forward_write_vargs(mem, "hello %d %d %d", 1, 2, 3);
    ASSERT_STREQ("hello 1 2 3", mem.str().c_str());}

TEST_F(StreamFixture, writeLine)
{
    ASSERT_TRUE(mem.writeCRLF);

    mem.writeLine("abc");
    ASSERT_STREQ("abc\r\n", mem.str().c_str());

    mem.str("");
    mem.writeCRLF = false;
    mem.writeLine("abc");
    ASSERT_STREQ("abc\n", mem.str().c_str());
}

TEST_F(StreamFixture, writeLineF)
{
    ASSERT_TRUE(mem.writeCRLF);

    mem.writeLineF("hello %s", "world");
    ASSERT_STREQ("hello world\r\n", mem.str().c_str());

    mem.str("");
    mem.writeCRLF = false;
    mem.writeLineF("hello %s", "world");
    ASSERT_STREQ("hello world\n", mem.str().c_str());
}

TEST_F(StreamFixture, writeString)
{
    mem.writeString("hello world");
    ASSERT_STREQ("hello world", mem.str().c_str());

    mem.str("");
    mem.writeString(std::string("hello world"));
    ASSERT_STREQ("hello world", mem.str().c_str());

    mem.str("");
    mem.writeString(String("hello world"));
    ASSERT_STREQ("hello world", mem.str().c_str());
}

TEST_F(StreamFixture, writeStringF)
{
    mem.writeStringF("hello %s", "world");
    ASSERT_STREQ("hello world", mem.str().c_str());
}

TEST_F(StreamFixture, readBits_nibbles)
{
    mem.str("\xde\xad\xbe\xef");
    ASSERT_EQ(0xd, mem.readBits(4));
    ASSERT_EQ(0xe, mem.readBits(4));
    ASSERT_EQ(0xa, mem.readBits(4));
    ASSERT_EQ(0xd, mem.readBits(4));
    ASSERT_EQ(0xb, mem.readBits(4));
    ASSERT_EQ(0xe, mem.readBits(4));
    ASSERT_EQ(0xe, mem.readBits(4));
    ASSERT_EQ(0xf, mem.readBits(4));
}

TEST_F(StreamFixture, readBits_bits)
{
    mem.str("\xaa");
    ASSERT_EQ(1, mem.readBits(1));
    ASSERT_EQ(0, mem.readBits(1));
    ASSERT_EQ(1, mem.readBits(1));
    ASSERT_EQ(0, mem.readBits(1));
    ASSERT_EQ(1, mem.readBits(1));
    ASSERT_EQ(0, mem.readBits(1));
    ASSERT_EQ(1, mem.readBits(1));
    ASSERT_EQ(0, mem.readBits(1));
}

TEST_F(StreamFixture, readBits_bytes)
{
    mem.str("\xaa");
    ASSERT_EQ(0xaa, mem.readBits(8));
}

TEST_F(StreamFixture, readBits_boundary)
{
    mem.str("\xde\xad");
    ASSERT_EQ(0xd, mem.readBits(4));
    ASSERT_EQ(0xea, mem.readBits(8));
    ASSERT_EQ(0xd, mem.readBits(4));
}

TEST_F(StreamFixture, readBits_4bytes)
{
    mem.str("\xde\xad\xbe\xef");
    ASSERT_EQ(0xdeadbeef, mem.readBits(32));
}

TEST_F(StreamFixture, totalBytesIn)
{
    ASSERT_EQ(0, s.totalBytesIn());
    s.updateTotals(1, 0);
    ASSERT_EQ(1, s.totalBytesIn());

    s.updateTotals(1, 0);
    ASSERT_EQ(2, s.totalBytesIn());
}

TEST_F(StreamFixture, totalBytesOut)
{
    ASSERT_EQ(0, s.totalBytesOut());
    s.updateTotals(0, 1);
    ASSERT_EQ(1, s.totalBytesOut());

    s.updateTotals(0, 1);
    ASSERT_EQ(2, s.totalBytesOut());
}

// 時間を進めないと変化しない。
TEST_F(StreamFixture, lastBytesIn)
{
    ASSERT_EQ(0, s.lastBytesIn());
    s.updateTotals(1, 0);
    ASSERT_EQ(0, s.lastBytesIn());
}

// 時間を進めないと変化しない。
TEST_F(StreamFixture, lastBytesOut)
{
    ASSERT_EQ(0, s.lastBytesOut());
    s.updateTotals(0, 1);
    ASSERT_EQ(0, s.lastBytesOut());
}
