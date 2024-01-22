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
#include "chandir.h"

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
            { "fetch",                   &JrpcApi::fetch,                   { "url", "name", "desc", "genre", "contact", "bitrate", "type", "network" } },
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
            { "getServerStorageItem",    &JrpcApi::getServerStorageItem,    { "key" } },
            { "getSettings",             &JrpcApi::getSettings,             {} },
            { "getState",                &JrpcApi::getState,                { "objectNames" } },
            { "getStatus",               &JrpcApi::getStatus,               {} },
            { "getVersionInfo",          &JrpcApi::getVersionInfo,          {} },
            { "getYPChannels",           &JrpcApi::getYPChannels,           {} },
            { "getYellowPageProtocols",  &JrpcApi::getYellowPageProtocols,  {} },
            { "getYellowPages",          &JrpcApi::getYellowPages,          {} },
            { "playChannel",             &JrpcApi::playChannel,   { "channelId" } },
            { "removeYellowPage",        &JrpcApi::removeYellowPage,        { "yellowPageId" } },
            { "setChannelInfo",          &JrpcApi::setChannelInfo,          { "channelId", "info", "track" } },
            { "setLogSettings",          &JrpcApi::setLogSettings,          { "settings" } },
            { "setServerStorageItem",    &JrpcApi::setServerStorageItem,    { "key", "value" } },
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
    json getState(json::array_t);
    json getServerStorageItem(json::array_t);
    json setServerStorageItem(json::array_t);
    json getVersionInfo(json::array_t);
    json getYPChannels(json::array_t args);
    json getYPChannelsInternal(json::array_t args = {});
    json getYellowPageProtocols(json::array_t);
    json getYellowPages(json::array_t);
    json::array_t hostsToJson(std::shared_ptr<ChanHitList> hitList);
    ChanInfo mergeChanInfo(const ChanInfo& orig, json::object_t info, json::object_t track);
    json playChannel(json::array_t);
    json removeYellowPage(json::array_t args);
    json setChannelInfo(json::array_t args);
    json setLogSettings(json::array_t args);
    json setSettings(json::array_t args);
    json stopChannel(json::array_t args);
    json stopChannelConnection(json::array_t params);
    json toConnection(Servent* s);
    json toPositionalArguments(json named_params, std::vector<std::string> names);
    json toSourceConnection(std::shared_ptr<Channel> c);
    json to_json(std::shared_ptr<ChanHit> h);
    json to_json(std::shared_ptr<ChanHitList> hitList);
    json to_json(ChanInfo& info);
    json to_json(Channel::STATUS status);
    json to_json(GnuID id);
    json to_json(ServMgr::FW_STATE state);
    json to_json(TrackInfo& track);
    json to_json(std::shared_ptr<Channel> c);
    json to_json(Channel::IP_VERSION ipVersion);
};

#endif
