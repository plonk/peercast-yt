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
#include "html.h"
#include "stats.h"
#include "peercast.h"
#include "pcp.h"
#include "version2.h"
#include "jrpc.h"
#include "playlist.h"
#include "str.h"
#include "cgi.h"
#include "template.h"
#include "public.h"
#include "assets.h"
#include "uptest.h"
#include "gnutella.h"

using namespace std;

static bool isDecimal(const std::string& str);

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

void Servent::invokeCGIScript(HTTP &http, const char* fn)
{
    HTTPRequest req;
    try {
        req = http.getRequest();
    } catch (GeneralException& e) // request not ready
    {
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
    FileSystemMapper fs("/cgi-bin", (std::string) peercastApp->getPath() + "cgi-bin");
    std::string filePath = fs.toLocalFilePath(req.path);
    Environment env;

    env.set("SCRIPT_NAME", req.path);
    env.set("SCRIPT_FILENAME", filePath);
    env.set("GATEWAY_INTERFACE", "CGI/1.1");
    env.set("DOCUMENT_ROOT", peercastApp->getPath());
    env.set("QUERY_STRING", req.queryString);
    env.set("REQUEST_METHOD", "GET");
    env.set("REQUEST_URI", req.url);
    env.set("SERVER_PROTOCOL", "HTTP/1.0");
    env.set("SERVER_SOFTWARE", PCX_AGENT);
    if (!Regexp("[A-Za-z0-9\\-_.]+:\\d+").exec(req.headers.get("Host")).empty())
    {
        auto v = str::split(req.headers.get("Host"), ":");
        env.set("SERVER_NAME", v[0]);
        env.set("SERVER_PORT", v[1]);
    }else
    {
        LOG_ERROR("Host header missing");
        env.set("SERVER_NAME", servMgr->serverHost.str(false));
        env.set("SERVER_PORT", std::to_string(servMgr->serverHost.port));
    }

    env.set("PATH", getenv("PATH"));
    // Windows で Ruby が名前引きをするのに必要なので SYSTEMROOT を通す。
    if (getenv("SYSTEMROOT"))
        env.set("SYSTEMROOT", getenv("SYSTEMROOT"));

    if (filePath.empty())
        throw HTTPException(HTTP_SC_NOTFOUND, 404);

    Subprogram script(filePath);

    bool success = script.start({}, env);
    if (success)
        LOG_DEBUG("script started (pid = %d)", script.pid());
    else
    {
        LOG_ERROR("failed to start script `%s`", filePath.c_str());
        throw HTTPException(HTTP_SC_SERVERERROR, 500);
    }
    Stream& stream = script.inputStream();

    HTTPHeaders headers;
    int statusCode = 200;
    try {
        Regexp headerPattern("^([A-Za-z\\-]+):\\s*(.*)$");
        std::string line;
        while ((line = stream.readLine(8192)) != "")
        {
            LOG_DEBUG("Line: %s", line.c_str());
            auto caps = headerPattern.exec(line);
            if (caps.size() == 0)
            {
                LOG_ERROR("Invalid header: \"%s\"", line.c_str());
                continue;
            }
            if (str::capitalize(caps[1]) == "Status")
                statusCode = atoi(caps[2].c_str());
            else
                headers.set(caps[1], caps[2]);
        }
        if (headers.get("Location") != "")
            statusCode = 302; // Found
    } catch (StreamException&)
    {
        LOG_ERROR("CGI script did not finish the headers");
        throw HTTPException(HTTP_SC_SERVERERROR, 500);
    }

    HTTPResponse res(statusCode, headers);

    res.stream = &stream;
    http.send(res);
    stream.close();

    int status;
    bool normal = script.wait(&status);

    if (!normal)
    {
        LOG_ERROR("child process (PID %d) terminated abnormally", script.pid());
    }else
    {
        LOG_DEBUG("child process (PID %d) exited normally (status %d)", script.pid(), status);
    }
}

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
    }else if (strncmp(fn, "/admin/?", 8) == 0)
    {
        // 上に同じ

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Admin client");
        handshakeCMD(http, fn+8);
    }else if (strcmp(fn, "/html/index.html") == 0)
    {
        // PeerCastStation が "/" を "/html/index.html" に 301 Moved
        // でリダイレクトするので、ブラウザによっては無期限にキャッシュされる。
        // "/" に再リダイレクトしてキャッシュを無効化する。

        http.readHeaders();
        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: /");
        http.writeLine("");
    }else if (strncmp(fn, "/html/", 6) == 0)
    {
        // HTML UI

        String dirName = fn+1;

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeAuth(http, fn, true))
            handshakeLocalFile(dirName, http);
    }else if (strncmp(fn, "/admin.cgi", 10) == 0)
    {
        // ShoutCast トラック情報更新用エンドポイント

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        const char *pwdArg = getCGIarg(fn, "pass=");
        const char *songArg = getCGIarg(fn, "song=");
        const char *mountArg = getCGIarg(fn, "mount=");
        const char *urlArg = getCGIarg(fn, "url=");

        if (pwdArg && songArg)
        {
            int slen = strlen(fn);
            for (int i=0; i<slen; i++)
                if (fn[i]=='&') fn[i] = 0;

            auto c = chanMgr->channel;
            while (c)
            {
                if ((c->status == Channel::S_BROADCASTING) &&
                    (c->info.contentType == ChanInfo::T_MP3) )
                {
                    // if we have a mount point then check for it, otherwise update all channels.

                    bool match=true;

                    if (mountArg)
                        match = strcmp(c->mount, mountArg) == 0;

                    if (match)
                    {
                        ChanInfo newInfo = c->info;
                        newInfo.track.title.set(songArg, String::T_ESC);
                        newInfo.track.title.convertTo(String::T_UNICODE);

                        if (urlArg)
                            if (urlArg[0])
                                newInfo.track.contact.set(urlArg, String::T_ESC);
                        LOG_INFO("Channel Shoutcast update: %s", songArg);
                        c->updateInfo(newInfo);
                    }
                }
                c = c->next;
            }
        }
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
    }else if (strcmp(fn, "/public")== 0 ||
              strncmp(fn, "/public/", strlen("/public/"))==0)
    {
        // 公開ディレクトリ

        http.readHeaders();

        if (!servMgr->publicDirectoryEnabled)
        {
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);
        }

        try
        {
            PublicController controller(peercastApp->getPath() + std::string("public"));
            http.send(controller(http.getRequest(), *sock, sock->host));
        } catch (GeneralException& e)
        {
            LOG_ERROR("Error: %s", e.msg);
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }
    }else if (str::is_prefix_of("/assets/", fn))
    {
        // html と public の共有アセット。

        http.readHeaders();
        try
        {
            AssetsController controller(peercastApp->getPath() + std::string("assets"));
            http.send(controller(http.getRequest(), *sock, sock->host));
        } catch (GeneralException& e)
        {
            LOG_ERROR("Error: %s", e.msg);
            throw HTTPException(HTTP_SC_SERVERERROR, 500);
        }
    }else if (str::is_prefix_of("/cgi-bin/", fn))
    {
        // CGI スクリプトの実行

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (str::has_prefix(fn, "/cgi-bin/flv.cgi"))
        {
            if (!isPrivate() || !isFiltered(ServFilter::F_DIRECT))
                throw HTTPException(HTTP_SC_FORBIDDEN, 403);
            else
            {
                http.readHeaders();
                invokeCGIScript(http, fn);
            }
        }else if (handshakeAuth(http, fn, true))
        {
            invokeCGIScript(http, fn);
        }
    }else
    {
        // GET マッチなし

        http.readHeaders();
        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: /%s/index.html", servMgr->htmlPath);
        http.writeLine("");
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
    strcpy(ipstr, sock->host.str().c_str());

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
    }else if (servMgr->password[0] != '\0' && http.isRequest(servMgr->password))
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

// -----------------------------------
bool Servent::handshakeAuth(HTTP &http, const char *args, bool local)
{
    char user[64], pass[64];
    user[0] = pass[0] = 0;

    const char *pwd  = getCGIarg(args, "pass=");

    if ((pwd) && strlen(servMgr->password))
    {
        String tmp = pwd;
        char *as = strstr(tmp.cstr(), "&");
        if (as) *as = 0;
        if (strcmp(tmp, servMgr->password) == 0)
        {
            http.readHeaders();
            return true;
        }
    }

    while (http.nextHeader())
    {
        char *arg = http.getArgStr();
        if (!arg)
            continue;

        switch (servMgr->authType)
        {
            case ServMgr::AUTH_HTTPBASIC:
                if (http.isHeader("Authorization"))
                    http.getAuthUserPass(user, pass, sizeof(user), sizeof(pass));
                break;
            case ServMgr::AUTH_COOKIE:
                if (http.isHeader("Cookie"))
                {
                    LOG_DEBUG("Got cookie: %s", arg);
                    const std::string idKey = str::STR(servMgr->serverHost.port, "_id");
                    auto assignments = str::split(arg, "; ");

                    for (auto assignment : assignments) {
                        auto sides = str::split(assignment, "=", 2);
                        if (sides.size() != 2) {
                            LOG_ERROR("Invalid Cookie header: expected '='");
                            break;
                        } else if (sides[0] == idKey) {
                            Cookie gotCookie;
                            gotCookie.set(sides[1].c_str(), sock->host.ip);

                            if (servMgr->cookieList.contains(gotCookie)) {
                                LOG_DEBUG("Cookie found");
                                cookie = gotCookie;
                            }
                            break;
                        }
                    }
                }
                break;
        }
    }

    if (sock->host.isLocalhost())
        return true;

    switch (servMgr->authType)
    {
        case ServMgr::AUTH_HTTPBASIC:
            if ((strcmp(pass, servMgr->password) == 0) && strlen(servMgr->password))
                return true;
            break;
        case ServMgr::AUTH_COOKIE:
            if (servMgr->cookieList.contains(cookie))
                return true;
            break;
    }

    if (servMgr->authType == ServMgr::AUTH_HTTPBASIC)
    {
        http.writeLine(HTTP_SC_UNAUTHORIZED);
        http.writeLine("WWW-Authenticate: Basic realm=\"PeerCast Admin\"");
        http.writeLine("");
    }else if (servMgr->authType == ServMgr::AUTH_COOKIE)
    {
        String file = servMgr->htmlPath;
        file.append("/login.html");
        if (http.headers.get("X-Requested-With") == "XMLHttpRequest")
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);
        else
        {
            // XXX
            handshakeLocalFile(file, http);
        }
    }

    return false;
}

// -----------------------------------
void Servent::CMD_portcheck4(const char* cmd, HTTP& http, String& jumpStr)
{
    servMgr->checkFirewall();
    if (!http.headers.get("Referer").empty())
    {
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    }else
    {
        jumpStr.sprintf("/%s/index.html", servMgr->htmlPath);
    }
}

// -----------------------------------
void Servent::CMD_portcheck6(const char* cmd, HTTP& http, String& jumpStr)
{
    servMgr->checkFirewallIPv6();
    if (!http.headers.get("Referer").empty())
    {
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    }else
    {
        jumpStr.sprintf("/%s/index.html", servMgr->htmlPath);
    }
}

// -----------------------------------
void Servent::CMD_redirect(const char* cmd, HTTP& http, String& jumpStr)
{
    char buf[MAX_CGI_LEN];
    Sys::strcpy_truncate(buf, sizeof(buf), cmd);

    HTML html("", *sock);
    const char *j = getCGIarg(buf, "url=");

    if (j)
    {
        http.writeLine(HTTP_SC_OK);
        http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/html");
        http.writeLine("");

        termArgs(buf);
        String url;
        url.set(j, String::T_ESC);
        url.convertTo(String::T_ASCII);

        if (!url.startsWith("http://") && !url.startsWith("https://"))
            url.prepend("http://");

        html.setRefreshURL(url.cstr());
        html.startHTML();
            html.addHead();
            html.startBody();
                html.startTagEnd("h3", "Please wait...");
            html.end();
        html.end();
    } else {
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

void Servent::CMD_viewxml(const char* cmd, HTTP& http, String& jumpStr)
{
    handshakeXML();
}

void Servent::CMD_clearlog(const char* cmd, HTTP& http, String& jumpStr)
{
    sys->logBuf->clear();
    jumpStr.sprintf("/%s/viewlog.html", servMgr->htmlPath);
}

void Servent::CMD_apply(const char* cmd, HTTP& http, String& jumpStr)
{
    std::lock_guard<std::recursive_mutex> cs(servMgr->lock);

    servMgr->numFilters = 0;
    ServFilter *currFilter = servMgr->filters;
    servMgr->channelDirectory->clearFeeds();
    servMgr->publicDirectoryEnabled = false;
    servMgr->transcodingEnabled = false;
    servMgr->chat = false;
    servMgr->randomizeBroadcastingChannelID = false;

    bool brRoot = false;
    bool getUpd = false;
    int allowServer1 = 0;
    int newPort = servMgr->serverHost.port;

    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];
    const char *cp = cmd;
    while ((cp = nextCGIarg(cp, curr, arg)) != nullptr)
    {
        // server
        if (strcmp(curr, "servername") == 0)
            servMgr->serverName = cgi::unescape(arg);
        else if (strcmp(curr, "serveractive") == 0)
            servMgr->autoServe = getCGIargBOOL(arg);
        else if (strcmp(curr, "port") == 0)
            newPort = getCGIargINT(arg);
        else if (strcmp(curr, "icymeta") == 0)
        {
            int iv = getCGIargINT(arg);
            if (iv < 0) iv = 0;
            else if (iv > 16384) iv = 16384;

            chanMgr->icyMetaInterval = iv;
        }else if (strcmp(curr, "passnew") == 0)
            Sys::strcpy_truncate(servMgr->password, sizeof(servMgr->password), arg);
        else if (strcmp(curr, "root") == 0)
            servMgr->isRoot = getCGIargBOOL(arg);
        else if (strcmp(curr, "brroot") == 0)
            brRoot = getCGIargBOOL(arg);
        else if (strcmp(curr, "getupd") == 0)
            getUpd = getCGIargBOOL(arg);
        else if (strcmp(curr, "huint") == 0)
            chanMgr->setUpdateInterval(getCGIargINT(arg));
        else if (strcmp(curr, "forceip") == 0)
            servMgr->forceIP = arg;
        else if (strcmp(curr, "htmlPath") == 0)
        {
            strcpy(servMgr->htmlPath, "html/");
            strcat(servMgr->htmlPath, arg);
        }else if (strcmp(curr, "djmsg") == 0)
        {
            String msg;
            msg.set(arg, String::T_ESC);
            msg.convertTo(String::T_UNICODE);
            chanMgr->setBroadcastMsg(msg);
        }
        else if (strcmp(curr, "pcmsg") == 0)
        {
            servMgr->rootMsg.set(arg, String::T_ESC);
            servMgr->rootMsg.convertTo(String::T_UNICODE);
        }

        // connections
        else if (strcmp(curr, "maxcin") == 0)
            servMgr->maxControl = getCGIargINT(arg);
        else if (strcmp(curr, "maxsin") == 0)
            servMgr->maxServIn = getCGIargINT(arg);

        else if (strcmp(curr, "maxup") == 0)
            servMgr->maxBitrateOut = getCGIargINT(arg);
        else if (strcmp(curr, "maxrelays") == 0)
            servMgr->setMaxRelays(getCGIargINT(arg));
        else if (strcmp(curr, "maxdirect") == 0)
            servMgr->maxDirect = getCGIargINT(arg);
        else if (strcmp(curr, "maxrelaypc") == 0)
            chanMgr->maxRelaysPerChannel = getCGIargINT(arg);
        else if (strncmp(curr, "filt_", 5) == 0)
        {
            char *fs = curr+5;

            if (strncmp(fs, "ip", 2) == 0)        // ip must be first
            {
                currFilter = &servMgr->filters[servMgr->numFilters];
                currFilter->init();
                currFilter->setPattern(arg);
                if (currFilter->isSet() && (servMgr->numFilters < (ServMgr::MAX_FILTERS-1)))
                {
                    servMgr->numFilters++;
                    servMgr->filters[servMgr->numFilters].init();   // clear new entry
                }
            }else if (strncmp(fs, "bn", 2) == 0)
                currFilter->flags |= ServFilter::F_BAN;
            else if (strncmp(fs, "pr", 2) == 0)
                currFilter->flags |= ServFilter::F_PRIVATE;
            else if (strncmp(fs, "nw", 2) == 0)
                currFilter->flags |= ServFilter::F_NETWORK;
            else if (strncmp(fs, "di", 2) == 0)
                currFilter->flags |= ServFilter::F_DIRECT;
        }
        else if (strcmp(curr, "channel_feed_url") == 0)
        {
            if (strcmp(arg, "") != 0)
            {
                String str(arg, String::T_ESC);
                str.convertTo(String::T_ASCII);
                servMgr->channelDirectory->addFeed(str.cstr());
            }
        }
        else if (strncmp(curr, "channel_feed_public", strlen("channel_feed_public")) == 0)
        {
            int index = atoi(curr + strlen("channel_feed_public"));
            servMgr->channelDirectory->setFeedPublic(index, true);
        }

        // client
        else if (strcmp(curr, "clientactive") == 0)
            servMgr->autoConnect = getCGIargBOOL(arg);
        else if (strcmp(curr, "yp") == 0)
        {
            String str(arg, String::T_ESC);
            str.convertTo(String::T_ASCII);
            servMgr->rootHost = str;
        }
        else if (strcmp(curr, "deadhitage") == 0)
            chanMgr->deadHitAge = getCGIargINT(arg);
        else if (strcmp(curr, "refresh") == 0)
            servMgr->refreshHTML = getCGIargINT(arg);
        else if (strcmp(curr, "chat") == 0)
            servMgr->chat = getCGIargBOOL(arg);
        else if (strcmp(curr, "randomizechid") == 0)
            servMgr->randomizeBroadcastingChannelID = getCGIargBOOL(arg);
        else if (strcmp(curr, "public_directory") == 0)
            servMgr->publicDirectoryEnabled = true;
        else if (strcmp(curr, "genreprefix") == 0)
            servMgr->genrePrefix = arg;
        else if (strcmp(curr, "auth") == 0)
        {
            if (strcmp(arg, "cookie") == 0)
                servMgr->authType = ServMgr::AUTH_COOKIE;
            else if (strcmp(arg, "http") == 0)
                servMgr->authType = ServMgr::AUTH_HTTPBASIC;
        }else if (strcmp(curr, "expire") == 0)
        {
            if (strcmp(arg, "session") == 0)
                servMgr->cookieList.neverExpire = false;
            else if (strcmp(arg, "never") == 0)
                servMgr->cookieList.neverExpire = true;
        }

        else if (strcmp(curr, "logLevel") == 0)
            servMgr->logLevel(atoi(arg));

        else if (strcmp(curr, "allowHTML1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_HTML) : 0;
        else if (strcmp(curr, "allowNetwork1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_NETWORK) : 0;
        else if (strcmp(curr, "allowBroadcast1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_BROADCAST) : 0;
        else if (strcmp(curr, "allowDirect1") == 0)
            allowServer1 |= atoi(arg) ? (ALLOW_DIRECT) : 0;

        else if (strcmp(curr, "transcoding_enabled") == 0)
            servMgr->transcodingEnabled = getCGIargBOOL(arg);
        else if (strcmp(curr, "preset") == 0)
            servMgr->preset = arg;
        else if (strcmp(curr, "audio_codec") == 0)
            servMgr->audioCodec = arg;
        else if (strcmp(curr, "wmvProtocol") == 0)
            servMgr->wmvProtocol = arg;
    }

    servMgr->allowServer1 = allowServer1;

    if (servMgr->serverHost.port != newPort)
    {
        std::string ipstr;
        if (http.headers.get("Host") != "")
        {
            auto vec = str::split(http.headers.get("Host"), ":");
            ipstr =  vec[0]+":"+std::to_string(newPort);
        }else
        {
            ipstr = Host(ClientSocket::getIP(NULL), newPort).str();
        }
        jumpStr.sprintf("http://%s/%s/settings.html", ipstr.c_str(), servMgr->htmlPath);

        servMgr->serverHost.port = newPort;
        servMgr->restartServer = true;

        sys->sleep(500); // give server time to restart itself
    }else
    {
        jumpStr.sprintf("/%s/settings.html", servMgr->htmlPath);
    }

    peercastInst->saveSettings();
    peercast::notifyMessage(ServMgr::NT_PEERCAST, "設定を保存しました。");
    peercastApp->updateSettings();

    if ((servMgr->isRoot) && (brRoot))
        servMgr->broadcastRootSettings(getUpd);
}

void Servent::CMD_fetch(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    ChanInfo info;

    auto curl = query.get("url");
    info.name = query.get("name");
    info.desc = query.get("desc");
    info.genre = query.get("genre");
    info.url = query.get("contact");
    info.bitrate = atoi(query.get("bitrate").c_str());
    auto type = query.get("type");
    info.contentType = type;
    info.MIMEType = ChanInfo::getMIMEType(type.c_str());
    info.streamExt = ChanInfo::getTypeExt(type.c_str());
    info.bcID = chanMgr->broadcastID;

    // id がセットされていないチャンネルがあるといろいろまずいので、事
    // 前に設定してから登録する。
    if (servMgr->randomizeBroadcastingChannelID) {
        info.id = GnuID::random();
    } else {
        info.id = chanMgr->broadcastID;
        info.id.encode(NULL, info.name, info.genre, info.bitrate);
    }

    auto c = chanMgr->createChannel(info, NULL);
    if (c) {
        if (query.get("ipv") == "6") {
            c->ipVersion = Channel::IP_V6;
            LOG_INFO("Channel IP version set to 6");

            // YPv6ではIPv6のポートチェックができないのでがんばる。
            servMgr->checkFirewallIPv6();
        }
        c->startURL(curl.c_str());
    }

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_fetch_feeds(const char* cmd, HTTP& http, String& jumpStr)
{
    servMgr->channelDirectory->update(ChannelDirectory::kUpdateManual);

    if (!http.headers.get("Referer").empty())
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    else
        jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

// サーバントを停止する。
void Servent::CMD_stop_servent(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    std::string serventIndex = query.get("servent_id");

    if (!isDecimal(serventIndex))
    {
        http.send(HTTPResponse::badRequest("invalid servent_id"));
        return;
    }

    Servent *s = servMgr->findServentByID(std::stoi(serventIndex));
    if (s)
    {
        s->abort();
        jumpStr.sprintf("/%s/connections.html", servMgr->htmlPath);
        return;
    }else
    {
        http.send(HTTPResponse::notFound("servent not found"));
        return;
    }
}

void Servent::CMD_clear(const char* cmd, HTTP& http, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    const char *cp = cmd;
    while ((cp = nextCGIarg(cp, curr, arg)) != nullptr)
    {
        if (strcmp(curr, "hostcache") == 0)
            servMgr->clearHostCache(ServHost::T_SERVENT);
        else if (strcmp(curr, "hitlists") == 0)
            chanMgr->clearHitLists();
        else if (strcmp(curr, "packets") == 0)
            stats.clearRange(Stats::PACKETSSTART, Stats::PACKETSEND);
        else if (strcmp(curr, "channels") == 0)
            chanMgr->closeIdles();
    }

    if (!http.headers.get("Referer").empty())
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    else
        jumpStr.sprintf("/%s/index.html", servMgr->htmlPath);
}

void Servent::CMD_shutdown(const char* cmd, HTTP& http, String& jumpStr)
{
    http.send(HTTPResponse::ok({}, "Server is shutting down..."));
    servMgr->shutdownTimer = 1;
}

void Servent::CMD_stop(const char* cmd, HTTP& http, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    GnuID id;
    const char *cp = cmd;
    while ((cp = nextCGIarg(cp, curr, arg)) != nullptr)
    {
        if (strcmp(curr, "id") == 0)
            id.fromStr(arg);
    }

    auto c = chanMgr->findChannelByID(id);
    if (c)
        c->thread.shutdown();

    sys->sleep(500);
    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_bump(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    GnuID id;
    std::string ip;

    if (!query.get("id").empty())
        id.fromStr(query.get("id").c_str());

    ip = query.get("ip");
    Host designation;
    designation.fromStrIP(ip.c_str(), 7144);

    auto c = chanMgr->findChannelByID(id);
    if (c)
    {
        if (!ip.empty())
        {
            ChanHitList* chl = chanMgr->findHitList(c->info);
            ChanHit theHit;

            if (chl)
            {
                chl->forEachHit(
                    [&] (ChanHit* hit)
                    {
                        if (hit->host == designation)
                            theHit = *hit;
                    });
            }

            if (theHit.host.ip)
            {
                c->designatedHost = theHit;
            } else
            {
                // ChanHitをでっちあげる
                c->designatedHost.init();
                c->designatedHost.host = c->designatedHost.rhost[0] = designation;
            }
        }
        c->bump = true;
    }

    if (!http.headers.get("Referer").empty())
    {
        jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
    }else
    {
        jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
    }
}

void Servent::CMD_keep(const char* cmd, HTTP& http, String& jumpStr)
{
    char arg[MAX_CGI_LEN];
    char curr[MAX_CGI_LEN];

    GnuID id;
    const char *cp = cmd;
    while ((cp = nextCGIarg(cp, curr, arg)) != nullptr)
    {
        if (strcmp(curr, "id") == 0)
            id.fromStr(arg);
    }

    auto c = chanMgr->findChannelByID(id);
    if (c)
        c->stayConnected = !c->stayConnected;

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

void Servent::CMD_logout(const char* cmd, HTTP& http, String& jumpStr)
{
    jumpStr.sprintf("%s", "/");
    servMgr->cookieList.remove(cookie);
}

void Servent::CMD_login(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    GnuID id;
    char idstr[64];
    id.generate();
    id.toStr(idstr);

    cookie.set(idstr, sock->host.ip);
    servMgr->cookieList.add(cookie);

    http.writeLine(HTTP_SC_FOUND);
    if (servMgr->cookieList.neverExpire)
        http.writeLineF("%s %d_id=%s; path=/; expires=\"Mon, 01-Jan-3000 00:00:00 GMT\"", HTTP_HS_SETCOOKIE, (int) servMgr->serverHost.port, idstr);
    else
        http.writeLineF("%s %d_id=%s; path=/", HTTP_HS_SETCOOKIE, (int) servMgr->serverHost.port, idstr);

    if (query.get("requested_path") != "")
        http.writeLineF("Location: %s", query.get("requested_path").c_str());
    else
        http.writeLineF("Location: /%s/index.html", servMgr->htmlPath);
    http.writeLine("");
}

void Servent::CMD_control_rtmp(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    auto action = query.get("action");

    if (action == "start")
    {
        if (query.get("name") == "")
            throw HTTPException(HTTP_SC_BADREQUEST, 400); // name required to start RTMP server

        uint16_t port = std::atoi(query.get("port").c_str());
        {
            std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
            ChanInfo& info = servMgr->defaultChannelInfo;

            servMgr->rtmpPort = port;

            info.name    = query.get("name").c_str();
            info.genre   = query.get("genre").c_str();
            info.desc    = query.get("desc").c_str();
            info.url     = query.get("url").c_str();
            info.comment = query.get("comment").c_str();
        }

        servMgr->rtmpServerMonitor.ipVersion = (query.get("ipv") == "6") ? 6 : 4;
        servMgr->rtmpServerMonitor.enable();
        // Give serverProc the time to actually start the process.
        sys->sleep(500);
        jumpStr.sprintf("/%s/rtmp.html", servMgr->htmlPath);
    }else if (action == "stop")
    {
        servMgr->rtmpServerMonitor.disable();
        jumpStr.sprintf("/%s/rtmp.html", servMgr->htmlPath);
    }else
    {
        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

void Servent::CMD_update_channel_info(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    GnuID id(query.get("id"));
    auto ch = chanMgr->findChannelByID(id);
    if (ch == NULL)
        throw HTTPException(HTTP_SC_SERVERERROR, 500);

    // I don't think this operation is safe

    ChanInfo info = ch->info;

    info.name    = query.get("name").c_str();
    info.desc    = query.get("desc").c_str();
    info.genre   = query.get("genre").c_str();
    info.url     = query.get("contactURL").c_str();
    info.comment = query.get("comment").c_str();

    info.track.contact = query.get("track.contactURL").c_str();
    info.track.title   = query.get("track.title").c_str();
    info.track.artist  = query.get("track.artist").c_str();
    info.track.album   = query.get("track.album").c_str();
    info.track.genre   = query.get("track.genre").c_str();

    ch->updateInfo(info);

    jumpStr.sprintf("/%s/channels.html", servMgr->htmlPath);
}

static std::string dumpHit(ChanHit* hit)
{
    using namespace str;

    std::string buf;
    buf += STR("host = ", hit->host.str(), "\n");
    buf += STR("rhost[0] = ", hit->rhost[0].str(), "\n");
    buf += STR("rhost[1] = ", hit->rhost[1].str(), "\n");
    buf += STR("numlisteners = ", hit->numListeners, "\n");
    buf += STR("numRelays = ", hit->numRelays, "\n");
    buf += STR("numHops = ", hit->numHops, "\n");

    buf += STR("sessionID = ", hit->sessionID.str(), "\n");
    buf += STR("chanID = ", hit->chanID.str(), "\n");

    //buf += STR("versionString = ", hit->versionString(), "\n");

    buf += STR("version = ", hit->version, "\n");
    buf += STR("versionVP = ", hit->versionVP, "\n");
    buf += STR("versionExPrefix = ", inspect(std::string(hit->versionExPrefix, hit->versionExPrefix + 2)), "\n");
    buf += STR("versionExNumber = ", hit->versionExNumber, "\n");

    buf += STR("oldestPos = ", hit->oldestPos, "\n");
    buf += STR("newestPos = ", hit->newestPos, "\n");

    buf += STR("firewalled = ", hit->firewalled, "\n");
    buf += STR("stable = ", hit->stable, "\n");
    buf += STR("tracker = ", hit->tracker, "\n");
    buf += STR("recv = ", hit->recv, "\n");
    buf += STR("yp = ", hit->yp, "\n");
    buf += STR("dead = ", hit->dead, "\n");
    buf += STR("direct = ", hit->direct, "\n");
    buf += STR("relay = ", hit->relay, "\n");
    buf += STR("cin = ", hit->cin, "\n");

    buf += STR("uphost = ", hit->uphost.str(), "\n");
    buf += STR("uphostHops = ", hit->uphostHops, "\n");

    buf += STR("time = ", hit->time,
               " ", Servent::formatTimeDifference(hit->time, sys->getTime()), "\n");
    buf += STR("upTime = ", hit->upTime, "\n");
    buf += STR("lastContact = ", hit->lastContact,
               (hit->lastContact) ? " "+Servent::formatTimeDifference(hit->lastContact,sys->getTime()) : "",
               "\n");

    return "ChanHit\n" + indent_tab(buf);
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
    b += STR("contentType = ", info.contentType.c_str(), "\n");
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

static std::string dumpHitList(ChanHitList* hitlist)
{
    using namespace str;

    std::string b;
    b += STR("ChanHitList\n");

    b += STR("\tused = ", hitlist->isUsed(), "\n");
    b += STR("\tlastHitTime = ", hitlist->lastHitTime, "\n");
    b += STR("\tinfo:\n");
    b += indent_tab(dumpChanInfo(hitlist->info));

    int count = 0;
    for (auto hit = hitlist->hit;
              hit != nullptr;
              hit = hit->next)
        count++;

    b += STR("\thit (", count, " entries):\n");
    for (auto hit = hitlist->hit;
              hit != nullptr;
              hit = hit->next)
        b += indent_tab(dumpHit(hit), 2);

    return b;
}

void Servent::CMD_dump_hitlists(const char* cmd, HTTP& http, String& jumpStr)
{
    using namespace str;

    std::string buf;

    {
        std::lock_guard<std::recursive_mutex> cs(chanMgr->lock);

        int nlists = 0;
        for (auto hitlist = chanMgr->hitlist;
                  hitlist != nullptr;
                  hitlist = hitlist->next)
            nlists++;

        buf += STR(nlists, " hit list", nlists!=1 ? "s" : "", " found.\n\n");

        for (auto hitlist = chanMgr->hitlist;
                  hitlist != nullptr;
                  hitlist = hitlist->next)
        {
            buf += dumpHitList(hitlist);

            if (hitlist->next)
                buf += '\n';
        }
    }

    http.writeLine(HTTP_SC_OK);
    http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
    http.writeLineF("%s %zu", HTTP_HS_LENGTH, buf.size());
    http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/plain;charset=utf-8");
    http.writeLine("");

    http.writeString(buf);
}

void Servent::CMD_add_speedtest(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    if (query.get("url") == "")
    {
        http.send(HTTPResponse::badRequest("empty url"));
        return;
    }

    auto status = servMgr->uptestServiceRegistry->addURL(query.get("url"));
    if (status.first != true)
    {
        http.send(HTTPResponse::serverError(status.second));
    }else
    {
        if (!http.headers.get("Referer").empty())
            http.send(HTTPResponse::redirectTo(http.headers.get("Referer")));
        else
            http.send(HTTPResponse::redirectTo("/speedtest.html"));
    }
}

static bool isDecimal(const std::string& str)
{
    static const Regexp decimal("^(0|[1-9][0-9]*)$");
    return decimal.matches(str);
}

void Servent::CMD_delete_speedtest(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    if (!isDecimal(query.get("index")))
    {
        http.send(HTTPResponse::badRequest("invalid index"));
        return;
    }

    auto index = std::stoi(query.get("index"));
    auto status = servMgr->uptestServiceRegistry->deleteByIndex(index);
    if (status.first != true)
    {
        http.send(HTTPResponse::serverError(status.second));
        return;
    }else
    {
        if (!http.headers.get("Referer").empty())
            http.send(HTTPResponse::redirectTo(http.headers.get("Referer")));
        else
            http.send(HTTPResponse::redirectTo("/speedtest.html"));
    }
}

void Servent::CMD_refresh_speedtest(const char* cmd, HTTP& http, String& jumpStr)
{
    servMgr->uptestServiceRegistry->forceUpdate();

    if (!http.headers.get("Referer").empty())
        http.send(HTTPResponse::redirectTo(http.headers.get("Referer")));
    else
        http.send(HTTPResponse::redirectTo("/speedtest.html"));
}

void Servent::CMD_take_speedtest(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    if (!isDecimal(query.get("index")))
    {
        http.send(HTTPResponse::badRequest("invalid index"));
        return;
    }

    auto index = std::stoi(query.get("index"));
    auto status = servMgr->uptestServiceRegistry->takeSpeedtest(index);
    if (status.first != true)
    {
        http.send(HTTPResponse::serverError(status.second));
        return;
    }else
    {
        // 新しい計測値を読み込む。
        servMgr->uptestServiceRegistry->forceUpdate();

        if (!http.headers.get("Referer").empty())
            http.send(HTTPResponse::redirectTo(http.headers.get("Referer")));
        else
            http.send(HTTPResponse::redirectTo("/speedtest.html"));
    }
}

void Servent::CMD_speedtest_cached_xml(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    if (!isDecimal(query.get("index")))
    {
        http.send(HTTPResponse::badRequest("invalid index"));
        return;
    }

    bool success;
    std::string reason;
    std::string xml;

    std::tie(success, reason) = servMgr->uptestServiceRegistry->getXML(std::stoi(query.get("index")), xml);
    if (success)
    {
        http.send(HTTPResponse::ok(
                      {
                          {"Content-Type", "application/xml"},
                          {"Content-Length", std::to_string(xml.size()) }
                      }, xml));
    }else
    {
        http.send(HTTPResponse::serverError(reason));
    }
}

void Servent::handshakeCMD(HTTP& http, char *q)
{
    String jumpStr;

    // q が http.cmdLine の一部を指しているので、http の操作に伴って変
    // 更されるため、コピーしておく。
    std::string query = q;

    if (!handshakeAuth(http, query.c_str(), true))
        return;

    std::string cmd = cgi::Query(query).get("cmd");

    try
    {
        if (cmd == "add_speedtest")
        {
            CMD_add_speedtest(query.c_str(), http, jumpStr);
        }else if (cmd == "apply")
        {
            CMD_apply(query.c_str(), http, jumpStr);
        }else if (cmd == "bump")
        {
            CMD_bump(query.c_str(), http, jumpStr);
        }else if (cmd == "clear")
        {
            CMD_clear(query.c_str(), http, jumpStr);
        }else if (cmd == "clearlog")
        {
            CMD_clearlog(query.c_str(), http, jumpStr);
        }else if (cmd == "control_rtmp")
        {
            CMD_control_rtmp(query.c_str(), http, jumpStr);
        }else if (cmd == "delete_speedtest")
        {
            CMD_delete_speedtest(query.c_str(), http, jumpStr);
        }else if (cmd == "dump_hitlists")
        {
            CMD_dump_hitlists(query.c_str(), http, jumpStr);
        }else if (cmd == "fetch")
        {
            CMD_fetch(query.c_str(), http, jumpStr);
        }else if (cmd == "fetch_feeds")
        {
            CMD_fetch_feeds(query.c_str(), http, jumpStr);
        }else if (cmd == "keep")
        {
            CMD_keep(query.c_str(), http, jumpStr);
        }else if (cmd == "login")
        {
            CMD_login(query.c_str(), http, jumpStr);
        }else if (cmd == "logout")
        {
            CMD_logout(query.c_str(), http, jumpStr);
        }else if (cmd == "portcheck4")
        {
            CMD_portcheck4(query.c_str(), http, jumpStr);
        }else if (cmd == "portcheck6")
        {
            CMD_portcheck6(query.c_str(), http, jumpStr);
        }else if (cmd == "redirect")
        {
            CMD_redirect(query.c_str(), http, jumpStr);
        }else if (cmd == "refresh_speedtest")
        {
            CMD_refresh_speedtest(query.c_str(), http, jumpStr);
        }else if (cmd == "shutdown")
        {
            CMD_shutdown(query.c_str(), http, jumpStr);
        }else if (cmd == "speedtest_cached_xml")
        {
            CMD_speedtest_cached_xml(query.c_str(), http, jumpStr);
        }else if (cmd == "stop")
        {
            CMD_stop(query.c_str(), http, jumpStr);
        }else if (cmd == "stop_servent")
        {
            CMD_stop_servent(query.c_str(), http, jumpStr);
        }else if (cmd == "update_channel_info")
        {
            CMD_update_channel_info(query.c_str(), http, jumpStr);
        }else if (cmd == "take_speedtest")
        {
            CMD_take_speedtest(query.c_str(), http, jumpStr);
        }else if (cmd == "viewxml")
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

    XML::Node *hc = new XML::Node("host_cache");
    for (int i=0; i<ServMgr::MAX_HOSTCACHE; i++)
    {
        ServHost *sh = &servMgr->hostCache[i];
        if (sh->type != ServHost::T_NONE)
        {
            hc->add(new XML::Node("host ip=\"%s\" type=\"%s\" time=\"%d\"", sh->host.str().c_str(), ServHost::getTypeStr(sh->type), sh->time));
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
void Servent::handshakeWMHTTPPush(HTTP& http, const std::string& path)
{
    // At this point, http has read all the headers.

    ASSERT(http.headers.get("CONTENT-TYPE") == "application/x-wms-pushsetup");
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

    ASSERT(http.headers.get("CONTENT-TYPE") == "application/x-wms-pushstart");

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

    if (servMgr->randomizeBroadcastingChannelID) {
        info.id = GnuID::random();
    } else {
        info.id = chanMgr->broadcastID;
        info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);
    }

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

    if (servMgr->randomizeBroadcastingChannelID) {
        info.id = GnuID::random();
    } else {
        info.id = broadcastID;
        info.id.encode(NULL, info.name.cstr(), info.genre.cstr(), info.bitrate);
    }
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
    if (query.get("ipv") == "6") {
        c->ipVersion = Channel::IP_V6;
        LOG_INFO("Channel IP version set to 6");

        // YPv6ではIPv6のポートチェックができないのでがんばる。
        servMgr->checkFirewallIPv6();
    }
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

    if (servMgr->randomizeBroadcastingChannelID) {
        info.id = GnuID::random();
    } else {
        info.id = chanMgr->broadcastID;
        info.id.encode(NULL, info.name.cstr(), loginMount.cstr(), info.bitrate);
    }

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

// -----------------------------------
static void validFileOrThrow(const char* filePath, const std::string& documentRoot)
{
    ASSERT(documentRoot.size() > 0);
    ASSERT(documentRoot.back() == sys->getDirectorySeparator()[0]);

    std::string abspath;
    try {
        abspath = sys->realPath(filePath);
    } catch (GeneralException &e) {
        LOG_ERROR("Cannot determine absolute path: %s", e.what());
        throw HTTPException(HTTP_SC_NOTFOUND, 404);
    }
    if (!str::has_prefix(abspath, documentRoot)) {
        LOG_ERROR("Requested file is outside of the document root: %s",
                  abspath.c_str());
        // ファイルが存在することを知らせたくないので 404 を返す。
        throw HTTPException(HTTP_SC_NOTFOUND, 404);
    }
}

// -----------------------------------
void Servent::handshakeLocalFile(const char *fn, HTTP& http)
{
    std::string documentRoot;
    documentRoot = sys->realPath(peercastApp->getPath()) + sys->getDirectorySeparator();

    String fileName = documentRoot.c_str();
    fileName.append(fn);

    LOG_DEBUG("Writing HTML file: %s", fileName.cstr());

    WriteBufferedStream bufferedSock(sock.get());
    HTML html("", bufferedSock);

    const char* mimeType = fileNameToMimeType(fileName);
    if (mimeType == nullptr)
        throw HTTPException(HTTP_SC_NOTFOUND, 404);

    if (strcmp(mimeType, MIME_HTML) == 0)
    {
        if (str::contains(fn, "play.html"))
        {
            // 視聴ページだった場合はあらかじめチャンネルのリレーを開
            // 始しておく。

            auto vec = str::split(fn, "?");
            if (vec.size() != 2)
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            String id = cgi::Query(vec[1]).get("id").c_str();

            if (id.isEmpty())
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            ChanInfo info;
            if (!servMgr->getChannel(id.cstr(), info, true))
                throw HTTPException(HTTP_SC_NOTFOUND, 404);
        }

        char *args = strstr(fileName.cstr(), "?");
        if (args)
            *args = '\0';

        auto req = http.getRequest();

        validFileOrThrow(fileName.c_str(), documentRoot);

        html.writeOK(MIME_HTML);
        HTTPRequestScope scope(req);
        html.writeTemplate(fileName.cstr(), req.queryString.c_str(), scope);
    }else
    {
        validFileOrThrow(fileName.c_str(), documentRoot);

        html.writeRawFile(fileName.cstr(), mimeType);
    }
}

