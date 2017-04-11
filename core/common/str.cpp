#include <string>

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
        tail = in.substr(in.find('.'));
    else
        end = in.size();

    std::string integer;
    for (int i = 0; i < end; i++)
    {
        if (i != 0 && i % 3 == 0 && i != end - 1)
            integer += separator;
        integer += in[i];
    }
    return integer + tail;
}

} // namespace str
