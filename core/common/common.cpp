// (c) peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------


#include <string.h>
#ifdef ADD_BACKTRACE
#include <execinfo.h>
#include <cxxabi.h>
#endif

#include "common.h"
#include "str.h"

GeneralException::GeneralException(const char *m, int e)
{
    msgbuf = m;
    err = e;

#ifdef ADD_BACKTRACE
    constexpr size_t kMaxBacktrace = 16;
    void* backtraceBuffer[kMaxBacktrace];
    int addressCount;

    addressCount = ::backtrace(backtraceBuffer, kMaxBacktrace);

    char** strings = backtrace_symbols(backtraceBuffer, addressCount);
    if (!strings) {
        LOG_DEBUG("Failed to get backtrace");
        return;
    }

    for (int i = 0; i < addressCount; i++) {
        char* a = strchr(strings[i], '(');
        char* b = NULL;
        if (a)
            b = strchr(a, '+');
        if (a && b) {
            std::string fname(a + 1, b);
            if (!fname.empty()) {
                int status;
                char *demangled;
                demangled = abi::__cxa_demangle(fname.c_str(), NULL, NULL, &status);
                if (status == 0) {
                    backtrace.push_back(std::string(strings[i], a + 1)
                                        + demangled
                                        + b);
                } else {
                    backtrace.push_back(strings[i]);
                }
                if (demangled)
                    free(demangled);
            } else {
                backtrace.push_back(strings[i]);
            }
        } else {
            backtrace.push_back(strings[i]);
        }
    }

    free(strings);
#endif

    msg = msgbuf.c_str();
}

const char* GeneralException::what() const throw()
{
    return msg;
}

