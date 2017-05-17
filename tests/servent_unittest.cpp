#include <gtest/gtest.h>
#include "str.h"

#include "servent.h"
#include "sstream.h"

#include "defer.h"

using namespace cgi;

class ServentFixture : public ::testing::Test {
public:
    ServentFixture()
        : s(0) {}
    Servent s;
};

TEST_F(ServentFixture, initialState)
{
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
    MockClientSocket* mock;
    Defer reclaim([&]() { delete mock; });

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
    MockClientSocket* mock;
    Defer reclaim([&]() { delete mock; });

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
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("\r\n");

    ASSERT_THROW(s.handshakeIncoming(), StreamException);

    delete mock;
}

TEST_F(ServentFixture, handshakeIncomingHTMLRoot)
{
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

    delete mock;
}

TEST_F(ServentFixture, handshakeIncomingJRPCGetUnauthorized)
{
    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /api/1 HTTP/1.0\r\n\r\n");

    ASSERT_NO_THROW(s.handshakeIncoming());

    std::string output = mock->outgoing.str();

    ASSERT_STREQ("HTTP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"PeerCast Admin\"\r\n\r\n", output.c_str());

    delete mock;
}

#include "servmgr.h"

TEST_F(ServentFixture, handshakeIncomingJRPCGetAuthorized)
{
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

//                  |<---------------------- 77 characters long  -------------------------------->|
#define LONG_STRING "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI" \
    "longURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURIlongURI"

// 8470 bytes
#define LONG_LONG_STRING LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING \
    LONG_STRING

// 8191 バイト以上のリクエストに対してエラーを返す。
TEST_F(ServentFixture, handshakeIncomingLongURI)
{
    ASSERT_EQ(8470, strlen(LONG_LONG_STRING));

    MockClientSocket* mock;

    s.sock = mock = new MockClientSocket();
    mock->incoming.str("GET /" LONG_LONG_STRING " HTTP/1.0\r\n"
                       "\r\n");

    ASSERT_THROW(s.handshakeIncoming(), HTTPException);

    delete mock;
}

TEST_F(ServentFixture, createChannelInfoNullCase)
{
    Query query("");
    auto info = s.createChannelInfo(GnuID(), String(), query, "");

    ASSERT_EQ(ChanInfo::T_UNKNOWN, info.contentType);
    ASSERT_STREQ("", info.name.cstr());
    ASSERT_STREQ("", info.genre.cstr());
    ASSERT_STREQ("", info.desc.cstr());
    ASSERT_STREQ("", info.url.cstr());
    ASSERT_EQ(0, info.bitrate);
    ASSERT_STREQ("", info.comment);
}

TEST_F(ServentFixture, createChannelInfoComment)
{
    Query query("");
    auto info = s.createChannelInfo(GnuID(), "俺たちみんなトドだぜ (・ω・｀з)3", query, "");

    ASSERT_STREQ("俺たちみんなトドだぜ (・ω・｀з)3", info.comment);
}

TEST_F(ServentFixture, createChannelInfoCommentOverride)
{
    Query query("comment=スレなし");
    auto info = s.createChannelInfo(GnuID(), "俺たちみんなトドだぜ (・ω・｀з)3", query, "");

    ASSERT_STREQ("スレなし", info.comment);
}

TEST_F(ServentFixture, createChannelInfoTypicalCase)
{
    Query query("name=予定地&genre=テスト&desc=てすと&url=http://example.com&comment=スレなし&bitrate=400&type=mkv");
    auto info = s.createChannelInfo(GnuID(), String(), query, "");

    ASSERT_EQ(ChanInfo::T_MKV, info.contentType);
    ASSERT_STREQ("予定地", info.name.cstr());
    ASSERT_STREQ("テスト", info.genre.cstr());
    ASSERT_STREQ("てすと", info.desc.cstr());
    ASSERT_STREQ("http://example.com", info.url.cstr());
    ASSERT_EQ(400, info.bitrate);
    ASSERT_STREQ("スレなし", info.comment);
}

TEST_F(ServentFixture, createChannelInfoNonnumericBitrate)
{
    Query query("bitrate=BITRATE");
    ChanInfo info;

    ASSERT_NO_THROW(info = s.createChannelInfo(GnuID(), String(), query, ""));
    ASSERT_EQ(0, info.bitrate);
}

TEST_F(ServentFixture, hasValidAuthToken)
{
    ASSERT_TRUE(s.hasValidAuthToken("01234567890123456789012345678901.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
    ASSERT_FALSE(s.hasValidAuthToken("01234567890123456789012345678901.flv?auth=00000000000000000000000000000000"));
    ASSERT_FALSE(s.hasValidAuthToken("01234567890123456789012345678901.flv?"));
    ASSERT_FALSE(s.hasValidAuthToken("01234567890123456789012345678901.flv"));
    ASSERT_FALSE(s.hasValidAuthToken(""));
    ASSERT_FALSE(s.hasValidAuthToken("ほげほげ.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
    ASSERT_FALSE(s.hasValidAuthToken("ほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげほげ.flv?auth=44d5299e57ad9274fee7960a9fa60bfd"));
    ASSERT_FALSE(s.hasValidAuthToken("?auth=44d5299e57ad9274fee7960a9fa60bfd"));
}
