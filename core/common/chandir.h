#ifndef _CHANDIR_H
#define _CHANDIR_H

#include <cstdlib>
#include <vector>
#include <stdexcept> // runtime_error

#include "cgi.h"
#include "sys.h" // WLock

class ChannelEntry
{
public:
    static std::vector<ChannelEntry> textToChannelEntries(const std::string& text, const std::string& aFeedUrl);

    ChannelEntry(const std::vector<std::string>& fields, const std::string& aFeedUrl)
        : feedUrl(aFeedUrl)
    {
        if (fields.size() < 19)
            throw std::runtime_error("too few fields");

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
        encodedName    = fields[14];
        uptime         = fields[15];
    }

    std::string chatUrl();
    std::string statsUrl();

    std::string name;

    // URLエンコードされたチャンネル名、このフィールドがあればチャット
    // や統計のページがある。
    std::string encodedName;

    GnuID id;
    int bitrate;
    std::string contentTypeStr;
    std::string desc, genre, url;
    std::string tip;
    std::string uptime;
    int numDirects;
    int numRelays;
    //TrackInfo track;

    std::string feedUrl;
};

class ChannelFeed
{
public:
    enum class Status {
        UNKNOWN,
        OK,
        ERROR,
    };

    ChannelFeed()
        : url(""),
          status(Status::UNKNOWN)
    {
    }

    ChannelFeed(std::string aUrl)
        : url(aUrl),
          status(Status::UNKNOWN)
    {
    }

    static std::string statusToString(Status);

    std::string url;
    Status status;
};

// 外部からチャンネルリストを取得して保持する。
class ChannelDirectory
{
public:
    ChannelDirectory();

    int numChannels();
    int numFeeds();
    std::vector<ChannelFeed> feeds();
    bool addFeed(std::string url);
    void clearFeeds();

    int totalListeners();
    int totalRelays();

    bool update();

    bool writeChannelVariable(Stream& out, const String& varName, int index);
    bool writeFeedVariable(Stream& out, const String& varName, int index);
    bool writeVariable(Stream& out, const String& varName);
    bool writeVariable(Stream &, const String &, int);

    std::string findTracker(GnuID id);

    std::vector<ChannelEntry> m_channels;
    std::vector<ChannelFeed> m_feeds;

    unsigned int m_lastUpdate;
    WLock m_lock;
};

#endif
