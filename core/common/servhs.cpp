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

#include "sstream.h"
#include "defer.h"
#include <assert.h>

#include <queue>

#include "chunker.h"
#include "commands.h"

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
        return nullptr;

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
    int pid = -1;
    if (success)
    {
        pid = script.pid(); // wait した後は script.pid() がリセットされるので値を取っておく。
        LOG_DEBUG("script started (pid = %d)", pid);
    }else
    {
        LOG_ERROR("failed to start script `%s`", filePath.c_str());
        throw HTTPException(HTTP_SC_SERVERERROR, 500);
    }
    Stream& stream = *script.inputStream();

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
        LOG_ERROR("child process (PID %d) terminated abnormally", pid);
    }else
    {
        LOG_DEBUG("child process (PID %d) exited normally (status %d)", pid, status);
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

        if (handshakeAuth(http, fn))
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
                        newInfo.track.title = cgi::unescape(songArg).c_str();

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
            handshakePLS(info, http);
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

        http.readHeaders();
        JrpcApi api;
        std::string response = api.getVersionInfo(nlohmann::json::array_t()).dump();

        http.writeLine(HTTP_SC_OK);
        http.writeLineF("%s %zu", HTTP_HS_LENGTH, response.size());
        http.writeLine("");
        http.writeString(response.c_str());
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
        }else if (handshakeAuth(http, fn))
        {
            invokeCGIScript(http, fn);
        }
    }else if (str::is_prefix_of("/cmd?", fn))
    {
        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        cgi::Query query(fn + strlen("/cmd?"));
        auto q = query.get("q");
        if (q == "")
        {
            throw HTTPException(HTTP_SC_BADREQUEST, 400, "q missing");
        }else
        {
            if (handshakeAuth(http, fn))
            {
                this->type = T_COMMAND;

                http.readHeaders();
#if 1
                // Use HTTP/1.0 without Content-Length.
                http.writeResponseStatus("HTTP/1.0", 200);
                http.writeResponseHeaders
                    ({
                        {"Content-Type", "text/plain; charset=utf-8"},
                        {"Connection", "close"},
                    });
                auto& stdoutVar = http;
#else
                // Use HTTP/1.1 with chunked encoding.
                http.writeResponseStatus("HTTP/1.1" /* important */, 200);
                http.writeResponseHeaders
                    ({
                        {"Transfer-Encoding", "chunked"},
                        {"Content-Type", "text/plain; charset=utf-8"},
                        {"Connection", "close"},
                    });

                Chunker stdoutVar(http);
                Defer defer([&]() {
                                try {
                                    // Finalize the stream by sending the end-of-stream marker.
                                    stdoutVar.close();
                                } catch (GeneralException&) {
                                    // We don't want to throw here if the underlying socket is closed.
                                }
                            });
#endif

                try {
                    auto cancellationRequested = [&]() -> bool { return !(thread.active() && sock->active()); };
                    Commands::system(stdoutVar, q, cancellationRequested);
                } catch (GeneralException& e)
                {
                    LOG_ERROR("Error: cmd '%s': %s", query.get("q").c_str(), e.msg);
                }
            }
        }
#if 0
    }else if (strcmp("/speedtest", fn) == 0)
    {
        //getSpeedTest();
        
        http.readHeaders();
        http.writeLine("HTTP/1.1 200 OK");
        http.writeLine("Content-Type: application/binary");
        http.writeLine("Server: " PCX_AGENT);
        http.writeLine("Connection: close");
        http.writeLine("Transfer-Encoding: chunked");
        http.writeLine("");

        auto generateRandomBytes = [](size_t size) -> std::string {
                                       std::string buf;
                                       peercast::Random r;

                                       for (size_t i = 0; i < size; ++i)
                                           buf += (char) (r.next() & 0xff);
                                       return buf;
                                   };
        auto block = generateRandomBytes(1024);
        const auto startTime = sys->getDTime();
        size_t sent = 0;
        Chunker stream(http);

        while (sys->getDTime() - startTime < 15.0) {
            stream.write(block.data(), block.size());
            sent += 1024;
        }
        auto t = sys->getDTime();
        stream.close();
        LOG_INFO("[Speedtest] client %s downloaded at %.0f bps", sock->host.ip.str().c_str(), (sent * 8) / (t - startTime));
#endif
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
#include "dechunker.h"
#include <limits>
void Servent::handshakePOST(HTTP &http)
{
    auto vec = str::split(http.cmdLine, " ");
    if (vec.size() != 3)
        throw HTTPException(HTTP_SC_BADREQUEST, 400);

    std::string args;
    auto vec2 = str::split(vec[1], "?", 2);

    if (vec2.size() == 2)
        args = vec2[1];

    std::string path = vec2[0];

    if (path == "/api/1")
    {
        // JSON API

        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        if (handshakeAuth(http, args.c_str()))
            handshakeJRPC(http);
    }else if (path == "/")
    {
        // HTTP Push

        if (!isAllowed(ALLOW_BROADCAST))
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        if (!isPrivate())
            throw HTTPException(HTTP_SC_FORBIDDEN, 403);

        handshakeHTTPPush(args);
    }else if (path == "/admin")
    {
        if (!isAllowed(ALLOW_HTML))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        http.readHeaders();
        auto req = http.getRequest();
        LOG_DEBUG("Admin (POST)");
        handshakeCMD(http, req.body);
#if 0
    }else if (path == "/speedtest")
    {
        // postSpeedtest();
        http.readHeaders();

        std::shared_ptr<Stream> stream;
        if (http.headers.get("Transfer-Encoding") == "chunked") {
            stream = std::make_shared<Dechunker>(http);
        } else {
            auto tmp = std::make_shared<IndirectStream>();
            tmp->init(&http);
            stream = tmp;
        }

        int remaining = std::numeric_limits<int>::max();
        bool bounded = false;
        if (http.headers.get("Content-Length") != "") {
            remaining = std::atoi(http.headers.get("Content-Length").c_str());
            bounded = true;
        }

        const auto startTime = sys->getDTime();
        size_t received = 0;
        char buffer[1024];

        while (sys->getDTime() - startTime <= 20.0 && remaining > 0) {
            int r;
            try {
                r = stream->read(buffer, std::min(1024, remaining));
            } catch (StreamException& e) {
                break;
            }
            received += r;
            if (bounded)
                remaining -= r;
        }
        auto t = sys->getDTime();

        if ((t - startTime) < 15.0) {
            auto res = HTTPResponse::badRequest("Time less than 15 seconds. Need more data.");
            http.send(res);
        } else {
            double bps = (received * 8) / (t - startTime);
            LOG_INFO("[Speedtest] client %s uploaded %d bytes in %.3f seconds (%.0f bps)",
                     sock->host.ip.str().c_str(),
                     (int) received,
                     t - startTime,
                     (received * 8) / (t - startTime));

            auto res = HTTPResponse::ok({{"Content-Type", "text/plain"}}, str::format("%.0f bps", bps));
            http.send(res);
        }
#endif
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

    if (!isAllowed(ALLOW_NETWORK) || !isFiltered(ServFilter::F_NETWORK))
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
        sock = nullptr;                  // release this servent but dont close socket.
    }else
    {
        if (!servMgr->acceptGIV(sock))
            throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

        LOG_DEBUG("Accepted GIV PCP from: %s", ipstr);
        sock = nullptr;                  // release this servent but dont close socket.
    }
}

// -----------------------------------
void Servent::handshakeSOURCE(char * in, bool isHTTP)
{
    if (!isAllowed(ALLOW_BROADCAST))
        throw HTTPException(HTTP_SC_UNAVAILABLE, 503);

    char *mount = nullptr;

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
    sock = nullptr;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::handshakeHTTP(HTTP &http, bool isHTTP)
{
    LOG_DEBUG("%s \"%s\"", sock->host.ip.str().c_str(), http.cmdLine);

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
        sock = nullptr;    // socket is taken over by channel, so don`t close it
    }else
    {
        // リクエスト解釈失敗

        throw HTTPException(HTTP_SC_BADREQUEST, 400);
    }
}

// -----------------------------------
#include "sslclientsocket.h"
void Servent::handshakeIncoming()
{
    setStatus(S_HANDSHAKE);

    if (servMgr->flags.get("enableSSLServer")) {
        if (sock->readReady(sock->readTimeout)) {
            char c = sock->peekChar();
            LOG_TRACE("peekChar -> %d", (unsigned char) c);
            if (c == 22) { // SSL handshake detected.
                sock = SslClientSocket::upgrade(sock);
            }
        }
    }

    char buf[8192];

    if ((size_t)sock->readLine(buf, sizeof(buf)) >= sizeof(buf)-1)
    {
        throw HTTPException(HTTP_SC_URITOOLONG, 414);
    }

    bool isHTTP = (stristr(buf, HTTP_PROTO1) != nullptr);

    if (isHTTP)
        LOG_TRACE("HTTP from %s '%s'", sock->host.str().c_str(), buf);
    else
        LOG_TRACE("Connect from %s '%s'", sock->host.str().c_str(), buf);

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

    processStream(info);
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
void Servent::handshakePLS(ChanInfo &info, HTTP& http)
{
    std::string url;

    url = getLocalURL(http.headers.get("Host"));

    PlayList::TYPE type = PlayList::getPlayListType(info.contentType);

    writePLSHeader(*sock, type);

    PlayList pls(type, 1);
    pls.wmvProtocol = servMgr->wmvProtocol;
    pls.addChannel(url.c_str(), info);
    pls.write(*sock);
}

// -----------------------------------
std::string Servent::getLocalURL(const std::string& hostHeader)
{
    if (!sock)
        throw StreamException("Not connected");

    if (hostHeader.empty()) {
        Host h;

        if (sock->host.localIP())
            h = sock->getLocalHost();
        else
            h = servMgr->serverHost;

        h.port = servMgr->serverHost.port;

        return str::STR("http://", h.str());
    } else {
        return str::STR("http://", hostHeader);
    }
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
bool Servent::handshakeAuth(HTTP &http, const char *args)
{
    std::string user, pass;

    http.readHeaders();

    if (sock->host.isLocalhost())
        return true;

    cgi::Query query(args);
    if (strlen(servMgr->password) && query.get("pass") == servMgr->password)
    {
        return true;
    }

    switch (servMgr->authType)
    {
    case ServMgr::AUTH_HTTPBASIC:
        if (http.headers.get("Authorization") != "") {
            HTTP::parseAuthorizationHeader(http.headers.get("Authorization"), user, pass);
            if (strlen(servMgr->password) && pass == servMgr->password) {
                return true;
            }
        }
        break;
    case ServMgr::AUTH_COOKIE:
        if (http.headers.get("Cookie") != "")
        {
            auto arg = http.headers.get("Cookie");
            LOG_TRACE("Got cookie: %s", arg.c_str());
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
                    cookie = gotCookie;
                    break;
                }
            }

            if (servMgr->cookieList.contains(cookie)){
                LOG_TRACE("Cookie ID found");
                return true;
            }
        }
        break;
    }

    // Auth failure
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
static std::string runProcess(std::function<void(Stream&)> action)
{
    StringStream ss;
    try {
        assert(AUX_LOG_FUNC_VECTOR != nullptr);
        AUX_LOG_FUNC_VECTOR->push_back([&](LogBuffer::TYPE type, const char* msg) -> void
                                      {
                                          if (type == LogBuffer::T_ERROR)
                                              ss.writeString("Error: ");
                                          else if (type == LogBuffer::T_WARN)
                                              ss.writeString("Warning: ");
                                          ss.writeLine(msg);
                                      });
        Defer defer([]() { AUX_LOG_FUNC_VECTOR->pop_back(); });

        action(ss);
    } catch(GeneralException& e) {
        ss.writeLineF("Error: %s", e.what());
    }
    return ss.str();
}

// -----------------------------------
void Servent::CMD_portcheck4(const char* cmd, HTTP& http, String& jumpStr)
{
    auto output = runProcess([](Stream& s)
                             {
                                 servMgr->setFirewall(4, ServMgr::FW_UNKNOWN);
                                 servMgr->checkFirewall();
                                 s.writeLineF("IPv4 firewall is %s",
                                              ServMgr::getFirewallStateString(servMgr->getFirewall(4)));
                             });

    auto res = HTTPResponse::ok({ {"Content-Type", "text/plain; charset=UTF-8"} }, output);
    http.send(res);
}

// -----------------------------------
void Servent::CMD_portcheck6(const char* cmd, HTTP& http, String& jumpStr)
{
    auto output = runProcess([](Stream& s)
                             {
                                 servMgr->setFirewall(6, ServMgr::FW_UNKNOWN);
                                 servMgr->checkFirewallIPv6();
                                 s.writeLineF("IPv6 firewall is %s",
                                              ServMgr::getFirewallStateString(servMgr->getFirewall(6)));
                             });

    auto res = HTTPResponse::ok({ {"Content-Type", "text/plain; charset=UTF-8"} }, output);
    http.send(res);
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

void Servent::CMD_customizeApperance(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    std::map<std::string,amf0::Value> dict;
    for (auto key : query.getKeys())
    {
        if (key == "cmd")
            continue;
        else if (key == "preferredTheme")
        {
            std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
            servMgr->preferredTheme = query.get(key);
        }else if (key == "accentColor")
        {
            std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
            servMgr->accentColor = query.get(key);
        }else
            LOG_WARN("Unexpected key `%s`", key.c_str());
    }

    http.send(http.headers.get("Referer").empty()
              ? HTTPResponse::redirectTo("/")
              : HTTPResponse::redirectTo(http.headers.get("Referer")));
}

void Servent::CMD_chooseLanguage(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);

    auto referer = http.headers.get("Referer");
    LOG_DEBUG("old referer: %s", referer.c_str());

    for (auto key : query.getKeys())
    {
        if (key == "cmd")
            continue;
        else if (key == "htmlPath")
        {
            std::lock_guard<std::recursive_mutex> cs(servMgr->lock);

            auto newHtmlPath = "html/" + query.get(key);
            auto vec = Regexp("html/[^/]+").exec(referer);
            if (vec.size())
            {
                auto pos = referer.find(vec[0]);
                if (pos != std::string::npos)
                {
                    referer = referer.substr(0, pos) + newHtmlPath + referer.substr(pos + vec[0].size());
                    LOG_DEBUG("new referer: %s", referer.c_str());
                }
            }else{
                LOG_WARN("CMD_chooseLanguage: Failed to rewrite referer: %s", referer.c_str());
            }

            Sys::strcpy_truncate(servMgr->htmlPath, sizeof(servMgr->htmlPath), newHtmlPath.c_str());

            http.send(referer.empty()
                      ? HTTPResponse::redirectTo("/")
                      : HTTPResponse::redirectTo(referer));
            return;
        }else
            LOG_WARN("Unexpected key `%s`", key.c_str());
    }
    http.send(HTTPResponse::badRequest()); // No htmlPath key.
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
    servMgr->flags.get("randomizeBroadcastingChannelID") = false;

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
            chanMgr->setBroadcastMsg(cgi::unescape(arg).c_str());
        }
        else if (strcmp(curr, "pcmsg") == 0)
        {
            servMgr->rootMsg = cgi::unescape(arg).c_str();
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
                auto str = cgi::unescape(arg);

                currFilter = &servMgr->filters[servMgr->numFilters];
                currFilter->init();
                currFilter->setPattern(str.c_str());
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
                servMgr->channelDirectory->addFeed(cgi::unescape(arg).c_str());
            }
        }

        // client
        else if (strcmp(curr, "clientactive") == 0)
            servMgr->autoConnect = getCGIargBOOL(arg);
        else if (strcmp(curr, "yp") == 0)
        {
            auto val = cgi::unescape(arg);
            if (val != servMgr->rootHost.cstr())
            {
                LOG_INFO("Root host changed from '%s' to '%s'", servMgr->rootHost.cstr(), val.c_str());
                servMgr->rootHost = val.c_str();
                servMgr->rootMsg = "";
            }
        }
        else if (strcmp(curr, "deadhitage") == 0)
            chanMgr->deadHitAge = getCGIargINT(arg);
        else if (strcmp(curr, "refresh") == 0)
            servMgr->refreshHTML = getCGIargINT(arg);
        else if (strcmp(curr, "chat") == 0)
            servMgr->chat = getCGIargBOOL(arg);
        else if (strcmp(curr, "randomizechid") == 0)
            servMgr->flags.get("randomizeBroadcastingChannelID") = getCGIargBOOL(arg);
        else if (strcmp(curr, "public_directory") == 0)
            servMgr->publicDirectoryEnabled = true;
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
        else if (strcmp(curr, "preferredTheme") == 0)
            servMgr->preferredTheme = arg;
        else if (strcmp(curr, "accentColor") == 0)
            servMgr->accentColor = arg;
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
            ipstr = Host(sys->getInterfaceIPv4Address(), newPort).str();
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

void Servent::CMD_applyflags(const char* cmd, HTTP& http, String& jumpStr)
{
    std::lock_guard<std::recursive_mutex> cs(servMgr->lock);

    cgi::Query query(cmd);

    servMgr->flags.forEachFlag([&](Flag& flag)
    {
        if (query.get(flag.name) == "") {
            flag.currentValue = false;
        } else {
            flag.currentValue = true;
        }
    });

    peercastInst->saveSettings();
    peercast::notifyMessage(ServMgr::NT_PEERCAST, "フラグ設定を保存しました。");
    peercastApp->updateSettings();

    jumpStr.sprintf("/%s/flags.html", servMgr->htmlPath);
}

void Servent::CMD_fetch(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    ChanInfo info;

    auto curl = query.get("url");
    info.name  = str::truncate_utf8(str::valid_utf8(query.get("name")), 255);
    info.desc  = str::truncate_utf8(str::valid_utf8(query.get("desc")), 255);
    info.genre = str::truncate_utf8(str::valid_utf8(query.get("genre")), 255);
    info.url   = str::truncate_utf8(str::valid_utf8(query.get("contact")), 255);
    info.bitrate = atoi(query.get("bitrate").c_str());
    auto type = query.get("type");
    info.setContentType(type.c_str());

    // id がセットされていないチャンネルがあるといろいろまずいので、事
    // 前に設定してから登録する。
    setBroadcastIdChannelId(info, chanMgr->broadcastID);

    auto c = chanMgr->createChannel(info);
    if (c) {
        if (query.get("ipv") == "6") {
            c->ipVersion = Channel::IP_V6;
            LOG_INFO("Channel IP version set to 6");

            // YPv6ではIPv6のポートチェックができないのでがんばる。
            servMgr->checkFirewallIPv6();
        }
        c->startURL(curl.c_str());
    }

    jumpStr.sprintf("/%s/relays.html", servMgr->htmlPath);
}

void Servent::CMD_fetch_feeds(const char* cmd, HTTP& http, String& jumpStr)
{
    servMgr->channelDirectory->update(ChannelDirectory::kUpdateManual);

    // Referer に戻すと、admin?cmd=fetch_feeds でログインした場合に同
    // URLが Referer になってループ転送になるので決め打ちする。
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

        if (!http.headers.get("Referer").empty())
            jumpStr.sprintf("%s", http.headers.get("Referer").c_str());
        else
            jumpStr.sprintf("/%s/connections.html", servMgr->htmlPath);
        return;
    }else
    {
        http.send(HTTPResponse::notFound("servent not found"));
        return;
    }
}

void Servent::CMD_chanfeedlog(const char* cmd, HTTP& http, String& jumpStr)
{
    cgi::Query query(cmd);
    int index = atoi(query.get("index").c_str());

    try {
        HTTPResponse res(200, { {"Content-Type","text/plain; charset=UTF-8"} });
        res.body = servMgr->channelDirectory->feeds().at(index).log;
        http.send(res);
    } catch (std::out_of_range& e) {
        HTTPResponse res(404, { {"Content-Type","text/plain; charset=UTF-8"} });
        res.body = e.what();
        http.send(res);
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
    {
        c->thread.shutdown();
        sys->waitThread(&c->thread);
    }

    if (!http.headers.get("Referer").empty())
        jumpStr = http.headers.get("Referer").c_str();
    else
        jumpStr.sprintf("/%s/relays.html", servMgr->htmlPath);
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
            auto chl = chanMgr->findHitList(c->info);
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

    if (!http.headers.get("Referer").empty())
        jumpStr = http.headers.get("Referer").c_str();
    else
        jumpStr.sprintf("/%s/relays.html", servMgr->htmlPath);
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

            info.name    = str::truncate_utf8(str::valid_utf8(query.get("name")), 255);
            info.genre   = str::truncate_utf8(str::valid_utf8(query.get("genre")), 255);
            info.desc    = str::truncate_utf8(str::valid_utf8(query.get("desc")), 255);
            info.url     = str::truncate_utf8(str::valid_utf8(query.get("url")), 255);
            info.comment = str::truncate_utf8(str::valid_utf8(query.get("comment")), 255);
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
    if (ch == nullptr)
        throw HTTPException(HTTP_SC_SERVERERROR, 500, "No such channel");

    // I don't think this operation is safe

    ChanInfo info = ch->info;

    info.name    = str::truncate_utf8(str::valid_utf8(query.get("name")), 255);
    info.desc    = str::truncate_utf8(str::valid_utf8(query.get("desc")), 255);
    info.genre   = str::truncate_utf8(str::valid_utf8(query.get("genre")), 255);
    info.url     = str::truncate_utf8(str::valid_utf8(query.get("contactURL")), 255);
    info.comment = str::truncate_utf8(str::valid_utf8(query.get("comment")), 255);

    info.track.contact = str::truncate_utf8(str::valid_utf8(query.get("track.contactURL")), 255);
    info.track.title   = str::truncate_utf8(str::valid_utf8(query.get("track.title")), 255);
    info.track.artist  = str::truncate_utf8(str::valid_utf8(query.get("track.artist")), 255);
    info.track.album   = str::truncate_utf8(str::valid_utf8(query.get("track.album")), 255);
    info.track.genre   = str::truncate_utf8(str::valid_utf8(query.get("track.genre")), 255);

    bool changed = ch->updateInfo(info);
    if (!changed)
        throw HTTPException(HTTP_SC_SERVERERROR, 500, "Failed to update channel info");

    jumpStr.sprintf("/%s/relays.html", servMgr->htmlPath);
}

static std::string dumpHit(std::shared_ptr<ChanHit> hit)
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

static std::string dumpHitList(std::shared_ptr<ChanHitList> hitlist)
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

void Servent::handshakeCMD(HTTP& http, const std::string& query)
{
    String jumpStr;

    if (!handshakeAuth(http, query.c_str()))
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
        }else if (cmd == "applyflags")
        {
            CMD_applyflags(query.c_str(), http, jumpStr);
        }else if (cmd == "bump")
        {
            CMD_bump(query.c_str(), http, jumpStr);
        }else if (cmd == "chanfeedlog")
        {
            CMD_chanfeedlog(query.c_str(), http, jumpStr);
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
        }else if (cmd == "customizeAppearance")
        {
            CMD_customizeApperance(query.c_str(), http, jumpStr);
        }else if (cmd == "chooseLanguage")
        {
            CMD_chooseLanguage(query.c_str(), http, jumpStr);
        }else{
            throw HTTPException(HTTP_SC_BADREQUEST, 400);
        }
    }catch (HTTPException &e)
    {
        http.writeLine(e.msg);
        http.writeLineF("%s %s", HTTP_HS_SERVER, PCX_AGENT);
        http.writeLineF("%s %s", HTTP_HS_CONTENT, "text/html; charset=utf-8");
        http.writeLine("");

        http.writeStringF("<h1>ERROR - %s</h1>\n", cgi::escape_html(e.msg).c_str());

        if (e.additionalMessage != "")
        {
            http.writeStringF("<pre>%s</pre>\n", cgi::escape_html(e.additionalMessage).c_str());
        }

        LOG_ERROR("html: %s", e.msg);
    }

    if (jumpStr != "")
    {
        http.writeLine(HTTP_SC_FOUND);
        http.writeLineF("Location: %s", jumpStr.c_str());
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
static XML::Node *createChannelXML(std::shared_ptr<ChanHitList> chl)
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

        auto chl = chanMgr->hitlist;
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
            http.getAuthUserPass(nullptr, pwd, 0, plen);
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
    std::string setup = http.Stream::read(size);
    LOG_DEBUG("setup: %s", str::inspect(setup).c_str());

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

    setBroadcastIdChannelId(info, chanMgr->broadcastID);

    auto c = chanMgr->findChannelByID(info.id);
    if (c)
    {
        LOG_INFO("WMHTTP Push channel already active, closing old one");
        c->thread.shutdown();
    }

    info.comment = chanMgr->broadcastMsg;

    c = chanMgr->createChannel(info);
    if (!c)
        throw HTTPException(HTTP_SC_SERVERERROR, 500);

    c->startWMHTTPPush(sock);
    sock = nullptr;    // socket is taken over by channel, so don`t close it
}

// -----------------------------------
void Servent::setBroadcastIdChannelId(ChanInfo& info, const GnuID& broadcastID)
{
    // ブロードキャストIDをセットし、info にセットされたチャンネル名、
    // ジャンル、ビットレートからチャンネルIDを設定する。ただし、チャ
    // ンネルIDランダム化フラグがセットされていた場合はランダムなチャ
    // ンネルIDをセットする。

    info.bcID = broadcastID;

    if (servMgr->flags.get("randomizeBroadcastingChannelID")) {
        info.id = GnuID::random();
    } else {
        info.id = broadcastID;
        info.id.encode(nullptr, info.name, info.genre, info.bitrate);
    }
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

    setBroadcastIdChannelId(info, broadcastID);

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

    c = chanMgr->createChannel(info);
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
    sock = nullptr;    // socket is taken over by channel, so don`t close it
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

    if (servMgr->flags.get("randomizeBroadcastingChannelID")) {
        info.id = GnuID::random();
    } else {
        info.id = chanMgr->broadcastID;
        info.id.encode(nullptr, info.name.cstr(), loginMount.cstr(), info.bitrate);
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
    try {
        documentRoot = sys->realPath(peercastApp->getPath()) + sys->getDirectorySeparator();
    } catch (GeneralException &e) {
        LOG_ERROR("documentRoot: %s (%s)", e.what(), sys->fromFilenameEncoding(peercastApp->getPath()).c_str());
        throw HTTPException(HTTP_SC_SERVERERROR, 500);
    }

    String fileName = documentRoot.c_str();
    fileName.append(fn);

    LOG_TRACE("Writing HTML file: %s", sys->fromFilenameEncoding(fileName.cstr()).c_str());

    WriteBufferedStream bufferedSock(sock.get());
    HTML html("", bufferedSock);

    const char* mimeType = fileNameToMimeType(fileName);
    if (mimeType == nullptr)
        throw HTTPException(HTTP_SC_NOTFOUND, 404);

    if (strcmp(mimeType, MIME_HTML) == 0)
    {
        auto req = http.getRequest();
        GenericScope locals;
        HTTPRequestScope reqScope(req);
        std::vector<Template::Scope*> scopes = { &reqScope, &locals };

        if (str::contains(fn, "/play.html"))
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

            auto ch = chanMgr->findChannelByID(GnuID(id.c_str()));
            if (!ch)
                throw HTTPException(HTTP_SC_NOTFOUND, 404);

            locals.vars["channel"] = ch->getState();
        }else if (str::contains(fn, "/relayinfo.html") || str::contains(fn, "/head.html"))
        {
            auto vec = str::split(fn, "?");
            if (vec.size() != 2)
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            String id = cgi::Query(vec[1]).get("id").c_str();

            if (id.isEmpty())
                throw HTTPException(HTTP_SC_BADREQUEST, 400);

            auto ch = chanMgr->findChannelByID(GnuID(id.c_str()));
            locals.vars["channel"] = ch ? ch->getState() : nullptr;
        }else if (str::contains(fn, "connections.html") || str::contains(fn, "editinfo.html"))
        {
            auto vec = str::split(fn, "?");
            if (vec.size() == 2)
            {
                String id = cgi::Query(vec[1]).get("id").c_str();

                if (!id.isEmpty())
                {
                    auto ch = chanMgr->findChannelByID(GnuID(id.c_str()));
                    locals.vars["channel"] = ch ? ch->getState() : nullptr;
                }
            }
        }

        char *args = strstr(fileName.cstr(), "?");
        if (args)
            *args = '\0';


        validFileOrThrow(fileName.c_str(), documentRoot);

        html.writeOK("text/html; charset=utf-8");
        html.writeTemplate(fileName.cstr(), req.queryString.c_str(), scopes);
    }else
    {
        validFileOrThrow(fileName.c_str(), documentRoot);

        html.writeRawFile(fileName.cstr(), mimeType);
    }
}
