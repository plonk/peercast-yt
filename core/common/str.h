#ifndef _STR_H
#define _STR_H

#include <string>
#include <vector>

namespace str
{

    std::string hexdump(const std::string& in);
    std::string inspect(const std::string str);
    std::string repeat(const std::string&, int n);
    std::string group_digits(const std::string& in, const std::string& separator = ",");
    std::vector<std::string> split(const std::string& in, const std::string& separator);
    std::string codepoint_to_utf8(uint32_t);
    bool contains(const std::string& haystack, const std::string& needle);

}

#endif
