#ifndef _RTMPMONIT_H
#define _RTMPMONIT_H

#include "critsec.h"
#include "subprog.h"

struct RTMPServerMonitor
{
    RTMPServerMonitor(const std::string& aPath);

    std::string status();
    bool isEnabled();

    void update();
    void enable();
    void disable();

    bool writeVariable(Stream &out, const String &var);

    void start();
    std::string makeEndpointURL();

    Subprogram m_rtmpServer;
    bool m_enabled;
    WLock m_lock;
};

#endif
