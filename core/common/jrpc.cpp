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
        return error_object(kParseError, "Parse error");
    }

    if (!j.is_object() ||
        j.count("jsonrpc") == 0 ||
        j.at("jsonrpc") != "2.0")
    {
        return error_object(kInvalidRequest, "Invalid Request");
    }

    try {
        id = j.at("id");
    } catch (std::out_of_range) {
        return error_object(kInvalidRequest, "Invalid Request");
    }

    try {
        method = j.at("method");
    } catch (std::out_of_range) {
        return error_object(kInvalidRequest, "Invalid Request", id);
    }

    if (j.count("params") == 0)
    {
        params = json::array();
    } else if (!j.at("params").is_object() &&
               !j.at("params").is_array())
    {
        return error_object(kInvalidParams, "Invalid params", id, "params must be either object or array");
    } else {
        params = j.at("params");
    }

    try {
        result = dispatch(method, params);
    } catch (method_not_found& e)
    {
        LOG_DEBUG("Method not found: %s", e.what());
        return error_object(kMethodNotFound, "Method not found", id, e.what());
    } catch (invalid_params& e)
    {
        return error_object(kInvalidParams, "Invalid params", id, e.what());
    } catch (application_error& e)
    {
        return error_object(e.m_errno, e.what(), id);
    } catch (std::exception& e)
    {
        // Unexpected errors are handled here.
        return error_object(kInternalError, e.what(), id);
    }

    return {
        { "jsonrpc", "2.0" },
        { "result", result },
        { "id", id }
    };
}
