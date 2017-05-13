#include <algorithm>
#include <sstream>
#include <memory> // unique_ptr
#include <stdexcept> // runtime_error

#include "http.h"
#include "version2.h"
#include "socket.h"
#include "chandir.h"
#include "critsec.h"
#include "uri.h"
#include "servmgr.h"

using namespace std;

std::vector<ChannelEntry>
ChannelEntry::textToChannelEntries(const std::string& text, const std::string& aFeedUrl)
{
    istringstream in(text);
    string line;
    vector<ChannelEntry> res;
    int lineno = 0;

    while (getline(in, line)) {
        lineno++;

        vector<string> fields;
        const char *p = line.c_str();
        const char *q;

        while ((q = strstr(p, "<>")) != nullptr) {
            fields.push_back(string(p, q));
            p = q + 2; // <>をスキップ
        }
        fields.push_back(p);

        if (fields.size() != 19) {
            throw std::runtime_error(String::format("Parse error at line %d.", lineno).cstr());
        }

        res.push_back(ChannelEntry(fields, aFeedUrl));
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

int ChannelDirectory::numChannels()
{
    CriticalSection cs(m_lock);
    return m_channels.size();
}

int ChannelDirectory::numFeeds()
{
    CriticalSection cs(m_lock);
    return m_feeds.size();
}

// index.txt を指す URL である url からチャンネルリストを読み込み、out
// に格納する。成功した場合は true が返る。失敗した場合は false が返り、
// out は変更されない。
static bool getFeed(std::string url, std::vector<ChannelEntry>& out)
{
    URI feed(url);
    if (!feed.isValid()) {
        LOG_ERROR("invalid URL (%s)", url.c_str());
        return false;
    }
    if (feed.scheme() != "http") {
        LOG_ERROR("unsupported protocol (%s)", url.c_str());
        return false;
    }

    Host host;
    host.fromStrName(feed.host().c_str(), feed.port());
    if (host.ip==0) {
        LOG_ERROR("Could not resolve %s", feed.host().c_str());
        return false;
    }

    unique_ptr<ClientSocket> rsock(sys->createSocket());
    WriteBufferedStream brsock(&*rsock);

    try {
        LOG_DEBUG("Connecting to %s ...", feed.host().c_str());
        rsock->open(host);
        rsock->connect();

        HTTP rhttp(brsock);

        auto request_line = "GET " + feed.path() + "?host=" + cgi::escape(servMgr->serverHost) + " HTTP/1.0";
        LOG_DEBUG("Request line: %s", request_line.c_str());

        rhttp.writeLineF("%s", request_line.c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_HOST, feed.host().c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
        rhttp.writeLineF("%s %s", HTTP_HS_AGENT, PCX_AGENT);
        rhttp.writeLine("");

        auto code = rhttp.readResponse();
        if (code != 200) {
            LOG_DEBUG("%s: status code %d", feed.host().c_str(), code);
            return false;
        }

        while (rhttp.nextHeader())
            ;

        std::string text;
        char line[1024];

        try {
            while (rhttp.readLine(line, 1024)) {
                text += line;
                text += '\n';
            }
        } catch (SockException& e) {
            // end of body reached.
        }

        try {
            out = ChannelEntry::textToChannelEntries(text, url);
        } catch (std::runtime_error& e) {
            LOG_ERROR("%s", e.what());
            return false;
        }
        return true;
    } catch (SockException& e) {
        LOG_ERROR("%s", e.msg);
        return false;
    } catch (TimeoutException& e) {
        LOG_ERROR("%s", e.msg);
        return false;
    }
}

bool ChannelDirectory::update()
{
    CriticalSection cs(m_lock);

    if (sys->getTime() - m_lastUpdate < 5 * 60)
        return false;

    m_channels.clear();
    for (auto& feed : m_feeds) {
        std::vector<ChannelEntry> channels;
        bool success;

        success = getFeed(feed.url, channels);

        if (success) {
            feed.status = ChannelFeed::Status::OK;
            LOG_DEBUG("Got %zu channels from %s", channels.size(), feed.url.c_str());
            for (auto ch : channels) {
                m_channels.push_back(ch);
            }
        } else {
            feed.status = ChannelFeed::Status::ERROR;
            LOG_ERROR("Failed to get channels from %s", feed.url.c_str());
        }
    }
    sort(m_channels.begin(), m_channels.end(), [](ChannelEntry&a, ChannelEntry&b) { return a.numDirects > b.numDirects; });
    m_lastUpdate = sys->getTime();
    return true;
}

// index番目のチャンネル詳細のフィールドを出力する。成功したら true を返す。
bool ChannelDirectory::writeChannelVariable(Stream& out, const String& varName, int index)
{
    CriticalSection cs(m_lock);

    if (!(index >= 0 && index < m_channels.size()))
        return false;

    char buf[1024];
    ChannelEntry& ch = m_channels[index];

    if (varName == "name") {
        sprintf(buf, "%s", ch.name.c_str());
    } else if (varName == "id") {
        ch.id.toStr(buf);
    } else if (varName == "bitrate") {
        sprintf(buf, "%d", ch.bitrate);
    } else if (varName == "contentTypeStr") {
        sprintf(buf, "%s", ch.contentTypeStr.c_str());
    } else if (varName == "desc") {
        sprintf(buf, "%s", ch.desc.c_str());
    } else if (varName == "genre") {
        sprintf(buf, "%s", ch.genre.c_str());
    } else if (varName == "url") {
        sprintf(buf, "%s", ch.url.c_str());
    } else if (varName == "tip") {
        sprintf(buf, "%s", ch.tip.c_str());
    } else if (varName == "encodedName") {
        sprintf(buf, "%s", ch.encodedName.c_str());
    } else if (varName == "uptime") {
        sprintf(buf, "%s", ch.uptime.c_str());
    } else if (varName == "numDirects") {
        sprintf(buf, "%d", ch.numDirects);
    } else if (varName == "numRelays") {
        sprintf(buf, "%d", ch.numRelays);
    } else if (varName == "chatUrl") {
        sprintf(buf, "%s", ch.chatUrl().c_str());
    } else if (varName == "statsUrl") {
        sprintf(buf, "%s", ch.statsUrl().c_str());
    } else if (varName == "isPlayable") {
        sprintf(buf, "%d", ch.id.isSet());
    } else {
        return false;
    }

    out.writeString(buf);
    return true;
}

bool ChannelDirectory::writeFeedVariable(Stream& out, const String& varName, int index)
{
    CriticalSection cs(m_lock);

    if (!(index >= 0 && index < m_feeds.size())) {
        // empty string
        return true;
    }

    string value;

    if (varName == "url") {
        value = m_feeds[index].url;
    } else if (varName == "directoryUrl") {
        value = str::replace_suffix(m_feeds[index].url, "index.txt", "");
    } else if (varName == "status") {
        value = ChannelFeed::statusToString(m_feeds[index].status);
    } else if (varName == "isPublic") {
        value = String::format("%d", m_feeds[index].isPublic).cstr();
    } else {
        return false;
    }

    out.writeString(value.c_str());
    return true;
}

bool ChannelDirectory::writeVariable(Stream& out, const String& varName, int index)
{
    if (varName.startsWith("externalChannel.")) {
        return writeChannelVariable(out, varName + strlen("externalChannel."), index);
    } else if (varName.startsWith("channelFeed.")) {
        return writeFeedVariable(out, varName + strlen("channelFeed."), index);
    } else {
        return false;
    }
}

bool ChannelDirectory::writeVariable(Stream& out, const String& varName)
{
    if (varName == "totalListeners") {
        out.writeString(to_string(totalListeners()).c_str());
    } else if (varName == "totalRelays") {
        out.writeString(to_string(totalRelays()).c_str());
    } else if (varName == "lastUpdate") {
        auto diff = sys->getTime() - m_lastUpdate;
        auto min = diff / 60;
        auto sec = diff % 60;
        if (min == 0) {
            out.writeString(String::format("%ds", sec).cstr());
        } else {
            out.writeString(String::format("%dm %ds", min, sec).cstr());
        }
    } else {
        return false;
    }
    return true;
}

int ChannelDirectory::totalListeners()
{
    CriticalSection cs(m_lock);
    int res = 0;

    for (ChannelEntry& e : m_channels) {
        res += std::max(0, e.numDirects);
    }
    return res;
}

int ChannelDirectory::totalRelays()
{
    CriticalSection cs(m_lock);
    int res = 0;

    for (ChannelEntry& e : m_channels) {
        res += std::max(0, e.numRelays);
    }
    return res;
}

std::vector<ChannelFeed> ChannelDirectory::feeds()
{
    CriticalSection cs(m_lock);
    return m_feeds;
}

bool ChannelDirectory::addFeed(std::string url)
{
    CriticalSection cs(m_lock);

    auto iter = find_if(m_feeds.begin(), m_feeds.end(), [&](ChannelFeed& f) { return f.url == url;});

    if (iter != m_feeds.end()) {
        LOG_ERROR("Already have feed %s", url.c_str());
        return false;
    }

    URI u(url);
    if (!u.isValid() || u.scheme() != "http") {
        LOG_ERROR("Invalid feed URL %s", url.c_str());
        return false;
    }

    m_feeds.push_back(ChannelFeed(url));
    return true;
}

void ChannelDirectory::clearFeeds()
{
    CriticalSection cs(m_lock);

    m_feeds.clear();
    m_channels.clear();
    m_lastUpdate = 0;
}

void ChannelDirectory::setFeedPublic(int index, bool isPublic)
{
    CriticalSection cs(m_lock);
    if (index < m_feeds.size())
        m_feeds[index].isPublic = isPublic;
    else
        LOG_DEBUG("setFeedPublic: index %d out of range", index);
}

std::string ChannelDirectory::findTracker(GnuID id)
{
    for (ChannelEntry& entry : m_channels)
    {
        if (entry.id.isSame(id))
            return entry.tip;
    }
    return "";
}

std::string ChannelFeed::statusToString(ChannelFeed::Status s)
{
    switch (s) {
    case Status::UNKNOWN:
        return "UNKNOWN";
    case Status::OK:
        return "OK";
    case Status::ERROR:
        return "ERROR";
    }
    throw std::logic_error("should be unreachable");
}
