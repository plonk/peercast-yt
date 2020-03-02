#ifndef _REGEXP_H
#define _REGEXP_H

#include <vector>
#include <string>
#include <regex>

class Regexp
{
public:
    Regexp(const std::string& exp);
    Regexp(const Regexp& orig);
    ~Regexp();

    std::vector<std::string> exec(const std::string& str) const;
    bool matches(const std::string& str) const;

    std::regex m_reg;
    const std::string m_exp;
};

#endif
