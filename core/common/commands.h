#ifndef COMMANDS_H
#define COMMANDS_H

#include "stream.h"
#include <functional>

class Commands
{
public:
    // static shared_ptr<Command> makeCommand(name, stdin, stdout, cancel)
    static void system(Stream& stream, const std::string& cmdline, std::function<bool()> cancel);
    static void log(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void nslookup(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void helo(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void filter(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void get(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void chan(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void echo(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void bbs(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void notify(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);
    static void help(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel);

    // typedef termination_point std::function<bool()>
    // stdin
    // stdout
    // cancel
};

#endif
