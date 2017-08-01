#ifndef _RTMPMONIT_H
#define _RTMPMONIT_H

#include "critsec.h"
#include "subprog.h"

struct RTMPServerMonitor
{
    std::string status() // RUNNING etc.
    {
        CriticalSection cs(m_lock);
        return "RUNNING";
    }

    void update()
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

    bool isEnabled()
    {
        CriticalSection cs(m_lock);
        return m_enabled;
    }

    void enable()
    {
        CriticalSection cs(m_lock);
        m_enabled = true;
    }

    void disable()
    {
        CriticalSection cs(m_lock);
        m_enabled = false;
        if (m_rtmpServer.isAlive())
            m_rtmpServer.terminate();
    }

    RTMPServerMonitor(const std::string& aPath)
        : m_rtmpServer(aPath, false, false)
        , m_enabled(false)
    {
    }

    Subprogram m_rtmpServer;
    bool m_enabled;
    WLock m_lock;

    std::string makeEndpointURL();

    void start()
    {
        // Environment env を現在の環境から初期化する。
        Environment env;
        env.copyFromCurrentProcess();

        m_rtmpServer.start({makeEndpointURL()}, env);
    }
};

#endif
