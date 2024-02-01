#include "commands.h"
#include <queue>
#include "logbuf.h"
#include "defer.h"

#include <algorithm>
static std::pair< std::map<std::string, bool>,
                  std::vector<std::string> >
parse_options(const std::vector<std::string>& args,
              const std::vector<std::string>& option_names)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--") { // end of options
            for (size_t j = i + 1; j < args.size(); j++) {
                positionals.push_back(args[j]);
            }
            break;
        } else if (std::find(option_names.begin(), option_names.end(), args[i]) != option_names.end()) {
            options[args[i]] = true;
        } else {
            positionals.push_back(args[i]);
        }
    }
    return { options, positionals };
}

static std::vector<std::string> command_words(const std::string& cmdline)
{
    std::string word;
    std::vector<std::string> words;

    auto p = cmdline.begin();
    while (p != cmdline.end()) {
        if (*p == ' ') {
            if (word.size()) {
                words.push_back(word);
                word = "";
            }
            do {
                p++;
            } while (*p == ' ');
            continue;
        } else if (*p == '"') {
            p++;
            while (true) {
                if (p == cmdline.end()) {
                    throw FormatException("Premature end of quoted string");
                } else if (*p == '\\') {
                    p++;
                    if (p == cmdline.end()) {
                        throw FormatException("Premature end of escaped character");
                    } else {
                        word += *p;
                    }
                } else if (*p == '"') {
                    words.push_back(word);
                    word = "";
                    break;
                } else {
                    word += *p;
                }
                p++;
            }
        } else {
            word += *p;
        }
        p++;
    }
    if (word.size()) {
        words.push_back(word);
        word = "";
    }
    return words;
}

static std::map<std::string,
                std::function< void(Stream&,const std::vector<std::string>&,std::function<bool()>) > >
s_commands = {
              { "log", Commands::log },
              { "nslookup", Commands::nslookup },
              { "helo", Commands::helo },
              //{ "filter", Commands::filter },
              { "get", Commands::get },
              //{ "chan", Commands::chan },
              { "echo", Commands::echo },
              //{ "bbs", Commands::bbs },
              { "notify", Commands::notify },
              { "help", Commands::help },
              { "pid", Commands::pid },
};

void Commands::system(Stream& stream, const std::string& _cmdline, std::function<bool()> cancel)
{
    try {
        auto words = command_words(_cmdline);
        if (words.size() == 0) {
            stream.writeLine("Error: Empty command line");
            return;
        }

        const auto cmd = words[0];
        const std::vector<std::string> args(words.begin() + 1, words.end());

        if (s_commands.count(cmd)) {
            s_commands[cmd](stream, args, cancel);
        } else {
            stream.writeLineF("Error: No such command '%s'", cmd.c_str());
        }
    } catch (GeneralException& e) {
        stream.writeLineF("Error: %s", e.what());
    }
}

#ifdef WIN32
#include "processthreadsapi.h"
#else
#include <sys/types.h>
#include <unistd.h>
#endif
void Commands::pid(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 0 || options.count("--help")) {
        stream.writeLine("Usage: pid");
        stream.writeLine("Show the process ID of the server process.");
        return;
    }

#ifdef WIN32
    auto id = GetCurrentProcessId();
#else
    auto id = getpid();
#endif

    stream.writeLineF("%s", std::to_string(id).c_str());
}

void Commands::help(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {});

    if (positionals.size() == 0) {
        for (const auto& pair : s_commands) {
            stream.writeLineF("%s", pair.first.c_str());
        }
    } else if (positionals.size() == 1) {
        const auto cmd = positionals[0];
        if (s_commands.count(cmd)) {
            s_commands[cmd](stream, {"--help"}, cancel);
        } else {
            stream.writeLineF("Error: No such command '%s'", cmd.c_str());
        }
    }else {
        stream.writeLine("Usage: help [COMMAND]");
        stream.writeLine("List available commands. Or print the help message of a specified command.");
    }
    return;
}

#include "servmgr.h"
#include "peercast.h"
void Commands::notify(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 1 || options.count("--help")) {
        stream.writeLine("Usage: notify MESSAGE");
        stream.writeLine("Send a notification message.");
        return;
    }
    peercast::notifyMessage(ServMgr::NT_PEERCAST, positionals[0]);
    stream.writeLine("OK");
}

#include "bbs.h"
void Commands::bbs(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() < 1 || options.count("--help")) {
        stream.writeLine("Usage: bbs URL");
        return;
    }
    auto url = positionals[0];
    auto board = bbs::Board::tryCreate(url);
    stream.writeLine(board->datUrl);
}

void Commands::echo(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "-v", "--help" });

    if (options.count("--help")) {
        stream.writeLine("Usage: echo [-v] WORDS...");
        return;
    }

    if (options.count("-v")) {
        int i = 1;
        for (auto& word : positionals) {
            stream.writeLineF("[%d] %s", i, word.c_str());
            i++;
        }
    } else {
        stream.writeLine(str::join(" ", positionals).c_str());
    }
}

#include "chanmgr.h"
void Commands::chan(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (options.count("--help")) {
        stream.writeLine("Usage: chan");
        return;
    }

    for (auto it = chanMgr->channel; it; it = it->next) {
        stream.writeLineF("%s %s %s",
                          it->getName(),
                          it->getID().str().c_str(),
                          it->getStatusStr());
    }
}

#include "http.h"

void Commands::get(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (positionals.size() != 1 || options.count("--help")) {
        stream.writeLine("Usage: get URL");
        stream.writeLine("Get a web resource and print it.");
        return;
    }

    std::string body;
    try {
        body = http::get(argv[0]);
    } catch (GeneralException& e) {
        stream.writeStringF("Error: %s", e.what());
        return;
    }
    stream.writeString(body);
}

#include "amf0.h"
#include "servmgr.h"
void Commands::filter(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (positionals.size() < 1 || options.count("--help")) {
        stream.writeLine("Usage: filter show");
        stream.writeLine("       filter ban TARGET");
        return;
    }

    if (positionals[0] == "show") {//
        std::lock_guard<std::recursive_mutex> cs(servMgr->lock);

        for (int i = 0; i < servMgr->numFilters; i++) {
            ServFilter* filter = &servMgr->filters[i];
            std::vector<std::string> labels;

            if (filter->flags & ServFilter::F_BAN) {
                labels.push_back("banned");
            }
            if (filter->flags & ServFilter::F_NETWORK) {
                labels.push_back("network");
            }
            if (filter->flags & ServFilter::F_DIRECT) {
                labels.push_back("direct");
            }
            if (filter->flags & ServFilter::F_PRIVATE) {
                labels.push_back("private");
            }
        
            stream.writeLineF("%-20s %s",
                              filter->getPattern().c_str(),
                              str::join(" ", labels).c_str());
        }
    } else if (positionals[0] == "ban") {
        stream.writeLine("not implemented yet");
    } else {
        stream.writeLineF("Unknown subcommand '%s'", positionals[0].c_str());
    }
}

void Commands::log(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (options.count("--help")) {
        stream.writeLine("Usage: log");
        stream.writeLine("Start printing the log.");
        return;
    }

    std::recursive_mutex lock;

    std::queue<std::string> queue;
    auto id = sys->logBuf->addListener([&](unsigned int time, LogBuffer::TYPE type, const char* msg) -> void
                                       {
                                           auto chunk = str::format("[%s] %s\n", LogBuffer::getTypeStr(type), msg);
                                           std::lock_guard<std::recursive_mutex> cs(lock);
                                           queue.push(chunk);
                                       });
    Defer defer([=]() { sys->logBuf->removeListener(id); });

    while (!cancel())
    {
        {
            std::lock_guard<std::recursive_mutex> cs(lock);
            while (!queue.empty())
                {
                    auto chunk = queue.front();
                    stream.writeString(chunk);
                    queue.pop();
                }
        }
        sys->sleep(100);
    }
}

void Commands::nslookup(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (positionals.size() != 1 || options.count("--help"))
    {
        stream.writeLine("Usage: nslookup NAME");
        stream.writeLine("Lookup a host name.");
        return;
    }

    const auto str = positionals[0];
    IP ip;
    if (IP::tryParse(str, ip))
    {
        std::string out;
        if (sys->getHostnameByAddress(ip, out))
        {
            stream.writeLine(out);
        }else
        {
            stream.writeLineF("Error: '%s' not found", str.c_str());
        }
    }else
    {
        try {
            auto ips = sys->getIPAddresses(str);
            for (const auto& ip : ips)
            {
                stream.writeLine(ip);
            }
        } catch (GeneralException &e)
        {
            stream.writeLineF("Error: '%s' not found: %s", str.c_str(), e.what());
        }
    }
    return;
}

#include "socket.h"
#include "atom.h"
#include "pcp.h"
#include "servmgr.h" //DEFAULT_PORT
void Commands::helo(Stream& stdout, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "-v", "--help" });

    if (positionals.size() != 1 || options.count("--help"))
    {
        stdout.writeLine("Usage: helo [-v] HOST");
        stdout.writeLine("Perform a PCP handshake with HOST.");
        return;
    }
    const auto target = positionals[0];

    try {
        assert(AUX_LOG_FUNC_VECTOR != nullptr);
        AUX_LOG_FUNC_VECTOR->push_back([&](LogBuffer::TYPE type, const char* msg) -> void
                                      {
                                          if (type == LogBuffer::T_ERROR)
                                              stdout.writeString("Error: ");
                                          else if (type == LogBuffer::T_WARN)
                                              stdout.writeString("Warning: ");
                                          stdout.writeLine(msg);
                                      });
        Defer defer([]() { AUX_LOG_FUNC_VECTOR->pop_back(); });

        Host host = Host::fromString(target, DEFAULT_PORT);

        stdout.writeLineF("HELO %s", host.str().c_str());

        auto sock = sys->createSocket();
        sock->setReadTimeout(30000);
        sock->open(host);
        sock->connect();

        CopyingStream cs(sock.get());
        Defer defer2([&]() {
                         if (options.count("-v")) {
                             stdout.writeLineF("--- %d bytes written", (int)cs.dataWritten.size());
                             if (cs.dataWritten.size()) {
                                 stdout.writeLine(str::ascii_dump(cs.dataWritten));
                                 stdout.writeLine(str::hexdump(cs.dataWritten));
                             }
                             stdout.writeLineF("--- %d bytes read", (int)cs.dataRead.size());
                             if (cs.dataRead.size()) {
                                 stdout.writeLine(str::ascii_dump(cs.dataRead));
                                 stdout.writeLine(str::hexdump(cs.dataRead));
                             }
                         }
                     });
        AtomStream atom(cs);

        atom.writeInt(PCP_CONNECT, 1);

        GnuID remoteID;
        String agent;
        Servent::handshakeOutgoingPCP(atom,
                                      sock->host,
                                      remoteID,
                                      agent,
                                      false /* isTrusted */);
        stdout.writeLineF("Remote ID: %s", remoteID.str().c_str());
        stdout.writeLineF("Remote agent: %s", agent.c_str());

        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT);

        sock->close();

        stdout.writeLine("OK");

    } catch(GeneralException& e) {
        stdout.writeLineF("Error: %s", e.what());
    }
}
