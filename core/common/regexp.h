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
    ~Regexp();

    std::vector<std::string> exec(const std::string& str);

    onig::regex_t *m_reg;
};

#endif
