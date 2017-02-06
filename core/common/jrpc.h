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

    class application_error : public std::runtime_error
    {
    public:
        application_error(int error, const std::string& msg) : runtime_error(msg), m_errno(error) {}
        int m_errno;
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
            { "getChannelConnections", &JrpcApi::getChannelConnections, { "channelId"} },
            { "getChannelInfo", &JrpcApi::getChannelInfo, { "channelId"} },
            { "getChannelStatus", &JrpcApi::getChannelStatus, { "channelId"} },
            { "getChannels", &JrpcApi::getChannels, {} },
            { "getNewVersions", &JrpcApi::getNewVersions, {} },
            { "getNotificationMessages", &JrpcApi::getNotificationMessages, {} },
            { "getPlugins", &JrpcApi::getPlugins, {} },
            { "getSettings", &JrpcApi::getSettings, {} },
            { "getStatus", &JrpcApi::getStatus, {} },
            { "getYellowPages", &JrpcApi::getYellowPages, {} },
            { "getYellowPageProtocols", &JrpcApi::getYellowPageProtocols, {} },
            { "setChannelInfo", &JrpcApi::setChannelInfo, { "channelId", "info", "track" } },
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


    json fetch(json::array_t params)
    {
        try
        {
            std::string url     = params[0];
            std::string name    = params[1];
            std::string desc    = params[2];
            std::string genre   = params[3];
            std::string contact = params[4];
            int bitrate         = params[5];
            std::string typeStr = params[6];

            ChanInfo info;
            info.name    = name.c_str();
            info.desc    = desc.c_str();
            info.genre   = genre.c_str();
            info.url     = contact.c_str();
            info.bitrate = bitrate;
            {
                auto type = ChanInfo::getTypeFromStr(typeStr.c_str());

                info.contentType    = type;
                info.contentTypeStr = ChanInfo::getTypeStr(type);
                info.streamType     = ChanInfo::getMIMEType(type);
                info.streamExt      = ChanInfo::getTypeExt(type);
            }
            info.bcID = chanMgr->broadcastID;

            Channel *c = chanMgr->createChannel(info, NULL); // info, mount
            if (!c)
            {
                throw application_error(0, "failed to create channel");
            }
            c->startURL(url.c_str());

            while (!c->getID().isSet())
            {
                sys->sleepIdle();
            }

            return (std::string) c->getID();
        } catch (std::domain_error& e)
        {
            throw invalid_params(e.what());
        }
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
            {"name", info.name.cstr()},
            {"url", info.url.cstr()},
            {"genre", info.genre.cstr()},
            {"desc", info.desc.cstr()},
            {"comment", comment.cstr()},
            {"bitrate", info.bitrate},
            {"contentType", info.getTypeStr()}, //?
            {"mimeType", info.getMIMEType()}
        };
    }

    json to_json(TrackInfo& track)
    {
        return {
            {"name", track.title.cstr()},
            {"genre", track.genre.cstr()},
            {"album", track.album.cstr()},
            {"creator", track.artist.cstr()},
            {"url", track.contact.cstr()}
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
        case Channel::S_BROADCASTING:
            return "Receiving";
        case Channel::S_SEARCHING:
        case Channel::S_CONNECTING:
            return "Searching";
        case Channel::S_ERROR:
            return "Error";
        case Channel::S_IDLE:
            return "Idle";
        default:
            return Channel::statusMsgs[status];
        }
    }

    json channelStatus(Channel *c)
    {
        return {
            {"status", to_json(c->status)},
            {"source", c->sourceURL.cstr() },
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
    }

    json to_json(Channel *c)
    {
        json j;

        j["channelId"] = to_json(c->info.id);
        j["status"] = channelStatus(c);
        j["info"] = to_json(c->info);
        j["track"] = to_json(c->info.track);
        j["yellowPages"] = json::array();

        return j;
    }

    json getChannelConnections(json::array_t params)
    {
        json result = json::array();

        GnuID id = params[0].get<std::string>();

        Channel *c = chanMgr->findChannelByID(id);
        if (!c)
            throw application_error(-1, "Channel not found");

        json remoteEndPoint;
        if (c->sock)
            remoteEndPoint = (std::string) c->sock->host;
        else
            remoteEndPoint = nullptr;
        json remoteName = c->sourceURL.isEmpty() ? ((std::string) c->sourceHost.host).c_str() : c->sourceURL.cstr();

        json sourceConnection =  {
            { "connectionId", (uintptr_t) c },
            { "type", "source" },
            { "status", to_json(c->status) },
            { "sendRate", 0.0 },
            { "recvRate", c->sourceData ? c->sourceData->getSourceRate() : 0 },
            { "protocolName", ChanInfo::getProtocolStr(c->info.srcProtocol) },
            { "localRelays", nullptr },
            { "localDirects", nullptr },
            { "contentPosition", c->streamPos },
            { "agentName", nullptr },
            { "remoteEndPoint", remoteEndPoint },
            { "remoteHostStatus", json::array() }, // 何を入れたらいいのかわからない。
            { "remoteName", remoteName },
        };
        result.push_back(sourceConnection);

        servMgr->lock.on();
        for (Servent* s = servMgr->servents; s != NULL; s = s->next)
        {
            if (s->type != Servent::T_RELAY)
                continue;

            unsigned int bytesInPerSec = s->sock ? s->sock->bytesInPerSec : 0;
            unsigned int bytesOutPerSec = s->sock ? s->sock->bytesOutPerSec : 0;

            json remoteEndPoint = c->sock ? ((std::string) c->sock->host).c_str() : nullptr;

            json connection = {
                { "connectionId", (uintptr_t) s },
                { "type", "relay" },
                { "status", s->getStatusStr() },
                { "sendRate", bytesOutPerSec },
                { "recvRate", bytesInPerSec },
                { "protocolName", "PCP Relay" },
                { "localRelays", nullptr },
                { "localDirects", nullptr },
                { "contentPosition", nullptr },
                { "agentName", s->agent.cstr() },
                { "remoteEndPoint", remoteEndPoint },
                { "remoteHostStatus", json::array() }, // 何を入れたらいいのかわからない。
                { "remoteName", remoteEndPoint },
            };

            result.push_back(connection);
        }
        servMgr->lock.off();

        return result;
    }

    json getChannelInfo(json::array_t params)
    {
        GnuID id(params[0].get<std::string>());

        Channel *c = chanMgr->findChannelByID(id);
        if (!c)
            throw application_error(-1, "Channel not found");

        json j = {
            { "info", to_json(c->info) },
            { "track", to_json(c->info.track) },
            { "yellowPages", json::array() },
        };

        return j;
    }

    json getChannelStatus(json::array_t params)
    {
        GnuID id(params[0].get<std::string>());

        Channel *c = chanMgr->findChannelByID(id);

        if (!c)
            throw application_error(-1, "Channel not found");

        return channelStatus(c);
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
        json j;
        const char* root = servMgr->rootHost.cstr();

        j = {
            { "yellowPageId", 0 },
            { "name",  root },
            { "uri", String::format("pcp://%s/", root).cstr() },
            { "announceUri", String::format("pcp://%s/", root).cstr() },
            { "channelsUri", nullptr },
            { "protocol", "pcp" },
            { "channels", announcingChannels() }
        };

        return json::array({j});
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
        json j = {
            { "maxRelays", servMgr->maxRelays },
            { "maxRelaysPerChannel", chanMgr->maxRelaysPerChannel },
            { "maxDirects", servMgr->maxDirect },
            { "maxDirectsPerChannel", servMgr->maxDirect },
            { "maxUpstreamRate", servMgr->maxBitrateOut },
            { "maxUpstreamRatePerChannel", servMgr->maxBitrateOut },
            // channelCleaner は無視。
        };

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
        std::string globalIP = servMgr->serverHost.IPtoStr();
        auto port            = servMgr->serverHost.port;
        std::string localIP  = Host(ClientSocket::getIP(NULL), port).IPtoStr();

        json j = {
            { "uptime", servMgr->getUptime() },
            { "isFirewalled", to_json(servMgr->getFirewall()) },
            { "globalRelayEndPoint", { globalIP, port } },
            { "globalDirectEndPoint", { globalIP, port } },
            { "localRelayEndPoint", { localIP, port } },
            { "localDirectEndPoint", { localIP, port } }
        };

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

    json setChannelInfo(json::array_t)
    {
        // unimplemented
        return nullptr;
    }
};

#endif
