#include "regexp.h"

#include <stdexcept>

using namespace onig;

Regexp::Regexp(const std::string& exp)
{
    const UChar *pattern = (decltype(pattern)) exp.c_str();
    int r;
    OnigErrorInfo einfo;

    r = onig_new(&m_reg,
                 pattern, pattern + exp.size(),
                 ONIG_OPTION_DEFAULT,
                 ONIG_ENCODING_UTF8,
                 ONIG_SYNTAX_DEFAULT,
                 &einfo);
    if (r != ONIG_NORMAL)
    {
        char message[ONIG_MAX_ERROR_MESSAGE_LEN];

        onig_error_code_to_str((OnigUChar*) message, r, &einfo);
        throw std::runtime_error(message);
    }
}

Regexp::~Regexp()
{
    onig_free(m_reg);
}

std::vector<std::string> Regexp::exec(const std::string& str)
{
    int r;
    unsigned char *start, *range, *end;
    OnigRegion *region;

    region = onig_region_new();

    end   = (decltype(end)) str.c_str() + str.size();
    start = (decltype(start)) str.c_str();
    range = end;

    r = onig_search(m_reg,
                    (OnigUChar*) str.c_str(), end,
                    start, range,
                    region,
                    ONIG_OPTION_NONE);

    if (r >= 0) {
        std::vector<std::string> res;

        for (int i = 0; i < region->num_regs; i++)
        {
            res.push_back(std::string(str.c_str() + region->beg[i], str.c_str() + region->end[i]));
        }
        onig_region_free(region, 1);
        return res;
    } else if (r == ONIG_MISMATCH) {
        onig_region_free(region, 1);
        return {};
    } else {
        onig_region_free(region, 1);

        char message[ONIG_MAX_ERROR_MESSAGE_LEN];
        onig_error_code_to_str((OnigUChar*) message, r);
        throw std::runtime_error(message);
    }
}

bool Regexp::matches(const std::string& str)
{
    return exec(str).size() > 0;
}
