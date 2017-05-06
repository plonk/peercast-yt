#include <gtest/gtest.h>

#include "channel.h"
#include "version2.h"
#include "md5.h"

class ChanMgrFixture : public ::testing::Test {
public:
    void SetUp()
    {
        x = new ChanMgr();
    }

    void TearDown()
    {
        delete x;
    }
    ChanMgr* x;
};

TEST_F(ChanMgrFixture, constants)
{
    ASSERT_EQ(8, ChanMgr::MAX_IDLE_CHANNELS);
    ASSERT_EQ(8192, ChanMgr::MAX_METAINT);
}

TEST_F(ChanMgrFixture, initialState)
{
    GnuID id;
    ChanInfo info;

    id.clear();

    ASSERT_EQ(NULL, x->channel);
    ASSERT_EQ(NULL, x->hitlist);
    ASSERT_EQ(PCP_BROADCAST_FLAGS, x->broadcastID.getFlags());

    ASSERT_TRUE(id.isSame(x->searchInfo.id));
    ASSERT_TRUE(id.isSame(x->searchInfo.bcID));
    // ...

    // ASSERT_EQ(0, x->numFinds); // 初期化されない。
    ASSERT_EQ(String(), x->broadcastMsg);
    EXPECT_EQ(10, x->broadcastMsgInterval);
    EXPECT_EQ(0, x->lastHit);
    EXPECT_EQ(0, x->lastQuery);
    EXPECT_EQ(0, x->maxUptime);
    // EXPECT_EQ(true, x->searchActive); // 初期化されない。
    EXPECT_EQ(600, x->deadHitAge);
    EXPECT_EQ(8192, x->icyMetaInterval);
    EXPECT_EQ(0, x->maxRelaysPerChannel);
    EXPECT_EQ(1, x->minBroadcastTTL);
    EXPECT_EQ(7, x->maxBroadcastTTL);
    EXPECT_EQ(60, x->pushTimeout);
    EXPECT_EQ(5, x->pushTries);
    EXPECT_EQ(8, x->maxPushHops);
    EXPECT_EQ(0, x->autoQuery);
    EXPECT_EQ(10, x->prefetchTime);
    EXPECT_EQ(0, x->lastYPConnect);
    // EXPECT_EQ(0, x->lastYPConnect2);
    EXPECT_EQ(0, x->icyIndex);
    EXPECT_EQ(120, x->hostUpdateInterval);
    EXPECT_EQ(5, x->bufferTime);
    EXPECT_TRUE(id.isSame(x->currFindAndPlayChannel));
}

TEST_F(ChanMgrFixture, createChannel)
{
    ChanInfo info;
    Channel *c;

    ASSERT_EQ(NULL, x->channel);

    c = x->createChannel(info, NULL);

    ASSERT_TRUE(c);
    ASSERT_EQ(c, x->channel);

    x->deleteChannel(c);
}

TEST_F(ChanMgrFixture, authSecret)
{
    ASSERT_STREQ("00151515151515151515151515151515:01234567890123456789012345678901", x->authSecret("01234567890123456789012345678901").c_str());
}

TEST_F(ChanMgrFixture, authToken)
{
    ASSERT_STREQ("44d5299e57ad9274fee7960a9fa60bfd", x->authToken("01234567890123456789012345678901").c_str());
    ASSERT_STREQ("44d5299e57ad9274fee7960a9fa60bfd", md5::hexdigest("00151515151515151515151515151515:01234567890123456789012345678901").c_str());
}
