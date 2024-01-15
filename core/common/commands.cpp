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

    for (const auto& ip : sys->getIPAddresses(argv[0]))
    {
        stream.writeLine(ip);
    }
}
