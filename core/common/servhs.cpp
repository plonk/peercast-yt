// ------------------------------------------------
// File : servhs.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      Servent handshaking, TODO: should be in its own class
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include "servent.h"
#include "servmgr.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "version2.h"
#include "jrpc.h"
#include "playlist.h"
#include "str.h"
#include "cgi.h"
#include "gnutella.h"

using namespace std;

// -----------------------------------
static void termArgs(char *str)
{
    if (str)
    {
        int slen = strlen(str);
        for (int i=0; i<slen; i++)
            if (str[i]=='&') str[i] = 0;
    }
}

// -----------------------------------
const char *nextCGIarg(const char *cp, char *cmd, char *arg)
{
    if (!*cp)
        return NULL;

    int cnt=0;

    // fetch command
    while (*cp)
    {
        char c = *cp++;
        if (c == '=')
            break;
        else
            *cmd++ = c;

        cnt++;
        if (cnt >= (MAX_CGI_LEN-1))
            break;
    }
    *cmd = 0;

    cnt=0;
    // fetch arg
    while (*cp)
    {
        char c = *cp++;
        if (c == '&')
            break;
        else
            *arg++ = c;

        cnt++;
        if (cnt >= (MAX_CGI_LEN-1))
            break;
    }
    *arg = 0;

    return cp;
}

// -----------------------------------
bool getCGIargBOOL(char *a)
{
    return (strcmp(a, "1") == 0);
}

// -----------------------------------
int getCGIargINT(char *a)
{
    return atoi(a);
}

// -----------------------------------
void Servent::handshakeJRPC(HTTP &http)
{
    int content_length = -1;

    string lenstr = http.headers.get("Content-Length");
    if (!lenstr.empty())
        content_length = atoi(lenstr.c_str());

    if (content_length == -1)
        throw HTTPException("HTTP/1.0 411 Length required", 411);

    if (content_length == 0)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    unique_ptr<char> body(new char[content_length + 1]);
    try {
        http.stream->read(body.get(), content_length);
        body.get()[content_length] = '\0';
    }catch (EOFException&)
    {
        // body too short
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }

    JrpcApi api;
    std::string response = api.call(body.get());

    http.writeLine(HTTP_SC_OK);
    http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    http.writeLineF("%s %zu", HTTP_HS_LENGTH, response.size());
    http.writeLineF("%s %s", HTTP_HS_CONTENT, "application/json");
    http.writeLine("");

    http.write(response.c_str(), response.size());
}

// -----------------------------------
bool Servent::hasValidAuthToken(const std::string& requestFilename)
{
    auto vec = str::split(requestFilename, "?");

    if (vec.size() != 2)
        return false;

    cgi::Query query(vec[1]);

    auto token = query.get("auth");
    auto chanid = str::upcase(vec[0].substr(0, 32));
    auto validToken = chanMgr->authToken(chanid);

    if (validToken == token)
    {
        return true;
    }

    return false;
}

// -----------------------------------
#include "subprog.h"
#include "env.h"
#include "regexp.h"

// -----------------------------------
void Servent::handshakeGET(HTTP &http)
{
    char *fn = http.cmdLine + 4;

    char *pt = strstr(fn, HTTP_PROTO1);
    if (pt)
        pt[-1] = 0;

    if (strncmp(fn, "/admin?", 7) == 0)
    {
        // フォーム投稿用エンドポイント

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Admin client");
        handshakeCMD(http, fn+7);
    }else if (strncmp(fn, "/pls/", 5) == 0)
    {
        // プレイリスト

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        ChanInfo info;
        if (servMgr->getChannel(fn+5, info, isPrivate() || hasValidAuthToken(fn+5)))
        {
            http.readHeaders(); // this smashes fn
            LOG_DEBUG("User-Agent: %s", http.headers.get("User-Agent").c_str());
            handshakePLS(info);
        }else
        {
            http.readHeaders();
            throw HTTPException(HTTP_SC_NOTFOUND, 404);
        }
    }else if (strncmp(fn, "/stream/", 8) == 0)
    {
        // ストリーム

        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_DIRECT) || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        triggerChannel(fn+8, ChanInfo::SP_HTTP, isPrivate() || hasValidAuthToken(fn+8));
    }else if (strncmp(fn, "/channel/", 9) == 0)
    {
        if (!sock->host.isLocalhost())
            if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
                throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        triggerChannel(fn+9, ChanInfo::SP_PCP, false);
    }else if (strcmp(fn, "/api/1") == 0)
    {
        // JSON RPC バージョン情報取得用

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeHTTPBasicAuth(http))
        {
            JrpcApi api;
            std::string response = api.getVersionInfo(nlohmann::json::array_t()).dump();

            http.writeLine(HTTP_SC_OK);
            http.writeLineF("%s %zu", HTTP_HS_LENGTH, response.size());
            http.writeLine("");
            http.writeString(response.c_str());
        }
    }else
    {
        // GET マッチなし

        http.readHeaders();
        throw HTTPException(HTTP_SC_NOTFOUND, 404);
    }
}

// -----------------------------------
void Servent::handshakePOST(HTTP &http)
{
    LOG_DEBUG("cmdLine: %s", http.cmdLine);

    auto vec = str::split(http.cmdLine, " ");
    if (vec.size() != 3)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    std::string args;
    auto vec2 = str::split(vec[1], "?", 2);

    if (vec2.size() == 2)
        args = vec2[1];

    std::string path = vec2[0];

    if (strcmp(path.c_str(), "/api/1") == 0)
    {
        // JSON API

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeHTTPBasicAuth(http))
            handshakeJRPC(http);
    }else if (strcmp(path.c_str(), "/") == 0)
    {
        // HTTP Push

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        if (!isPrivate())
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        handshakeHTTPPush(args);
    }else
    {
        http.readHeaders();
        auto contentType = http.headers.get("Content-Type");
        if (contentType == "application/x-wms-pushsetup")
        {
            // WMHTTP

            if (!isAllowed(ALLOW_BROADCAST))
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);

            if (!isPrivate())
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);

            handshakeWMHTTPPush(http, path);
        }else
        {
            // POST マッチなし

            throw HTTPException(HTTP_SC_BADREQUEST, 400);
        }
    }
}

// -----------------------------------
void Servent::handshakeGIV(const char *requestLine)
{
    HTTP(*sock).readHeaders();

    if (!isAllowed(ALLOW_NETWORK))
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    GnuID id;

    auto *idstr = strstr(requestLine, "/");
    if (idstr)
        id.fromStr(idstr+1);

    char ipstr[64];
    sock->host.toStr(ipstr);

    if (id.isSet())
    {
        // at the moment we don`t really care where the GIV came from, so just give to chan. no. if its waiting.
        auto ch = chanMgr->findChannelByID(id);

        if (!ch)
            throw HTTPException(HTTP_SC_NOTFOUND, 404);

        if (!ch->acceptGIV(sock))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Accepted GIV channel %s from: %s", idstr, ipstr);
        sock = NULL;                  // release this servent but dont close socket.
    }else
    {
        if (!servMgr->acceptGIV(sock))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Accepted GIV PCP from: %s", ipstr);
        sock = NULL;                  // release this servent but dont close socket.
    }
}

// -----------------------------------
void Servent::handshakeSOURCE(char * in, bool isHTTP)
{
    if (!isAllowed(ALLOW_BROADCAST))
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    char *mount = NULL;

    char *ps;
    if ((ps = strstr(in, "ICE/1.0")) != nullptr)
    {
        mount = in+7;
        *ps = 0;
        LOG_DEBUG("ICE 1.0 client to %s", mount?mount:"unknown");
    }else{
        mount = in+strlen(in);
        while (*--mount)
            if (*mount == '/')
            {
                mount[-1] = 0; // password preceeds
                break;
            }
        loginPassword.set(in+7);

        LOG_DEBUG("ICY client: %s %s", loginPassword.cstr(), mount?mount:"unknown");
    }

    if (mount)
        loginMount.set(mount);

    handshakeICY(Channel::SRC_ICECAST, isHTTP);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::handshakeHTTP(HTTP &http, bool isHTTP)
{
    if (http.isRequest("GET /"))
    {
        handshakeGET(http);
    }else if (http.isRequest("POST /"))
    {
        handshakePOST(http);
    }else if (http.isRequest("GIV"))
    {
        // Push リレー

        handshakeGIV(http.cmdLine);
    }else if (http.isRequest(PCX_PCP_CONNECT)) // "pcp"
    {
        // CIN

        if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        processIncomingPCP(true);
    }else if (http.isRequest("SOURCE"))
    {
        // Icecast 放送

        handshakeSOURCE(http.cmdLine, isHTTP);
    }else if (http.isRequest(servMgr->password)) // FIXME: check for empty password!
    {
        // ShoutCast broadcast

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        loginPassword.set(servMgr->password);   // pwd already checked

        sock->writeLine("OK2");
        sock->writeLine("icy-caps:11");
        sock->writeLine("");
        LOG_DEBUG("ShoutCast client");

        handshakeICY(Channel::SRC_SHOUTCAST, isHTTP);
        sock = NULL;    // socket is taken over by channel, so don`t close it
    }else
    {
        // リクエスト解釈失敗

        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

// -----------------------------------
void Servent::handshakeIncoming()
{
    setStatus(S_HANDSHAKE);

    char buf[8192];

    if ((size_t)sock->readLine(buf, sizeof(buf)) >= sizeof(buf)-1)
    {
        throw HTTPException(HTTP_SC_URITOOLONG, 414);
    }

    bool isHTTP = (stristr(buf, HTTP_PROTO1) != NULL);

    if (isHTTP)
        LOG_DEBUG("HTTP from %s '%s'", sock->host.str().c_str(), buf);
    else
        LOG_DEBUG("Connect from %s '%s'", sock->host.str().c_str(), buf);

    HTTP http(*sock);
    http.initRequest(buf);
    handshakeHTTP(http, isHTTP);
}

// -----------------------------------
// リレー接続、あるいはダイレクト接続に str で指定されたチャンネルのス
// トリームを流す。relay が true ならば、チャンネルが無かったり受信中
// でなくても、チャンネルを受信中の状態にしようとする。
void Servent::triggerChannel(char *str, ChanInfo::PROTOCOL proto, bool relay)
{
    ChanInfo info;

    servMgr->getChannel(str, info, relay);

    if (proto == ChanInfo::SP_PCP)
        type = T_RELAY;
    else
        type = T_DIRECT;

    outputProtocol = proto;

    processStream(false, info);
}

// -----------------------------------
void writePLSHeader(Stream &s, PlayList::TYPE type)
{
    s.writeLine(HTTP_SC_OK);
    s.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);

    const char *content;
    switch(type)
    {
        case PlayList::T_PLS:
            content = MIME_XM3U;
            break;
        case PlayList::T_ASX:
            content = MIME_ASX;
            break;
        case PlayList::T_RAM:
            content = MIME_RAM;
            break;
        default:
            content = MIME_TEXT;
            break;
    }
    s.writeLineF("%s %s", HTTP_HS_CONTENT, content);
    s.writeLine("Content-Disposition: inline");
    s.writeLine("Cache-Control: private" );
    s.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");

    s.writeLine("");
}

// -----------------------------------
void Servent::handshakePLS(ChanInfo &info)
{
    char url[256];

    if (getLocalURL(url))
    {
        PlayList::TYPE type = PlayList::getPlayListType(info.contentType);

        writePLSHeader(*sock, type);

        PlayList pls(type, 1);
        pls.wmvProtocol = servMgr->wmvProtocol;
        pls.addChannel(url, info);
        pls.write(*sock);
    }
}

// -----------------------------------
bool Servent::getLocalURL(char *str)
{
    if (!sock)
        throw StreamException("Not connected");

    Host h;

    if (sock->host.localIP())
        h = sock->getLocalHost();
    else
        h = servMgr->serverHost;

    h.port = servMgr->serverHost.port;

    sprintf(str, "http://%s", h.str().c_str());
    return true;
}

// -----------------------------------
bool Servent::handshakeHTTPBasicAuth(HTTP &http)
{
    char user[64] = "", pass[64] = "";

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        if (http.isHeader("Authorization"))
            http.getAuthUserPass(user, pass, sizeof(user), sizeof(pass));
    }

    if (sock->host.isLocalhost())
    {
        LOG_DEBUG("HTTP Basic Auth: host is localhost");
        return true;
    }

    if (strlen(servMgr->password) != 0 && strcmp(pass, servMgr->password) == 0)
    {
        LOG_DEBUG("HTTP Basic Auth: password matches");
        return true;
    }

    http.writeLine(HTTP_SC_UNAUTHORIZED);
    http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
    http.writeLine("");

    LOG_DEBUG("HTTP Basic Auth: rejected");
    return false;
}

void Servent::CMD_viewxml(const char* cmd, HTTP& http, String& jumpStr)
{
    handshakeXML();
}

std::string Servent::formatTimeDifference(unsigned int t, unsigned int currentTime)
{
    using namespace str;

    int d = (uint64_t) currentTime - t;

    if (d < 0)
        return STR("(", d, "s into the future)");
    else if (d > 0)
        return STR("(", d, "s ago)");
    else
        return "just now";
}

static std::string dumpTrack(const TrackInfo& track)
{
    using namespace str;

    std::string b;

    b += STR("contact = ", inspect(track.contact), "\n");
    b += STR("title = ", inspect(track.title), "\n");
    b += STR("artist = ", inspect(track.artist), "\n");
    b += STR("album = ", inspect(track.album), "\n");
    b += STR("genre = ", inspect(track.genre), "\n");

    return "TrackInfo\n" + indent_tab(b);
}

static std::string dumpChanInfo(const ChanInfo& info)
{
    using namespace str;

    std::string b;

    b += STR("name = ", inspect(info.name), "\n");
    b += STR("id = ", info.id.str(), "\n");
    b += STR("bcID = ", info.bcID.str(), "\n");
    b += STR("bitrate = ", info.bitrate, "\n");
    b += STR("contentType = ", info.contentType, "\n");
    b += STR("contentTypeStr = ", inspect(info.contentTypeStr), "\n");
    b += STR("MIMEType = ", inspect(info.MIMEType), "\n");
    b += STR("streamExt = ", inspect(info.streamExt), "\n");
    b += STR("srcProtocol = ", info.srcProtocol, "\n");
    b += STR("lastPlayStart = ", info.lastPlayStart, "\n");
    b += STR("lastPlayEnd = ", info.lastPlayEnd, "\n");
    b += STR("numSkips = ", info.numSkips, "\n");
    b += STR("createdTime = ", info.createdTime,
             " ", Servent::formatTimeDifference(info.createdTime, sys->getTime()), "\n");
    b += STR("status = ", info.status, "\n");

    b += dumpTrack(info.track);

    b += STR("desc = ", inspect(info.desc), "\n");
    b += STR("genre = ", inspect(info.genre), "\n");
    b += STR("url = ", inspect(info.url), "\n");
    b += STR("comment = ", inspect(info.comment), "\n");

    return "ChanInfo\n" + indent_tab(b);
}

void Servent::handshakeCMD(HTTP& http, char *q)
{
    String jumpStr;

    // q が http.cmdLine の一部を指しているので、http の操作に伴って変
    // 更されるため、コピーしておく。
    std::string query = q;

    if (!handshakeHTTPBasicAuth(http))
        return;

    std::string cmd = cgi::Query(query).get("cmd");

    try
    {
        if (cmd == "viewxml")
        {
            CMD_viewxml(query.c_str(), http, jumpStr);
        }else{
            throw HTTPException(HTTP_SC_BADREQUEST, 400);
        }
    }catch (HTTPException &e)
    {
        http.writeLine(e.msg);
        http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/html");
        http.writeLine("");

        http.writeStringF("<h1>ERROR - %s</h1>\n", cgi::escape_html(e.msg).c_str());

        LOG_ERROR("html: %s", e.msg);
    }

    if (jumpStr != "")
    {
        String jmp(jumpStr, String::T_HTML);
        jmp.convertTo(String::T_ASCII);

        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: %s", jmp.c_str());
        http.writeLine("");
    }
}

// -----------------------------------
static XML::Node *createChannelXML(std::shared_ptr<Channel> c)
{
    XML::Node *n = c->info.createChannelXML();
    n->add(c->createRelayXML(true));
    n->add(c->info.createTrackXML());
//  n->add(c->info.createServentXML());
    return n;
}

// -----------------------------------
static XML::Node *createChannelXML(ChanHitList *chl)
{
    XML::Node *n = chl->info.createChannelXML();
    n->add(chl->createXML());
    n->add(chl->info.createTrackXML());
//  n->add(chl->info.createServentXML());
    return n;
}

// -----------------------------------
void Servent::handshakeXML()
{
    XML xml;

    XML::Node *rn = new XML::Node("peercast");
    xml.setRoot(rn);

    rn->add(new XML::Node("servent uptime=\"%d\"", servMgr->getUptime()));

    rn->add(new XML::Node("bandwidth out=\"%d\" in=\"%d\"",
        stats.getPerSecond(Stats::BYTESOUT)-stats.getPerSecond(Stats::LOCALBYTESOUT),
        stats.getPerSecond(Stats::BYTESIN)-stats.getPerSecond(Stats::LOCALBYTESIN)
        ));

    rn->add(new XML::Node("connections total=\"%d\" relays=\"%d\" direct=\"%d\"", servMgr->totalConnected(), servMgr->numStreams(Servent::T_RELAY, true), servMgr->numStreams(Servent::T_DIRECT, true)));

    XML::Node *an = new XML::Node("channels_relayed total=\"%d\"", chanMgr->numChannels());
    rn->add(an);

    auto c = chanMgr->channel;
    while (c)
    {
        if (c->isActive())
            an->add(createChannelXML(c));
        c = c->next;
    }

    // add public channels
    {
        XML::Node *fn = new XML::Node("channels_found total=\"%d\"", chanMgr->numHitLists());
        rn->add(fn);

        ChanHitList *chl = chanMgr->hitlist;
        while (chl)
        {
            if (chl->isUsed())
                fn->add(createChannelXML(chl));
            chl = chl->next;
        }
    }

#if 0
    if (servMgr->isRoot)
    {
        // add private channels
        {
            XML::Node *pn = new XML::Node("priv_channels");
            rn->add(pn);

            ChanHitList *chl = chanMgr->hitlist;
            while (chl)
            {
                if (chl->isUsed())
                    if (chl->info.isPrivate())
                        pn->add(createChannelXML(chl));
                chl = chl->next;
            }
        }
    }
#endif

    XML::Node *hc = new XML::Node("host_cache");
    for (int i=0; i<ServMgr::MAX_HOSTCACHE; i++)
    {
        ServHost *sh = &servMgr->hostCache[i];
        if (sh->type != ServHost::T_NONE)
        {
            char ipstr[64];
            sh->host.toStr(ipstr);

            hc->add(new XML::Node("host ip=\"%s\" type=\"%s\" time=\"%d\"", ipstr, ServHost::getTypeStr(sh->type), sh->time));
        }
    }
    rn->add(hc);

    sock->writeLine(HTTP_SC_OK);
    sock->writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    sock->writeLineF("%s %s", HTTP_HS_CONTENT, MIME_XML);
    sock->writeLine("Connection: close");

    sock->writeLine("");

    xml.write(*sock);
}

// -----------------------------------
void Servent::readICYHeader(HTTP &http, ChanInfo &info, char *pwd, size_t plen)
{
    char *arg = http.getArgStr();
    if (!arg) return;

    if (http.isHeader("x-audiocast-name") || http.isHeader("icy-name") || http.isHeader("ice-name"))
    {
        info.name.set(arg, String::T_ASCII);
        info.name.convertTo(String::T_UNICODE);
    }else if (http.isHeader("x-audiocast-url") || http.isHeader("icy-url") || http.isHeader("ice-url"))
        info.url.set(arg, String::T_ASCII);
    else if (http.isHeader("x-audiocast-bitrate") || (http.isHeader("icy-br")) || http.isHeader("ice-bitrate") || http.isHeader("icy-bitrate"))
        info.bitrate = atoi(arg);
    else if (http.isHeader("x-audiocast-genre") || http.isHeader("ice-genre") || http.isHeader("icy-genre"))
    {
        info.genre.set(arg, String::T_ASCII);
        info.genre.convertTo(String::T_UNICODE);
    }else if (http.isHeader("x-audiocast-description") || http.isHeader("ice-description"))
    {
        info.desc.set(arg, String::T_ASCII);
        info.desc.convertTo(String::T_UNICODE);

    }else if (http.isHeader("Authorization")) {
        if (pwd)
            http.getAuthUserPass(NULL, pwd, 0, plen);
    }
    else if (http.isHeader(PCX_HS_CHANNELID))
        info.id.fromStr(arg);
    else if (http.isHeader("ice-password"))
    {
        if (pwd)
            if (strlen(arg) < 64)
                strcpy(pwd, arg);
    }else if (http.isHeader("content-type"))
    {
        if (stristr(arg, MIME_OGG))
            info.contentType = ChanInfo::T_OGG;
        else if (stristr(arg, MIME_XOGG))
            info.contentType = ChanInfo::T_OGG;

        else if (stristr(arg, MIME_MP3))
            info.contentType = ChanInfo::T_MP3;
        else if (stristr(arg, MIME_XMP3))
            info.contentType = ChanInfo::T_MP3;

        else if (stristr(arg, MIME_WMA))
            info.contentType = ChanInfo::T_WMA;
        else if (stristr(arg, MIME_WMV))
            info.contentType = ChanInfo::T_WMV;
        else if (stristr(arg, MIME_ASX))
            info.contentType = ChanInfo::T_ASX;

        else if (stristr(arg, MIME_NSV))
            info.contentType = ChanInfo::T_NSV;
        else if (stristr(arg, MIME_RAW))
            info.contentType = ChanInfo::T_RAW;

        else if (stristr(arg, MIME_MMS))
            info.srcProtocol = ChanInfo::SP_MMS;
        else if (stristr(arg, MIME_XPCP))
            info.srcProtocol = ChanInfo::SP_PCP;
        else if (stristr(arg, MIME_XPEERCAST))
            info.srcProtocol = ChanInfo::SP_PEERCAST;

        else if (stristr(arg, MIME_XSCPLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_PLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_XPLS))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_M3U))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_MPEGURL))
            info.contentType = ChanInfo::T_PLS;
        else if (stristr(arg, MIME_TEXT))
            info.contentType = ChanInfo::T_PLS;
    }
}

// -----------------------------------
// Windows Media HTTP Push Distribution Protocol
#define ASSERT(x)
void Servent::handshakeWMHTTPPush(HTTP& http, const std::string& path)
{
    // At this point, http has read all the headers.

    ASSERT(http.headers["CONTENT-TYPE"] == "application/x-wms-pushsetup");
    LOG_DEBUG("%s", nlohmann::json(http.headers.m_headers).dump().c_str());

    int size = std::atoi(http.headers.get("Content-Length").c_str());

    // エンコーダーの設定要求を読む。0 バイトの空の設定要求も合法。
    unique_ptr<char> buffer(new char[size + 1]);
    http.read(buffer.get(), size);
    buffer.get()[size] = '\0';
    LOG_DEBUG("setup: %s", str::inspect(buffer.get()).c_str());

    // わかったふりをする
    http.writeLine("HTTP/1.1 204 No Content"); // これってちゃんと CRLF 出る？
    std::vector< std::pair<const char*,const char*> > headers =
        {
            { "Server"         , "Cougar/9.01.01.3814" },
            { "Cache-Control"  , "no-cache" },
            { "Supported"      , "com.microsoft.wm.srvppair, "
              "com.microsoft.wm.sswitch, "
              "com.microsoft.wm.predstrm, "
              "com.microsoft.wm.fastcache, "
              "com.microsoft.wm.startupprofile" },
            { "Content-Length" , "0" },
            { "Connection"     , "Keep-Alive" },
        };
    for (auto hline : headers)
    {
        http.writeLineF("%s: %s", hline.first, hline.second);
    }
    http.writeLine("");

    // -----------------------------------------
    http.reset();
    http.readRequest();
    LOG_DEBUG("Request line: %s", http.cmdLine);

    http.readHeaders();
    LOG_DEBUG("Setup: %s", nlohmann::json(http.headers.m_headers).dump().c_str());

    ASSERT(http.headers["CONTENT-TYPE"] == "application/x-wms-pushstart");

    // -----------------------------------------

    // User-Agent ヘッダーがあれば agent をセット
    if (http.headers.get("User-Agent") != "")
        this->agent = http.headers.get("User-Agent");

    auto vec = str::split(cgi::unescape(path.substr(1)), ";");
    if (vec.size() == 0)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    ChanInfo info;
    info.setContentType(ChanInfo::getTypeFromStr("WMV"));
    info.name = vec[0];
    if (vec.size() > 1) info.genre = vec[1];
    if (vec.size() > 2) info.desc  = vec[2];
    if (vec.size() > 3) info.url   = vec[3];

    info.id = chanMgr->broadcastID;
    info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);

    auto c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_INFO("WMHTTP Push channel already active, closing old one");
        c->thread.shutdown();
    }

    info.comment = chanMgr->broadcastMsg;
    info.bcID    = chanMgr->broadcastID;

    c = chanMgr->createChannel(info, NULL);
    if (!c)
        throw HTTPException(HTTP_SC_SERVERERROR, 500);

    c->startWMHTTPPush(sock);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
ChanInfo Servent::createChannelInfo(GnuID broadcastID, const String& broadcastMsg, cgi::Query& query, const std::string& contentType)
{
    ChanInfo info;

    auto type = ChanInfo::getTypeFromStr(query.get("type").c_str());
    // type が空、あるいは認識できない場合はリクエストの Content-Type から自動判別する
    if (type == ChanInfo::T_UNKNOWN)
        type = ChanInfo::getTypeFromMIME(contentType);

    info.setContentType(type);
    info.name    = query.get("name");
    info.genre   = query.get("genre");
    info.desc    = query.get("desc");
    info.url     = query.get("url");
    info.bitrate = atoi(query.get("bitrate").c_str());
    info.comment = query.get("comment").empty() ? broadcastMsg : query.get("comment");

    info.id = broadcastID;
    info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);
    info.bcID = broadcastID;

    return info;
}

// -----------------------------------
// HTTP Push 放送
void Servent::handshakeHTTPPush(const std::string& args)
{
    using namespace cgi;

    Query query(args);

    // HTTP ヘッダーを全て読み込む
    HTTP http(*sock);
    http.readHeaders();

    if (query.get("name").empty())
    {
        LOG_ERROR("handshakeHTTPPush: name parameter is mandatory");
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }

    ChanInfo info = createChannelInfo(chanMgr->broadcastID, chanMgr->broadcastMsg, query, http.headers.get("Content-Type"));

    auto c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_INFO("HTTP Push channel already active, closing old one");
        c->thread.shutdown();
    }
    // ここでシャットダウン待たなくていいの？

    c = chanMgr->createChannel(info, NULL);
    if (!c)
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    bool chunked = (http.headers.get("Transfer-Encoding") == "chunked");
    c->startHTTPPush(sock, chunked);
    sock = NULL;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::handshakeICY(Channel::SRC_TYPE type, bool isHTTP)
{
    ChanInfo info;

    HTTP http(*sock);

    // default to mp3 for shoutcast DSP (doesn`t send content-type)
    if (type == Channel::SRC_SHOUTCAST)
        info.contentType = ChanInfo::T_MP3;

    while (http.nextHeader())
    {
        LOG_DEBUG("ICY %s", http.cmdLine);
        readICYHeader(http, info, loginPassword.cstr(), String::MAX_LEN);
    }

    // check password before anything else, if needed
    if (loginPassword != servMgr->password)
    {
        if (!sock->host.isLocalhost() || !loginPassword.isEmpty())
            throw HTTPException(HTTP_SC_UNAUTHORIZED, 401);
    }

    // we need a valid IP address before we start
    servMgr->checkFirewall();

    // attach channel ID to name, channel ID is also encoded with IP address
    // to help prevent channel hijacking.

    info.id = chanMgr->broadcastID;
    info.id.encode(NULL, info.name.cstr(), loginMount.cstr(), info.bitrate);

    LOG_DEBUG("Incoming source: %s : %s", info.name.cstr(), info.getTypeStr());
    if (isHTTP)
        sock->writeStringF("%s\n\n", HTTP_SC_OK);
    else
        sock->writeLine("OK");

    auto c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_INFO("ICY channel already active, closing old one");
        c->thread.shutdown();
    }

    info.comment = chanMgr->broadcastMsg;
    info.bcID = chanMgr->broadcastID;

    c = chanMgr->createChannel(info, loginMount.cstr());
    if (!c)
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    c->startICY(sock, type);
}

// -----------------------------------
const char* Servent::fileNameToMimeType(const String& fileName)
{
    if (fileName.contains(".htm"))
        return MIME_HTML;
    else if (fileName.contains(".css"))
        return MIME_CSS;
    else if (fileName.contains(".jpg"))
        return MIME_JPEG;
    else if (fileName.contains(".gif"))
        return MIME_GIF;
    else if (fileName.contains(".png"))
        return MIME_PNG;
    else if (fileName.contains(".js"))
        return MIME_JS;
    else if (fileName.contains(".ico"))
        return MIME_ICO;
    else
        return nullptr;
}

