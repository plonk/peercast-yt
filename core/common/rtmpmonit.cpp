#include "rtmpmonit.h"
#include "servmgr.h"

RTMPServerMonitor::RTMPServerMonitor(const std::string& aPath)
    : m_rtmpServer(aPath, false, false)
    , ipVersion(4)
    , m_enabled(false)
{
}

std::string RTMPServerMonitor::status()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    if (m_enabled)
        return "UP";
    else
        return "DOWN";
}

bool RTMPServerMonitor::isEnabled()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return m_enabled;
}

void RTMPServerMonitor::update()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);

    if (!m_enabled) return;

    // RTMP server の死活を監視する。
    if (!m_rtmpServer.isAlive())
    {
        LOG_ERROR("RTMP server is down! Restarting... ");
        start();
    }
}

void RTMPServerMonitor::enable()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    m_enabled = true;
}

void RTMPServerMonitor::disable()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    m_enabled = false;
    if (m_rtmpServer.isAlive())
        m_rtmpServer.terminate();
}

amf0::Value RTMPServerMonitor::getState()
{
    std::lock_guard<std::recursive_mutex> cs(m_lock);
    return amf0::Value(
        {
            {"status", status()},
            {"processID", std::to_string( m_rtmpServer.pid() )},
            {"ipVersion", std::to_string(ipVersion)},
        });
}

void RTMPServerMonitor::start()
{
    // Environment env を現在の環境から初期化する。
    Environment env;
    env.copyFromCurrentProcess();

    uint16_t port;
    {
        std::lock_guard<std::recursive_mutex> cs(servMgr->lock);
        port = servMgr->rtmpPort;
    }

    m_rtmpServer.start({"-p", std::to_string(port), makeEndpointURL()}, env);
}

std::string RTMPServerMonitor::makeEndpointURL()
{
    std::lock_guard<std::recursive_mutex> cs(servMgr->lock);

    ChanInfo info = servMgr->defaultChannelInfo;
    cgi::Query query;
    query.add("name", info.name.c_str());
    query.add("desc", info.desc.c_str());
    query.add("genre", info.genre.c_str());
    query.add("url", info.url.c_str());
    query.add("comment", info.comment.c_str());
    query.add("type", "FLV");
    query.add("ipv", std::to_string(ipVersion));

    return "http://localhost:" + std::to_string(servMgr->serverHost.port) + "/?" + query.str();
}
