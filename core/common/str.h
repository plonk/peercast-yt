#ifndef _STR_H
#define _STR_H

#include <string>

namespace str
{

    std::string hexdump(const std::string& in);
    std::string inspect(const std::string str);
    std::string repeat(const std::string&, int n);
    std::string group_digits(const std::string& in, const std::string& separator = ",");
}

#endif
