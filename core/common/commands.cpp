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

static std::map<std::string,
                std::function< void(Stream&,const std::vector<std::string>&,std::function<bool()>) > >
s_commands = {
              { "log", Commands::log },
              { "nslookup", Commands::nslookup },
              { "helo", Commands::helo },
              //{ "filter", Commands::filter },
              { "get", Commands::get },
              { "chan", Commands::chan },
              { "echo", Commands::echo },
              //{ "bbs", Commands::bbs },
              { "notify", Commands::notify },
              { "help", Commands::help },
              { "pid", Commands::pid },
              { "expr", Commands::expr },
              { "shutdown", Commands::shutdown },
              { "speedtest", Commands::speedtest },
              { "sleep", Commands::sleep },
              { "date", Commands::date },
};

void Commands::system(Stream& stream, const std::string& _cmdline, std::function<bool()> cancel)
{
    try {
        auto words = str::shellwords(_cmdline);
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

void Commands::date(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 0 || options.count("--help")) {
        stream.writeLine("Usage: date");
        stream.writeLine("Display the current time.");
        return;
    }

    String s;
    s.setFromTime(sys->getTime());
    stream.writeString(s); // s is already terminated with \n since it's in the asctime format.
}

#include <cstdlib> // for std::atof
void Commands::sleep(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 1 || options.count("--help")) {
        stream.writeLine("Usage: sleep NUMBER");
        stream.writeLine("Pause for NUMBER seconds.");
        return;
    }
    double seconds = std::atof(positionals[0].c_str());
    sys->sleep(seconds * 1000);
}

#include "host.h"
#include "socket.h"
#include "http.h"
#include "chunker.h"
#include "dechunker.h"
#include "version2.h"
void Commands::speedtest(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help", "--enable-nagle", "--disable-nagle"});

    if (positionals.size() != 1 || options.count("--help")) {
        stream.writeLine("Usage: speedtest HOST:PORT");
        stream.writeLine("Perform a bandwidth test with a server.");
        return;
    }

    Host host;
    host.fromStrName(positionals[0].c_str(), 7144);

    do { // Download
        stream.writeLineF("Downloading from http://%s/speedtest ...", host.str(true /*with port*/).c_str());

        auto sock = sys->createSocket();
        sock->open(host);
        sock->connect();

        if (options.count("--enable-nagle")) {
            sock->setNagle(true);
            stream.writeLine("Nagle on");
        } else if (options.count("--disable-nagle")) {
            sock->setNagle(false);
            stream.writeLine("Nagle off");
        }

        HTTP http(*sock);
        http.writeLine("GET /speedtest HTTP/1.1");
        http.writeLine("User-Agent: " PCX_AGENT);
        http.writeLine("Host: " + positionals[0]);
        http.writeLine("");

        int status = http.readResponse();
        if (status != 200) {
            stream.writeLineF("Error: Status: %d", status);
            break;
        }
        http.readHeaders();
        if (http.headers.get("Transfer-Encoding") != "chunked") {
            http.writeLine("Error: Not a chunked stream!");
            break;
        }

        Dechunker dechunker(http);
        size_t received = 0;

        const auto startTime = sys->getDTime();
        char buffer[1024];

        while (sys->getDTime() - startTime < 15.0) {
            int r = dechunker.read(buffer, 1024);
            received += r;
        }

        const auto endTime = sys->getDTime();

        stream.writeLineF("%d bps", (int) (received*8 / (endTime - startTime)));
    } while(0);

    stream.writeLine("");

    do { // Uplaod
        stream.writeLineF("Uploading to http://%s/speedtest ...", host.str(true /*with port*/).c_str());

        auto sock = sys->createSocket();
        sock->open(host);
        sock->connect();

        if (options.count("--enable-nagle")) {
            sock->setNagle(true);
            stream.writeLine("Nagle on");
        } else if (options.count("--disable-nagle")) {
            sock->setNagle(false);
            stream.writeLine("Nagle off");
        }

        HTTP http(*sock);
        http.writeLine("POST /speedtest HTTP/1.1");
        http.writeLine("Content-Type: application/binary");
        http.writeLine("Transfer-Encoding: chunked");
        http.writeLine("");

        Chunker chunker(http);
    
        auto generateRandomBytes = [](size_t size) -> std::string {
                                       std::string buf;
                                       peercast::Random r;

                                       for (size_t i = 0; i < size; ++i)
                                           buf += (char) (r.next() & 0xff);
                                       return buf;
                                   };
        std::string chunk = generateRandomBytes(1024);
        size_t sent = 0;

        const auto startTime = sys->getDTime();

        while (sys->getDTime() - startTime < 15.0) {
            chunker.writeString(chunk);
            sent += 1024;
        }
        chunker.close();

        const auto endTime = sys->getDTime();

        auto res = http.getResponse();
        if (res.statusCode != 200) {
            stream.writeLineF("Error: Status: %d", res.statusCode);
            // Print the body anyway.
        }
        stream.writeString(res.body);
    } while(0);
}

#include "servmgr.h"
void Commands::shutdown(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 0 || options.count("--help")) {
        stream.writeLine("Usage: shutdown");
        stream.writeLine("Shutdown server.");
        return;
    }

    stream.writeLine("Server is shutting down NOW!");

    servMgr->shutdownTimer = 1;
}

#include "template.h"
void Commands::expr(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() == 0 || options.count("--help")) {
        stream.writeLine("Usage: expr EXPRESSION...");
        stream.writeLine("Evaluate expression.");
        return;
    }

    Template temp("");
    RootObjectScope globals;
    temp.prependScope(globals);

    auto expression = str::join(" ", positionals);
    stream.writeLine(amf0::format(temp.evalExpression(expression)));
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
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() > 1 || options.count("--help")) {
        stream.writeLine("Usage: help [COMMAND]");
        stream.writeLine("List available commands or print the usage of the specified command.");
        return;
    }

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
#include "url.h"
void Commands::chan(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, bool> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (positionals.size() < 1 || options.count("--help")) {
    ShowUsageAndReturn:
        stream.writeLine("Usage: chan ls");
        stream.writeLine("       chan hit");
        stream.writeLine("       chan set-url ID URL");
        return;
    }


    if (positionals.size() >= 1 && positionals[0] == "hit") {
        for (auto hitlist = chanMgr->hitlist; hitlist != nullptr; hitlist = hitlist->next) {
            auto chid = hitlist->info.id;
            int size = hitlist->numHits();
            stream.writeLineF("%s %d", chid.str().c_str(), size);
        }
    } else if (positionals.size() >= 3 && positionals[0] == "set-url") {
        for (auto it = chanMgr->channel; it; it = it->next) {
            if (str::has_prefix(it->getID().str(), positionals[1])) {
                std::lock_guard<std::recursive_mutex>(it->lock);

                if (it->srcType != Channel::SRC_URL) {
                    stream.writeLineF("Error: The source type of %s is not URL.", it->getID().str().c_str());
                    continue;
                }
                auto newUrl = positionals[2];

                auto info = it->info;

                stream.writeLine("Stopping channel.");
                it->thread.shutdown();
                sys->waitThread(&it->thread);
                stream.writeLine("Channel stopped.");

                sys->sleep(5000);
                stream.writeLine("Creating new channel.");
                auto ch = chanMgr->createChannel(info);
                ch->startURL(newUrl.c_str());
                stream.writeLine("Started.");
                break;
            }
        }
    } else if (positionals.size() >= 1 && positionals[0] == "ls") {
        for (auto it = chanMgr->channel; it; it = it->next) {
            stream.writeLineF("%s %s %s",
                              it->getName(),
                              it->getID().str().c_str(),
                              it->getStatusStr());
        }
    } else {
        goto ShowUsageAndReturn;
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
        stream.writeLine("Look up a host name.");
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

        if (host.ip.isIPv4Mapped()) {
            atom.writeInt(PCP_CONNECT, 1);
            LOG_DEBUG("PCP_CONNECT 1");
        } else {
            atom.writeInt(PCP_CONNECT, 100);
            LOG_DEBUG("PCP_CONNECT 100");
        }

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
    } catch(GeneralException& e) {
        stdout.writeLineF("Error: %s", e.what());
    }
}
