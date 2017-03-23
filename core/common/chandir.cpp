#include <sstream>
#include <memory> // unique_ptr
#include <boost/network/protocol/http/client.hpp>

#include "http.h"
#include "version2.h"
#include "socket.h"
#include "chandir.h"

using namespace std;

// クリティカルセクションをマークする。コンストラクターでロックを取得
// し、デストラクタで開放する。
class CriticalSection
{
public:
    CriticalSection(WLock& lock)
        : m_lock(lock)
    {
        m_lock.on();
    }

    ~CriticalSection()
    {
        m_lock.off();
    }

    WLock& m_lock;
};

std::vector<ChannelEntry> ChannelEntry::textToChannelEntries(std::string text)
{
    istringstream in(text);
    string line;
    vector<ChannelEntry> res;

    while (getline(in, line)) {
        vector<string> fields;
        const char *p = line.c_str();
        const char *q;

        while ((q = strstr(p, "<>")) != nullptr) {
            fields.push_back(string(p, q));
            p = q + 2; // <>をスキップ
        }
        fields.push_back(p);

        res.push_back(ChannelEntry(fields));
    }

    return res;
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

static bool getFeed(std::string url, std::vector<ChannelEntry>& out)
{
    using namespace boost::network;

    uri::uri feed(url);
    if (!feed.is_valid())
        return false;

    if (feed.scheme() != "http")
        return false;

    int port;
    if (feed.port() == "")
        port = 80;
    else
        port = atoi(feed.port().c_str());

    Host host;
    host.fromStrName(feed.host().c_str(), port);

    unique_ptr<ClientSocket> rsock (sys->createSocket());

    try {
        LOG_DEBUG("Connecting to %s ...", feed.host().c_str());
        rsock->open(host);
        rsock->connect();

        HTTP rhttp(*rsock);

        LOG_DEBUG("GET %s HTTP/1.0", feed.path().c_str());

        rhttp.writeLineF("GET %s HTTP/1.0", feed.path().c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_HOST, feed.host().c_str());
        rhttp.writeLineF("%s %s", HTTP_HS_CONNECTION, "close");
        rhttp.writeLineF("%s %s", HTTP_HS_AGENT, PCX_AGENT);
        rhttp.writeLine("");

        auto code = rhttp.readResponse();
        if (code != 200)
            return false;

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
        }

        out = ChannelEntry::textToChannelEntries(text);
        return true;
    } catch (SockException& e) {
        LOG_ERROR("ChannelDirectory::update: %s", e.msg);
        return false;
    }
}

bool ChannelDirectory::update()
{
    using namespace boost::network;

    CriticalSection cs(m_lock);

    if (sys->getTime() - m_lastUpdate < 5 * 60)
        return false;

    m_channels.clear();
    for (auto url : m_feeds) {
        std::vector<ChannelEntry> channels;
        bool success = getFeed(url, channels);
        LOG_DEBUG("success = %d", success);
        LOG_DEBUG("%lu channels", channels.size());


        if (success) {
            for (auto ch : channels) {
                m_channels.push_back(ch);
            }
        }
    }
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
    } else if (varName == "comment") {
        sprintf(buf, "%s", ch.comment.c_str());
    } else if (varName == "tip") {
        sprintf(buf, "%s", ch.tip.c_str());
    } else if (varName == "uptime") {
        sprintf(buf, "%s", ch.uptime.c_str());
    } else if (varName == "numDirects") {
        sprintf(buf, "%d", ch.numDirects);
    } else if (varName == "numRelays") {
        sprintf(buf, "%d", ch.numRelays);
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
        value = m_feeds[index];
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
