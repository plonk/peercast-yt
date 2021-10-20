#include <algorithm>
#include <iterator>

#include "public.h"
#include "str.h"
#include "sstream.h"
#include "template.h"
#include "jrpc.h"

using namespace std;

// ------------------------------------------------------------
PublicController::PublicController(const string& documentRoot)
    : mapper("/public", documentRoot)
{
}

// ------------------------------------------------------------
static string MIMEType(const string& path)
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

// ------------------------------------------------------------
string PublicController::formatUptime(unsigned int totalSeconds)
{
    auto totalMinutes = totalSeconds / 60;
    auto minutes = totalMinutes % 60;
    auto hours = totalMinutes / 60;

    return str::format("%02d:%02d", hours, minutes);
}

// ------------------------------------------------------------
static string getDirectPermission()
{
    // TODO:
    return "0";
}

// ------------------------------------------------------------
// このサーバーから配信しているチャンネルとCINから教わったチャンネルの index.txt を作る
string PublicController::createChannelIndex()
{
    string res;
    JrpcApi api;
    json::array_t channels = api.getChannels({});
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

    json::array_t foundChannels = api.getChannelsFound({});
    LOG_DEBUG("%s", json(foundChannels).dump().c_str());
    for (auto& c : foundChannels) {
        if (c["hits"].size() == 0) {
            LOG_DEBUG("createChannelIndex: skipping `%s` because no hits", c["name"].get<std::string>().c_str());
            continue;
        }
        const std::string tip = c["hits"][0]["push"] ? "" : c["hits"][0]["ip"];
        vector<string> vec = {
            /*  0 */ c["name"],
            /*  1 */ c["id"],
            /*  2 */ tip,
            /*  3 */ c["url"],
            /*  4 */ c["genre"],
            /*  5 */ c["desc"],
            /*  6 */ to_string((int) c["hit_stat"]["listeners"]),
            /*  7 */ to_string((int) c["hit_stat"]["relays"]),
            /*  8 */ to_string((int) c["bitrate"]),
            /*  9 */ c["type"],
            /* 10 */ c["track"]["creator"],
            /* 11 */ c["track"]["album"],
            /* 12 */ c["track"]["name"],
            /* 13 */ c["track"]["url"],
            /* 14 */ cgi::escape(c["name"]),
            /* 15 */ formatUptime(c["uptime"]),
            /* 16 */ "click",
            /* 17 */ c["comment"],
            /* 18 */ to_string((int) c["hits"][0]["direct"]),
        };
        res += str::join("<>", vec) + "\n";
    }

    return res;
}

// ------------------------------------------------------------
vector<string>
PublicController::acceptableLanguages(const string& acceptLanguage)
{
    if (acceptLanguage == "")
        return {};

    vector<string> res;
    vector<pair<string,double> > tags;

    for (auto tagSpec : str::split(acceptLanguage, ","))
    {
        auto ws = str::split(tagSpec, ";");
        if (ws.size() == 1)
            tags.push_back(make_pair(ws[0], 1.0));
        else if (ws.size() > 1)
            tags.push_back(make_pair(ws[0], atof(str::replace_prefix(ws[1], "q=", "").c_str())));
        else
            throw runtime_error("parse error");
    }

    auto sortfunc = [](pair<string,double>& a,
                       pair<string,double>& b)
    {
        return a.second > b.second;
    };

    sort(tags.begin(), tags.end(), sortfunc);
    transform(tags.begin(), tags.end(),
                   back_inserter(res),
                   [](pair<string,double>& x) { return x.first; });
    return res;
}

// ------------------------------------------------------------
HTTPResponse PublicController::operator()(const HTTPRequest& req, Stream& stream, Host& remoteHost)
{
    vector<string> langs = acceptableLanguages(req.headers.get("Accept-Language"));

    if (req.path == "/public")
    {
        return HTTPResponse::redirectTo("/public/");
    }else if (req.path == "/public/")
    {
        return HTTPResponse::redirectTo("/public/index.html");
    }else if (req.path == "/public/index.txt")
    {
        return HTTPResponse::ok({{"Content-Type","text/plain"}}, createChannelIndex());
    }else if (req.path == "/public/play.html")
    {
        String id = cgi::Query(req.queryString).get("id").c_str();
        ChanInfo info;
        bool success = servMgr->getChannel(id.cstr(), info, true);

        if (!success)
            return HTTPResponse::notFound();

        string path, lang;
        tie(path, lang) = mapper.toLocalFilePath(req.path, langs);

        FileStream file;
        StringStream mem;
        HTTPRequestScope scope(req);

        file.openReadOnly(path.c_str());
        Template engine(req.queryString);
        engine.prependScope(scope);
        engine.readTemplate(file, &mem, 0);

        map<string,string> headers;
        headers["Content-Type"]     = "text/html";
        if (lang != "")
            headers["Content-Language"] = lang;
        headers["Content-Length"]   = to_string(mem.getLength());
        return HTTPResponse::ok(headers, mem.str());
    }else
    {
        string path, lang;
        tie(path, lang) = mapper.toLocalFilePath(req.path, langs);

        if (path.empty())
            return HTTPResponse::notFound();
        else
        {
            LOG_DEBUG("Writing `%s` lang=%s", path.c_str(), lang.c_str());

            auto type = MIMEType(path);

            StringStream mem;
            FileStream file;
            HTTPRequestScope scope(req);

            try
            {
                if (type == "text/html")
                {
                    file.openReadOnly(path.c_str());
                    Template engine(req.queryString);
                    engine.prependScope(scope);
                    engine.readTemplate(file, &mem, 0);
                }else
                {
                    file.openReadOnly(path.c_str());
                    file.writeTo(mem, file.length());
                }
            }catch (StreamException &)
            {
                LOG_DEBUG("StreamException in %s", __FUNCTION__);
            }
            file.close();

            string body = mem.str();
            map<string,string> headers = {
                {"Content-Type",type},
                {"Content-Length",to_string(body.size())}
            };
            if (lang != "")
                headers["Content-Language"] = lang;
            return HTTPResponse::ok(headers, body);
        }
    }
}
