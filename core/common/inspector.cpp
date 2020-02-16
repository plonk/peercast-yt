#include "inspector.h"
#include "servmgr.h"
#include "chanmgr.h"
#include "notif.h"
#include "stats.h"
#include "sys.h"
#include "version2.h"
#include "uptest.h"
#include "chandir.h"
#include "logbuf.h"

GlobalObject::GlobalObject() {
  servMgr = ::servMgr;
  chanMgr = ::chanMgr;
  stats = &::stats;
  notificationBuffer = &g_notificationBuffer;
  sys = ::sys;
}

nlohmann::json Inspector::inspect(GlobalObject& g)
{
  return {
    {"servMgr", inspect(*g.servMgr)},
    {"chanMgr", inspect(*g.chanMgr)},
    {"stats", inspect(*g.stats)},
    {"notificationBuffer", inspect(*g.notificationBuffer)},
    {"sys", inspect(*g.sys)},
    };
}

nlohmann::json Inspector::inspect(Servent* s)
{
  std::lock_guard<std::recursive_mutex> cs(s->lock);

  return {
    {"id", s->serventIndex},
    {"type", s->getTypeStr()},
    {"status", s->getStatusStr()},
    {"address", s->getHost().str()},
    {"agent", s->agent.c_str()},
    {"bitrate", s->bitrate()},
    {"bitrateAvg", s->bitrateAvg()},
    {"uptime", s->uptime()},
    {"chanID", s->chanID.str()},
  };
}

nlohmann::json Inspector::inspect(ServFilter& f)
{
  return {
    {"network", !!(f.flags & ServFilter::F_NETWORK)},
    {"private", !!(f.flags & ServFilter::F_PRIVATE)},
    {"direct", !!(f.flags & ServFilter::F_DIRECT)},
    {"banned", !!(f.flags & ServFilter::F_BAN)},
    {"ip", f.getPattern()},
  };
}

nlohmann::json Inspector::inspect(ServMgr& m)
{
  std::lock_guard<std::recursive_mutex> cs(m.lock);

  using namespace std;

  std::vector<Servent*> servents;
  for (auto s = m.servents; s != nullptr; s = s->next)
    servents.push_back(s);

  std::vector<ServFilter> filters;
  for (int i = 0; i < m.numFilters; i++) {
    filters.push_back(m.filters[i]);
  }

  return {
    {"allow", nlohmann::json::object_t({
          {"HTML1", (bool)(m.allowServer1 & Servent::ALLOW_HTML)},
          {"broadcasting1", (bool)(m.allowServer1 & Servent::ALLOW_BROADCAST)},
          {"direct1", (bool)(m.allowServer1 & Servent::ALLOW_DIRECT)},
          {"network1", (bool)(m.allowServer1 & Servent::ALLOW_NETWORK)},
        })},
    {"audioCodec", m.audioCodec },
    {"auth", nlohmann::json::object_t({
          {"useCookies", (m.authType==ServMgr::AUTH_COOKIE)},
          {"useHTTP", (m.authType==ServMgr::AUTH_HTTPBASIC)},
          {"useSessionCookies", (m.cookieList.neverExpire==false)}, //?
        })},
    {"channelDirectory", inspect(*m.channelDirectory)},
    {"defaultChannelInfo", inspect(m.defaultChannelInfo)},
    {"disabled", m.isDisabled},
    {"filters", inspect(filters)},
    {"firewallKnown", m.getFirewall()!=ServMgr::FW_UNKNOWN},
    {"forceYP", false},
    {"hasUnsafeFilterSettings", m.hasUnsafeFilterSettings()},
    {"isFirewalled", m.getFirewall()==ServMgr::FW_ON},
    {"isPrivate", false},
    {"isRoot", m.isRoot},
    {"log.level", m.logLevel()},
    {"maxBitrateOut", m.maxBitrateOut},
    {"maxControlsIn", m.maxControl},
    {"maxDirect", m.maxDirect},
    {"maxRelays", m.maxRelays},
    {"maxServIn", m.maxServIn},
    {"numActive1", m.numActiveOnPort(m.serverHost.port)},
    {"numCIN", m.numConnected(Servent::T_CIN)},
    {"numCOUT", m.numConnected(Servent::T_COUT)},
    {"numChannelFeeds", m.channelDirectory->numFeeds()},
    {"numChannelFeedsPlusOne", m.channelDirectory->numFeeds() + 1},
    {"numDirect", m.numStreams(Servent::T_DIRECT, true)},
    {"numExternalChannels", m.channelDirectory->numChannels()}, // ChannelDirectory に移譲しよう。
    {"numFilters", m.numFilters+1},
    {"numIncoming", m.numActive(Servent::T_INCOMING)},
    {"numRelays", m.numStreams(Servent::T_RELAY, true) },
    {"numServHosts", m.numHosts(ServHost::T_SERVENT)},
    {"numServents", m.numServents()},
    {"password", m.password},
    {"preset", m.preset},
    {"refreshHTML", m.refreshHTML ? m.refreshHTML : 0x0fffffff},
    {"rootMsg", m.rootMsg.c_str()},
    {"rtmpPort", m.rtmpPort },
    {"rtmpServerMonitor", inspect(m.rtmpServerMonitor)},
    {"servents", inspect(servents)},
    {"serverIP", m.serverHost.str(false)},
    {"serverLocalIP", Host(ClientSocket::getIP(NULL), 0).str(false)},
    {"serverName", m.serverName.c_str()},
    {"serverPort", m.serverHost.port},
    {"serverPort1", m.serverHost.port},
    {"totalConnected", m.totalConnected()},
    {"transcodingEnabled", m.transcodingEnabled},
    {"upgradeURL", m.downloadURL},
    {"uptestServiceRegistry", inspect(*m.uptestServiceRegistry)},
    {"uptime", String().setFromStopwatch(m.getUptime()).c_str() },
    {"version", PCX_VERSTRING },
    {"wmvProtocol", m.wmvProtocol},
    {"ypAddress", m.rootHost.c_str()},
  };
}

nlohmann::json Inspector::inspect(ChannelDirectory& d)
{
  std::lock_guard<std::recursive_mutex> cs(d.m_lock);

  return {
    {"totalListeners", d.totalListeners()},
    {"totalRelays", d.totalRelays()},
    {"lastUpdate", [&](){
                     auto diff = sys->getTime() - d.m_lastUpdate;
                     auto min = diff / 60;
                     auto sec = diff % 60;
                     return (min) ? str::format("%dm %ds", min, sec) : str::format("%ds", sec);
                   }()},
    {"channels", inspect(d.m_channels)},
    {"feeds", inspect(d.m_feeds)},
  };
}

nlohmann::json Inspector::inspect(ChannelFeed& f)
{
  return {
    {"url", f.url},
    {"directoryUrl", str::replace_suffix(f.url, "index.txt", "")},
    {"status", ChannelFeed::statusToString(f.status)},
  };
}

template<typename T>
nlohmann::json Inspector::inspect(std::vector<T>& arr)
{
  std::vector<nlohmann::json> res;
  for (auto& elt : arr) {
    res.push_back(inspect(elt));
  }
  return res;
}

nlohmann::json Inspector::inspect(ChannelEntry& ch)
{
  return {
    {"name", ch.name},
    {"id", ch.id.str()},
    {"bitrate", ch.bitrate},
    {"contentTypeStr", ch.contentTypeStr},
    {"desc", ch.desc},
    {"genre", ch.genre},
    {"url", ch.url},
    {"tip", ch.tip},
    {"encodedName", ch.encodedName},
    {"uptime", ch.uptime},
    {"numDirects", ch.numDirects},
    {"numRelays", ch.numRelays},
    {"chatUrl", ch.chatUrl()},
    {"statsUrl", ch.statsUrl()},
    {"isPlayable", ch.id.isSet()},
  };
}

nlohmann::json Inspector::inspect(UptestServiceRegistry& r)
{
  std::lock_guard<std::recursive_mutex> cs(r.m_lock);

  return {
    {"numURLs", r.m_providers.size()},
    {"providers", inspect(r.m_providers)},
  };
}

nlohmann::json Inspector::inspect(UptestEndpoint& e)
{
  return {
    {"url", e.url},
    {"status", [&](){
                 switch (e.status)
                 {
                 case UptestEndpoint::kUntried:
                   return "Untried";
                 case UptestEndpoint::kSuccess:
                   return "Success";
                 case UptestEndpoint::kError:
                   return "Error";
                 default:
                   return "?";
                 }
               }()},
    {"speed", e.m_info.speed},
    {"over", e.m_info.over},
    {"checkable", e.m_info.checkable},
  };
}

nlohmann::json Inspector::inspect(TrackInfo& i)
{
  return {
    { "contact", i.contact.cstr() },
    { "title", i.title.cstr() },
    { "artist", i.artist.cstr() },
    { "album", i.album.cstr() },
    { "genre", i.genre.cstr() },
  };
}

nlohmann::json Inspector::inspect(ChanInfo& i)
{
  return {
    { "name", i.name.c_str() },
    { "desc", i.desc.c_str() },
    { "genre", i.genre.c_str() },
    { "url", i.url.c_str() },
    { "comment", i.comment.c_str() },

    { "type", i.getTypeStr() },
    { "typeLong", i.getTypeStringLong() },
    { "ext", i.getTypeExt() },
    { "id", i.id.str() },
    { "track", inspect(i.track) },
    { "plsExt", i.getPlayListExt() },
  };
}

nlohmann::json Inspector::inspect(RTMPServerMonitor& m)
{
  return {
    { "status", m.status() },
    { "processID", m.m_rtmpServer.pid() },
  };
}

nlohmann::json Inspector::inspect(ChanHitList* l)
{
  return nullptr;
}

nlohmann::json Inspector::inspect(std::shared_ptr<Channel> ch1)
{
  Channel& ch = *ch1;
  std::lock_guard<std::recursive_mutex> cs(ch.lock);

  auto numHits = 0;
  {
    ChanHitList *chl = chanMgr->findHitListByID(ch.info.id);
    if (chl)
      numHits = chl->numHits();
  }

  return {
    {"info", inspect(ch.info) },
    {"srcrate", ch.sourceData ? BYTES_TO_KBPS(ch.sourceData->getSourceRate()) : 0 },
    {"uptime", ch.info.lastPlayStart ? (nlohmann::json) (sys->getTime() - ch.info.lastPlayStart) : (nlohmann::json) nullptr },
    {"localRelays", ch.localRelays() },
    {"localListeners", ch.localListeners() },
    {"totalRelays", ch.totalRelays() },
    {"totalListeners", ch.totalListeners() },
    {"status", ch.getStatusStr() },
    {"keep", ch.stayConnected },
    {"streamPos", ch.streamPos },
    {"sourceType", ch.getSrcTypeStr() },
    {"sourceURL", ch.getSourceString() },
    {"headPos", ch.headPack.pos },
    {"headLen", ch.headPack.len },
    {"buffer", ch.getBufferString() },
    {"headDump", ch.renderHexDump(std::string(ch.headPack.data, ch.headPack.data + ch.headPack.len)) },
    {"numHits", numHits},
    {"authToken", chanMgr->authToken(ch.info.id) },
  };
}

nlohmann::json Inspector::inspect(ChanMgr& m)
{
  std::lock_guard<std::recursive_mutex> cs(m.lock);

  std::vector<std::shared_ptr<Channel>> channels;
  std::vector<ChanHitList*> hitLists;

  for (auto ch = m.channel; ch; ch = ch->next)
    channels.push_back(ch);
  for (auto hl = m.hitlist; hl; hl = hl->next)
    hitLists.push_back(hl);
  return {
    { "numHitLists", m.numHitLists() },
    { "numChannels", m.numChannels() },
    { "djMessage", m.broadcastMsg.cstr() },
    { "icyMetaInterval", m.icyMetaInterval },
    { "maxRelaysPerChannel", m.maxRelaysPerChannel },
    { "hostUpdateInterval", m.hostUpdateInterval },
    { "broadcastID", m.broadcastID.str() },
    { "channels", inspect(channels) },
    { "hitLists", inspect(hitLists) },
  };
}

nlohmann::json Inspector::inspect(Sys& s)
{
  return {
    { "log", inspect(*s.logBuf) },
    { "time", s.getTime() },
  };
}

nlohmann::json Inspector::inspect(Stats& s)
{
  std::lock_guard<std::recursive_mutex> cs(s.lock);

  return {
    { "totalInPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::BYTESIN)) },
    { "totalOutPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::BYTESOUT)) },
    { "totalPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::BYTESIN)+s.getPerSecond(Stats::BYTESOUT)) },
    { "wanInPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::BYTESIN)-s.getPerSecond(Stats::LOCALBYTESIN)) },
    { "wanOutPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::BYTESOUT)-s.getPerSecond(Stats::LOCALBYTESOUT)) },
    { "wanTotalPerSec", BYTES_TO_KBPS((s.getPerSecond(Stats::BYTESIN)-s.getPerSecond(Stats::LOCALBYTESIN))+(s.getPerSecond(Stats::BYTESOUT)-s.getPerSecond(Stats::LOCALBYTESOUT))) },
    { "netInPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::PACKETDATAIN)) },
    { "netOutPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::PACKETDATAOUT)) },
    { "netTotalPerSec", BYTES_TO_KBPS(s.getPerSecond(Stats::PACKETDATAOUT)+s.getPerSecond(Stats::PACKETDATAIN)) },
    { "packInPerSec", s.getPerSecond(Stats::NUMPACKETSIN) },
    { "packOutPerSec", s.getPerSecond(Stats::NUMPACKETSOUT) },
    { "packTotalPerSec", s.getPerSecond(Stats::NUMPACKETSOUT)+s.getPerSecond(Stats::NUMPACKETSIN) },
  };
}

#include "sstream.h"

nlohmann::json Inspector::inspect(LogBuffer& b)
{
  std::lock_guard<std::recursive_mutex> cs(b.lock);
  StringStream ss;

  b.dumpHTML(ss);
  return {
    { "dumpHTML", ss.str() },
  };
}

nlohmann::json Inspector::inspect(NotificationBuffer::Entry& e)
{
  String t;
  t.setFromTime(e.notif.time);
  if (t.cstr()[strlen(t)-1] == '\n')
    t.cstr()[strlen(t)-1] = '\0';

  return {
    { "message", e.notif.message },
    { "isRead", e.isRead },
    { "type", e.notif.getTypeStr() },
    { "unixTime", e.notif.time },
    { "time", t.cstr() },
  };
}

nlohmann::json Inspector::inspect(NotificationBuffer& b)
{
  std::lock_guard<std::recursive_mutex> cs(b.lock);
  std::vector<NotificationBuffer::Entry> v(b.notifications.begin(), b.notifications.end());

  return {
    { "numNotifications", b.numNotifications() },
    { "numUnread", b.numUnread() },
    // markAsRead
    { "notifications", inspect(v) },
  };
}
