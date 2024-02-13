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

    static std::string escape(const std::string&);
    static std::vector<std::string> grep(const Regexp&, const std::vector<std::string>&);

    std::vector<std::string> exec(const std::string& str) const;
    bool matches(const std::string& str) const;

    std::regex m_reg;
    const std::string m_exp;
};

#endif
