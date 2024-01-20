#include "commands.h"
#include <queue>
#include "logbuf.h"
#include "defer.h"

void Commands::log(Stream& stream, const std::vector<std::string>&, std::function<bool()> cancellationRequested)
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

    while (!cancellationRequested())
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

void Commands::nslookup(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested)
{
    Defer defer([&]() { stream.close(); });

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
void Commands::helo(Stream& stdout, const std::vector<std::string>& argv, std::function<bool()> cancellationRequested)
{
    Defer defer([&]() { stdout.close(); });

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
