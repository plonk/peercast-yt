#ifndef _STR_H
#define _STR_H

#include <string>
#include <vector>

namespace str
{
    std::string ascii_dump(const std::string& in, const std::string& replacement = ".");
    std::string capitalize(const std::string&);
    std::string codepoint_to_utf8(uint32_t);
    bool        contains(const std::string& haystack, const std::string& needle);
    std::string downcase(const std::string& input);
    std::string format(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    std::string group_digits(const std::string& in, const std::string& separator = ",");
    std::string hexdump(const std::string& in);
    std::string inspect(const std::string str);
    bool        is_prefix_of(const std::string&, const std::string&); // deprecated
    bool        has_prefix(const std::string& subject, const std::string& prefix);
    bool        has_suffix(const std::string& subject, const std::string& suffix);
    std::string join(const std::string& delimiter, const std::vector<std::string>& vec);
    std::string repeat(const std::string&, int n);
    std::string replace_prefix(const std::string& s, const std::string& prefix, const std::string& replacement);
    std::string replace_suffix(const std::string& s, const std::string& suffix, const std::string& replacement);
    std::vector<std::string> split(const std::string& in, const std::string& separator);
    std::string upcase(const std::string& input);
    std::string extension_without_dot(const std::string& filename);

    // Count how many occurrences of the substring `needle` are in `haystack`.
    // Throws std::domain_error if `needle` is an empty string.
    int         count(const std::string& haystack, const std::string& needle);

    std::string rstrip(const std::string& str);
    std::string escapeshellarg_unix(const std::string& str);
}

#endif
