#include <gtest/gtest.h>

#include "cstream.h"

class ChanPacketBufferFixture : public ::testing::Test {
public:
    ChanPacketBufferFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChanPacketBufferFixture()
    {
    }

    ChanPacketBuffer data;
};

TEST_F(ChanPacketBufferFixture, init)
{
    data.init();

    ASSERT_EQ(0, data.lastPos);
    ASSERT_EQ(0, data.firstPos);
    ASSERT_EQ(0, data.safePos);
    ASSERT_EQ(0, data.readPos);
    ASSERT_EQ(0, data.writePos);
    ASSERT_EQ(0, data.accept);
    ASSERT_EQ(0, data.lastWriteTime);

    ASSERT_EQ(0, data.numPending());
}

TEST_F(ChanPacketBufferFixture, addPacket)
{
    ChanPacket packet;

    packet.type = ChanPacket::T_DATA;
    packet.len = 8192;
    packet.pos = 0;

    data.writePacket(packet);

    EXPECT_EQ(0, data.lastPos);
    EXPECT_EQ(0, data.firstPos);
    EXPECT_EQ(0, data.safePos);
    EXPECT_EQ(0, data.readPos);
    EXPECT_EQ(1, data.writePos);
    EXPECT_EQ(0, data.accept);
    EXPECT_EQ(0, data.lastWriteTime);
    EXPECT_EQ(1, data.numPending());
}
