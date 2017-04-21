#include <gtest/gtest.h>

#include "servent.h"
#include "dmstream.h"

class ServentFixture : public ::testing::Test {
public:
    ServentFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ServentFixture()
    {
    }
};

TEST_F(ServentFixture, initialState)
{
    Servent s(0);

    ASSERT_EQ(Servent::T_NONE, s.type);
    ASSERT_EQ(Servent::S_NONE, s.status);

    // static const char   *statusMsgs[], *typeMsgs[];

    // GnuStream           gnuStream;
    // GnuPacket           pack;

    ASSERT_EQ(0, s.lastConnect);
    ASSERT_EQ(0, s.lastPing);
    ASSERT_EQ(0, s.lastPacket);
    ASSERT_STREQ("", s.agent.cstr());

    ASSERT_EQ(0, s.seenIDs.numUsed());
    ASSERT_STREQ("00000000000000000000000000000000", s.networkID.str().c_str());
    ASSERT_EQ(0, s.serventIndex);

    ASSERT_STREQ("00000000000000000000000000000000", s.remoteID.str().c_str());
    ASSERT_STREQ("00000000000000000000000000000000", s.chanID.str().c_str());
    ASSERT_STREQ("00000000000000000000000000000000", s.givID.str().c_str());

    ASSERT_EQ(false, s.thread.active);

    ASSERT_STREQ("", s.loginPassword.cstr());
    ASSERT_STREQ("", s.loginMount.cstr());

    ASSERT_EQ(false, s.priorityConnect);
    ASSERT_EQ(false, s.addMetadata);

    ASSERT_EQ(0, s.nsSwitchNum);

    ASSERT_EQ(Servent::ALLOW_ALL, s.allow);

    ASSERT_EQ(nullptr, s.sock);
    ASSERT_EQ(nullptr, s.pushSock);

    // WLock               lock;

    ASSERT_EQ(true, s.sendHeader);
    // ASSERT_EQ(0, s.syncPos); // 不定
    // ASSERT_EQ(0, s.streamPos);  // 不定
    ASSERT_EQ(0, s.servPort);

    ASSERT_EQ(Servent::P_UNKNOWN, s.outputProtocol);

    // GnuPacketBuffer     outPacketsNorm, outPacketsPri;

    ASSERT_EQ(false, s.flowControl);

    ASSERT_EQ(nullptr, s.next);

    ASSERT_EQ(nullptr, s.pcpStream);

    ASSERT_EQ(0, s.cookie.time);
    ASSERT_STREQ("", s.cookie.id);
    ASSERT_EQ(0, s.cookie.ip);

}

#include "mockclientsocket.h"

TEST_F(ServentFixture, handshakeHTTP)
{
    Servent s(0);
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();

    HTTP http((Stream&)*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.writeString("\r\n");
    mock->incoming.rewind();

    s.handshakeHTTP(http, true);

    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                 mock->outgoing.str().c_str());
}
