#include <gtest/gtest.h>

#include "cstream.h"

class ChanPacketFixture : public ::testing::Test {
public:
    ChanPacketFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChanPacketFixture()
    {
    }

    ChanPacket p;
};

TEST_F(ChanPacketFixture, initialState)
{
    ASSERT_EQ(ChanPacket::T_UNKNOWN, p.type);
    ASSERT_EQ(0, p.len);
    ASSERT_EQ(0, p.pos);
    ASSERT_EQ(0, p.sync);

    // we don't know what's in p.data[]
}

TEST_F(ChanPacketFixture, init)
{
    p.type = ChanPacket::T_DATA;
    p.len = 1;
    p.pos = 2;
    p.sync = 3;

    p.init();

    ASSERT_EQ(ChanPacket::T_UNKNOWN, p.type);
    ASSERT_EQ(0, p.len);
    ASSERT_EQ(0, p.pos);
    ASSERT_EQ(0, p.sync);
}
