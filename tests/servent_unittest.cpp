#include <gtest/gtest.h>
#include "str.h"

#include "servent.h"
#include "sstream.h"

#include "regexp.h"

#include "mockclientsocket.h"
#include "version2.h"

using namespace cgi;

class ServentFixture : public ::testing::Test {
public:
    ServentFixture()
        : s(0) {}

    void SetUp()
    {
        mock = std::make_shared<MockClientSocket>();
        s.sock = mock;
    }

    void TearDown()
    {
    }

    Servent s;
    std::shared_ptr<MockClientSocket> mock;
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
    ASSERT_EQ(0, s.streamPos);
    ASSERT_EQ(0, s.servPort);

    ASSERT_EQ(ChanInfo::SP_UNKNOWN, s.outputProtocol);

    // GnuPacketBuffer     outPacketsNorm, outPacketsPri;

    ASSERT_EQ(nullptr, s.next);

    ASSERT_EQ(nullptr, s.pcpStream);

    ASSERT_STREQ("", s.cookie.id);
    ASSERT_EQ(0, s.cookie.ip);

}

TEST_F(ServentFixture, resetResetsCookie)
{
    strcpy(s.cookie.id, "hoge");
    s.cookie.ip = IP(127 << 3 | 1);

    s.reset();

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

TEST_F(ServentFixture, handshakeIncomingNonexistentFile)
{
    // ローカルホストでなければログインフォームが出る。
    mock->host.fromStrIP("127.0.0.1", 0);

    mock->incoming.str("GET /html/en/nonexistent.html HTTP/1.0\r\n\r\n");

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
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = nullptr;
    const ChanInfo chanInfo;

    ASSERT_FALSE(
        s.handshakeStream_returnResponse(gotPCP, chanReady, ch, chl, chanInfo));
    ASSERT_EQ("HTTP/1.0 404 Not Found\r\n\r\n", mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelNotReady_relay)
{
    bool gotPCP = true;
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = new ChanHitList();
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_PCP;
    // PCPハンドシェイクができない。
    ASSERT_THROW(
        s.handshakeStream_returnResponse(gotPCP, chanReady, ch, chl, chanInfo),
        StreamException);
    delete chl;
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelNotReady_direct)
{
    bool gotPCP = false;
    bool chanReady = false;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = new ChanHitList();
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    ASSERT_FALSE(
        s.handshakeStream_returnResponse(gotPCP, chanReady, ch, chl, chanInfo));
    ASSERT_EQ("HTTP/1.0 503 Service Unavailable\r\n\r\n", mock->outgoing.str());
    delete chl;
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelReady_relay)
{
    bool gotPCP = true;
    bool chanReady = true;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = new ChanHitList();
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_PCP;
    // PCPハンドシェイクができない。
    ASSERT_THROW(
        s.handshakeStream_returnResponse(gotPCP, chanReady, ch, chl, chanInfo),
        StreamException);
    delete chl;
}

TEST_F(ServentFixture, handshakeStream_returnResponse_channelReady_direct)
{
    bool gotPCP = false;
    bool chanReady = true;
    std::shared_ptr<Channel> ch = nullptr;
    ChanHitList* chl = new ChanHitList();
    const ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    ASSERT_TRUE(
        s.handshakeStream_returnResponse(gotPCP, chanReady, ch, chl, chanInfo));
    Regexp regexp("^"
                  "HTTP/1\\.0 200 OK\\r\\n"
                  "Server: PeerCast/0\\.1218 \\(YT\\d\\d\\)\\r\\n"
                  "Accept-Ranges: none\\r\\n"
                  "x-audiocast-name: \\r\\n"
                  "x-audiocast-bitrate: 0\\r\\n"
                  "x-audiocast-genre: \\r\\n"
                  "x-audiocast-description: \\r\\n"
                  "x-audiocast-url: \\r\\n"
                  "x-peercast-channelid: 00000000000000000000000000000000\\r\\n"
                  "Access-Control-Allow-Origin: \\*\\r\\n"
                  "Content-Type: application/octet-stream\\r\\n\\r\\n"
                  "$");
    ASSERT_TRUE(regexp.matches(mock->outgoing.str()));
    delete chl;
}

TEST_F(ServentFixture, handshakeStream_returnHits)
{
    AtomStream atom(*mock);
    GnuID channelID;
    ChanHitList* chl = nullptr;
    Host rhost;

    s.handshakeStream_returnHits(atom, channelID, chl, rhost);
    // quit int 1003
    ASSERT_EQ(std::string({'q','u','i','t', 4,0,0,0, (char)0xeb,0x03,0,0 }), mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnStreamHeaders)
{
    AtomStream atom(*mock);
    std::shared_ptr<Channel> ch = nullptr;
    ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    s.handshakeStream_returnStreamHeaders(atom, ch, chanInfo);

    ASSERT_EQ("HTTP/1.0 200 OK\r\n"
              "Server: " PCX_AGENT "\r\n"
              "Accept-Ranges: none\r\n"
              "x-audiocast-name: \r\n"
              "x-audiocast-bitrate: 0\r\n"
              "x-audiocast-genre: \r\n"
              "x-audiocast-description: \r\n"
              "x-audiocast-url: \r\n"
              "x-peercast-channelid: 00000000000000000000000000000000\r\n"
              "Access-Control-Allow-Origin: *\r\n"
              "Content-Type: application/octet-stream\r\n"
              "\r\n",
              mock->outgoing.str());
}

TEST_F(ServentFixture, handshakeStream_returnStreamHeaders_mov)
{
    // ストリームタイプが MOV の時は、以下の2つのヘッダーが追加される。
    // Connection: close
    // Content-Length: 10000000

    AtomStream atom(*mock);
    std::shared_ptr<Channel> ch = nullptr;
    ChanInfo chanInfo;

    s.outputProtocol = ChanInfo::SP_HTTP;
    chanInfo.contentType = ChanInfo::T_MOV;
    s.handshakeStream_returnStreamHeaders(atom, ch, chanInfo);

    ASSERT_EQ("HTTP/1.0 200 OK\r\n"
              "Server: " PCX_AGENT "\r\n"
              "Accept-Ranges: none\r\n"
              "x-audiocast-name: \r\n"
              "x-audiocast-bitrate: 0\r\n"
              "x-audiocast-genre: \r\n"
              "x-audiocast-description: \r\n"
              "x-audiocast-url: \r\n"
              "x-peercast-channelid: 00000000000000000000000000000000\r\n"
              "Connection: close\r\n"
              "Content-Length: 10000000\r\n"
              "Access-Control-Allow-Origin: *\r\n"
              "Content-Type: video/quicktime\r\n"
              "\r\n",
              mock->outgoing.str());
}

TEST_F(ServentFixture, denialReasonToName)
{
    ASSERT_STREQ("None", Servent::denialReasonToName(Servent::StreamRequestDenialReason::None));
    ASSERT_STREQ("InsufficientBandwidth", Servent::denialReasonToName(Servent::StreamRequestDenialReason::InsufficientBandwidth));
    ASSERT_STREQ("PerChannelRelayLimit", Servent::denialReasonToName(Servent::StreamRequestDenialReason::PerChannelRelayLimit));
    ASSERT_STREQ("RelayLimit", Servent::denialReasonToName(Servent::StreamRequestDenialReason::RelayLimit));
    ASSERT_STREQ("DirectLimit", Servent::denialReasonToName(Servent::StreamRequestDenialReason::DirectLimit));
    ASSERT_STREQ("NotPlaying", Servent::denialReasonToName(Servent::StreamRequestDenialReason::NotPlaying));
    ASSERT_STREQ("Other", Servent::denialReasonToName(Servent::StreamRequestDenialReason::Other));
    ASSERT_STREQ("unknown", Servent::denialReasonToName((Servent::StreamRequestDenialReason)100));
}

TEST_F(ServentFixture, canStream_null_channel)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;

    ASSERT_FALSE(s.canStream(nullptr, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::Other, reason);
}

TEST_F(ServentFixture, canStream_disabled)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();
    auto prev = servMgr->isDisabled;

    servMgr->isDisabled = true;
    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::Other, reason);
    servMgr->isDisabled = prev;
}

TEST_F(ServentFixture, canStream_notPlaying)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();

    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::NotPlaying, reason);
}

TEST_F(ServentFixture, canStream_isPrivate)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();

    ASSERT_EQ("0.0.0.0:0", s.sock->host.str());
    ASSERT_FALSE(s.isPrivate());
    s.sock->host.fromStrIP("127.0.0.1", 7144);
    ASSERT_TRUE(s.isPrivate());

    ASSERT_TRUE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::None, reason);
}

TEST_F(ServentFixture, canStream_insufficientBandwidth)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();

    ASSERT_EQ(0, servMgr->maxBitrateOut);
    servMgr->maxBitrateOut = 1;
    ch->info.bitrate = 2;

    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::InsufficientBandwidth, reason);

    servMgr->maxBitrateOut = 0;
}

TEST_F(ServentFixture, canStream_relaysFull)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();
    auto prev = servMgr->maxRelays;

    s.type = Servent::T_RELAY;
    servMgr->maxRelays = 0;
    ASSERT_TRUE(servMgr->relaysFull());

    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::RelayLimit, reason);
    servMgr->maxRelays = prev;
}

TEST_F(ServentFixture, canStream_directFull)
{
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<Channel>();
    auto prev = servMgr->maxDirect;

    s.type = Servent::T_DIRECT;
    servMgr->maxDirect = 0;
    ASSERT_TRUE(servMgr->directFull());

    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::DirectLimit, reason);

    servMgr->maxDirect = prev;
}

TEST_F(ServentFixture, canStream_perChannelRelayLimit)
{
    class MockChannel : public Channel {
        bool isFull() override { return true; }
    };
    Servent::StreamRequestDenialReason reason = Servent::StreamRequestDenialReason::None;
    auto ch = std::make_shared<MockChannel>();

    s.type = Servent::T_RELAY;
    ch->status = Channel::S_RECEIVING;

    ASSERT_FALSE(s.canStream(ch, &reason));
    ASSERT_EQ(Servent::StreamRequestDenialReason::PerChannelRelayLimit, reason);
}

TEST_F(ServentFixture, isTerminationCandidate_port7144_nonST)
{
    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 1;

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 0;
        h.numRelays = 0;

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 0;
        h.numRelays = 1;

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }
}

TEST_F(ServentFixture, isTerminationCandidate_port7144_ST)
{
    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 1;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 0;
        h.numRelays = 0;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 7144;
        h.relay = 0;
        h.numRelays = 1;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }
}

TEST_F(ServentFixture, isTerminationCandidate_port0_nonST)
{
    {
        ChanHit h;

        h.host.port = 0;
        h.relay = 1;

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 0;
        h.relay = 0;
        h.numRelays = 0;

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 0;
        h.relay = 0;
        h.numRelays = 1;

        ASSERT_FALSE(Servent::isTerminationCandidate(&h));
    }
}

TEST_F(ServentFixture, isTerminationCandidate_port0_ST)
{
    {
        ChanHit h;

        h.host.port = 0;
        h.relay = 1;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        h.host.port = 0;
        h.relay = 0;
        h.numRelays = 0;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }

    {
        ChanHit h;

        // ありえない…。
        h.host.port = 0;
        h.relay = 0;
        h.numRelays = 1;
        h.versionExPrefix[0] = 'S';
        h.versionExPrefix[1] = 'T';

        ASSERT_TRUE(Servent::isTerminationCandidate(&h));
    }
}

TEST_F(ServentFixture, fileNameToMimeType)
{
    ASSERT_STREQ(MIME_HTML, Servent::fileNameToMimeType("a.htm"));
    ASSERT_STREQ(MIME_HTML, Servent::fileNameToMimeType("a.html"));
    ASSERT_STREQ(MIME_HTML, Servent::fileNameToMimeType("a.HTM"));
    ASSERT_STREQ(MIME_HTML, Servent::fileNameToMimeType("a.HTML"));
    ASSERT_STREQ(MIME_CSS, Servent::fileNameToMimeType("a.css"));
    ASSERT_STREQ(MIME_JPEG, Servent::fileNameToMimeType("a.jpg"));
    ASSERT_STREQ(nullptr, Servent::fileNameToMimeType("a.jpeg"));
    ASSERT_STREQ(MIME_GIF, Servent::fileNameToMimeType("a.gif"));
    ASSERT_STREQ(MIME_PNG, Servent::fileNameToMimeType("a.png"));
    ASSERT_STREQ(MIME_JS, Servent::fileNameToMimeType("a.js"));
    ASSERT_STREQ(MIME_ICO, Servent::fileNameToMimeType("a.ico"));
    ASSERT_STREQ(nullptr, Servent::fileNameToMimeType("a.txt"));
}

TEST_F(ServentFixture, writeHeloAtom_all)
{
    StringStream s;
    AtomStream atom(s);

    //void Servent::writeHeloAtom(AtomStream &atom, bool sendPort, bool sendPing, bool sendBCID, const GnuID& sessionID, uint16_t port, const GnuID& broadcastID)
    Servent::writeHeloAtom(atom, true, true, true, GnuID("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), 7144, GnuID("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));

    s.rewind();

    int nchildren, size;
    ID4 id;
    char buf[17] = "";

    id = atom.read(nchildren, size);
    ASSERT_STREQ("helo", id.getString().str());
    ASSERT_EQ(6, nchildren);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("agnt", id.getString().str());
    ASSERT_TRUE( size > 0 );
    atom.skip(0, size);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("ver", id.getString().str());
    ASSERT_EQ(4, size);
    atom.skip(0, 4);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("sid", id.getString().str());
    ASSERT_EQ(16, size);
    atom.readBytes(buf, 16);
    ASSERT_STREQ("\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
                 buf);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("port", id.getString().str());
    ASSERT_EQ(2, size);
    ASSERT_EQ(7144, atom.readShort());

    id = atom.read(nchildren, size);
    ASSERT_STREQ("ping", id.getString().str());
    ASSERT_EQ(2, size);
    ASSERT_EQ(7144, atom.readShort());

    id = atom.read(nchildren, size);
    ASSERT_STREQ("bcid", id.getString().str());
    ASSERT_EQ(16, size);
    atom.readBytes(buf, 16);
    ASSERT_STREQ("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff",
                 buf);

    ASSERT_EQ(s.str().size(), s.getPosition());
}

TEST_F(ServentFixture, writeHeloAtom_none)
{
    StringStream s;
    AtomStream atom(s);

    //void Servent::writeHeloAtom(AtomStream &atom, bool sendPort, bool sendPing, bool sendBCID, const GnuID& sessionID, uint16_t port, const GnuID& broadcastID)
    Servent::writeHeloAtom(atom, false, false, false, GnuID("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), 7144, GnuID("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF"));

    s.rewind();

    int nchildren, size;
    ID4 id;
    char buf[17] = "";

    id = atom.read(nchildren, size);
    ASSERT_STREQ("helo", id.getString().str());
    ASSERT_EQ(3, nchildren);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("agnt", id.getString().str());
    ASSERT_TRUE( size > 0 );
    atom.skip(0, size);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("ver", id.getString().str());
    ASSERT_EQ(4, size);
    atom.skip(0, 4);

    id = atom.read(nchildren, size);
    ASSERT_STREQ("sid", id.getString().str());
    ASSERT_EQ(16, size);
    atom.readBytes(buf, 16);
    ASSERT_STREQ("\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa\xaa",
                 buf);

    ASSERT_EQ(s.str().size(), s.getPosition());
}
