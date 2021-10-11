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

        for (int i = 0; i < m.size(); i++)
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
