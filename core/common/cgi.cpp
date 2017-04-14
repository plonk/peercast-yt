#include <stdio.h>

#include "cgi.h"
#include "str.h"

namespace cgi {

// URLエスケープする。
std::string escape(const std::string& in)
{
    std::string res;

    for (unsigned char c : in) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == '_') || (c == '-') || (c == '.')) {
            res += c;
        } else if (c == ' ') {
            res += '+';
        } else {
            char buf[4];
            snprintf(buf, 4, "%%%02X", c);
            res += buf;
        }
    }
    return res;
}

std::string unescape(const std::string& in)
{
    std::string res;
    size_t i = 0;

    while (i < in.size()) {
        if (in[i] == '%') {
            char c;
            sscanf(in.substr(i + 1, 2).c_str(), "%hhx", &c);
            res += c;
            i += 3;
        } else if (in[i] == '+') {
            res += ' ';
            i += 1;
        } else {
            res += in[i];
            i += 1;
        }
    }
    return res;
}

Query::Query(const std::string& queryString)
{
    auto assignments = str::split(queryString, "&");
    for (auto& assignment : assignments)
    {
        auto sides = str::split(assignment, "=");
        if (sides.size() == 1)
            m_dict[sides[0]] = {};
        else
        {
            m_dict[sides[0]].push_back(unescape(sides[1]));
        }
    }
}

bool Query::hasKey(const std::string& key)
{
    return m_dict.find(key) != m_dict.end();
}

std::string Query::get(const std::string& key)
{
    try
    {
        if (m_dict.at(key).size() == 0)
            return "";
        else
            return m_dict.at(key)[0];
    } catch (std::out_of_range&)
    {
        return "";
    }
}

std::vector<std::string> Query::getAll(const std::string& key)
{
    try
    {
        return m_dict.at(key);
    } catch (std::out_of_range&)
    {
        return {};
    }
}

} // namespace cgi
