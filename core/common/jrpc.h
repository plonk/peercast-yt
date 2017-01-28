#ifndef _JRPC_H
#define _JRPC_H

#include <string>
#include "version2.h"
#include "json.hpp"

class JrpcApi
{
public:
    using json = nlohmann::json;

    class method_not_found : public std::runtime_error
    {
    public:
        method_not_found(const std::string& msg) : runtime_error(msg) {}
    };
    class invalid_params : public std::runtime_error
    {
    public:
        invalid_params(const std::string& msg) : runtime_error(msg) {}
    };

    std::string call(const std::string& request)
    {
        return call_internal(request).dump(4);
    }

    virtual json dispatch(const json&, const json&) = 0;

private:
    json call_internal(const std::string&);
};

class PeercastJrpcApi : public JrpcApi
{
public:
    json dispatch(const json&, const json&) override;

    json getVersionInfo()
    {
        return {
            { "agentName", PCX_AGENT },
            { "apiVesion", "1.0.0" },
            { "jsonrpc", "2.0" }
        };
    }
};

#endif
