#include "_string.h"
#include "stream.h"
#include "uptest.h"
#include "threading.h"
#include "defer.h"
#include "socket.h"
#include "version2.h"
#include "xml.h"
#include "sstream.h"
#include <algorithm>

// -------- class UptestServiceRegistry --------

std::pair<bool,std::string> UptestServiceRegistry::addURL(const std::string& url)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    URI uri(url);
    if (!uri.isValid())
        return std::make_pair(false, "invalid URL");
    if (uri.scheme() != "http")
        return std::make_pair(false, "unsupported protocol");
    if (0 < std::count_if(m_providers.begin(), m_providers.end(), [url](UptestEndpoint& e) { return e.url == url; }))
        return std::make_pair(false, "URL already exists");

    m_providers.push_back(UptestEndpoint(url));
    return std::make_pair(true, "");
}

bool UptestServiceRegistry::isIndexValid(int index) const
{
    return index >= 0 && index < m_providers.size();
}

std::pair<bool,std::string> UptestServiceRegistry::deleteByIndex(int index)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (!isIndexValid(index))
        return std::make_pair(false, "index out of range");

    m_providers.erase(m_providers.begin() + index);
    return std::make_pair(true, "");
}

std::pair<bool,std::string> UptestServiceRegistry::takeSpeedtest(int index)
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (!isIndexValid(index))
        return std::make_pair(false, "index out of range");

    return m_providers[index].takeSpeedtest();
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
        LOG_TRACE("UptestServiceRegistry: Index out of range");
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
    }else if (v == "speed")
    {
        s.writeString(m_providers[i].m_info.speed);
        return true;
    }else if (v == "over")
    {
        s.writeString(m_providers[i].m_info.over);
        return true;
    }else if (v == "checkable")
    {
        s.writeString(m_providers[i].m_info.checkable);
        return true;
    }

    return false;
}

void UptestServiceRegistry::update()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    for (auto& provider : m_providers)
    {
        if (provider.status != UptestEndpoint::kSuccess)
        {
            if (provider.isReady())
                provider.update();
            else
                LOG_TRACE("%s not ready to download", provider.url.c_str());
        }
    }
}

void UptestServiceRegistry::forceUpdate()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    for (auto& provider : m_providers)
        provider.update();
}

std::pair<bool,std::string> UptestServiceRegistry::getXML(int index, std::string& out) const
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (!isIndexValid(index))
        return std::make_pair(false, "index out of range");

    if (m_providers[index].status != UptestEndpoint::kSuccess)
        return std::make_pair(false, "no XML");

    out = m_providers[index].m_xml;
    return std::make_pair(true, "");
}

// -------- class UptestEndpoint --------

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
        m_xml = download(this->url);
        m_info = readInfo(m_xml);
        this->status = kSuccess;
        LOG_INFO("Speedtest result: %s kbps", m_info.speed.c_str());
    }catch (std::exception& e)
    {
        LOG_ERROR("UptestEndpoint %s: %s", this->url.c_str(), e.what());
        this->status = kError;
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

    if (!host.ip)
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

#define nonNull(p)                                                      \
    (((p) == nullptr) ?                                                 \
     throw std::runtime_error(                                          \
         str::format("non-null assertion failed on line %d in file %s", \
                     __LINE__, __FILE__))                               \
     : (p))

UptestInfo UptestEndpoint::readInfo(const std::string& body)
{
    UptestInfo info;
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

std::string UptestEndpoint::generateRandomBytes(size_t size)
{
    std::string buf;
    peercast::Random r;

    for (size_t i = 0; i < size; ++i)
        buf += (char) (r.next() & 0xff);
    return buf;
}

HTTPResponse UptestEndpoint::postRandomData(URI uri, size_t size)
{
    Host host;
    host.fromStrName(uri.host().c_str(), uri.port());

    if (!host.ip)
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
    return http.send(req);
}

std::pair<bool,std::string> UptestEndpoint::takeSpeedtest()
{
    // yp4g.xml をダウンロードして状態を得る。
    try
    {
        m_xml = download(this->url);
        m_info = readInfo(m_xml);
        LOG_DEBUG("Speedtest %s: checkable = %s", url.c_str(), m_info.checkable.c_str());
    }catch (std::exception& e)
    {
        LOG_ERROR("Speedtest %s: %s", url.c_str(), e.what());
        return std::make_pair(false, e.what());
    }

    if (m_info.checkable != "1")
    {
        LOG_ERROR("speedtest server unavailable: checkable != 1");
        return std::make_pair(false, "speedtest server unavailable");
    }else
    {
        // ランダムデータを POST する。
        const auto uptest_cgi = m_info.postURL();
        try
        {
            auto size = atoi(m_info.post_size.c_str()) * 1000;
            LOG_DEBUG("Posting %d bytes of random data to %s ...", (int) size, uptest_cgi.c_str());
            auto res = postRandomData(uptest_cgi, size);
            LOG_TRACE("... done");
            if (res.statusCode == 302)
                return std::make_pair(true, "");
            else
                return std::make_pair(false, str::format("unexpected status code %d", res.statusCode));
        }catch (std::exception& e)
        {
            LOG_ERROR("exception occurred while posting: %s", e.what());
            return std::make_pair(false, "exception occurred while posting");
        }
    }
}

// -------- class UptestInfo --------

std::string UptestInfo::postURL()
{
    return str::format("http://%s:%s%s", addr.c_str(), port.c_str(), object.c_str());
}
