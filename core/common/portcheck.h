#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include <vector>
#include <stdint.h>
#include "uri.h"
#include "ip.h"

struct PortCheckResult
{
    IP ip;
    std::vector<int> ports;
};

class IPv6PortChecker
{
public:
    IPv6PortChecker(const URI& uri = URI("http://v6.api.pecastation.org/portcheck"));
    PortCheckResult run(const std::vector<int>& ports);

    URI m_uri;
};

#endif
