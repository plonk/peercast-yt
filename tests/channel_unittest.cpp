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

    // double              startTime;
    ASSERT_EQ(0, c.startTime);

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

#include "sstream.h"

TEST_F(ChannelFixture, writeVariable)
{
    Channel c;
    StringStream mem;

    c.info.name = "A&B";

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "name"));
    ASSERT_STREQ("A&amp;B", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "srcrate"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "genre"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "desc"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "comment"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "uptime"));
    ASSERT_STREQ("-", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "type"));
    ASSERT_STREQ("UNKNOWN", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "typeLong"));
    ASSERT_STREQ("UNKNOWN (application/octet-stream; ) [no styp] [no sext]", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "ext"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "localRelays"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "localListeners"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "totalRelays"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "totalListeners"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "status"));
    ASSERT_STREQ("NONE", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "keep"));
    ASSERT_STREQ("No", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "id"));
    ASSERT_STREQ("00000000000000000000000000000000", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "track.title"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "track.artist"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "track.album"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "track.genre"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "track.contactURL"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "contactURL"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "streamPos"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "sourceType"));
    ASSERT_STREQ("NONE", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "sourceProtocol"));
    ASSERT_STREQ("UNKNOWN", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "sourceURL"));
    ASSERT_STREQ("0.0.0.0:0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "headPos"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "headLen"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "buffer"));
    ASSERT_STRNE("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "headDump"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "numHits"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "authToken"));
    ASSERT_STREQ("e2ed47ef8004826db92f10a7e5a402e9", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(c.writeVariable(mem, "plsExt"));
    ASSERT_STREQ(".m3u", mem.str().c_str());
}
