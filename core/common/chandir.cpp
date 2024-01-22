#include <algorithm>
#include <sstream>
#include <memory> // unique_ptr
#include <stdexcept> // runtime_error

#include "http.h"
#include "version2.h"
#include "socket.h"
#include "chandir.h"
#include "uri.h"
#include "servmgr.h"
#include "regexp.h"

using namespace std;

std::vector<ChannelEntry>
ChannelEntry::textToChannelEntries(const std::string& text, const std::string& aFeedUrl, std::vector<std::string>& errors)
{
    istringstream in(text);
    string line;
    vector<ChannelEntry> res;
    int lineno = 0;

    while (getline(in, line)) {
        lineno++;
        vector<string> fields = str::split(line, "<>");
        if (fields.size() != 19) {
            errors.push_back(str::format("Parse error at line %d.", lineno));
        } else {
            res.push_back(ChannelEntry(fields, aFeedUrl));
        }
    }

    return res;
}

std::string ChannelEntry::chatUrl()
{
    if (encodedName.empty())
        return "";

    auto index = feedUrl.rfind('/');
    if (index == std::string::npos)
        return "";
    else
        return feedUrl.substr(0, index) + "/chat.php?cn=" + encodedName;
}

std::string ChannelEntry::statsUrl()
{
    if (encodedName.empty())
        return "";

    auto index = feedUrl.rfind('/');
    if (index == std::string::npos)
        return "";
    else
        return feedUrl.substr(0, index) + "/getgmt.php?cn=" + encodedName;
}

ChannelDirectory::ChannelDirectory()
    : m_lastUpdate(0)
{
}

int ChannelDirectory::numChannels() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return m_channels.size();
}

int ChannelDirectory::numFeeds() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return m_feeds.size();
}

#include "sslclientsocket.h"
// index.txt を指す URL である url からチャンネルリストを読み込み、out
// に格納する。成功した場合は true が返る。エラーが発生した場合は
// false が返る。false を返した場合でも out に読み込めたチャンネル情報
// が入っている場合がある。
static bool getFeed(std::string url, std::vector<ChannelEntry>& out)
{
    const std::string originalUrl = url;

    out.clear();

    try {
        auto body = http::get(url);

        std::vector<std::string> errors;
        out = ChannelEntry::textToChannelEntries(body, originalUrl, errors);

        for (auto& message : errors) {
            LOG_ERROR("%s", message.c_str());
        }

        return errors.empty();
    } catch (GeneralException& e) {
        LOG_ERROR("%s", e.msg);
        return false;
    }
}

#include "sstream.h"
#include "defer.h"
#include "logbuf.h"

static std::string runProcess(std::function<void(Stream&)> action)
{
    StringStream ss;
    try {
        assert(AUX_LOG_FUNC_VECTOR != nullptr);
        AUX_LOG_FUNC_VECTOR->push_back([&](LogBuffer::TYPE type, const char* msg) -> void
                                      {
                                          if (type == LogBuffer::T_ERROR)
                                              ss.writeString("Error: ");
                                          else if (type == LogBuffer::T_WARN)
                                              ss.writeString("Warning: ");
                                          ss.writeLine(msg);
                                      });
        Defer defer([]() { AUX_LOG_FUNC_VECTOR->pop_back(); });

        action(ss);
    } catch(GeneralException& e) {
        ss.writeLineF("Error: %s\n", e.what());
    }
    return ss.str();
}

bool ChannelDirectory::update(UpdateMode mode)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    const unsigned int coolDownTime = (mode==kUpdateManual) ? 30 : 5 * 60;
    if (sys->getTime() - m_lastUpdate < coolDownTime)
        return false;

    double t0 = sys->getDTime();
    std::vector<std::thread> workers;
    std::mutex mutex; // m_channels を保護するミューテックス。
    m_channels.clear();
    for (auto& feed : m_feeds)
    {
        std::function<void(void)> getChannels =
            [&feed, &mutex, this]
            {
                assert(AUX_LOG_FUNC_VECTOR == nullptr);
                AUX_LOG_FUNC_VECTOR = new std::vector<std::function<void(LogBuffer::TYPE type, const char*)>>();
                assert(AUX_LOG_FUNC_VECTOR != nullptr);
                Defer defer([]()
                            {
                                assert(AUX_LOG_FUNC_VECTOR != nullptr);
                                delete AUX_LOG_FUNC_VECTOR;
                            });

                feed.log = runProcess([&feed, &mutex, this](Stream& s)
                                      {
                                          // print start time
                                          String time;
                                          time.setFromTime(sys->getTime());
                                          s.writeStringF("Start time: %s\n", time.c_str()); // two newlines at the end

                                          std::vector<ChannelEntry> channels;
                                          bool success;
                                          double t1 = sys->getDTime();
                                          success = getFeed(feed.url, channels);
                                          double t2 = sys->getDTime();

                                          LOG_TRACE("Got %zu channels from %s (%.6f seconds)", channels.size(), feed.url.c_str(), t2 - t1);
                                          if (success) {
                                              feed.status = ChannelFeed::Status::kOk;
                                          } else {
                                              feed.status = ChannelFeed::Status::kError;
                                          }

                                          {
                                              std::lock_guard<std::mutex> lock(mutex);
                                              for (auto& c : channels) m_channels.push_back(c);
                                          }
                                      });
            };
        workers.push_back(std::thread(getChannels));
    }

    for (auto& t : workers)
        t.join();

    sort(m_channels.begin(), m_channels.end(),
         [](ChannelEntry& a, ChannelEntry& b)
         {
             return a.numDirects > b.numDirects;
         });
    m_lastUpdate = sys->getTime();
    LOG_INFO("Channel feed update: total of %zu channels in %f sec",
             m_channels.size(),
             sys->getDTime() - t0);
    return true;
}

// index番目のチャンネル詳細のフィールドを出力する。成功したら true を返す。
bool ChannelDirectory::writeChannelVariable(Stream& out, const String& varName, int index)
{
    using namespace std;

    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (!(index >= 0 && (size_t)index < m_channels.size()))
        return false;

    string buf;
    ChannelEntry& ch = m_channels[index];

    if (varName == "name") {
        buf = ch.name;
    } else if (varName == "id") {
        buf = ch.id.str();
    } else if (varName == "bitrate") {
        buf = to_string(ch.bitrate);
    } else if (varName == "contentTypeStr") {
        buf = ch.contentTypeStr;
    } else if (varName == "desc") {
        buf = ch.desc;
    } else if (varName == "genre") {
        buf = ch.genre;
    } else if (varName == "url") {
        buf = ch.url;
    } else if (varName == "tip") {
        buf = ch.tip;
    } else if (varName == "encodedName") {
        buf = ch.encodedName;
    } else if (varName == "uptime") {
        buf = ch.uptime;
    } else if (varName == "numDirects") {
        buf = to_string(ch.numDirects);
    } else if (varName == "numRelays") {
        buf = to_string(ch.numRelays);
    } else if (varName == "chatUrl") {
        buf = ch.chatUrl();
    } else if (varName == "statsUrl") {
        buf = ch.statsUrl();
    } else if (varName == "isPlayable") {
        buf = to_string(ch.id.isSet());
    } else {
        return false;
    }

    out.writeString(buf);
    return true;
}

static std::string directoryUrlOf(const std::string& url)
{
    auto matches = Regexp("/[^/]*$").exec(url);
        
    if (matches.size() > 0) {
        return str::replace_suffix(url, matches[0], "/");
    } else {
        return url;
    }
}

static std::string formatTime(unsigned int diff)
{
    auto min = diff / 60;
    auto sec = diff % 60;
    if (min == 0) {
        return str::format("%ds", sec);
    } else {
        return str::format("%dm %ds", min, sec);
    }
}

amf0::Value ChannelDirectory::getState()
{
    std::vector<amf0::Value> channels;

    for (auto& c : this->channels())
    {
        channels.push_back(amf0::Value::object(
                          {
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
                          }));
    }

    std::vector<amf0::Value> feeds;
    for (auto& f : this->feeds())
    {
        int count = std::count_if(channels.begin(), channels.end(), [&](const amf0::Value& v) { return v.object().at("feedUrl")==f.url; });
        feeds.push_back(amf0::Value::object(
                            {
                                {"url", f.url},
                                {"directoryUrl", directoryUrlOf(f.url)},
                                {"status", ChannelFeed::statusToString(f.status) },
                                {"numChannels", count }
                            }));
    }

    return amf0::Value::object(
        {
            {"totalListeners", totalListeners()},
            {"totalRelays", totalRelays()},
            {"lastUpdate", formatTime(sys->getTime() - m_lastUpdate)},
            {"channels", amf0::Value::strictArray(channels) },
            {"feeds", amf0::Value::strictArray(feeds) },
        });
}

int ChannelDirectory::totalListeners() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    int res = 0;

    for (const ChannelEntry& e : m_channels) {
        res += std::max(0, e.numDirects);
    }
    return res;
}

int ChannelDirectory::totalRelays() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    int res = 0;

    for (const ChannelEntry& e : m_channels) {
        res += std::max(0, e.numRelays);
    }
    return res;
}

std::vector<ChannelFeed> ChannelDirectory::feeds() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return m_feeds;
}

bool ChannelDirectory::addFeed(const std::string& url)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    auto iter = find_if(m_feeds.begin(), m_feeds.end(), [&](ChannelFeed& f) { return f.url == url;});

    if (iter != m_feeds.end()) {
        LOG_ERROR("Already have feed %s", url.c_str());
        return false;
    }

    URI u(url);
    if (!u.isValid() || (u.scheme() != "http" && u.scheme() != "https")) {
        LOG_ERROR("Invalid feed URL %s", url.c_str());
        return false;
    }

    m_feeds.push_back(ChannelFeed(url));
    return true;
}

void ChannelDirectory::clearFeeds()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    m_feeds.clear();
    m_channels.clear();
    m_lastUpdate = 0;
}

std::string ChannelDirectory::findTracker(const GnuID& id) const
{
    for (const ChannelEntry& entry : m_channels)
    {
        if (entry.id.isSame(id))
            return entry.tip;
    }
    return "";
}

std::string ChannelFeed::statusToString(ChannelFeed::Status s)
{
    switch (s) {
    case Status::kUnknown:
        return "UNKNOWN";
    case Status::kOk:
        return "OK";
    case Status::kError:
        return "ERROR";
    }
    throw std::logic_error("should be unreachable");
}

std::vector<ChannelEntry> ChannelDirectory::channels() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return m_channels;
}
