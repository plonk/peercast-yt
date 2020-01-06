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

    enum ErrorCode
    {
        kParseError = -32700,
        kInvalidRequest = -32600,
        kMethodNotFound = -32601,
        kInvalidParams = -32602,
        kInternalError = -32603,

        kChannelNotFound = -1,
        kUnknownError = 0,
    };

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

    class HostGraph
    {
    public:
        HostGraph(std::shared_ptr<Channel> ch, ChanHitList *hitList)
        {
            if (ch == nullptr)
                throw std::invalid_argument("ch");

            if (ch == nullptr)
                throw std::invalid_argument("hitList");

            // 自分を追加する。
            {
                Host uphost;
                ChanHit self;

                if (!ch->isBroadcasting())
                    uphost = ch->sourceHost.host;

                self.initLocal(ch->localListeners(),
                               ch->localRelays(),
                               ch->info.numSkips,
                               ch->info.getUptime(),
                               ch->isPlaying(),
                               ch->rawData.getOldestPos(),
                               ch->rawData.getLatestPos(),
                               ch->canAddRelay(),
                               uphost);

                m_hit[endpoint(&self)] = self;
                m_children[self.uphost].push_back(endpoint(&self));
            }

            for (ChanHit *p = hitList->hit;
                 p;
                 p = p->next)
            {
                Host h = endpoint(p);

                LOG_DEBUG("HostGraph: endpoint = %s", h.IPtoStr().cstr());

                if (h.ip == 0)
                    continue;

                m_hit[h] = *p;
                m_hit[h].next = nullptr;

                m_children[p->uphost].push_back(h);
            }

            // // 上流の項目が見付からないノードはルートにする。
            // for (auto it = m_children.begin(); it != m_children.end(); ++it)
            // {
            //     try
            //     {
            //         m_hit.at(it->first);
            //     }
            //     catch (std::out_of_range&)
            //     {
            //         for (auto i : it->second)
            //             m_children[Host()].push_back(i);
            //     }
            // }
        }

        Host endpoint(ChanHit *hit)
        {
            if (hit->rhost[0].ip != 0)
                return hit->rhost[0];
            else
                return hit->rhost[1];
        }

        json toRelayTree(Host& endpoint, const std::vector<Host> path)
        {
            ChanHit& hit = m_hit[endpoint];
            json::array_t children;

            for (Host& child : m_children[endpoint])
            {
                if (find(path.begin(), path.end(), child) == path.end())
                {
                    std::vector<Host> p = path;
                    p.push_back(endpoint);
                    children.push_back(toRelayTree(child, p));
                }
                else
                {
                    LOG_DEBUG("toRelayTree: circularity detected. skipping %s", ((std::string) child).c_str());
                }
            }

            return {
                { "sessionId", (std::string) hit.sessionID },
                { "address", endpoint.IPtoStr().cstr() },
                { "port", endpoint.port },
                { "isFirewalled", hit.firewalled },
                { "localRelays", hit.numRelays },
                { "localDirects", hit.numListeners },
                { "isTracker", hit.tracker },
                { "isRelayFull", !hit.relay },
                { "isDirectFull", !hit.direct },
                { "isReceiving", hit.recv },
                { "isControlFull", !hit.cin },
                { "version", hit.version },
                { "children", children }
            };
        }

        json::array_t getRelayTree()
        {
            json::array_t result;

            for (Host& root : m_children[Host()])
            {
                result.push_back(toRelayTree(root, std::vector<Host>()));
            }
            return result;
        }

        std::map<Host, ChanHit> m_hit;
        std::map<Host, std::vector<Host> > m_children;
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
            { "bumpChannel",             &JrpcApi::bumpChannel,             { "channelId" } },
            { "clearLog",                &JrpcApi::clearLog,                {} },
            { "fetch",                   &JrpcApi::fetch,                   { "url", "name", "desc", "genre", "contact", "bitrate", "type" } },
            { "getChannelConnections",   &JrpcApi::getChannelConnections,   { "channelId" } },
            { "getChannelInfo",          &JrpcApi::getChannelInfo,          { "channelId" } },
            { "getChannelRelayTree",     &JrpcApi::getChannelRelayTree,     { "channelId" } },
            { "getChannelStatus",        &JrpcApi::getChannelStatus,        { "channelId" } },
            { "getChannels",             &JrpcApi::getChannels,             {} },
            { "getLog",                  &JrpcApi::getLog,                  { "from", "maxLines" } },
            { "getLogSettings",          &JrpcApi::getLogSettings,          {} },
            { "getNewVersions",          &JrpcApi::getNewVersions,          {} },
            { "getNotificationMessages", &JrpcApi::getNotificationMessages, {} },
            { "getPlugins",              &JrpcApi::getPlugins,              {} },
            { "getSettings",             &JrpcApi::getSettings,             {} },
            { "getStatus",               &JrpcApi::getStatus,               {} },
            { "getVersionInfo",          &JrpcApi::getVersionInfo,          {} },
            { "getYellowPageProtocols",  &JrpcApi::getYellowPageProtocols,  {} },
            { "getYellowPages",          &JrpcApi::getYellowPages,          {} },
            { "removeYellowPage",        &JrpcApi::removeYellowPage,        { "yellowPageId" } },
            { "setChannelInfo",          &JrpcApi::setChannelInfo,          { "channelId", "info", "track" } },
            { "setLogSettings",          &JrpcApi::setLogSettings,          { "settings" } },
            { "setSettings",             &JrpcApi::setSettings,             { "settings" } },
            { "stopChannel",             &JrpcApi::stopChannel,             { "channelId" } },
            { "stopChannelConnection",   &JrpcApi::stopChannelConnection,   { "channelId", "connectionId" } },
        })
    {
    }

    typedef struct {
        const char *name;
        JrpcMethod method;
        std::vector<std::string> parameter_names;
    } entry;
    std::vector<entry > m_methods;

    json::string_t announcingChannelStatus(std::shared_ptr<Channel> c);
    json::array_t announcingChannels();
    json bumpChannel(json::array_t args);
    json channelStatus(std::shared_ptr<Channel> c);
    json clearLog(json::array_t args);
    json dispatch(const json& m, const json& p);
    json fetch(json::array_t params);
    json getChannelConnections(json::array_t params);
    json getChannelInfo(json::array_t params);
    json getChannelRelayTree(json::array_t args);
    json getChannelStatus(json::array_t params);
    json getChannels(json::array_t);
    json getChannelsFound(json::array_t);
    json getLog(json::array_t args);
    json getLogSettings(json::array_t args);
    json getNewVersions(json::array_t);
    json getNotificationMessages(json::array_t);
    json getPlugins(json::array_t);
    json getSettings(json::array_t);
    json getStatus(json::array_t);
    json getVersionInfo(json::array_t);
    json getYellowPageProtocols(json::array_t);
    json getYellowPages(json::array_t);
    json::array_t hostsToJson(ChanHitList* hitList);
    ChanInfo mergeChanInfo(const ChanInfo& orig, json::object_t info, json::object_t track);
    json removeYellowPage(json::array_t args);
    json setChannelInfo(json::array_t args);
    json setLogSettings(json::array_t args);
    json setSettings(json::array_t args);
    json stopChannel(json::array_t args);
    json stopChannelConnection(json::array_t params);
    json toConnection(Servent* s);
    json toPositionalArguments(json named_params, std::vector<std::string> names);
    json toSourceConnection(std::shared_ptr<Channel> c);
    json to_json(ChanHit* h);
    json to_json(ChanHitList* hitList);
    json to_json(ChanInfo& info);
    json to_json(Channel::STATUS status);
    json to_json(GnuID id);
    json to_json(ServMgr::FW_STATE state);
    json to_json(TrackInfo& track);
    json to_json(std::shared_ptr<Channel> c);
};

#endif
