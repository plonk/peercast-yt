#include "public.h"
#include "str.h"
#include "dmstream.h"
#include "template.h"
#include "jrpc.h"

// ------------------------------------------------------------
PublicController::PublicController(const std::string& documentRoot)
    : mapper("/public", documentRoot)
{
}

// ------------------------------------------------------------
std::string PublicController::MIMEType(const std::string& path)
{
    using namespace str;

    if (contains(path, ".htm"))
    {
        return MIME_HTML;
    }else if (contains(path, ".css"))
    {
        return MIME_CSS;
    }else if (contains(path, ".jpg"))
    {
        return MIME_JPEG;
    }else if (contains(path, ".gif"))
    {
        return MIME_GIF;
    }else if (contains(path, ".png"))
    {
        return MIME_PNG;
    }else if (contains(path, ".js"))
    {
        return MIME_JS;
    }else
    {
        return "application/octet-stream";
    }
}

// ------------------------------------------------------------
static std::string getTIP()
{
    // TODO: ちゃんと servMgr から取得しましょうね。
    return "127.0.0.1:7144";
}

static std::string formatUptime(unsigned int totalMinutes)
{
    auto minutes = totalMinutes % 60;
    auto hours = totalMinutes / 60;

    return std::to_string(hours) + ":" + std::to_string(minutes);
}

static std::string getDirectPermission()
{
    // TODO:
    return "0";
}

// ------------------------------------------------------------
// このサーバーから配信しているチャンネルの index.txt を作る
std::string PublicController::createChannelIndex()
{
    std::string res;
    json::array_t channels = JrpcApi().getChannels({});
    auto notBroadcasting = [] (json channel) { return !channel["status"]["isBroadcasting"]; };
    //auto end = std::remove_if(channels.begin(), channels.end(), notBroadcasting);
// [{"channelId":"E3E4530F08C624B7FE8A9AABC1237B88",
//"info":{"bitrate":859,"comment":"","contentType":"FLV","desc":"","genre":"","mimeType":"video/flv","name":"しいな再放送","url":""},
// "status":{"isBroadcasting":true,"isDirectFull":null,"isReceiving":false,"isRelayFull":false,"localDirects":0,"localRelays":0,"source":"file:///home/plonk/siina.flv","status":"Receiving","totalDirects":0,"totalRelays":0,"uptime":6},
//"track":{"album":"","creator":"","genre":"","name":"","url":""},
//"yellowPages":[]}]

    LOG_DEBUG("%s", json(channels).dump().c_str());
    for (auto p = channels.begin(); p != channels.end(); p++)
    {
        std::vector<std::string> vec = {
            /*  0 */ (*p)["info"]["name"],
            /*  1 */ (*p)["channelId"],
            /*  2 */ getTIP(),
            /*  3 */ (*p)["info"]["url"],
            /*  4 */ (*p)["info"]["genre"],
            /*  5 */ (*p)["info"]["desc"],
            /*  6 */ std::to_string((int) (*p)["status"]["totalDirects"]),
            /*  7 */ std::to_string((int) (*p)["status"]["totalRelays"]),
            /*  8 */ std::to_string((int) (*p)["info"]["bitrate"]),
            /*  9 */ (*p)["info"]["contentType"],
            /* 10 */ (*p)["track"]["creator"],
            /* 11 */ (*p)["track"]["album"],
            /* 12 */ (*p)["track"]["name"],
            /* 13 */ (*p)["track"]["url"],
            /* 14 */ cgi::escape((*p)["info"]["name"]),
            /* 15 */ formatUptime((*p)["status"]["uptime"]),
            /* 16 */ "click",
            /* 17 */ (*p)["info"]["comment"],
            /* 18 */ getDirectPermission(),
        };
        res += str::join("<>", vec) + "\n";
    }

    return res;
}

// ------------------------------------------------------------
HTTPResponse PublicController::operator()(const HTTPRequest& request, Stream& stream, Host& remoteHost)
{
    auto fn = request.path.c_str();

    if (strcmp(fn, "/public/") == 0)
    {
        return HTTPResponse::redirectTo("/public/index.html");
    }else if (strcmp(fn, "/public/index.txt") == 0)
    {
        return HTTPResponse::ok({{"Content-Type","text/plain"}}, createChannelIndex());
    }else if (strcmp(fn, "/public/index.html") == 0)
    {
        FileStream file;
        DynamicMemoryStream mem;
        file.openReadOnly((mapper.documentRoot + "/index.html").c_str());
        HTTPRequestScope scope(request);

        Template(request.queryString).prependScope(scope).readTemplate(file, &mem, 0);
        return HTTPResponse::ok({{"Content-Type","text/html"}}, mem.str());
    }else
    {
        auto path = mapper.toLocalFilePath(fn);
        if (path.empty())
            return HTTPResponse::notFound();
        else
        {
            auto type = MIMEType(path);

            DynamicMemoryStream mem;
            FileStream file;
            std::map<std::string,std::string> additionalHeaders;

            try
            {
                file.openReadOnly(path.c_str());
                file.writeTo(mem, file.length());
            }catch (StreamException &)
            {
                LOG_DEBUG("StreamException in %s", __FUNCTION__);
            }
            file.close();

            return HTTPResponse::ok({{"Content-Type",type}}, mem.str());
        }
    }
}
