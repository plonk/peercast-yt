#include "chanmgr.h"

#include "playlist.h"
#include "peercast.h"
#include "version2.h" // PCP_BROADCAST_FLAGS
#include "md5.h"

// -----------------------------------
void ChanMgr::quit()
{
    LOG_DEBUG("ChanMgr is quitting..");
    closeAll();
}

// -----------------------------------
int ChanMgr::numIdleChannels()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    int cnt = 0;
    auto ch = channel;
    while (ch)
    {
        std::lock_guard<std::recursive_mutex> cs(ch->lock);

        if (ch->isActive())
            if (ch->thread.active())
                if (ch->status == Channel::S_IDLE)
                    cnt++;
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
void ChanMgr::closeIdles()
{
    // 直接 this->channel をたどって、個々のチャンネルのスレッドを
    // shutdownするとそのチャンネルスレッドが自身をリストから削除する
    // ためにChanMgrをロックしようとしてデッドロックになるため、ここで
    // チャンネルリストをコピーする。
    std::vector<std::shared_ptr<Channel>> channels;
    {
        std::lock_guard<std::recursive_mutex> cs(lock);
        for (auto ch = channel; ch; ch = ch->next)
            channels.push_back(ch);
    }

    for (auto ch : channels)
    {
        if (ch->isIdle())
        {
            ch->thread.shutdown();
            sys->waitThread(&ch->thread);
        }
    }
}

// -----------------------------------
void ChanMgr::closeOldestIdle()
{
    unsigned int idleTime = (unsigned int)-1;
    std::shared_ptr<Channel> ch = channel, oldest = NULL;

    while (ch)
    {
        if (ch->isActive())
            if (ch->thread.active())
                if (ch->status == Channel::S_IDLE)
                    if (ch->lastIdleTime < idleTime)
                    {
                        oldest = ch;
                        idleTime = ch->lastIdleTime;
                    }
        ch = ch->next;
    }

    if (oldest)
        oldest->thread.shutdown();
}

// -----------------------------------
void ChanMgr::closeAll()
{
    auto ch = channel;
    while (ch)
    {
        if (ch->thread.active())
            ch->thread.shutdown();
        ch = ch->next;
    }
}

// -----------------------------------
std::shared_ptr<Channel>ChanMgr::findChannelByNameID(ChanInfo &info)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    auto ch = channel;
    while (ch)
    {
        std::lock_guard<std::recursive_mutex> cs1(ch->lock);
        if (ch->isActive())
            if (ch->info.matchNameID(info))
                return ch;
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
std::shared_ptr<Channel>ChanMgr::findChannelByName(const char *n)
{
    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (Sys::stricmp(ch->info.name, n) == 0)
                return ch;
        ch = ch->next;
    }

    return NULL;
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::findChannelByIndex(int index)
{
    int cnt = 0;
    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
        {
            if (cnt == index)
                return ch;
            cnt++;
        }
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::findChannelByMount(const char *str)
{
    auto ch = channel;
    while (ch)
    {
        std::lock_guard<std::recursive_mutex> lock(ch->lock);
        if (ch->isActive())
            if (strcmp(ch->mount, str) == 0)
                return ch;
        ch = ch->next;
    }

    return NULL;
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::findChannelByID(const GnuID &id)
{
    auto ch = channel;
    while (ch)
    {
        std::lock_guard<std::recursive_mutex> lock(ch->lock);
        if (ch->isActive())
            if (ch->info.id.isSame(id))
                return ch;
        ch = ch->next;
    }
    return NULL;
}

// -----------------------------------
int ChanMgr::findChannels(ChanInfo &info, std::shared_ptr<Channel> *chlist, int max)
{
    int cnt = 0;
    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->info.match(info))
            {
                chlist[cnt++] = ch;
                if (cnt >= max)
                    break;
            }
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
int ChanMgr::findChannelsByStatus(std::shared_ptr<Channel> *chlist, int max, Channel::STATUS status)
{
    int cnt = 0;
    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->status == status)
            {
                chlist[cnt++] = ch;
                if (cnt >= max)
                    break;
            }
        ch = ch->next;
    }
    return cnt;
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::createRelay(ChanInfo &info, bool stayConnected)
{
    auto c = chanMgr->createChannel(info, NULL);
    if (c)
    {
        c->stayConnected = stayConnected;
        c->startGet();
        return c;
    }
    return NULL;
}

// -----------------------------------
static std::string chName(ChanInfo& info)
{
    if (info.name.str().empty())
        return info.id.str().substr(0,7) + "...";
    else
        return info.name.str();
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::findAndRelay(ChanInfo &info)
{
    LOG_INFO("Searching for: %s (%s)", info.id.str().c_str(), info.name.cstr());
    //peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネルを検索中...");

    auto c = findChannelByNameID(info);

    if (!c)
    {
        c = chanMgr->createChannel(info, NULL);
        if (c)
        {
            c->setStatus(Channel::S_SEARCHING);
            c->startGet();
        }
    }

    for (int i = 0; i < 600; i++)    // search for 1 minute.
    {
        c = findChannelByNameID(info);

        if (!c)
        {
            peercast::notifyMessage(ServMgr::NT_PEERCAST, "チャンネル "+chName(info)+" は見付かりませんでした。");
            return NULL;
        }

        // if (c->isPlaying() && (c->info.contentType != ChanInfo::T_UNKNOWN))
        //     break;
        if (c->isPlaying()) // UNKNOWN でもかまわないことにする。
            break;

        sys->sleep(100);
    }

    return c;
}

// -----------------------------------
ChanMgr::ChanMgr()
{
    channel = NULL;

    hitlist = NULL;

    currFindAndPlayChannel.clear();

    broadcastMsg.clear();
    broadcastMsgInterval = 10;

    broadcastID.generate();

    deadHitAge = 600;

    icyIndex = 0;
    icyMetaInterval = 8192;
    maxRelaysPerChannel = 0;

    minBroadcastTTL = 1;
    maxBroadcastTTL = 7;

    pushTimeout = 60;   // 1 minute
    pushTries = 5;      // 5 times
    maxPushHops = 8;    // max 8 hops away
    maxUptime = 0;      // 0 = none

    prefetchTime = 10;  // n seconds

    hostUpdateInterval = 120; // 2 minutes

    bufferTime = 5;

    lastYPConnect = 0;
}

// -----------------------------------
ChanMgr::~ChanMgr()
{
    clearHitLists();
}

// -----------------------------------
amf0::Value ChanMgr::getState()
{
    std::vector<amf0::Value> channelArray;
    for (auto ch = channel; ch != nullptr; ch = ch->next)
        channelArray.push_back(ch->getState());

    return amf0::Value::object(
        {
            { "channels", channelArray },
            { "numHitLists",numHitLists()},
            { "numChannels",numChannels()},
            { "djMessage", broadcastMsg.c_str()},
            { "icyMetaInterval",icyMetaInterval},
            { "maxRelaysPerChannel",maxRelaysPerChannel},
            { "hostUpdateInterval",hostUpdateInterval},
            { "broadcastID",         broadcastID.str() },
        });
}

// -----------------------------------
void ChanMgr::broadcastTrackerUpdate(const GnuID &svID, bool force)
{
    auto c = channel;
    while (c)
    {
        if ( c->isActive() && c->isBroadcasting() )
            c->broadcastTrackerUpdate(svID, force);
        c = c->next;
    }
}

// -----------------------------------
int ChanMgr::broadcastPacketUp(ChanPacket &pack, const GnuID &chanID, const GnuID &srcID, const GnuID &destID)
{
    int cnt = 0;

    auto c = channel;
    while (c)
    {
        if (c->sendPacketUp(pack, chanID, srcID, destID))
            cnt++;
        c = c->next;
    }

    return cnt;
}

// -----------------------------------
void ChanMgr::setUpdateInterval(unsigned int v)
{
    hostUpdateInterval = v;
}

// -----------------------------------
void ChanMgr::setBroadcastMsg(const String &msg)
{
    if (!msg.isSame(broadcastMsg))
    {
        broadcastMsg = msg;

        auto c = channel;
        while (c)
        {
            if (c->isActive() && c->isBroadcasting())
            {
                ChanInfo newInfo = c->info;
                newInfo.comment = broadcastMsg;
                c->updateInfo(newInfo);
            }
            c = c->next;
        }
    }
}

// -----------------------------------
void ChanMgr::clearHitLists()
{
    while (hitlist)
    {
        peercastApp->delChannel(&hitlist->info);

        auto next = hitlist->next;

        hitlist = next;
    }
}

// -----------------------------------
void ChanMgr::deleteChannel(std::shared_ptr<Channel> delchan)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    std::shared_ptr<Channel> ch = channel, prev = NULL;

    while (ch)
    {
        if (ch == delchan)
        {
            std::shared_ptr<Channel> next = ch->next;
            if (prev)
                prev->next = next;
            else
                channel = next;
            break;
        }
        prev = ch;
        ch = ch->next;
    }
}

// -----------------------------------
std::shared_ptr<Channel> ChanMgr::createChannel(ChanInfo &info, const char *mount)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    auto nc = std::make_shared<Channel>();

    nc->next = channel;
    channel = nc;

    nc->info = info;
    nc->info.lastPlayStart = 0;
    nc->info.lastPlayEnd = 0;
    nc->info.status = ChanInfo::S_UNKNOWN;
    if (mount)
        nc->mount.set(mount);
    nc->setStatus(Channel::S_WAIT);
    nc->type = Channel::T_ALLOCATED;
    nc->info.createdTime = sys->getTime();

    nc->rootHost = servMgr->rootHost.c_str();

    LOG_INFO("New channel created");

    return nc;
}

// -----------------------------------
int ChanMgr::pickHits(ChanHitSearch &chs)
{
    auto chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            if (chl->pickHits(chs))
            {
                return 1;
            }
        chl = chl->next;
    }
    return 0;
}

// -----------------------------------
std::shared_ptr<ChanHitList> ChanMgr::findHitList(ChanInfo &info)
{
    auto chl = hitlist;
    while (chl)
    {
        std::lock_guard<std::recursive_mutex> lock(chl->lock);
        if (chl->isUsed())
            if (chl->info.matchNameID(info))
                return chl;

        chl = chl->next;
    }
    return NULL;
}

// -----------------------------------
std::shared_ptr<ChanHitList> ChanMgr::findHitListByID(const GnuID &id)
{
    auto chl = hitlist;
    while (chl)
    {
        std::lock_guard<std::recursive_mutex> lock(chl->lock);
        if (chl->isUsed())
            if (chl->info.id.isSame(id))
                return chl;
        chl = chl->next;
    }
    return NULL;
}

// -----------------------------------
int ChanMgr::numHitLists()
{
    int num = 0;
    auto chl = hitlist;
    while (chl)
    {
        if (chl->isUsed())
            num++;
        chl = chl->next;
    }
    return num;
}

// -----------------------------------
std::shared_ptr<ChanHitList> ChanMgr::addHitList(ChanInfo &info)
{
    auto chl = std::make_shared<ChanHitList>();

    chl->next = hitlist;
    hitlist = chl;

    chl->used = true;
    chl->info = info;
    chl->info.createdTime = sys->getTime();
    peercastApp->addChannel(&chl->info);

    return chl;
}

// -----------------------------------
void ChanMgr::clearDeadHits(bool clearTrackers)
{
    std::lock_guard<std::recursive_mutex> cs(lock);
    constexpr unsigned int interval = 180;

    std::shared_ptr<ChanHitList> chl = hitlist, prev = nullptr;
    while (chl)
    {
        if (chl->isUsed())
        {
            if (chl->clearDeadHits(interval, clearTrackers) == 0)
            {
                if (!isBroadcasting(chl->info.id))
                {
                    if (!chanMgr->findChannelByID(chl->info.id))
                    {
                        peercastApp->delChannel(&chl->info);

                        auto next = chl->next;
                        if (prev)
                            prev->next = next;
                        else
                            hitlist = next;

                        chl = next;
                        continue;
                    }
                }
            }
        }
        prev = chl;
        chl = chl->next;
    }
}

// -----------------------------------
bool    ChanMgr::isBroadcasting(const GnuID &id)
{
    auto ch = findChannelByID(id);
    if (ch)
        return ch->isBroadcasting();

    return false;
}

// -----------------------------------
bool    ChanMgr::isBroadcasting()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
            if (ch->isBroadcasting())
                return true;

        ch = ch->next;
    }
    return false;
}

// -----------------------------------
int ChanMgr::numChannels()
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    int tot = 0;
    auto ch = channel;
    while (ch)
    {
        if (ch->isActive())
            tot++;
        ch = ch->next;
    }
    return tot;
}

// -----------------------------------
void ChanMgr::deadHit(ChanHit &hit)
{
    auto chl = findHitListByID(hit.chanID);
    if (chl)
        chl->deadHit(hit);
}

// -----------------------------------
void ChanMgr::delHit(ChanHit &hit)
{
    auto chl = findHitListByID(hit.chanID);
    if (chl)
        chl->delHit(hit);
}

// -----------------------------------
void ChanMgr::addHit(Host &h, const GnuID &id, bool tracker)
{
    ChanHit hit;
    hit.init();
    hit.host = h;
    hit.rhost[0] = h;
    hit.rhost[1].init();
    if (!h.ip.isIPv4Mapped())
        hit.rhost[1].ip = IP::parse("::");
    hit.tracker = tracker;
    hit.recv = true;
    hit.chanID = id;
    addHit(hit);
}

// -----------------------------------
std::shared_ptr<ChanHit> ChanMgr::addHit(ChanHit &h)
{
    std::lock_guard<std::recursive_mutex> cs(lock);

    auto hl = findHitListByID(h.chanID);

    if (!hl)
    {
        ChanInfo info;
        info.id = h.chanID;
        hl = addHitList(info);
    }

    if (hl)
    {
        return hl->addHit(h);
    }else
        return NULL;
}

// -----------------------------------
class ChanFindInfo : public ThreadInfo
{
public:
    ChanInfo    info;
    bool        keep;
};

// -----------------------------------
THREAD_PROC findAndPlayChannelProc(ThreadInfo *th)
{
    ChanFindInfo *cfi = (ChanFindInfo *)th;

    sys->setThreadName("findAndPlayChannelProc");

    ChanInfo info;
    info = cfi->info;

    auto ch = chanMgr->findChannelByNameID(info);

    chanMgr->currFindAndPlayChannel = info.id;

    if (!ch)
        ch = chanMgr->findAndRelay(info);

    if (ch)
    {
        // check that a different channel hasn`t be selected already.
        if (chanMgr->currFindAndPlayChannel.isSame(ch->info.id))
            chanMgr->playChannel(ch->info);

        if (cfi->keep)
            ch->stayConnected = cfi->keep;
    }

    delete cfi;
    return 0;
}

// -----------------------------------
void ChanMgr::findAndPlayChannel(ChanInfo &info, bool keep)
{
    ChanFindInfo *cfi = new ChanFindInfo;
    cfi->info = info;
    cfi->keep = keep;
    cfi->func = findAndPlayChannelProc;

    sys->startThread(cfi);
}

// -----------------------------------
void ChanMgr::playChannel(ChanInfo &info)
{
    std::string str = str::format("http://localhost:%d", servMgr->serverHost.port);
    std::string fname;
    PlayList::TYPE type;

    if ((info.contentType == ChanInfo::T_WMA) || (info.contentType == ChanInfo::T_WMV))
    {
        type = PlayList::T_ASX;
        // WMP seems to have a bug where it doesn`t re-read asx files if they have the same name
        // so we prepend the channel id to make it unique - NOTE: should be deleted afterwards.
        fname = str::format("%s/%s.asx", peercastApp->getCacheDirPath(), info.id.str().c_str());
    }else if (info.contentType == ChanInfo::T_OGM)
    {
        type = PlayList::T_RAM;
        fname = str::format("%s/play.ram", peercastApp->getCacheDirPath());
    }else
    {
        type = PlayList::T_SCPLS;
        fname = str::format("%s/play.pls", peercastApp->getCacheDirPath());
    }

    auto pls = std::make_shared<PlayList>(type, 1);
    pls->addChannel(str, info);

    LOG_DEBUG("Writing %s", sys->fromFilenameEncoding(fname).c_str());
    FileStream file;
    file.openWriteReplace(fname);
    pls->write(file);
    file.close();

    LOG_DEBUG("Executing: %s", sys->fromFilenameEncoding(fname).c_str());
    sys->executeFile(fname);
}

// -----------------------------------
std::string ChanMgr::authSecret(const GnuID& id)
{
    return broadcastID.str() + ":" + id.str();
}

// --------------------------------------------------
std::string ChanMgr::authToken(const GnuID& id)
{
    return md5::hexdigest(authSecret(id));
}
