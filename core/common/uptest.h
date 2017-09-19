#ifndef _UPTEST_H
#define _UPTEST_H

#include "varwriter.h"
#include <vector>
#include "threading.h"
#include "http.h"
#include "uri.h"

struct uptestInfo
{
    std::string name;
    std::string ip;
    std::string port_open;
    std::string speed;
    std::string over;
    std::string checkable;
    std::string remain;
    std::string addr;
    std::string port;
    std::string object;
    std::string post_size;
    std::string limit;
    std::string interval;
    std::string enabled;
};

class UptestEndpoint
{
public:
    enum Status
    {
        kUntried,
        kSuccess,
        kError,
    };
    static const int kXmlTryInterval = 60; // in seconds

    UptestEndpoint(const std::string& aUrl)
        : url(aUrl)
        , status(kUntried)
        , bitrate(0) {}

    void update();
    std::string download(const std::string& url);
    HTTPResponse takeSpeedTest(URI uri, size_t size);
    uptestInfo readInfo(const std::string& body);
    bool isReady();

    std::string url;
    Status status;
    int bitrate; // in kbps
    unsigned int lastTriedAt;
};

class UptestServiceRegistry : public VariableWriter
{
public:
    std::pair<bool,std::string> addURL(const std::string&);
    std::vector<std::string> getURLs() const;
    std::pair<bool,std::string> deleteByIndex(int index);
    void clear();

    bool writeVariable(Stream&, const String&) override;
    bool writeVariable(Stream&, const String&, int) override;

    void update();

    mutable std::recursive_mutex m_lock;
    std::vector<UptestEndpoint> m_providers;
};

#endif
