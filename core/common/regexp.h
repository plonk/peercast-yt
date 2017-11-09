#ifndef _REGEXP_H
#define _REGEXP_H

#include <vector>
#include <string>

namespace onig
{
#include "onigmo.h"
}

class Regexp
{
public:
    Regexp(const std::string& exp);
    Regexp(const Regexp& orig);
    ~Regexp();

    std::vector<std::string> exec(const std::string& str) const;
    bool matches(const std::string& str) const;

    onig::regex_t *m_reg;
    const std::string m_exp;
};

#endif
