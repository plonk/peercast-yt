#include "rtmpmonit.h"
#include "servmgr.h"

std::string RTMPServerMonitor::makeEndpointURL()
{
        return "http://localhost:" + std::to_string(servMgr->serverHost.port) + "/?name=HOGE&type=FLV";
}
