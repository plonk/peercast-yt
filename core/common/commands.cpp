#include "commands.h"
#include <queue>
#include "logbuf.h"
#include "defer.h"

void Commands::system(Stream& stream, const std::string& _cmdline, std::function<bool()> cancel)
{
    std::string cmdline = str::strip(_cmdline);

    if (cmdline == "") {
        stream.writeLine("Error: Empty command line");
        return;
    }

    auto words = str::split(cmdline, " ");
    if (words.size() == 0) {
        stream.writeLine("Error: ???");
        return;
    }

    const auto cmd = words[0];
    const std::vector<std::string> args(words.begin() + 1, words.end());

    if (words[0] == "log")
        Commands::log(stream, args, cancel);
    else if (words[0] == "nslookup")
        Commands::nslookup(stream, args, cancel);
    else if (words[0] == "helo")
        Commands::helo(stream, args, cancel);
    else if (words[0] == "filter")
        Commands::filter(stream, args, cancel);
    else {
        stream.writeLineF("Error: No such command '%s'", words[0].c_str());
    }
}

#include "amf0.h"
#include "servmgr.h"
void Commands::filter(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    if (argv.size() < 1) {
        stream.writeLine("Usage: filter show");
        stream.writeLine("       filter ban TARGET");
        return;
    }

    if (argv[0] == "show") {//
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
    } else if (argv[0] == "ban") {
        stream.writeLine("not implemented yet");
    } else {
        stream.writeLineF("Unknown subcommand '%s'", argv[0].c_str());
    }
}

void Commands::log(Stream& stream, const std::vector<std::string>&, std::function<bool()> cancel)
{
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
    if (argv.size() != 1)
    {
        stream.writeLine("Usage: nslookup NAME");
        return;
    }

    const auto str = argv[0];
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
    if (argv.size() != 1)
    {
        stdout.writeLine("Usage: helo HOST");
        return;
    }

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

        Host host = Host::fromString(argv[0], DEFAULT_PORT);

        stdout.writeLineF("HELO %s", host.str().c_str());

        auto sock = sys->createSocket();
        sock->setReadTimeout(30000);
        sock->open(host);
        sock->connect();

        CopyingStream cs(sock.get());
        Defer defer2([&]() {
                         stdout.writeLine(str::ascii_dump(cs.dataWritten));
                         stdout.writeLine(str::hexdump(cs.dataWritten));
                         stdout.writeLine(str::ascii_dump(cs.dataRead));
                         stdout.writeLine(str::hexdump(cs.dataRead));
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
