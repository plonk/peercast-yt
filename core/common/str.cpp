#include <string>
#include <algorithm>
#include <cstring>

namespace str
{

using namespace std;

std::string hexdump(const std::string& in)
{
    std::string res;

    for (int i = 0; i < in.size(); i++) {
        if (i != 0)
            res += ' ';
        char buf[3];
        snprintf(buf, 3, "%02hhX", in[i]);
        res += buf;
    }
    return res;
}

static std::string inspect(char c)
{
    int d = static_cast<unsigned char>(c);

    // FIXME: UTF8 を通す為にしている。
    if (d >= 0x80)
        return string() + (char) d;

    if (isprint(d)) {
        switch (d) {
        case '\'': return "\\\'";
        case '\"': return "\\\"";
        case '\?': return "\\?";
        case '\\': return "\\\\";
        default:
            return string() + (char) d;
        }
    }

    switch (d) {
    case '\a': return "\\a";
    case '\b': return "\\b";
    case '\f': return "\\f";
    case '\n': return "\\n";
    case '\r': return "\\r";
    case '\t': return "\\t";
    default:
        return string("\\x")
            + "0123456789abcdef"[d>>4]
            + "0123456789abcdef"[d&0xf];
    }
}

std::string inspect(const std::string str)
{
    std::string res = "\"";

    for (auto c : str) {
        res += inspect(c);
    }
    res += "\"";
    return res;
}

std::string repeat(const std::string& in, int n)
{
    std::string res;

    for (int i = 0; i < n; i++) {
        res += in;
    }
    return res;
}

std::string group_digits(const std::string& in, const std::string& separator)
{
    std::string tail;

    auto end = in.find('.'); // end of integral part
    if (end != string::npos)
        tail = in.substr(end);
    else
        end = in.size();

    std::string res;
    std::string integer = in.substr(0, end);
    std::reverse(integer.begin(), integer.end());
    std::string delim = separator;
    std::reverse(delim.begin(), delim.end());
    for (int i = 0; i < end; i++)
    {
        if (i != 0 && i%3 == 0)
            res += delim;
        res += integer[i];
    }
    std::reverse(res.begin(), res.end());
    return res + tail;
}

std::vector<std::string> split(const std::string& in, const std::string& separator)
{
    std::vector<std::string> res;
    const char *p = in.c_str();
    const char *sep = separator.c_str();

    const char *q;

    while (true)
    {
        q = strstr(p, sep);
        if (q)
        {
            res.push_back(std::string(p, q));
            p = q + std::strlen(sep);
        }
        else
        {
            res.push_back(std::string(p));
            return res;
        }
    }
}

} // namespace str
