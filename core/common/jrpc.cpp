#include <iostream>
#include <string>

#include "jrpc.h"
#include "str.h"

using namespace std;
using json = nlohmann::json;

static json error_object(int code, const char* message, json id = nullptr, json data = nullptr)
{
    json j = {
        { "jsonrpc", "2.0" },
        { "error", { { "code", code }, { "message", message } } },
        { "id", id }
    };

    if (!data.is_null())
        j["error"]["data"] = data;

    return j;
}

json JrpcApi::call_internal(const string& input)
{
    json j, id, method, params, result;

    LOG_DEBUG("jrpc request: %s", input.c_str());

    try {
        j = json::parse(input);
    } catch (json::parse_error&) {
        return error_object(kParseError, "Parse error");
    }

    if (!j.is_object() ||
        j.count("jsonrpc") == 0 ||
        j.at("jsonrpc") != "2.0")
    {
        return error_object(kInvalidRequest, "Invalid Request");
    }

    try {
        id = j.at("id");
    } catch (json::out_of_range) {
        return error_object(kInvalidRequest, "Invalid Request");
    }

    try {
        method = j.at("method");
    } catch (json::out_of_range) {
        return error_object(kInvalidRequest, "Invalid Request", id);
    }

    if (j.count("params") == 0)
    {
        params = json::array();
    } else if (!j.at("params").is_object() &&
               !j.at("params").is_array())
    {
        return error_object(kInvalidParams, "Invalid params", id, "params must be either object or array");
    } else {
        params = j.at("params");
    }

    try {
        result = dispatch(method, params);
    } catch (method_not_found& e)
    {
        LOG_DEBUG("Method not found: %s", e.what());
        return error_object(kMethodNotFound, "Method not found", id, e.what());
    } catch (invalid_params& e)
    {
        return error_object(kInvalidParams, "Invalid params", id, e.what());
    } catch (application_error& e)
    {
        return error_object(e.m_errno, e.what(), id);
    } catch (std::exception& e)
    {
        // Unexpected errors are handled here.
        return error_object(kInternalError, e.what(), id);
    }

    return {
        { "jsonrpc", "2.0" },
        { "result", result },
        { "id", id }
    };
}

json JrpcApi::getLog(json::array_t args)
{
    json from = args[0];
    json maxLines = args[1];

    if (!from.is_null() && from.get<int>() < 0)     throw invalid_params("from must be non negative");
    if (!from.is_null() && maxLines.get<int>() < 0) throw invalid_params("maxLines must be non negative");

    std::vector<string> lines =
        sys->logBuf->toLines([](unsigned int time, LogBuffer::TYPE type, const char* line)
                             {
                                 std::string buf;

                                 if (type != LogBuffer::T_NONE)
                                 {
                                     buf += str::rstrip(String().setFromTime(time));
                                     buf += " [";
                                     buf += LogBuffer::getTypeStr(type);
                                     buf += "] ";
                                 }

                                 buf += line;

                                 return buf;
                             });
    lines.push_back("");

    if (from == nullptr)
        from = 0;
    lines.erase(lines.begin(), lines.begin() + (std::min)(from.get<size_t>(), lines.size()));

    if (maxLines == nullptr)
        maxLines = lines.size();
    while (lines.size() > (size_t)maxLines)
        lines.pop_back();

    auto log = str::join("\n", lines);

    return { { "from", from.get<int>() }, { "lines", lines.size() }, { "log", log} };
}

json JrpcApi::clearLog(json::array_t args)
{
    sys->logBuf->clear();
    return nullptr;
}

json JrpcApi::getLogSettings(json::array_t args)
{
    // YT -> PeerCastStation
    // 7 OFF   -> 0 OFF
    // 6 FATAL -> 1 FATAL
    // 5 ERROR -> 2 ERROR
    // 4 WARN  -> 3 WARN
    // 3 INFO  -> 4 INFO
    // 2 DEBUG -> 5 DEBUG
    // 1 TRACE -> 5 DEBUG

    return { { "level", std::min(5, 7 - servMgr->logLevel()) } };
}

json JrpcApi::setLogSettings(json::array_t args)
{
    int level = args[0]["level"];

    if (0 <= level && level <= 5)
        servMgr->logLevel(7 - level);

    return nullptr;
}

json JrpcApi::toPositionalArguments(json named_params, std::vector<std::string> names)
{
    json result = json::array();

    for (size_t i = 0; i < names.size(); i++)
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
json JrpcApi::dispatch(const json& m, const json& p)
{
    for (size_t i = 0; i < m_methods.size(); i++)
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

json JrpcApi::fetch(json::array_t params)
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
            info.MIMEType       = ChanInfo::getMIMEType(type);
            info.streamExt      = ChanInfo::getTypeExt(type);
        }
        info.bcID = chanMgr->broadcastID;

        // ソースに接続できなかった場合もチャンネルを同定したいの
        // で、事前にチャンネルIDを設定する。
        info.id = chanMgr->broadcastID;
        info.id.encode(NULL, info.name, info.genre, info.bitrate);

        auto c = chanMgr->createChannel(info, NULL); // info, mount
        if (!c)
        {
            throw application_error(kUnknownError, "failed to create channel");
        }
        c->startURL(url.c_str());

        return (std::string) c->getID();
    } catch (std::domain_error& e)
    {
        throw invalid_params(e.what());
    }
}

json JrpcApi::getVersionInfo(json::array_t)
{
    return {
        { "agentName", PCX_AGENT },
        { "apiVersion", "1.0.0" },
        { "jsonrpc", "2.0" }
    };
}

json JrpcApi::to_json(GnuID id)
{
        char idstr[100];

        id.toStr(idstr);
        return idstr;
}

json JrpcApi::to_json(ChanInfo& info)
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

json JrpcApi::to_json(TrackInfo& track)
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
json JrpcApi::to_json(Channel::STATUS status)
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

json JrpcApi::channelStatus(std::shared_ptr<Channel> c)
{
    return {
        {"status", to_json(c->status)},
        {"source", c->sourceURL.cstr() },
        {"uptime", c->info.getUptime()},
        {"localRelays", c->localRelays()},
        {"localDirects", c->localListeners()},
        {"totalRelays", c->totalRelays()},
        {"totalDirects", c->totalListeners()},
        {"isBroadcasting", c->isBroadcasting()},
        {"isRelayFull", c->isFull()},
        {"isDirectFull", nullptr},
        {"isReceiving", c->isReceiving()},
    };
}

json JrpcApi::to_json(std::shared_ptr<Channel> c)
{
    return {
        {"channelId", to_json(c->info.id)},
        {"status", channelStatus(c)},
        {"info", to_json(c->info)},
        {"track", to_json(c->info.track)},
        {"yellowPages", json::array()},
    };
}

// チャンネルに関して特定の接続を停止する。成功すれば true、失敗す
// れば false を返す。
json JrpcApi::stopChannelConnection(json::array_t params)
{
    GnuID id = params[0].get<std::string>();
    int connectionId = params[1].get<int>();
    bool success = false;

    std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
    for (Servent* s = servMgr->servents; s != NULL; s = s->next)
    {
         if (s->serventIndex == connectionId &&
             s->chanID.isSame(id) &&
             s->type == Servent::T_RELAY)
         {
             s->abort();
             success = true;
             break;
         }
    }

    return success;
}

json JrpcApi::toConnection(Servent* s)
{
    unsigned int bytesInPerSec = s->sock ? s->sock->bytesInPerSec() : 0;
    unsigned int bytesOutPerSec = s->sock ? s->sock->bytesOutPerSec() : 0;

    json remoteEndPoint;
    if (s->sock)
        remoteEndPoint = (std::string) s->sock->host;
    else
        remoteEndPoint = nullptr;

    return {
        { "connectionId", s->serventIndex },
        { "type", str::downcase(s->getTypeStr()) },
        { "status", s->getStatusStr() },
        { "sendRate", bytesOutPerSec },
        { "recvRate", bytesInPerSec },
        { "protocolName", ChanInfo::getProtocolStr(s->outputProtocol) },
        { "localRelays", nullptr },
        { "localDirects", nullptr },
        { "contentPosition", nullptr },
        { "agentName", s->agent.cstr() },
        { "remoteEndPoint", remoteEndPoint },
        { "remoteHostStatus", json::array() }, // 何を入れたらいいのかわからない。
        { "remoteName", remoteEndPoint },
    };
}


json JrpcApi::toSourceConnection(std::shared_ptr<Channel> c)
{
    json remoteEndPoint;
    if (c->sock)
        remoteEndPoint = (std::string) c->sock->host;
    else
        remoteEndPoint = nullptr;
    json remoteName = c->sourceURL.isEmpty() ? ((std::string) c->sourceHost.host).c_str() : c->sourceURL.cstr();

    return {
        { "connectionId", -1 },
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
}

json JrpcApi::getChannelConnections(json::array_t params)
{
    json result = json::array();

    GnuID id = params[0].get<std::string>();

    auto c = chanMgr->findChannelByID(id);
    if (!c)
        throw application_error(kChannelNotFound, "Channel not found");

    result.push_back(toSourceConnection(c));

    std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
    for (Servent* s = servMgr->servents; s != NULL; s = s->next)
    {
        if (!s->chanID.isSame(id))
            continue;

        result.push_back(toConnection(s));
    }

    return result;
}

json JrpcApi::getChannelInfo(json::array_t params)
{
    GnuID id(params[0].get<std::string>());

    auto c = chanMgr->findChannelByID(id);
    if (!c)
        throw application_error(kChannelNotFound, "Channel not found");

    json j = {
        { "info", to_json(c->info) },
        { "track", to_json(c->info.track) },
        { "yellowPages", json::array() },
    };

    return j;
}

json JrpcApi::getChannelStatus(json::array_t params)
{
    GnuID id(params[0].get<std::string>());

    auto c = chanMgr->findChannelByID(id);

    if (!c)
        throw application_error(kChannelNotFound, "Channel not found");

    return channelStatus(c);
}

json JrpcApi::getChannels(json::array_t)
{
    json result = json::array();

    std::lock_guard<std::recursive_mutex> cs(chanMgr->lock);
    for (auto c = chanMgr->channel; c != NULL; c = c->next)
    {
        result.push_back(to_json(c));
    }

    return result;
}

json JrpcApi::to_json(ChanHit* h)
{
    return {
        { "ip", (std::string) h->host },
        { "hops", h->numHops },
        { "listeners", h->numListeners },
        { "relays", h->numRelays },
        { "uptime", h->upTime },
        { "push", (bool) h->firewalled },
        { "relay", (bool) h->relay },
        { "direct", (bool) h->direct },
        { "cin", (bool) h->cin }, // これrootモードで動いてるかってこと？
        { "stable", (bool) h->stable },
        { "version", h->version },
        { "update", sys->getTime() - h->time },
        { "tracker", (bool) h->tracker }
   };
}

json::array_t JrpcApi::hostsToJson(ChanHitList* hitList)
{
    json::array_t result;

    for (ChanHit *h = hitList->hit;
         h;
         h = h->next)
    {
        if (h->host.ip)
            result.push_back(to_json(h));
    }

    return result;
}

json JrpcApi::to_json(ChanHitList* hitList)
{
    ChanInfo info = hitList->info;
    return {
        { "name", info.name.cstr() },
        { "id",  (std::string) info.id },
        { "bitrate", info.bitrate },
        { "type", info.getTypeStr() },
        { "genre", info.genre.cstr() },
        { "desc", info.desc.cstr() },
        { "url", info.url.cstr() },
        { "uptime", info.getUptime() },
        { "comment", info.comment.cstr() },
        { "skips", info.numSkips },
        { "age", info.getAge() },
        { "bcflags", info.bcID.getFlags() },

        { "hit_stat", {
                { "hosts", hitList->numHits() },
                { "listeners", hitList->numListeners() },
                { "relays", hitList->numRelays() },
                { "firewalled", hitList->numFirewalled() },
                { "closest", hitList->closestHit() },
                { "furthest", hitList->furthestHit() },
                { "newest", sys->getTime() - hitList->newestHit() } } },
        { "hits", hostsToJson(hitList) },
        { "track", to_json(info.track) }
    };
}

json JrpcApi::getChannelsFound(json::array_t)
{
    json result = json::array();

    chanMgr->lock.lock();

    for (ChanHitList *hitList = chanMgr->hitlist;
         hitList;
         hitList = hitList->next)
    {
        if (hitList->isUsed())
            result.push_back(to_json(hitList));
    }

    chanMgr->lock.unlock();

    return result;
}

// 配信中のチャンネルとルートサーバーとの接続状態。
// 返り値: "Idle" | "Connecting" | "Connected" | "Error"
json::string_t JrpcApi::announcingChannelStatus(std::shared_ptr<Channel> c)
{
    return "Connected";
}

json::array_t JrpcApi::announcingChannels()
{
    json::array_t result;

    std::lock_guard<std::recursive_mutex> cs(chanMgr->lock);
    for (auto c = chanMgr->channel; c != NULL; c = c->next)
    {
        if (!c->isBroadcasting())
            continue;

        json j = {
            { "channelId", to_json(c->info.id) },
            { "status", announcingChannelStatus(c) },
        };
        result.push_back(j);
    }

    return result;
}

json JrpcApi::getYellowPages(json::array_t)
{
    json j;
    const char* root = servMgr->rootHost.cstr();

    if (strcmp(root, "")==0) {
        return json::array({});
    }else
    {
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
}

json JrpcApi::getYellowPageProtocols(json::array_t)
{
    json pcp = {
        { "name", "PCP" },
        { "protocol", "pcp" }
    };

    return json::array({ pcp });
}

json JrpcApi::setSettings(json::array_t args)
{
    json::object_t settings = args[0];

    servMgr->setMaxRelays((int) settings["maxRelays"]);
    chanMgr->maxRelaysPerChannel = (int) settings["maxRelaysPerChannel"];
    servMgr->maxDirect = (int) settings["maxDirects"];
    // maxDirectsPerChannel は無視。
    servMgr->maxBitrateOut = (int) settings["maxUpstreamRate"];
    // maxUpstreamRatePerChannel は無視。
    // channelCleaner, portMapper は無視。
    return nullptr;
}

json JrpcApi::getSettings(json::array_t)
{
    json j = {
        { "maxRelays", servMgr->maxRelays },
        { "maxRelaysPerChannel", chanMgr->maxRelaysPerChannel },
        { "maxDirects", servMgr->maxDirect },
        { "maxDirectsPerChannel", 0 },
        { "maxUpstreamRate", servMgr->maxBitrateOut },
        { "maxUpstreamRatePerChannel", 0 },
        // channelCleaner は無視。
    };

    return j;
}

json JrpcApi::getPlugins(json::array_t)
{
    return json::array_t();
}

json JrpcApi::to_json(ServMgr::FW_STATE state)
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
    LOG_ERROR("Invalid firewall state");
    return nullptr;
}

json JrpcApi::getStatus(json::array_t)
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

json JrpcApi::getNewVersions(json::array_t)
{
    return json::array();
}

json JrpcApi::getNotificationMessages(json::array_t)
{
    return json::array();
}

ChanInfo JrpcApi::mergeChanInfo(const ChanInfo& orig, json::object_t info, json::object_t track)
{
    ChanInfo i = orig;

    i.name    = info.at("name").get<std::string>().c_str();
    i.desc    = info.at("desc").get<std::string>().c_str();
    i.genre   = info.at("genre").get<std::string>().c_str();
    i.url     = info.at("url").get<std::string>().c_str();
    i.comment = info.at("comment").get<std::string>().c_str();

    i.track.contact = track.at("url").get<std::string>().c_str();
    i.track.title   = track.at("name").get<std::string>().c_str();
    i.track.artist  = track.at("creator").get<std::string>().c_str();
    i.track.album   = track.at("album").get<std::string>().c_str();
    i.track.genre   = track.at("genre").get<std::string>().c_str();

    LOG_DEBUG("i = %s", to_json(i).dump().c_str());

    return i;
}

json JrpcApi::setChannelInfo(json::array_t args)
{
    std::string channelId = args[0];
    json info             = args[1];
    json track            = args[2];

    auto channel = chanMgr->findChannelByID(channelId);
    if (!channel)
        throw application_error(kChannelNotFound, "Channel not found");

    channel->updateInfo(mergeChanInfo(channel->info, info, track));

    return nullptr;
}

json JrpcApi::stopChannel(json::array_t args)
{
    std::string channelId = args[0];

    GnuID id(channelId);

    auto channel = chanMgr->findChannelByID(id);

    if (channel)
        channel->thread.shutdown();

    return nullptr;
}

json JrpcApi::getChannelRelayTree(json::array_t args)
{
    GnuID id = args[0].get<std::string>();

    auto channel = chanMgr->findChannelByID(id);
    if (!channel)
        throw application_error(kChannelNotFound, "Channel not found");

    ChanHitList *hitList = chanMgr->findHitListByID(id);
    if (!hitList)
        throw application_error(kUnknownError, "Hit list not found");

    HostGraph graph(channel, hitList);

    return graph.getRelayTree();
}

json JrpcApi::bumpChannel(json::array_t args)
{
    GnuID id = args[0].get<std::string>();

    if (!id.isSet())
        throw invalid_params("id");

    auto channel = chanMgr->findChannelByID(id);
    if (!channel)
        throw application_error(kChannelNotFound, "Channel not found");

    channel->bump = true;

    return nullptr;
}

json JrpcApi::removeYellowPage(json::array_t args)
{
    if (args[0].get<int>() == 0) {
        std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
        servMgr->rootHost.clear();
        return nullptr;
    } else {
        throw application_error(kUnknownError, "Unknown yellow page id");
    }
}

json JrpcApi::getYPChannels(json::array_t args)
{
    auto channels = servMgr->channelDirectory->channels();
    json::array_t res;

    for (auto& c : channels)
    {
        res.push_back({
                { "yellowPage",  c.feedUrl },
                { "name",        c.name },
                { "channelId",   c.id.str() },
                { "tracker",     c.tip },
                { "contactUrl",  c.url },
                { "genre",       c.genre },
                { "description", c.desc },
                { "comment",     c.comment },
                { "bitrate",     c.bitrate },
                { "contentType", c.contentTypeStr },
                { "trackTitle",  c.trackName },
                { "album",       c.trackAlbum },
                { "creator",     c.trackArtist },
                { "trackUrl",    c.trackContact },
                { "listeners",   c.numDirects },
                { "relays",      c.numRelays }
            });
    }
    return res;
}

json JrpcApi::getYPChannelsInternal(json::array_t args)
{
    auto channels = servMgr->channelDirectory->channels();
    json::array_t res;

    for (auto& c : channels)
    {
        res.push_back({
                { "name",           c.name },
                { "id",             c.id.str() },
                { "tip",            c.tip },
                { "url",            c.url },
                { "genre",          c.genre },
                { "desc",           c.desc },
                { "numDirects",     c.numDirects },
                { "numRelays",      c.numRelays },
                { "bitrate",        c.bitrate },
                { "contentTypeStr", c.contentTypeStr },
                { "trackArtist",    c.trackArtist },
                { "trackAlbum",     c.trackAlbum },
                { "trackName",      c.trackName },
                { "trackContact",   c.trackContact },
                { "encodedName",    c.encodedName },
                { "uptime",         c.uptime },
                { "status",         c.status },
                { "comment",        c.comment },
                { "direct",         c.direct },
                { "feedUrl",        c.feedUrl },
                { "chatUrl",        c.chatUrl() },
                { "statsUrl",       c.statsUrl() },
            });
    }
    return res;
}
