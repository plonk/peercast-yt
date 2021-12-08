#ifndef _RTMPMONIT_H
#define _RTMPMONIT_H

#include "subprog.h"
#include "varwriter.h"

class RTMPServerMonitor : public VariableWriter
{
public:
    RTMPServerMonitor(const std::string& aPath);

    std::string status();
    bool isEnabled();

    void update();
    void enable();
    void disable();

    amf0::Value getState() override;

    void start();
    std::string makeEndpointURL();

    Subprogram m_rtmpServer;
    int ipVersion;
    bool m_enabled;

    std::recursive_mutex m_lock;
};

#endif
