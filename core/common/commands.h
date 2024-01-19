#ifndef COMMANDS_H
#define COMMANDS_H

#include "stream.h"
#include <functional>

class Commands
{
public:
    // static shared_ptr<Command> makeCommand(name, stdin, stdout, cancellationRequested)
    static void log(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested);
    static void nslookup(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested);
    static void helo(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested);

    // typedef termination_point std::function<bool()>
    // stdin
    // stdout
    // cancellationRequested
};

#endif
