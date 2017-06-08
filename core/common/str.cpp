#include "str.h"
#include <string>
#include <algorithm>
#include <cstring>
#include <vector>
#include <cctype>

namespace str
{

using namespace std;

std::string hexdump(const std::string& in)
{
    std::string res;

    for (size_t i = 0; i < in.size(); i++) {
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
    for (size_t i = 0; i < end; i++)
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

std::string codepoint_to_utf8(uint32_t codepoint)
{
    std::string res;
    if (/* codepoint >= 0 && */ codepoint <= 0x7f) {
        res += (char) codepoint;
    } else if (codepoint >= 0x80 && codepoint <= 0x7ff) {
        res += (char) (0xb0 | (codepoint >> 6));
        res += (char) (0x80 | (codepoint & 0x3f));
    } else if (codepoint >= 0x800 && codepoint <= 0xffff) {
        res += (char) (0xe0 | (codepoint >> 12));
        res += (char) (0x80 | ((codepoint >> 6) & 0x3f));
        res += (char) (0x80 | (codepoint & 0x3f));
    } else if (codepoint >= 0x10000 && codepoint <= 0x1fffff) {
        res += (char) (0xf0 | (codepoint >> 18));
        res += (char) (0x80 | ((codepoint >> 12) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 6) & 0x3f));
        res += (char) (0x80 | (codepoint & 0x3f));
    } else if (codepoint >= 0x200000 && codepoint <= 0x3ffffff) {
        res += (char) (0xf8 | (codepoint >> 24));
        res += (char) (0x80 | ((codepoint >> 18) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 12) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 6) & 0x3f));
        res += (char) (0x80 | (codepoint & 0x3f));
    } else { // [0x4000000, 0x7fffffff]
        res += (char) (0xfb | (codepoint >> 30));
        res += (char) (0x80 | ((codepoint >> 24) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 18) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 12) & 0x3f));
        res += (char) (0x80 | ((codepoint >> 6) & 0x3f));
        res += (char) (0x80 | (codepoint & 0x3f));
    }
    return res;
}

#include <stdarg.h>
std::string format(const char* fmt, ...)
{
    va_list ap, aq;
    std::string res;

    va_start(ap, fmt);
    va_copy(aq, ap);
    int size = vsnprintf(NULL, 0, fmt, ap);
    char *data = new char[size + 1];
    vsnprintf(data, size + 1, fmt, aq);
    va_end(aq);
    va_end(ap);

    res = data;
    delete[] data;
    return res;
}

bool contains(const std::string& haystack, const std::string& needle)
{
    return haystack.find(needle) != std::string::npos;
}

std::string replace_prefix(const std::string& s, const std::string& prefix, const std::string& replacement)
{
    if (s.size() < prefix.size()) return s;

    if (s.substr(0, prefix.size()) == prefix)
        return replacement + s.substr(prefix.size());
    else
        return s;
}

std::string replace_suffix(const std::string& s, const std::string& suffix, const std::string& replacement)
{
    if (s.size() < suffix.size()) return s;

    if (s.substr(s.size() - suffix.size(), suffix.size()) == suffix)
        return s.substr(0, s.size() - suffix.size()) + replacement;
    else
        return s;
}

std::string upcase(const std::string& input)
{
    std::string res;
    for (auto c : input)
    {
        if (isalpha(c))
            res += toupper(c);
        else
            res += c;
    }
    return res;
}

std::string downcase(const std::string& input)
{
    std::string res;
    for (auto c : input)
    {
        if (isalpha(c))
            res += tolower(c);
        else
            res += c;
    }
    return res;
}

std::string capitalize(const std::string& input)
{
    std::string res;
    bool prevWasAlpha = false;

    for (auto c : input)
    {
        if (isalpha(c))
        {
            if (prevWasAlpha)
                res += tolower(c);
            else
            {
                res += toupper(c);
                prevWasAlpha = true;
            }
        }else
        {
            res += c;
            prevWasAlpha = false;
        }
    }
    return res;
}

bool is_prefix_of(const std::string& prefix, const std::string& string)
{
    if (string.size() < prefix.size())
        return false;

    return string.substr(0, prefix.size()) == prefix;
}

std::string join(const std::string& delimiter, const std::vector<std::string>& vec)
{
    std::string res;

    for (auto it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin())
            res += delimiter;
        res += *it;
    }

    return res;
}

std::string ascii_dump(const std::string& in, const std::string& replacement)
{
    std::string res;

    for (auto c : in)
    {
        if (std::isprint(c))
            res += c;
        else
            res += replacement;
    }
    return res;
}

std::string extension_without_dot(const std::string& filename)
{
    auto i = filename.rfind('.');
    if (i == std::string::npos)
        return "";
    else
        return filename.substr(i + 1);
}

} // namespace str
