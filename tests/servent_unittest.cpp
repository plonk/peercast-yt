#include <gtest/gtest.h>
#include "str.h"

#include "servent.h"
#include "sstream.h"

#include "regexp.h"

#include "mockclientsocket.h"

using namespace cgi;

class ServentFixture : public ::testing::Test {
public:
    ServentFixture()
        : s(0) {}

    void SetUp()
    {
        mock = new MockClientSocket();
        s.sock = mock;
    }

    void TearDown()
    {
        delete mock;
    }

    Servent s;
    MockClientSocket* mock;
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

    ASSERT_EQ(false, s.thread.active());

    ASSERT_STREQ("", s.loginPassword.cstr());
    ASSERT_STREQ("", s.loginMount.cstr());

    ASSERT_EQ(false, s.priorityConnect);
    ASSERT_EQ(false, s.addMetadata);

    ASSERT_EQ(0, s.nsSwitchNum);

    ASSERT_EQ(Servent::ALLOW_ALL, s.allow);

    ASSERT_EQ(mock, s.sock);
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

TEST_F(ServentFixture, handshakeHTTP)
{
    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("\r\n");

    s.handshakeHTTP(http, true);

    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                 mock->outgoing.str().c_str());
}

TEST_F(ServentFixture, handshakeIncomingGetRoot)
{
    mock->incoming.str("GET / HTTP/1.0\r\n\r\n");

    s.handshakeIncoming();

    ASSERT_STREQ("HTTP/1.0 302 Found\r\nLocation: /html/en/index.html\r\n\r\n",
                 mock->outgoing.str().c_str());
}

// servMgr->password が設定されていない時に ShoutCast クライアントから
// の放送要求だとして通してしまうが、良いのか？
TEST_F(ServentFixture, handshakeIncomingBadRequest)
{
    mock->incoming.str("\r\n");

    ASSERT_THROW(s.handshakeIncoming(), StreamException);
}

TEST_F(ServentFixture, handshakeIncomingHTMLRoot)
{
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
    mock->incoming.str("GET /api/1 HTTP/1.0\r\n\r\n");

    ASSERT_NO_THROW(s.handshakeIncoming());

    std::string output = mock->outgoing.str();

    ASSERT_STREQ("HTTP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"PeerCast Admin\"\r\n\r\n", output.c_str());
}

#include "servmgr.h"

TEST_F(ServentFixture, handshakeIncomingJRPCGetAuthorized_noAuthInfoSupplied)
{
    strcpy(servMgr->password, "Passw0rd");

    mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                       "\r\n");

    s.handshakeIncoming();

    ASSERT_TRUE(str::contains(mock->outgoing.str(), "401 Unauthorized"));
}

TEST_F(ServentFixture, handshakeIncomingJRPCGetAuthorized_authInfoSupplied)
{
    strcpy(servMgr->password, "Passw0rd");

    mock->incoming.str("GET /api/1 HTTP/1.0\r\n"
                       "Authorization: BASIC OlBhc3N3MHJk\r\n" // ruby -rbase64 -e 'p Base64.strict_encode64 ":Passw0rd"'
                       "\r\n");

    s.handshakeIncoming();

    ASSERT_TRUE(str::contains(mock->outgoing.str(), "200 OK"));
    ASSERT_TRUE(str::contains(mock->outgoing.str(), "jsonrpc"));
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

    mock->incoming.str("GET /" LONG_LONG_STRING " HTTP/1.0\r\n"
                       "\r\n");

    ASSERT_THROW(s.handshakeIncoming(), HTTPException);
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

TEST_F(ServentFixture, handshakeHTTPBasicAuth_nonlocal_correctpass)
{
    ASSERT_EQ(0, s.sock->host.ip);
    ASSERT_FALSE(s.sock->host.isLocalhost());

    strcpy(servMgr->password, "Passw0rd");

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("Authorization: BASIC OlBhc3N3MHJk\r\n\r\n");

    ASSERT_TRUE(s.handshakeHTTPBasicAuth(http));
}

TEST_F(ServentFixture, handshakeHTTPBasicAuth_nonlocal_wrongpass)
{
    ASSERT_EQ(0, s.sock->host.ip);
    ASSERT_FALSE(s.sock->host.isLocalhost());

    strcpy(servMgr->password, "hoge");

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("Authorization: BASIC OlBhc3N3MHJk\r\n\r\n");

    ASSERT_FALSE(s.handshakeHTTPBasicAuth(http));
}

TEST_F(ServentFixture, handshakeHTTPBasicAuth_local_correctpass)
{
    s.sock->host.ip = 127 << 24 | 1;
    ASSERT_TRUE(s.sock->host.isLocalhost());

    strcpy(servMgr->password, "Passw0rd");

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("Authorization: BASIC OlBhc3N3MHJk\r\n\r\n");

    ASSERT_TRUE(s.handshakeHTTPBasicAuth(http));
}

TEST_F(ServentFixture, handshakeHTTPBasicAuth_local_wrongpass)
{
    s.sock->host.ip = 127 << 24 | 1;
    ASSERT_TRUE(s.sock->host.isLocalhost());

    strcpy(servMgr->password, "hoge");

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("Authorization: BASIC OlBhc3N3MHJk\r\n\r\n");

    ASSERT_TRUE(s.handshakeHTTPBasicAuth(http));
}

TEST_F(ServentFixture, handshakeHTTPBasicAuth_noauthorizationheader)
{
    ASSERT_FALSE(s.sock->host.isLocalhost());

    strcpy(servMgr->password, "Passw0rd");

    HTTP http(*mock);
    http.initRequest("GET / HTTP/1.0");
    mock->incoming.str("\r\n");

    ASSERT_FALSE(s.handshakeHTTPBasicAuth(http));
    ASSERT_EQ("HTTP/1.0 401 Unauthorized\r\nWWW-Authenticate: Basic realm=\"PeerCast Admin\"\r\n\r\n", mock->outgoing.str());
}

TEST_F(ServentFixture, writeVariable)
{
    StringStream mem;

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "type"));
    ASSERT_STREQ("NONE", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "status"));
    ASSERT_STREQ("NONE", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "address"));
    ASSERT_STREQ("0.0.0.0:0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "agent"));
    ASSERT_STREQ("", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "bitrate"));
    ASSERT_STREQ("0.0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "bitrateAvg"));
    ASSERT_STREQ("0.0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "uptime"));
    ASSERT_STREQ("-", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.packetsIn"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.packetsInPerSec"));
    ASSERT_STREQ("0.0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.packetsOut"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.packetsOutPerSec"));
    ASSERT_STREQ("0.0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.normQueue"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.priQueue"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.flowControl"));
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    ASSERT_TRUE(s.writeVariable(mem, "gnet.routeTime"));
    ASSERT_STREQ("-", mem.str().c_str());
}

TEST_F(ServentFixture, handshakeIncoming_viewxml)
{
    s.sock->host = Host("127.0.0.1", 12345);
    mock->incoming.str("GET /admin?cmd=viewxml HTTP/1.0\r\n"
                       "\r\n");

    ASSERT_NO_THROW(s.handshakeIncoming());

    ASSERT_TRUE(str::has_prefix(mock->outgoing.str(), "HTTP/1.0 200 OK\r\n"));
}

TEST_F(ServentFixture, handshakeStream_emptyInput)
{
    ChanInfo info;

    ASSERT_THROW(s.handshakeStream(info), StreamException);
}

TEST_F(ServentFixture, handshakeStream_noHeaders)
{
    ChanInfo info;

    mock->incoming.str("\r\n");
    ASSERT_NO_THROW(s.handshakeStream(info));

    ASSERT_EQ("HTTP/1.0 404 Not Found\r\n\r\n", mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelNotFound)
{
    bool gotPCP = false;
    bool chanFound = false;
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    ASSERT_FALSE(
        s.handshakeStream_returnResponse(gotPCP, chanFound, chanReady, ch, chl, chanInfo));
    ASSERT_EQ("HTTP/1.0 404 Not Found\r\n\r\n", mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelNotReady_relay)
{
    bool gotPCP = true;
    bool chanFound = true;
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_PCP;
    // PCPハンドシェイクができない。
    ASSERT_THROW(
        s.handshakeStream_returnResponse(gotPCP, chanFound, chanReady, ch, chl, chanInfo),
        StreamException);
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelNotReady_direct)
{
    bool gotPCP = false;
    bool chanFound = true;
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    ASSERT_FALSE(
        s.handshakeStream_returnResponse(gotPCP, chanFound, chanReady, ch, chl, chanInfo));
    ASSERT_EQ("HTTP/1.0 503 Service Unavailable\r\n\r\n", mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelReady_relay)
{
    bool gotPCP = true;
    bool chanFound = true;
    bool chanReady = true;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_PCP;
    // PCPハンドシェイクができない。
    ASSERT_THROW(
        s.handshakeStream_returnResponse(gotPCP, chanFound, chanReady, ch, chl, chanInfo),
        StreamException);
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelReady_direct)
{
    bool gotPCP = false;
    bool chanFound = true;
    bool chanReady = true;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    ASSERT_TRUE(
        s.handshakeStream_returnResponse(gotPCP, chanFound, chanReady, ch, chl, chanInfo));
    Regexp regexp("\\A"
                  "HTTP/1\\.0 200 OK\\r\\n"
                  "Server: PeerCast/0\\.1218 \\(YT\\d\\d\\)\\r\\n"
                  "Accept-Ranges: none\\r\\n"
                  "x-audiocast-name: \\r\\n"
                  "x-audiocast-bitrate: 0\\r\\n"
                  "x-audiocast-genre: \\r\\n"
                  "x-audiocast-description: \\r\\n"
                  "x-audiocast-url: \\r\\n"
                  "x-peercast-channelid: 00000000000000000000000000000000\\r\\n"
                  "Content-Type: application/octet-stream\\r\\n\\r\\n"
                  "\\z");
    ASSERT_EQ(1, regexp.exec(mock->outgoing.str()).size());
}
