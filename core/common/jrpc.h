#ifndef _JRPC_H
#define _JRPC_H

#include "peercast.h"
#include "channel.h"
#include "version2.h"

#include <stdarg.h>
#include <string>
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
        return call_internal(request).dump(4);
    }

    virtual json dispatch(const json&, const json&) = 0;

private:
    json call_internal(const std::string&);
};

class PeercastJrpcApi : public JrpcApi
{
public:
    // dispatchメソッドを実装。パラメータの数や型が合わない場合は
    // invalid_params 例外、メソッドが存在しない場合は
    // method_not_found 例外を上げる。
    json dispatch(const json& m, const json& p) override
    {
        try {
            if (m == "getVersionInfo")
                return getVersionInfo();
            if (m == "getChannels")
                return getChannels();
            if (m == "getSettings")
                return getSettings();
            if (m == "getYellowPages")
                return getYellowPages();
            if (m == "getYellowPageProtocols")
                return getYellowPageProtocols();
        } catch (std::out_of_range& e) {
            throw invalid_params(e.what());
        }
        throw method_not_found(m.get<std::string>());
    }

    json getVersionInfo()
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
            {"source", nullptr},
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

    json getChannels()
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

    ::String format(const char* fmt, ...)
    {
        String result;
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(result.data, ::String::MAX_LEN - 1, fmt, ap);
        va_end(ap);

        return result;
    }

    json getYellowPages()
    {
        servMgr->lock.on();

        json j;
        const char* root = servMgr->rootHost.cstr();

        j = {
            { "yellowPageId", 0 },
            { "name",  root },
            { "uri", format("pcp://%s/", root).cstr() },
            { "announceUri", format("pcp://%s/", root).cstr() },
            { "channelsUri", nullptr },
            { "protocol", "pcp" },
            { "channels", announcingChannels() }
        };

        servMgr->lock.off();

        return j;
    }

    json getYellowPageProtocols()
    {
        json pcp = {
            { "name", "PCP" },
            { "protocol", "pcp" }
        };

        return json::array({ pcp });
    }

    json getSettings()
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

};

#endif
