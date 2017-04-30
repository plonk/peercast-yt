#include <gtest/gtest.h>
#include "str.h"

#include "servent.h"
#include "dmstream.h"

class ServentFixture : public ::testing::Test {
public:
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

    ASSERT_EQ(ChanInfo::SP_UNKNOWN, s.outputProtocol);

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

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("\r\n");

    s.handshakeHTTP(http, true);

    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                 mock->outgoing.str().c_str());
}

TEST_F(ServentFixture, handshakeIncomingGetRoot)
{
    Servent s(0);
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET / HTTP/1.0\r\n\r\n");

    s.handshakeIncoming();

    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                 mock->outgoing.str().c_str());
}

// servMgr->password が設定されていない時に ShoutCast クライアントから
// の放送要求だとして通してしまうが、良いのか？
TEST_F(ServentFixture, handshakeIncomingBadRequest)
{
    Servent s(0);
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("\r\n");

    ASSERT_THROW(s.handshakeIncoming(), StreamException);
}

TEST_F(ServentFixture, handshakeIncomingHTMLRoot)
{
    Servent s(0);
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /html/en/index.html HTTP/1.0\r\n\r\n");

    s.handshakeIncoming();

    std::string output = mock->outgoing.str();

    // ファイルが無いのに OK はおかしくないか…
    ASSERT_TRUE(str::contains(output, "200 OK"));
    ASSERT_TRUE(str::contains(output, "Server: "));
    ASSERT_TRUE(str::contains(output, "Date: "));
    ASSERT_TRUE(str::contains(output, "Unable to open file"));
}

TEST_F(ServentFixture, handshakeIncomingJRPCGetUnauthorized)
{
    Servent s(0);
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /api/1 HTTP/1.0\r\n\r\n");

    ASSERT_NO_THROW(s.handshakeIncoming());

    std::string output = mock->outgoing.str();

    ASSERT_STREQ("HTTP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"PeerCast Admin\"\r\n\r\n", output.c_str());
}

#include "servmgr.h"

TEST_F(ServentFixture, handshakeIncomingJRPCGetAuthorized)
{
    Servent s(0);
    MockClientSocket* mock;

    strcpy(servMgr->password, "Passw0rd");

    // --------------------------------------------
    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                       "\r\n");

    s.handshakeIncoming();

    ASSERT_TRUE(str::contains(mock->outgoing.str(), "401 Unauthorized"));

    delete mock;

    // --------------------------------------------

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                       "Authorization: BASIC OlBhc3N3MHJk\r\n" // ruby -rbase64 -e 'p Base64.strict_encode64 ":Passw0rd"'
                       "\r\n");

    s.handshakeIncoming();

    ASSERT_TRUE(str::contains(mock->outgoing.str(), "200 OK"));
    ASSERT_TRUE(str::contains(mock->outgoing.str(), "jsonrpc"));

    delete mock;
}

