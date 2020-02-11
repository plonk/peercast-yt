#include "json.hpp"
#include <string>

class GlobalObject;
class ServMgr;
class ChanMgr;
class Sys;
class LogBuffer;
class Stats;
class Sys;
class UptestServiceRegistry;
class RTMPServerMonitor;
class ChanInfo;
class ChannelDirectory;
class ChannelEntry;
class ChannelFeed;
class UptestEndpoint;
class TrackInfo;

// for NotificationBuffer::Entry
#include "notif.h"

class Inspector {
public:
  static nlohmann::json inspect(GlobalObject&);

  static nlohmann::json inspect(UptestEndpoint& e);
  template<typename T>
  static nlohmann::json inspect(std::vector<T>& arr);
  static nlohmann::json inspect(ChannelEntry&);
  static nlohmann::json inspect(ChannelFeed& f);
  static nlohmann::json inspect(ChannelDirectory&);
  static nlohmann::json inspect(UptestServiceRegistry&);
  static nlohmann::json inspect(ChanInfo&);
  static nlohmann::json inspect(RTMPServerMonitor&);
  static nlohmann::json inspect(ServMgr&);
  static nlohmann::json inspect(ChanMgr&);
  static nlohmann::json inspect(Sys&);
  static nlohmann::json inspect(Stats&);
  static nlohmann::json inspect(LogBuffer&);
  static nlohmann::json inspect(NotificationBuffer&);
  static nlohmann::json inspect(NotificationBuffer::Entry&);
  static nlohmann::json inspect(std::shared_ptr<Channel>);
  static nlohmann::json inspect(TrackInfo& i);
  static nlohmann::json inspect(ChanHitList* l);
};

class GlobalObject {
public:
  GlobalObject();

  ServMgr* servMgr;
  ChanMgr* chanMgr;
  Stats* stats;
  NotificationBuffer* notificationBuffer;
  Sys* sys;
};
