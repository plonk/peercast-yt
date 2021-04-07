#include "json.hpp"
#include "http.h"
#include <memory>
#include "portcheck.h"
#include "servmgr.h"

IPv6PortChecker::IPv6PortChecker(const URI& uri)
    : m_uri(uri)
{
}

PortCheckResult IPv6PortChecker::run(const std::vector<int>& ports)
{
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

    HTTPRequest req("POST", "/portcheck", "HTTP/1.1",
                    {{"Host","v6.api.pecastation.org"}, {"Content-Type","application/json"}});

    req.body = str::STR("{instanceId:\"", servMgr->sessionID.str(), "\",ports:[", servMgr->serverHostIPv6.port, "]}");
    auto res = http.send(req);
    if (res.statusCode != 200) {
        throw GeneralException("HTTP request failed", res.statusCode);
    }

    auto data = nlohmann::json::parse(res.body);
    PortCheckResult result;
    result.ip = IP::parse(data["ip"]);
    for (nlohmann::json& j : data["ports"]) {
        result.ports.push_back(j.get<int>());
    }

    return result;
}
