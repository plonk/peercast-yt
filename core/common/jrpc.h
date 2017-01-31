#ifndef _JRPC_H
#define _JRPC_H

#include "peercast.h"
#include "channel.h"
#include "version2.h"

#include <stdarg.h>
#include <string>
#include <vector>
#include <tuple>
#include "json.hpp"

class JrpcApi
{
public:
    using json = nlohmann::json;

    class method_not_found : public std::runtime_error
    {
    public:
        method_not_found(const std::string& msg) : runtime_error(msg) {}
    };
    class invalid_params : public std::runtime_error
    {
    public:
        invalid_params(const std::string& msg) : runtime_error(msg) {}
    };

    std::string call(const std::string& request)
    {
        std::string result = call_internal(request).dump();

        LOG_DEBUG("jrpc response: %s", result.c_str());

        return result;
    }

private:
    json call_internal(const std::string&);

    typedef json (JrpcApi::*JrpcMethod)(json::array_t);

public:
    JrpcApi() :
        m_methods
        ({
            { "fetch", &JrpcApi::fetch, { "url", "name", "desc", "genre", "contact", "bitrate", "type" } },
            { "getVersionInfo", &JrpcApi::getVersionInfo, {} },
            { "getChannels", &JrpcApi::getChannels, {} },
            { "getNewVersions", &JrpcApi::getNewVersions, {} },
            { "getNotificationMessages", &JrpcApi::getNotificationMessages, {} },
            { "getPlugins", &JrpcApi::getPlugins, {} },
            { "getSettings", &JrpcApi::getSettings, {} },
            { "getStatus", &JrpcApi::getStatus, {} },
            { "getYellowPages", &JrpcApi::getYellowPages, {} },
            { "getYellowPageProtocols", &JrpcApi::getYellowPageProtocols, {} },
        })
    {
    }

    typedef struct {
        const char *name;
        JrpcMethod method;
        std::vector<std::string> parameter_names;
    } entry;
    std::vector<entry > m_methods;

    json toPositionalArguments(json named_params, std::vector<std::string> names)
    {
        json result = json::array();

        for (int i = 0; i < names.size(); i++)
            result.push_back(nullptr);

        for (std::pair<std::string, json> pair : named_params.get<json::object_t>())
        {
            auto iter = std::find(names.begin(), names.end(), pair.first);
            if (iter != names.end())
            {
                result[iter - names.begin()] = pair.second;
            }
        }
        return result;
    }

    // dispatchメソッドを実装。パラメータの数や型が合わない場合は
    // invalid_params 例外、メソッドが存在しない場合は
    // method_not_found 例外を上げる。
    json dispatch(const json& m, const json& p)
    {
        for (int i = 0; i < m_methods.size(); i++)
        {
            entry& info = m_methods[i];

            if (m != info.name)
                continue;

            json arguments;
            if (p.is_array())
                arguments = p;
            else if (p.is_object())
                arguments = toPositionalArguments(p, info.parameter_names);
            else if (info.parameter_names.size() == 0 && p.is_null())
                arguments = json::array();

            if (arguments.size() != info.parameter_names.size())
                throw invalid_params("Wrong number of arguments");

            return (this->*(info.method))(arguments);
        }
        throw method_not_found(m.get<std::string>());
    }

    json fetch(json::array_t)
    {
        return "OK";
    }

    json getVersionInfo(json::array_t)
    {
        return {
            { "agentName", PCX_AGENT },
            { "apiVesion", "1.0.0" },
            { "jsonrpc", "2.0" }
        };
    }

    json to_json(GnuID id)
    {
            char idstr[100];

            id.toStr(idstr);
            return idstr;
    }

    json to_json(ChanInfo& info)
    {
        String comment = info.comment;
        comment.convertTo(String::T_UNICODE);  // should not be needed

        return {
            {"name", info.name},
            {"url", info.url},
            {"genre", info.genre},
            {"desc", info.desc},
            {"comment", comment},
            {"bitrate", info.bitrate},
            {"contentType", info.getTypeStr()}, //?
            {"mimeType", info.getMIMEType()}
        };
    }

    json to_json(TrackInfo& track)
    {
        return {
            {"name", track.title},
            {"genre", track.genre},
            {"album", track.album},
            {"creator", track.artist},
            {"url", track.contact}
        };
    }

    // PeerCastStation では Receiving, Searching, Error, Idle のどれか
    // が返る。peercast では NONE, WAIT, CONNECT, REQUEST, CLOSE,
    // RECEIVE, BROADCAST, ABORT, SEARCH, NOHOSTS, IDLE, ERROR,
    // NOTFOUND のどれか。
    json to_json(Channel::STATUS status)
    {
        switch (status)
        {
        case Channel::S_RECEIVING:
            return "Receiving";
        case Channel::S_SEARCHING:
            return "Searching";
        case Channel::S_ERROR:
            return "Error";
        case Channel::S_IDLE:
            return "Idle";
        default:
            return Channel::statusMsgs[status];
        }
    }

    json to_json(Channel *c)
    {
        json j;

        j["channelId"] = to_json(c->info.id);
        j["status"] = {
            {"status", to_json((Channel::STATUS) c->status)},
            {"source", c->sourceURL},
            {"uptime", c->info.getUptime()},
            {"localRelays", c->localRelays()},
            {"localDirects", c->localListeners()},
            {"totalRelays", c->totalRelays()},
            {"totalDirects", c->localListeners()},
            {"isBroadcasting", c->isBroadcasting()},
            {"isRelayFull", c->isFull()},
            {"isDirectFull", nullptr},
            {"isReceiving", c->isReceiving()},
        };
        j["info"] = to_json(c->info);
        j["track"] = to_json(c->info.track);
        j["yellowPages"] = json::array();

        return j;
    }

    json getChannels(json::array_t)
    {
        json result = json::array();

        chanMgr->lock.on();

        for (Channel *c = chanMgr->channel; c != NULL; c = c->next)
        {
            result.push_back(to_json(c));
        }

        chanMgr->lock.off();

        return result;
    }

    // 配信中のチャンネルとルートサーバーとの接続状態。
    // 返り値: "Idle" | "Connecting" | "Connected" | "Error"
    json::string_t announcingChannelStatus(Channel* c)
    {
        return "Connected";
    }

    json::array_t announcingChannels()
    {
        json::array_t result;

        chanMgr->lock.on();

        for (Channel *c = chanMgr->channel; c != NULL; c = c->next)
        {
            if (!c->isBroadcasting())
                continue;

            json j = {
                { "channelId", to_json(c->info.id) },
                { "status", announcingChannelStatus(c) },
            };
            result.push_back(j);
        }

        chanMgr->lock.off();

        return result;
    }

    json getYellowPages(json::array_t)
    {
        servMgr->lock.on();

        json j;
        const char* root = servMgr->rootHost.cstr();

        j = {
            { "yellowPageId", 0 },
            { "name",  root },
            { "uri", String::format("pcp://%s/", root) },
            { "announceUri", String::format("pcp://%s/", root) },
            { "channelsUri", nullptr },
            { "protocol", "pcp" },
            { "channels", announcingChannels() }
        };

        servMgr->lock.off();

        return j;
    }

    json getYellowPageProtocols(json::array_t)
    {
        json pcp = {
            { "name", "PCP" },
            { "protocol", "pcp" }
        };

        return json::array({ pcp });
    }

    json getSettings(json::array_t)
    {
        servMgr->lock.on();
        chanMgr->lock.on();
        json j = {
            { "maxRelays", servMgr->maxRelays },
            { "maxRelaysPerChannel", chanMgr->maxRelaysPerChannel },
            { "maxDirects", servMgr->maxDirect },
            { "maxDirectsPerChannel", servMgr->maxDirect },
            { "maxUpstreamRate", servMgr->maxBitrateOut },
            { "maxUpstreamRatePerChannel", servMgr->maxBitrateOut },
            // channelCleaner は無視。
        };

        chanMgr->lock.off();
        servMgr->lock.off();

        return j;
    }

    json getPlugins(json::array_t)
    {
        return json::array_t();
    }

    json to_json(ServMgr::FW_STATE state)
    {
        switch (state)
        {
        case ServMgr::FW_OFF:
            return false;
        case ServMgr::FW_ON:
            return true;
        case ServMgr::FW_UNKNOWN:
            return nullptr;
        }
    }

    json getStatus(json::array_t)
    {
        servMgr->lock.on();

        std::string globalIP = servMgr->serverHost.IPtoStr();
        auto port       = servMgr->serverHost.port;
        std::string localIP  = Host(ClientSocket::getIP(NULL), port).IPtoStr();

        json j = {
            { "uptime", servMgr->getUptime() },
            { "isFirewalled", to_json(servMgr->getFirewall()) },
            { "globalRelayEndPoint", { globalIP, port } },
            { "globalDirectEndPoint", { globalIP, port } },
            { "localRelayEndPoint", { localIP, port } },
            { "localDirectEndPoint", { localIP, port } }
        };

        servMgr->lock.off();

        return j;
    }

    json getNewVersions(json::array_t)
    {
        return json::array();
    }

    json getNotificationMessages(json::array_t)
    {
        return json::array();
    }
};

#endif
