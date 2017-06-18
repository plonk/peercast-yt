#include <stdio.h>

#include "cgi.h"
#include "str.h"

namespace cgi {

// URLエスケープする。
std::string escape(const std::string& in)
{
    std::string res;

    for (unsigned char c : in) {
        if ((c >= '0' && c <= '9') ||
            (c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c == '_') || (c == '-') || (c == '.')) {
            res += c;
        } else if (c == ' ') {
            res += '+';
        } else {
            char buf[4];
            snprintf(buf, 4, "%%%02X", c);
            res += buf;
        }
    }
    return res;
}

std::string unescape(const std::string& in)
{
    std::string res;
    size_t i = 0;

    while (i < in.size()) {
        if (in[i] == '%') {
            int c;
            sscanf(in.substr(i + 1, 2).c_str(), "%x", &c);
            res += (char) c;
            i += 3;
        } else if (in[i] == '+') {
            res += ' ';
            i += 1;
        } else {
            res += in[i];
            i += 1;
        }
    }
    return res;
}

Query::Query(const std::string& queryString)
{
    auto assignments = str::split(queryString, "&");
    for (auto& assignment : assignments)
    {
        auto sides = str::split(assignment, "=");
        if (sides.size() == 1)
            m_dict[sides[0]] = {};
        else
        {
            m_dict[sides[0]].push_back(unescape(sides[1]));
        }
    }
}

bool Query::hasKey(const std::string& key)
{
    return m_dict.find(key) != m_dict.end();
}

std::string Query::get(const std::string& key)
{
    try
    {
        if (m_dict.at(key).size() == 0)
            return "";
        else
            return m_dict.at(key)[0];
    } catch (std::out_of_range&)
    {
        return "";
    }
}

std::vector<std::string> Query::getAll(const std::string& key)
{
    try
    {
        return m_dict.at(key);
    } catch (std::out_of_range&)
    {
        return {};
    }
}

static const char* daysOfWeek[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* monthNames[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Nov", "Oct", "Dec" };

std::string rfc1123Time(time_t t)
{
    char fmt[30], buf[30];
#if !defined(WIN32) && (_POSIX_C_SOURCE >= 1 || _XOPEN_SOURCE || _BSD_SOURCE || _SVID_SOURCE || _POSIX_SOURCE)
    tm tm;

    gmtime_r(&t, &tm);
    strftime(fmt, sizeof(fmt), "%%s, %d %%s %Y %H:%M:%S GMT", &tm);
    std::snprintf(buf, sizeof(buf), fmt, daysOfWeek[tm.tm_wday], monthNames[tm.tm_mon]);
#else
    // FIXME: The code that follows should have a race!
    struct tm* ptm;

    ptm = gmtime(&t);
    strftime(fmt, sizeof(fmt), "%%s, %d %%s %Y %H:%M:%S GMT", ptm);
    std::snprintf(buf, sizeof(buf), fmt, daysOfWeek[ptm->tm_wday], monthNames[ptm->tm_mon]);
#endif

    return buf;
}

static const std::map<std::string,uint32_t> entities = {
    { "quot",     0x0022 },
    { "amp",      0x0026 },
    { "apos",     0x0027 },
    { "lt",       0x003C },
    { "gt",       0x003E },
    { "nbsp",     0x00A0 },
    { "iexcl",    0x00A1 },
    { "cent",     0x00A2 },
    { "pound",    0x00A3 },
    { "curren",   0x00A4 },
    { "yen",      0x00A5 },
    { "brvbar",   0x00A6 },
    { "sect",     0x00A7 },
    { "uml",      0x00A8 },
    { "copy",     0x00A9 },
    { "ordf",     0x00AA },
    { "laquo",    0x00AB },
    { "not",      0x00AC },
    { "shy",      0x00AD },
    { "reg",      0x00AE },
    { "macr",     0x00AF },
    { "deg",      0x00B0 },
    { "plusmn",   0x00B1 },
    { "sup2",     0x00B2 },
    { "sup3",     0x00B3 },
    { "acute",    0x00B4 },
    { "micro",    0x00B5 },
    { "para",     0x00B6 },
    { "middot",   0x00B7 },
    { "cedil",    0x00B8 },
    { "sup1",     0x00B9 },
    { "ordm",     0x00BA },
    { "raquo",    0x00BB },
    { "frac14",   0x00BC },
    { "frac12",   0x00BD },
    { "frac34",   0x00BE },
    { "iquest",   0x00BF },
    { "Agrave",   0x00C0 },
    { "Aacute",   0x00C1 },
    { "Acirc",    0x00C2 },
    { "Atilde",   0x00C3 },
    { "Auml",     0x00C4 },
    { "Aring",    0x00C5 },
    { "AElig",    0x00C6 },
    { "Ccedil",   0x00C7 },
    { "Egrave",   0x00C8 },
    { "Eacute",   0x00C9 },
    { "Ecirc",    0x00CA },
    { "Euml",     0x00CB },
    { "Igrave",   0x00CC },
    { "Iacute",   0x00CD },
    { "Icirc",    0x00CE },
    { "Iuml",     0x00CF },
    { "ETH",      0x00D0 },
    { "Ntilde",   0x00D1 },
    { "Ograve",   0x00D2 },
    { "Oacute",   0x00D3 },
    { "Ocirc",    0x00D4 },
    { "Otilde",   0x00D5 },
    { "Ouml",     0x00D6 },
    { "times",    0x00D7 },
    { "Oslash",   0x00D8 },
    { "Ugrave",   0x00D9 },
    { "Uacute",   0x00DA },
    { "Ucirc",    0x00DB },
    { "Uuml",     0x00DC },
    { "Yacute",   0x00DD },
    { "THORN",    0x00DE },
    { "szlig",    0x00DF },
    { "agrave",   0x00E0 },
    { "aacute",   0x00E1 },
    { "acirc",    0x00E2 },
    { "atilde",   0x00E3 },
    { "auml",     0x00E4 },
    { "aring",    0x00E5 },
    { "aelig",    0x00E6 },
    { "ccedil",   0x00E7 },
    { "egrave",   0x00E8 },
    { "eacute",   0x00E9 },
    { "ecirc",    0x00EA },
    { "euml",     0x00EB },
    { "igrave",   0x00EC },
    { "iacute",   0x00ED },
    { "icirc",    0x00EE },
    { "iuml",     0x00EF },
    { "eth",      0x00F0 },
    { "ntilde",   0x00F1 },
    { "ograve",   0x00F2 },
    { "oacute",   0x00F3 },
    { "ocirc",    0x00F4 },
    { "otilde",   0x00F5 },
    { "ouml",     0x00F6 },
    { "divide",   0x00F7 },
    { "oslash",   0x00F8 },
    { "ugrave",   0x00F9 },
    { "uacute",   0x00FA },
    { "ucirc",    0x00FB },
    { "uuml",     0x00FC },
    { "yacute",   0x00FD },
    { "thorn",    0x00FE },
    { "yuml",     0x00FF },
    { "OElig",    0x0152 },
    { "oelig",    0x0153 },
    { "Scaron",   0x0160 },
    { "scaron",   0x0161 },
    { "Yuml",     0x0178 },
    { "fnof",     0x0192 },
    { "circ",     0x02C6 },
    { "tilde",    0x02DC },
    { "Alpha",    0x0391 },
    { "Beta",     0x0392 },
    { "Gamma",    0x0393 },
    { "Delta",    0x0394 },
    { "Epsilon",  0x0395 },
    { "Zeta",     0x0396 },
    { "Eta",      0x0397 },
    { "Theta",    0x0398 },
    { "Iota",     0x0399 },
    { "Kappa",    0x039A },
    { "Lambda",   0x039B },
    { "Mu",       0x039C },
    { "Nu",       0x039D },
    { "Xi",       0x039E },
    { "Omicron",  0x039F },
    { "Pi",       0x03A0 },
    { "Rho",      0x03A1 },
    { "Sigma",    0x03A3 },
    { "Tau",      0x03A4 },
    { "Upsilon",  0x03A5 },
    { "Phi",      0x03A6 },
    { "Chi",      0x03A7 },
    { "Psi",      0x03A8 },
    { "Omega",    0x03A9 },
    { "alpha",    0x03B1 },
    { "beta",     0x03B2 },
    { "gamma",    0x03B3 },
    { "delta",    0x03B4 },
    { "epsilon",  0x03B5 },
    { "zeta",     0x03B6 },
    { "eta",      0x03B7 },
    { "theta",    0x03B8 },
    { "iota",     0x03B9 },
    { "kappa",    0x03BA },
    { "lambda",   0x03BB },
    { "mu",       0x03BC },
    { "nu",       0x03BD },
    { "xi",       0x03BE },
    { "omicron",  0x03BF },
    { "pi",       0x03C0 },
    { "rho",      0x03C1 },
    { "sigmaf",   0x03C2 },
    { "sigma",    0x03C3 },
    { "tau",      0x03C4 },
    { "upsilon",  0x03C5 },
    { "phi",      0x03C6 },
    { "chi",      0x03C7 },
    { "psi",      0x03C8 },
    { "omega",    0x03C9 },
    { "thetasym", 0x03D1 },
    { "upsih",    0x03D2 },
    { "piv",      0x03D6 },
    { "ensp",     0x2002 },
    { "emsp",     0x2003 },
    { "thinsp",   0x2009 },
    { "zwnj",     0x200C },
    { "zwj",      0x200D },
    { "lrm",      0x200E },
    { "rlm",      0x200F },
    { "ndash",    0x2013 },
    { "mdash",    0x2014 },
    { "lsquo",    0x2018 },
    { "rsquo",    0x2019 },
    { "sbquo",    0x201A },
    { "ldquo",    0x201C },
    { "rdquo",    0x201D },
    { "bdquo",    0x201E },
    { "dagger",   0x2020 },
    { "Dagger",   0x2021 },
    { "bull",     0x2022 },
    { "hellip",   0x2026 },
    { "permil",   0x2030 },
    { "prime",    0x2032 },
    { "Prime",    0x2033 },
    { "lsaquo",   0x2039 },
    { "rsaquo",   0x203A },
    { "oline",    0x203E },
    { "frasl",    0x2044 },
    { "euro",     0x20AC },
    { "image",    0x2111 },
    { "weierp",   0x2118 },
    { "real",     0x211C },
    { "trade",    0x2122 },
    { "alefsym",  0x2135 },
    { "larr",     0x2190 },
    { "uarr",     0x2191 },
    { "rarr",     0x2192 },
    { "darr",     0x2193 },
    { "harr",     0x2194 },
    { "crarr",    0x21B5 },
    { "lArr",     0x21D0 },
    { "uArr",     0x21D1 },
    { "rArr",     0x21D2 },
    { "dArr",     0x21D3 },
    { "hArr",     0x21D4 },
    { "forall",   0x2200 },
    { "part",     0x2202 },
    { "exist",    0x2203 },
    { "empty",    0x2205 },
    { "nabla",    0x2207 },
    { "isin",     0x2208 },
    { "notin",    0x2209 },
    { "ni",       0x220B },
    { "prod",     0x220F },
    { "sum",      0x2211 },
    { "minus",    0x2212 },
    { "lowast",   0x2217 },
    { "radic",    0x221A },
    { "prop",     0x221D },
    { "infin",    0x221E },
    { "ang",      0x2220 },
    { "and",      0x2227 },
    { "or",       0x2228 },
    { "cap",      0x2229 },
    { "cup",      0x222A },
    { "int",      0x222B },
    { "there4",   0x2234 },
    { "sim",      0x223C },
    { "cong",     0x2245 },
    { "asymp",    0x2248 },
    { "ne",       0x2260 },
    { "equiv",    0x2261 },
    { "le",       0x2264 },
    { "ge",       0x2265 },
    { "sub",      0x2282 },
    { "sup",      0x2283 },
    { "nsub",     0x2284 },
    { "sube",     0x2286 },
    { "supe",     0x2287 },
    { "oplus",    0x2295 },
    { "otimes",   0x2297 },
    { "perp",     0x22A5 },
    { "sdot",     0x22C5 },
    { "lceil",    0x2308 },
    { "rceil",    0x2309 },
    { "lfloor",   0x230A },
    { "rfloor",   0x230B },
    { "lang",     0x2329 },
    { "rang",     0x232A },
    { "loz",      0x25CA },
    { "spades",   0x2660 },
    { "clubs",    0x2663 },
    { "hearts",   0x2665 },
    { "diams",    0x2666 }
};

std::string unescape_html(const std::string& input)
{
    auto isDecimal =
        [](const std::string& s)
        {
            if (s.size() < 2)
                return false;

            if (s[0] != '#')
                return false;

            for (size_t i = 1; i < s.size(); i++)
            {
                if (!(s[i] >= '0' && s[i] <= '9'))
                    return false;
            }
            return true;
        };

    auto isHexadecimal =
        [](const std::string& s)
        {
            if (s.size() < 3)
                return false;

            if (s[0] != '#' || !(s[1] == 'x' || s[1] == 'X'))
                return false;

            for (size_t i = 2; i < s.size(); i++)
            {
                if (!((s[i] >= '0' && s[i] <= '9') ||
                      (s[i] >= 'a' && s[i] <= 'f') ||
                      (s[i] >= 'A' && s[i] <= 'F')))
                    return false;
            }
            return true;
        };

    auto it = input.begin();
    std::string res;

    while (it != input.end())
    {
        if (*it == '&')
        {
            it++;
            std::string ref;
            while (it != input.end() &&
                   ((*it >= '0' && *it <= '9') ||
                    (*it >= 'a' && *it <= 'z') ||
                    (*it >= 'A' && *it <= 'Z') ||
                    *it == '#'))
            {
                ref += *it++;
            }

            if (it == input.end())
                return res + "&" + ref;

            if (*it != ';')
            {
                res += '&';
                res += ref;
                continue;
            }

            it++; // skip semicolon

            uint32_t codepoint;
            if (ref.empty())
            {
                res += "&" + ref + ";";
                continue;
            } else if (isDecimal(ref))
            {
                codepoint = std::stoll(ref.substr(1));
            } else if (isHexadecimal(ref))
            {
                codepoint = std::stoll(ref.substr(2), 0, 16);
            } else
            {
                try
                {
                    codepoint = entities.at(ref);
                } catch (std::out_of_range&)
                {
                    res += "&" + ref + ";";
                    continue;
                }
            }
            res += str::codepoint_to_utf8(codepoint);
            continue;
        } else {
            res += *it;
            it++;
        }
    }
    return res;
}

std::string escape_html(const std::string& input)
{
    std::string dest;
    auto src = input.begin();

    while (src != input.end())
    {
        switch (*src)
        {
        case '&':
            dest += "&amp;";
            break;
        case '<':
            dest += "&lt;";
            break;
        case '>':
            dest += "&gt;";
            break;
        case '\"':
            dest += "&quot;";
            break;
        case '\'':
            dest += "&#39;";
            break;
        default:
            dest += *src;
            break;
        }
        src++;
    }
    return dest;
}

std::string escape_javascript(const std::string& input)
{
    std::string res;

    for (char c : input)
    {
        if (c == '\'' || c == '\"' || c == '\\')
        {
            res += '\\';
            res += c;
        } else if (iscntrl(c))
        {
            char buf[3];

            res += "\\x";
            sprintf(buf, "%02hhX", c);
            res += buf;
        } else
            res += c;
    }
    return res;
}

} // namespace cgi
