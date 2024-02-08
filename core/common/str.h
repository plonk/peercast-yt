#ifndef _STR_H
#define _STR_H

#include <stdint.h>
#include <string>
#include <sstream> // to implement str
#include <vector>
#include <stdarg.h>

namespace str
{
    std::string ascii_dump(const std::string& in, const std::string& replacement = ".");
    std::string capitalize(const std::string&);
    std::string codepoint_to_utf8(uint32_t);
    bool        contains(const std::string& haystack, const std::string& needle);
    std::string downcase(const std::string& input);
    std::string format(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
    std::string vformat(const char* fmt, va_list ap);
    std::string group_digits(const std::string& in, const std::string& separator = ",");
    std::string hexdump(const std::string& in);
    std::string inspect(const std::string& str);
    std::string json_inspect(const std::string& str);
    bool        is_prefix_of(const std::string&, const std::string&); // deprecated
    bool        has_prefix(const std::string& subject, const std::string& prefix);
    bool        has_suffix(const std::string& subject, const std::string& suffix);
    std::string join(const std::string& delimiter, const std::vector<std::string>& vec);
    std::string repeat(const std::string&, int n);
    std::string replace_prefix(const std::string& s, const std::string& prefix, const std::string& replacement);
    std::string replace_suffix(const std::string& s, const std::string& suffix, const std::string& replacement);
    std::vector<std::string> split(const std::string& in, const std::string& separator);
    std::vector<std::string> split(const std::string& in, const std::string& separator, int limit);
    std::string upcase(const std::string& input);
    std::string extension_without_dot(const std::string& filename);

    // Count how many occurrences of the substring `needle` are in `haystack`.
    // Throws std::domain_error if `needle` is an empty string.
    int         count(const std::string& haystack, const std::string& needle);

    std::string rstrip(const std::string& str);
    std::string strip(const std::string&);
    std::string escapeshellarg_unix(const std::string& str);

    std::vector<std::string> shellwords(const std::string& str);

    std::vector<std::string> to_lines(const std::string& text);
    std::string indent_tab(const std::string& text, int n = 1);

    // str::STR(...)
    // Convert the arguments to strings (as the << operator does) and concatenate them.
    std::string STR();
    template<typename T>
    std::string STR(T value)
    {
        std::stringstream os;
        os << value;
        return os.str();
    }
    template<typename T, typename... Ts>
    std::string STR(T value, Ts... rest)
    {
        return STR(value) + STR(rest...);
    }

    bool validate_utf8(const std::string& str);
    // Truncate the UTF-8 string str to the maximum size of length bytes.
    std::string truncate_utf8(const std::string& str, size_t length);

    // If str is a valid UTF-8 string, return a copy of it. Otherwise,
    // escape non-ASCII bytes in hexadecimal notation surrounded by
    // brackets ([XX]).
    std::string valid_utf8(const std::string& str);
}

#endif
