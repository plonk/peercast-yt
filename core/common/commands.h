#ifndef COMMANDS_H
#define COMMANDS_H

#include "stream.h"
#include <functional>

class Commands
{
public:
    static void log(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested);
    static void nslookup(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested);
};

#endif
