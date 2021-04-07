#include "json.hpp"
#include "http.h"
#include <memory>
#include "portcheck.h"
#include "servmgr.h"
#include "str.h"
#include "version2.h"

IPv6PortChecker::IPv6PortChecker(const URI& uri)
    : m_uri(uri)
{
}

PortCheckResult IPv6PortChecker::run(const std::vector<int>& ports)
{
    using json = nlohmann::json;
    std::shared_ptr<ClientSocket> sock(sys->createSocket());

    if (!sock)
        throw StreamException("Unable to create socket");

    auto ips = sys->getIPAddresses(m_uri.host());
    IP ip;
    for (auto& str : ips) {
        ip = IP::parse(str);
        if (!ip.isIPv4Mapped()) // IPv6 アドレスを見つけた。
            break;
    }
    if (!ip || ip.isIPv4Mapped()) {
        throw GeneralException(("No IPv6 address associated with " + m_uri.host()).c_str());
    }        

    sock->open(Host(ip, m_uri.port()));
    sock->connect();

    HTTP http(*sock);

    std::string reqbody = json({{"instanceId",servMgr->sessionID.str()}, {"ports",json::array({servMgr->serverHostIPv6.port})}}).dump();
    HTTPRequest req("POST", "/portcheck", "HTTP/1.1",
                    {{"Host", "v6.api.pecastation.org"},
                     {"Content-Type", "application/json"},
                     {"User-Agent", PCX_AGENT},
                     {"Content-Length", std::to_string(reqbody.size())}});

    req.body = reqbody;
    LOG_DEBUG("req: %s", req.body.c_str());
    auto res = http.send(req);
    if (res.statusCode != 200) {
        throw GeneralException(str::STR("HTTP request failed: ", res.statusCode).c_str(), res.statusCode);
    }

    auto data = json::parse(res.body);
    LOG_DEBUG("res: %s", res.body.c_str());
    PortCheckResult result;
    result.ip = IP::parse(data["ip"]);
    for (json& j : data["ports"]) {
        result.ports.push_back(j.get<int>());
    }

    return result;
}
