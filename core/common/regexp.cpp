#include "regexp.h"

#include <stdexcept>

Regexp::Regexp(const std::string& exp)
    : m_reg(exp.c_str())
    , m_exp(exp)
{
}

Regexp::Regexp(const Regexp& orig)
    : Regexp(orig.m_exp)
{
}

Regexp::~Regexp()
{
}

std::vector<std::string> Regexp::exec(const std::string& str) const
{
    std::smatch m;
    if (std::regex_search(str, m, m_reg))
    {
        std::vector<std::string> v;

        for (size_t i = 0; i < m.size(); i++)
            v.push_back(m[i].str());
        return v;
    }else
    {
        return {};
    }
}

bool Regexp::matches(const std::string& str) const
{
    std::smatch m;
    return std::regex_search(str, m, m_reg);
}

std::string Regexp::escape(const std::string& str)
{
    std::string result;
    for (const auto c : str) {
        switch (c) {
        case '[':
        case ']':
        case '\\':
        case '^':
        case '$':
        case '.':
        case '|':
        case '?':
        case '*':
        case '+':
        case '(':
        case ')':
            result += '\\';
            result += c;
            break;
        default:
            result += c;
            break;
        }
    }
    return result;
}

std::vector<std::string> Regexp::grep(const Regexp& reg, const std::vector<std::string>& ss)
{
    std::vector<std::string> result;

    for (const auto& s : ss) {
        if (reg.matches(s)) {
            result.push_back(s);
        }
    }
    return result;
}

std::string Regexp::replaceAll(const std::string& orig, std::function<std::string(const std::string&)> func) const
{
    std::string result;
    std::string s = orig;

    std::smatch m;
    while (std::regex_search(s, m, m_reg)) {
        result += s.substr(0, m.position(0));
        result += func(m[0].str());
        s = s.substr(m.position(0) + m.length(0));
    }
    result += s;

    return result;
}
