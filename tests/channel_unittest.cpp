#include <gtest/gtest.h>

#include "channel.h"

class ChannelFixture : public ::testing::Test {
public:
    ChannelFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ChannelFixture()
    {
    }
};

TEST_F(ChannelFixture, initialState)
{
    Channel c;

    // ::String            mount;
    ASSERT_STREQ("", c.mount.cstr());
    // ChanMeta            insertMeta;
    // ChanPacket          headPack;

    // ChanPacketBuffer    rawData;

    // ChannelStream       *sourceStream;
    ASSERT_EQ(nullptr, c.sourceStream);
    // unsigned int        streamIndex;
    ASSERT_EQ(0, c.streamIndex);

    // ChanInfo            info;
    // ChanHit             sourceHost;
    // ChanHit             designatedHost;

    // GnuID               remoteID;
    ASSERT_FALSE(c.remoteID.isSet());

    // ::String            sourceURL;
    ASSERT_STREQ("", c.sourceURL.cstr());

    // bool                bump, stayConnected;
    ASSERT_FALSE(c.bump);
    ASSERT_FALSE(c.stayConnected);
    // int                 icyMetaInterval;
    ASSERT_EQ(0, c.icyMetaInterval);
    // unsigned int        streamPos;
    ASSERT_EQ(0, c.streamPos);
    // bool                readDelay;
    ASSERT_FALSE(c.readDelay);

    // TYPE                type;
    ASSERT_EQ(Channel::T_NONE, c.type);
    // ChannelSource       *sourceData;
    ASSERT_EQ(nullptr, c.sourceData);

    // SRC_TYPE            srcType;
    ASSERT_EQ(Channel::SRC_NONE, c.srcType);

    // MP3Header           mp3Head;
    // ThreadInfo          thread;

    // unsigned int        lastIdleTime;
    ASSERT_EQ(0, c.lastIdleTime);
    // STATUS              status;
    ASSERT_EQ(Channel::S_NONE, c.status);

    // ClientSocket        *sock;
    ASSERT_EQ(nullptr, c.sock);
    // ClientSocket        *pushSock;
    ASSERT_EQ(nullptr, c.pushSock);

    // unsigned int        lastTrackerUpdate;
    ASSERT_EQ(0, c.lastTrackerUpdate);
    // unsigned int        lastMetaUpdate;
    ASSERT_EQ(0, c.lastMetaUpdate);

    // double              startTime, syncTime;
    ASSERT_EQ(0, c.startTime);
    ASSERT_EQ(0, c.syncTime);

    // WEvent              syncEvent;

    // Channel             *next;
    ASSERT_EQ(nullptr, c.next);
}

TEST_F(ChannelFixture, renderHexDump)
{
    ASSERT_STREQ("",
                 Channel::renderHexDump("").c_str());
    ASSERT_STREQ("41                                               A\n",
                 Channel::renderHexDump("A").c_str());
    ASSERT_STREQ("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n",
                 Channel::renderHexDump("AAAAAAAAAAAAAAAA").c_str());
    ASSERT_STREQ("41 41 41 41 41 41 41 41 41 41 41 41 41 41 41 41  AAAAAAAAAAAAAAAA\n"
                 "41                                               A\n",
                 Channel::renderHexDump("AAAAAAAAAAAAAAAAA").c_str());
}
