#include <gtest/gtest.h>

#include "pcp.h"

class BroadcastStateFixture : public ::testing::Test {
public:
    BroadcastStateFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~BroadcastStateFixture()
    {
    }
    BroadcastState bcs;
};

TEST_F(BroadcastStateFixture, initialState)
{
    ASSERT_FALSE(bcs.chanID.isSet());
    ASSERT_FALSE(bcs.bcID.isSet());
    ASSERT_EQ(0, bcs.numHops);
    ASSERT_FALSE(bcs.forMe);
    ASSERT_EQ(0, bcs.streamPos);
    ASSERT_EQ(0, bcs.group);
}

TEST_F(BroadcastStateFixture, initPacketSettings)
{
    bcs.forMe = true;
    bcs.group = 1;
    bcs.numHops = 5;
    bcs.bcID.fromStr("DEADBEEFDEADBEEFDEADBEEFDEADBEEF");
    bcs.chanID.fromStr("DEADBEEFDEADBEEFDEADBEEFDEADBEEF");
    bcs.streamPos = 1234;

    bcs.initPacketSettings();

    ASSERT_FALSE(bcs.chanID.isSet());
    ASSERT_FALSE(bcs.bcID.isSet());
    ASSERT_EQ(0, bcs.numHops);
    ASSERT_FALSE(bcs.forMe);
    ASSERT_EQ(1234, bcs.streamPos); // Ç±ÇÍÇÕÉNÉäÉAÇ≥ÇÍÇ»Ç¢ÅB
    ASSERT_EQ(0, bcs.group);
}
