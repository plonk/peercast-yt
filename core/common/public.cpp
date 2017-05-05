#include "public.h"
#include "str.h"
#include "dmstream.h"
#include "template.h"
#include "jrpc.h"

using namespace std;

// ------------------------------------------------------------
PublicController::PublicController(const string& documentRoot)
    : mapper("/public", documentRoot)
{
}

// ------------------------------------------------------------
string PublicController::MIMEType(const string& path)
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
static string getTIP()
{
    return servMgr->serverHost.str();
}

static string formatUptime(unsigned int totalMinutes)
{
    auto minutes = totalMinutes % 60;
    auto hours = totalMinutes / 60;

    return to_string(hours) + ":" + to_string(minutes);
}

static string getDirectPermission()
{
    // TODO:
    return "0";
}

// ------------------------------------------------------------
// このサーバーから配信しているチャンネルの index.txt を作る
string PublicController::createChannelIndex()
{
    string res;
    json::array_t channels = JrpcApi().getChannels({});
    auto notBroadcasting = [] (json channel) { return !channel["status"]["isBroadcasting"]; };
    channels.erase(remove_if(channels.begin(), channels.end(), notBroadcasting),
                   channels.end());

    LOG_DEBUG("%s", json(channels).dump().c_str());
    for (auto it = channels.begin(); it != channels.end(); it++)
    {
        auto& c = *it;
        vector<string> vec = {
            /*  0 */ c["info"]["name"],
            /*  1 */ c["channelId"],
            /*  2 */ getTIP(),
            /*  3 */ c["info"]["url"],
            /*  4 */ c["info"]["genre"],
            /*  5 */ c["info"]["desc"],
            /*  6 */ to_string((int) c["status"]["totalDirects"]),
            /*  7 */ to_string((int) c["status"]["totalRelays"]),
            /*  8 */ to_string((int) c["info"]["bitrate"]),
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
HTTPResponse PublicController::operator()(const HTTPRequest& req, Stream& stream, Host& remoteHost)
{
    if (req.path == "/public")
    {
        return HTTPResponse::redirectTo("/public/");
    }else if (req.path == "/public/")
    {
        return HTTPResponse::redirectTo("/public/index.html");
    }else if (req.path == "/public/index.txt")
    {
        return HTTPResponse::ok({{"Content-Type","text/plain"}}, createChannelIndex());
    }else if (req.path == "/public/index.html")
    {
        FileStream file;
        DynamicMemoryStream mem;
        HTTPRequestScope scope(req);

        file.openReadOnly(mapper.documentRoot + str::replace_prefix(req.path, "/public", ""));
        Template(req.queryString).prependScope(scope).readTemplate(file, &mem, 0);
        return HTTPResponse::ok({{"Content-Type","text/html"}}, mem.str());
    }else if (req.path == "/public/play.html")
    {
        FileStream file;
        DynamicMemoryStream mem;
        HTTPRequestScope scope(req);

        String id = cgi::Query(req.queryString).get("id").c_str();
        ChanInfo info;
        bool success = servMgr->getChannel(id.cstr(), info, true);

        if (success)
        {
            file.openReadOnly(mapper.documentRoot + str::replace_prefix(req.path, "/public", ""));
            Template(req.queryString).prependScope(scope).readTemplate(file, &mem, 0);
            return HTTPResponse::ok({{"Content-Type","text/html"}}, mem.str());
        }else
        {
            return HTTPResponse::notFound();
        }
    }else
    {
        auto path = mapper.toLocalFilePath(req.path);
        if (path.empty())
            return HTTPResponse::notFound();
        else
        {
            auto type = MIMEType(path);

            DynamicMemoryStream mem;
            FileStream file;

            try
            {
                file.openReadOnly(path.c_str());
                file.writeTo(mem, file.length());
            }catch (StreamException &)
            {
                LOG_DEBUG("StreamException in %s", __FUNCTION__);
            }
            file.close();

            std::string body = mem.str();
            map<string,string> headers = {
                {"Content-Type",type},
                {"Content-Length",to_string(body.size())}
            };
            return HTTPResponse::ok(headers, body);
        }
    }
}
