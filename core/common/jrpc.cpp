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

json JrpcApi::getLog(json::array_t args)
{
    json from = args[0];
    json maxLines = args[1];

    if (!from.is_null() && from.get<int>() < 0)     throw invalid_params("from must be non negative");
    if (!from.is_null() && maxLines.get<int>() < 0) throw invalid_params("maxLines must be non negative");

    std::vector<string> lines =
        sys->logBuf->toLines([](unsigned int time, LogBuffer::TYPE type, const char* line)
                             {
                                 std::string buf;

                                 if (type != LogBuffer::T_NONE)
                                 {
                                     buf += str::rstrip(String().setFromTime(time));
                                     buf += " [";
                                     buf += LogBuffer::getTypeStr(type);
                                     buf += "] ";
                                 }

                                 buf += line;

                                 return buf;
                             });
    lines.push_back("");

    if (from == nullptr)
        from = 0;
    lines.erase(lines.begin(), lines.begin() + (std::min)(from.get<size_t>(), lines.size()));

    if (maxLines == nullptr)
        maxLines = lines.size();
    while (lines.size() > maxLines)
        lines.pop_back();

    auto log = str::join("\n", lines);

    return { { "from", from.get<int>() }, { "lines", lines.size() }, { "log", log} };
}

json JrpcApi::clearLog(json::array_t args)
{
    sys->logBuf->clear();
    return nullptr;
}

json JrpcApi::getLogSettings(json::array_t args)
{
    // YT -> PeerCastStation
    // 7 OFF   -> 0 OFF
    // 6 FATAL -> 1 FATAL
    // 5 ERROR -> 2 ERROR
    // 4 WARN  -> 3 WARN
    // 3 INFO  -> 4 INFO
    // 2 DEBUG -> 5 DEBUG
    // 1 TRACE -> 5 DEBUG

    return { { "level", std::min(5, 7 - servMgr->logLevel()) } };
}

json JrpcApi::setLogSettings(json::array_t args)
{
    int level = args[0]["level"];

    if (0 <= level && level <= 5)
        servMgr->logLevel(7 - level);

    return nullptr;
}
