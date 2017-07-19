#include <iostream>
#include <string>

#include "jrpc.h"

using namespace std;
using json = nlohmann::json;

static json error_object(int code, const char* message, json id = nullptr, json data = nullptr)
{
    json j = {
        { "jsonrpc", "2.0" },
        { "error", { { "code", code }, { "message", message } } },
        { "id", id }
    };

    if (!data.is_null())
        j["error"]["data"] = data;

    return j;
}

json JrpcApi::call_internal(const string& input)
{
    json j, id, method, params, result;

    LOG_DEBUG("jrpc request: %s", input.c_str());

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
    } catch (std::out_of_range) {
        return error_object(-32600, "Invalid Request");
    }

    try {
        method = j.at("method");
    } catch (std::range_error&) {
        return error_object(-32600, "Invalid Request", id);
    } catch (std::out_of_range) {
        return error_object(-32600, "Invalid Request", id));
    }

    if (j.find("params") == j.end())
    {
        params = json::array();
    } else if (!j.at("params").is_object() &&
               !j.at("params").is_array())
    {
        return error_object(-32602, "Invalid params", id, "params must be either object or array");
    } else {
        params = j.at("params");
    }

    try {
        result = dispatch(method, params);
    } catch (method_not_found& e) {
        LOG_DEBUG("Method not found: %s", e.what());
        return error_object(-32601, "Method not found", id, e.what());
    } catch (invalid_params& e) {
        return error_object(-32602, "Invalid params", id, e.what());
    } catch (application_error& e) {
        return error_object(e.m_errno, e.what(), id);
    }

    return {
        { "jsonrpc", "2.0" },
        { "result", result },
        { "id", id }
    };
}
