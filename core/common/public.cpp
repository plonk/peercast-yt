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
    channels.erase(std::remove_if(channels.begin(), channels.end(), notBroadcasting),
                   channels.end());

    LOG_DEBUG("%s", json(channels).dump().c_str());
    for (auto it = channels.begin(); it != channels.end(); it++)
    {
        auto& c = *it;
        std::vector<std::string> vec = {
            /*  0 */ c["info"]["name"],
            /*  1 */ c["channelId"],
            /*  2 */ getTIP(),
            /*  3 */ c["info"]["url"],
            /*  4 */ c["info"]["genre"],
            /*  5 */ c["info"]["desc"],
            /*  6 */ std::to_string((int) c["status"]["totalDirects"]),
            /*  7 */ std::to_string((int) c["status"]["totalRelays"]),
            /*  8 */ std::to_string((int) c["info"]["bitrate"]),
            /*  9 */ c["info"]["contentType"],
            /* 10 */ c["track"]["creator"],
            /* 11 */ c["track"]["album"],
            /* 12 */ c["track"]["name"],
            /* 13 */ c["track"]["url"],
            /* 14 */ cgi::escape(c["info"]["name"]),
            /* 15 */ formatUptime(c["status"]["uptime"]),
            /* 16 */ "click",
            /* 17 */ c["info"]["comment"],
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
    }else if (strcmp(fn, "/public/play.html") == 0)
    {
        FileStream file;
        DynamicMemoryStream mem;
        file.openReadOnly((mapper.documentRoot + "/play.html").c_str());
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
