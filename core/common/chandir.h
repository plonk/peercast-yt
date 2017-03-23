#include <cstdlib>
#include <vector>

#include "sys.h" // WLock

class ChannelEntry
{
public:
    static std::vector<ChannelEntry> textToChannelEntries(std::string text);

    ChannelEntry(std::vector<std::string> fields)
    {
        name           = fields[0];
        id             = fields[1];
        tip            = fields[2];
        url            = fields[3];
        genre          = fields[4];
        desc           = fields[5];
        numDirects     = std::atoi(fields[6].c_str());
        numRelays      = std::atoi(fields[7].c_str());
        bitrate        = std::atoi(fields[8].c_str());
        contentTypeStr = fields[9];
        uptime         = fields[15];
    }

    std::string name;
    GnuID id;
    int bitrate;
    std::string contentTypeStr;
    std::string desc, genre, url, comment;
    std::string tip;
    std::string uptime;
    int numDirects;
    int numRelays;
    //TrackInfo track;
};

// 外部からチャンネルリストを取得して保持する。
class ChannelDirectory
{
public:
    ChannelDirectory()
        : m_lastUpdate(0)
    {
        m_feeds.push_back("http://bayonet.ddo.jp/sp/index.txt");
        m_feeds.push_back("http://temp.orz.hm/yp/index.txt");
    }

    int numChannels();
    int numFeeds();

    bool update();

    bool writeChannelVariable(Stream& out, const String& varName, int index);
    bool writeFeedVariable(Stream& out, const String& varName, int index);
    bool writeVariable(Stream &, const String &, int);

    std::vector<ChannelEntry> m_channels;
    std::vector<std::string> m_feeds;

    unsigned int m_lastUpdate;
    WLock m_lock;
};
