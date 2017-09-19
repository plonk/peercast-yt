#include "_string.h"
#include "stream.h"
#include "uptest.h"
#include "threading.h"
#include "defer.h"
#include "socket.h"
#include "version2.h"
#include "xml.h"
#include "sstream.h"

void UptestServiceRegistry::addURL(const std::string& url)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    m_providers.push_back(UptestEndpoint(url));
}

std::vector<std::string> UptestServiceRegistry::getURLs() const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    std::vector<std::string> res;
    for (auto provider : m_providers)
        res.push_back(provider.url);
    return res;
}

void UptestServiceRegistry::clear()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    m_providers.clear();
}

bool UptestServiceRegistry::writeVariable(Stream& s, const String& v)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (v == "numURLs")
    {
        s.writeString(std::to_string(m_providers.size()));
        return true;
    }

    return false;
}

bool UptestServiceRegistry::writeVariable(Stream& s, const String& v, int i)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (i < 0 || i >= m_providers.size())
    {
        LOG_ERROR("UptestServiceRegistry: Index out of range");
        return false;
    }

    if (v == "url")
    {
            s.writeString(m_providers[i].url);
            return true;
    }else if (v == "status")
    {
        std::string text;
        switch (m_providers[i].status)
        {
            case UptestEndpoint::kUntried:
                text = "Untried";
                break;
            case UptestEndpoint::kSuccess:
                text = "Success";
                break;
            case UptestEndpoint::kError:
                text = "Error";
                break;
            default:
                abort();
        }
        s.writeString(text);
        return true;
    }else if (v == "bitrate")
    {
        s.writeString(std::to_string(m_providers[i].bitrate));
        return true;
    }

    return false;
}

void UptestServiceRegistry::update()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    for (auto& provider : m_providers)
    {
        if (provider.isReady())
        {
            provider.update();
        }else
        {
            LOG_TRACE("%s not ready to download", provider.url.c_str());
        }
    }
}

bool UptestEndpoint::isReady()
{
    return (status == kUntried) || (sys->getTime() - lastTriedAt > kXmlTryInterval);
}

void UptestEndpoint::update()
{
    this->lastTriedAt = sys->getTime();
    LOG_DEBUG("Speedtest: %s", this->url.c_str());
    try
    {
        auto info = readInfo(download(this->url));
        this->status = kSuccess;
        this->bitrate = atoi(info.speed.c_str());
        LOG_INFO("Speedtest result: %d kbps", bitrate);
    }catch (std::exception& e)
    {
        LOG_ERROR("UptestEndpoint %s: %s", this->url.c_str(), e.what());
        this->status = kError;
        this->bitrate = 0;
    }
}

std::string UptestEndpoint::download(const std::string& url)
{
    URI uri(url);

    if (!uri.isValid())
        throw std::runtime_error("invalid URI");
    if (uri.scheme() != "http")
        throw std::runtime_error("unsupported protocol");

    Host host;
    host.fromStrName(uri.host().c_str(), uri.port());

    if (host.ip == 0)
        throw std::runtime_error("could not resolve host name");

    ClientSocket* rsock = sys->createSocket();
    Defer cb([=] { delete rsock; });
    rsock->open(host);
    rsock->connect();

    HTTP http(*rsock);
    HTTPRequest req("GET", uri.path(), "HTTP/1.0",
                    {
                        { "Host", uri.host() },
                        { "Connection", "close" },
                        { "User-Agent", PCX_AGENT }
                    });
    HTTPResponse res = http.send(req);

    if (res.statusCode != 200)
        throw std::runtime_error(str::format("unexpected status code %d", res.statusCode));

    return res.body;
}

static const char* yesNo(const std::string& number)
{
    return (number == "0") ? "No" : "Yes";
}

#define nonNull(p)                                                      \
    (((p) == nullptr) ?                                                 \
     throw std::runtime_error(                                          \
         str::format("non-null assertion failed on line %d in file %s", \
                     __LINE__, __FILE__))                               \
     : (p))

uptestInfo UptestEndpoint::readInfo(const std::string& body)
{
    uptestInfo info;
    XML xml;
    StringStream mem(body);

    xml.read(mem);

    XML::Node* yp = nonNull(xml.findNode("yp"));
    info.name      = nonNull(yp->findAttr("name"));

    XML::Node* host = nonNull(xml.findNode("host"));
    info.ip        = nonNull(host->findAttr("ip"));
    info.port_open = nonNull(host->findAttr("port_open"));
    info.speed     = nonNull(host->findAttr("speed"));
    info.over      = nonNull(host->findAttr("over"));

    XML::Node* uptest = nonNull(xml.findNode("uptest"));
    info.checkable = nonNull(uptest->findAttr("checkable"));
    info.remain    = nonNull(uptest->findAttr("remain"));

    XML::Node* uptest_srv = nonNull(xml.findNode("uptest_srv"));
    info.addr      = nonNull(uptest_srv->findAttr("addr"));
    info.port      = nonNull(uptest_srv->findAttr("port"));
    info.object    = nonNull(uptest_srv->findAttr("object"));
    info.post_size = nonNull(uptest_srv->findAttr("post_size"));
    info.limit     = nonNull(uptest_srv->findAttr("limit"));
    info.interval  = nonNull(uptest_srv->findAttr("interval"));
    info.enabled   = nonNull(uptest_srv->findAttr("enabled"));

    return info;
}

static std::string generateRandomBytes(size_t size)
{
    std::string buf;
    peercast::Random r;

    for (size_t i = 0; i < size; ++i)
        buf += (char) (r.next() & 0xff);
    return buf;
}

HTTPResponse UptestEndpoint::takeSpeedTest(URI uri, size_t size)
{
    Host host;
    host.fromStrName(uri.host().c_str(), uri.port());

    if (host.ip == 0)
        throw std::runtime_error("Could not resolve host name");

    ClientSocket* rsock = sys->createSocket();
    Defer cb([=] { delete rsock; });
    rsock->open(host);
    rsock->connect();

    HTTP http(*rsock);
    HTTPRequest req("POST", uri.path(), "HTTP/1.0",
                    {
                        { "Host", uri.host() }, // this may not be necessary
                        { "Connection", "close" },
                        { "User-Agent", PCX_AGENT },
                        { "Content-Length", std::to_string(size) },
                        { "Content-Type", "application/octet-stream" }
                    });
    req.body = generateRandomBytes(size);
    HTTPResponse res = http.send(req);

    if (res.statusCode != 302)
        throw std::runtime_error(str::format("unexpected status code %d", res.statusCode));

    return res;
}
