#include <stdio.h>

#include "cgi.h"

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

} // namespace cgi
