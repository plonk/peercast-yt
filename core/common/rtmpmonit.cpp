#include "rtmpmonit.h"
#include "servmgr.h"

RTMPServerMonitor::RTMPServerMonitor(const std::string& aPath)
    : m_rtmpServer(aPath, false, false)
    , m_enabled(false)
{
}

std::string RTMPServerMonitor::status()
{
    CriticalSection cs(m_lock);
    if (m_enabled)
        return "UP";
    else
        return "DOWN";
}

bool RTMPServerMonitor::isEnabled()
{
    CriticalSection cs(m_lock);
    return m_enabled;
}

void RTMPServerMonitor::update()
{
    CriticalSection cs(m_lock);

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
    CriticalSection cs(m_lock);
    m_enabled = true;
}

void RTMPServerMonitor::disable()
{
    CriticalSection cs(m_lock);
    m_enabled = false;
    if (m_rtmpServer.isAlive())
        m_rtmpServer.terminate();
}

bool RTMPServerMonitor::writeVariable(Stream &out, const String &var)
{
    std::string buf;

    if (var == "status")
        buf = status();
    else if (var == "processID")
        buf = std::to_string( m_rtmpServer.pid() );
    else
        return false;

    out.writeString(buf);
    return true;
}

void RTMPServerMonitor::start()
{
    // Environment env を現在の環境から初期化する。
    Environment env;
    env.copyFromCurrentProcess();

    m_rtmpServer.start({makeEndpointURL()}, env);
}

std::string RTMPServerMonitor::makeEndpointURL()
{
    CriticalSection cs(servMgr->lock);

    ChanInfo info = servMgr->defaultChannelInfo;
    cgi::Query query;
    query.add("name", info.name.c_str());
    query.add("desc", info.desc.c_str());
    query.add("genre", info.genre.c_str());
    query.add("url", info.url.c_str());
    query.add("comment", info.comment.c_str());
    query.add("type", "FLV");

    return "http://localhost:" + std::to_string(servMgr->serverHost.port) + "/?" + query.str();
}
