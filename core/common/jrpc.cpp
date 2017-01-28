#include <iostream>
#include <string>

#include "jrpc.h"

using namespace std;
using json = nlohmann::json;

static json error_object(int code, const char* message, json id = nullptr)
{
    return {
        { "jsonrpc", "2.0" },
        { "error", { { "code", code }, { "message", message } } },
        { "id", id }
    };
}

json JrpcApi::call_internal(const string& input)
{
    json j, id, method, params, result;

    try {
        j = json::parse(input);
    } catch (std::invalid_argument&) {
        return error_object(-32700, "Parse error");
    }

    if (!j.is_object() ||
        j.find("jsonrpc") == j.end() ||
        j.at("jsonrpc") != "2.0")
    {
        return error_object(-32600, "Invalid Request");
    }

    try {
        id = j.at("id");
    } catch (std::range_error&) {
        return error_object(-32600, "Invalid Request");
    }

    try {
        method = j.at("method");
    } catch (std::range_error&) {
        return error_object(-32600, "Invalid Request", id);
    }

    if (j.find("params") == j.end() ||
        !j.at("params").is_object())
    {
        params = json({});
    } else {
        params = j.at("params");
    }

    try {
        result = dispatch(method, params);
    } catch (method_not_found&) {
        return error_object(-32601, "Method not found", id);
    } catch (invalid_params&) {
        return error_object(-32602, "Invalid params", id);
    }

    return {
        { "jsonrpc", "2.0" },
        { "result", result },
        { "id", id }
    };
}

json PeercastJrpcApi::dispatch(const json& m, const json& p)
{
    try {
        if (m == "getVersionInfo")
            return getVersionInfo();
    } catch (std::out_of_range& e) {
        throw invalid_params(e.what());
    }
    throw method_not_found(m.get<string>());
}
