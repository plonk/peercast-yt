#include <string.h>
#include "strerror.h"

namespace str {

std::string strerror(int errnum)
{
    char buf[128];
#if !defined(__GLIBC__) || ((_POSIX_C_SOURCE >= 200112L) && ! _GNU_SOURCE)
    // Using the XSI-compliant strerror_r.
    if (strerror_r(errnum, buf, sizeof(buf)) != 0) {
        return str::STR("Unknown error ", errnum);
    } else {
        return buf;
    }
#else
    // Using the GNU-specific strerror_r.
    char* msg = strerror_r(errnum, buf, sizeof(buf));
    return msg;
#endif
}

} // namespace
