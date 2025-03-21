#include "commands.h"
#include <queue>
#include "logbuf.h"
#include "defer.h"

#include <algorithm>
#include <tuple> // std::tie

static std::pair< std::map<std::string, std::string>,
                  std::vector<std::string> >
parse_options(const std::vector<std::string>& args,
              const std::vector<std::string>& option_names)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;

    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == "--") { // end of options
            for (size_t j = i + 1; j < args.size(); j++) {
                positionals.push_back(args[j]);
            }
            break;
        } else if (args[i][0] == '-') {
            if (args[i][1] == '-') { // long option
                auto vec = str::split(args[i], "=", 2);
                if (std::find(option_names.begin(), option_names.end(), vec[0]) == option_names.end()) {
                    throw FormatException(str::format("Unknown long option: %s", vec[0].c_str()));
                } else if (vec.size() == 2) { // assignment
                    options[vec[0]] = vec[1];
                } else {
                    options[vec[0]] = "";
                }
            } else { // short option
                if (std::find(option_names.begin(), option_names.end(), args[i]) == option_names.end()) {
                    throw FormatException(str::format("Unknown short option: %s", args[i].c_str()));
                } else {
                    options[args[i]] = "";
                }
            }
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
              //{ "speedtest", Commands::speedtest },
              { "sleep", Commands::sleep },
              { "date", Commands::date },
              { "pwd", Commands::pwd },
              { "ssl", Commands::ssl },
              { "flag", Commands::flag },
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

void Commands::pwd(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help"});

    if (positionals.size() != 0 || options.count("--help")) {
        stream.writeLine("Usage: pwd");
        stream.writeLine("Print the name of the current working directory.");
        return;
    }

    stream.writeLine(sys->getCurrentWorkingDirectory());
}

void Commands::date(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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


static std::vector<std::shared_ptr<Channel>> pickChannels(const std::string& desig)
{
    std::vector<std::shared_ptr<Channel>> result;

    // チャンネル名が完全一致するチャンネルを返す。
    for (auto ch = chanMgr->channel; ch; ch = ch->next) {
        if (ch->info.name.isSame(desig.c_str())) {
            result.push_back(ch);
        }
    }

    if (result.size() > 0) {
        return result;
    }

    // さもなくば、チャンネルIDかそのプレフィックスと解釈して実行する。

    const auto prefix = str::upcase(desig);
    
    if (prefix.empty()) {
        throw ArgumentException("Empty channel ID prefix");
    }
    
    for (auto ch = chanMgr->channel; ch; ch = ch->next) {
        if (str::has_prefix(str::upcase(ch->getID().str()), prefix)) {
            result.push_back(ch);
        }
    }
    return result;
}

#include "chanmgr.h"
#include "url.h"
void Commands::chan(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, {"--help",
                                                          "--name", "--genre", "--url", "--bitrate", "--type", "--ipv" /*for fetch*/ });

    if (positionals.size() < 1 || options.count("--help")) {
    ShowUsageAndReturn:
        stream.writeLine("Usage: chan ls");
        stream.writeLine("       chan show CHANNEL");
        // stream.writeLine("       chan hit");
        stream.writeLine("       chan set-url CHANNEL URL");
        stream.writeLine("       chan fetch SOURCE_URL [--name=NAME] [--genre==GENRE] [--bitrate=KBPS] [--type=TYPE] [--ipv=<4|6>]");
        return;
    }

    /*if (positionals.size() >= 1 && positionals[0] == "hit") {
        for (auto hitlist = chanMgr->hitlist; hitlist != nullptr; hitlist = hitlist->next) {
            auto chid = hitlist->info.id;
            int size = hitlist->numHits();
            stream.writeLineF("%s %d", chid.str().c_str(), size);
        }
    } else*/ if (positionals.size() >= 3 && positionals[0] == "set-url") {
        const auto designator = positionals[1];
        const auto newUrl = positionals[2];

        auto channels = pickChannels(designator);

        if (channels.size() == 0) {
            stream.writeLineF("Error: Channel not found: %s", designator.c_str());
        } else if (channels.size() > 1) {
            stream.writeLineF("'%s' is ambiguous between the following:", designator.c_str());
            for (auto ch : channels) {
                stream.writeLineF("%s %s", ch->getID().str().c_str(), ch->getName());
            }
        } else {
            auto it = channels[0];
            std::lock_guard<std::recursive_mutex>(it->lock);

            if (it->type != Channel::T_BROADCAST) {
                stream.writeLineF("Error: %s is not a broadcasting channel.", it->getID().str().c_str());
                return;
            }
            if (it->srcType != Channel::SRC_URL) {
                stream.writeLineF("Error: The source type of %s is not URL.", it->getID().str().c_str());
                return;
            }
            auto info = it->info;

            stream.writeLineF("Stopping channel %s ...", it->getID().str().c_str());
            it->thread.shutdown();
            sys->waitThread(&it->thread);
            stream.writeLine("Channel stopped.");

            sys->sleep(5000);
            stream.writeLine("Restarting channel ...");
            auto ch = chanMgr->createChannel(info);
            ch->startURL(newUrl.c_str());
            stream.writeLine("Started.");
        }
    } else if (positionals.size() >= 1 && positionals[0] == "ls") {
        for (auto it = chanMgr->channel; it; it = it->next) {
            stream.writeLineF("%s %s %s",
                              it->getID().str().c_str(),
                              it->getName(),
                              it->getStatusStr());
        }
    } else if (positionals.size() == 2 && positionals[0] == "show") {
        const auto designator = positionals[1];
        auto channels = pickChannels(designator);
        if (channels.size() == 0) {
            stream.writeLineF("Error: Channel not found: %s", designator.c_str());
        } else if (channels.size() > 1) {
            stream.writeLineF("'%s' is ambiguous between the following:", designator.c_str());
            for (auto ch : channels) {
                stream.writeLineF("%s %s", ch->getID().str().c_str(), ch->getName());
            }
        } else {
            stream.writeLine(amf0::format(channels[0]->getState()));
        }
    } else if (positionals.size() >= 1 && positionals[0] == "fetch") {
        // chan fetch "pipe:ffmpeg ... -f flv -" --name="NAME"
        //            --genre=="GENRE" --bitrate --type=FLV --ipv=4|6

        if (positionals.size() != 2) {
            stream.writeLine("Error: chan-fetch only needs one argument.");
            return;
        }
        auto curl = positionals[1]; // URL to fetch
        auto GET =
            [&](const std::string& key) -> const char* {
                if (options.count(key)) {
                    return options.at(key).c_str();
                } else {
                    return "";
                }
            };
        ChanInfo info;
        info.name    = str::truncate_utf8(str::valid_utf8(GET("--name")), 255);
        info.desc    = str::truncate_utf8(str::valid_utf8(GET("--desc")), 255);
        info.genre   = str::truncate_utf8(str::valid_utf8(GET("--genre")), 255);
        info.url     = str::truncate_utf8(str::valid_utf8(GET("--contact")), 255);
        info.bitrate = std::atoi(GET("--bitrate"));
        info.setContentType(GET("--type"));

        // ソースに接続できなかった場合もチャンネルを同定したいの
        // で、事前にチャンネルIDを設定する。
        Servent::setBroadcastIdChannelId(info, chanMgr->broadcastID);

        auto c = chanMgr->createChannel(info);
        if (c) {
            if (std::string(GET("--ipv")) == "6") {
                c->ipVersion = Channel::IP_V6;
                LOG_INFO("Channel IP version set to 6");

                // YPv6ではIPv6のポートチェックができないのでがんばる。
                servMgr->checkFirewallIPv6();
            }
            c->startURL(curl.c_str());
            stream.writeLine(info.id.str());
        } else {
            stream.writeLine("Error: failed to create channel");
            return;
        }
    } else if (positionals.size() >= 1 && positionals[0] == "stop") {
        if (positionals.size() != 2) {
            stream.writeLine("chan-stop only needs one argument.");
            return;
        }

        auto designator = positionals[1];
        auto channels = pickChannels(designator);

        if (channels.size() == 0) {
            stream.writeLineF("Error: Channel not found: %s", designator.c_str());
        } else if (channels.size() > 1) {
            stream.writeLineF("'%s' is ambiguous between the following:", designator.c_str());
            for (auto ch : channels) {
                stream.writeLineF("%s %s", ch->getID().str().c_str(), ch->getName());
            }
        } else {
            auto it = channels[0];

            stream.writeStringF("Stopping channel %s ... ", it->getID().str().c_str());
            it->thread.shutdown();
            sys->waitThread(&it->thread);
            stream.writeLine("done.");
        }
    } else {
        goto ShowUsageAndReturn;
    }
}

#include "http.h"

void Commands::get(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
    std::map<std::string, std::string> options;
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
void Commands::helo(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "-v", "--help" });

    if (positionals.size() != 1 || options.count("--help"))
    {
        stream.writeLine("Usage: helo [-v] HOST");
        stream.writeLine("Perform a PCP handshake with HOST.");
        return;
    }
    const auto target = positionals[0];

    try {
        assert(AUX_LOG_FUNC_VECTOR != nullptr);
        AUX_LOG_FUNC_VECTOR->push_back([&](LogBuffer::TYPE type, const char* msg) -> void
                                      {
                                          if (type == LogBuffer::T_ERROR)
                                              stream.writeString("Error: ");
                                          else if (type == LogBuffer::T_WARN)
                                              stream.writeString("Warning: ");
                                          stream.writeLine(msg);
                                      });
        Defer defer([]() { AUX_LOG_FUNC_VECTOR->pop_back(); });

        Host host = Host::fromString(target, DEFAULT_PORT);

        stream.writeLineF("HELO %s", host.str().c_str());

        auto sock = sys->createSocket();
        sock->setReadTimeout(30000);
        sock->open(host);
        sock->connect();

        CopyingStream cs(sock.get());
        Defer defer2([&]() {
                         if (options.count("-v")) {
                             stream.writeLineF("--- %d bytes written", (int)cs.dataWritten.size());
                             if (cs.dataWritten.size()) {
                                 stream.writeLine(str::ascii_dump(cs.dataWritten));
                                 stream.writeLine(str::hexdump(cs.dataWritten));
                             }
                             stream.writeLineF("--- %d bytes read", (int)cs.dataRead.size());
                             if (cs.dataRead.size()) {
                                 stream.writeLine(str::ascii_dump(cs.dataRead));
                                 stream.writeLine(str::hexdump(cs.dataRead));
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
        stream.writeLineF("Remote ID: %s", remoteID.str().c_str());
        stream.writeLineF("Remote agent: %s", agent.c_str());

        atom.writeInt(PCP_QUIT, PCP_ERROR_QUIT);

        sock->close();
    } catch(GeneralException& e) {
        stream.writeLineF("Error: %s", e.what());
    }
}

#include "sslclientsocket.h"
void Commands::ssl(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (positionals.size() != 0 || options.count("--help"))
    {
        stream.writeLine("Usage: ssl");
        stream.writeLine("Display the SSL server configuration.");
        return;
    }

    std::string crt, key;
    std::tie(crt, key) = SslClientSocket::getServerConfiguration();

    auto isReadable =
        [](const std::string path){
            FileStream fs;
            try {
                fs.openReadOnly(path);
                return true;
            } catch (StreamException &e) {
                return false;
            }
        };

    auto showInfo =
        [&](const std::string path) {
            stream.writeLineF("         Path: %s", path.c_str());
            stream.writeLineF("    Full path: %s", sys->realPath(path).c_str());
            stream.writeLineF("       Status: %s", isReadable(sys->realPath(path)) ? "OK, Readable" : "Cannot open!");
        };

    stream.writeLineF("SSL server: %s", (servMgr->flags.get("enableSSLServer")) ? "Enabled" : "Disabled");

    stream.writeLine("\nCertificate File");
    showInfo(crt);
    stream.writeLine("\nPrivate Key File");
    showInfo(key);
}

void Commands::flag(Stream& stream, const std::vector<std::string>& argv, std::function<bool()> cancel)
{
    std::map<std::string, std::string> options;
    std::vector<std::string> positionals;
    std::tie(options, positionals) = parse_options(argv, { "--help" });

    if (options.count("--help"))
    {
    ShowUsageAndReturn:
        stream.writeLine("Usage: flag ls");
        stream.writeLine("       flag set FLAG <true|false|default>");
        stream.writeLine("Manipulate flags.");
        return;
    }

    if (positionals.size() == 1 && positionals[0] == "ls") {
        servMgr->flags.forEachFlag([&](Flag& f)
                                   {
                                       stream.writeLineF("%-31s %s%s",
                                                         f.name.c_str(),
                                                         (f.currentValue) ? "true" : "false",
                                                         (f.currentValue == f.defaultValue) ? " (default)" : "");
                                   });
    } else if (positionals.size() == 3 && positionals[0] == "set") {
        const auto flagName = positionals[1];
        const auto newValue = positionals[2];

        try {
            Flag& f = servMgr->flags.get(flagName); // may throw std::out_of_range

            if (newValue == "true") {
                f.currentValue = true;
            } else if (newValue == "false") {
                f.currentValue = false;
            } else if (newValue == "default") {
                f.currentValue = f.defaultValue;
            } else {
                stream.writeLineF("Invalid value: %s", newValue.c_str());
                return;
            }
            stream.writeLineF("%-31s %s%s",
                              f.name.c_str(),
                              (f.currentValue) ? "true" : "false",
                              (f.currentValue == f.defaultValue) ? " (default)" : "");

            // Servent::CMD_applyflags() からのコピペ。
            peercastInst->saveSettings();
            peercast::notifyMessage(ServMgr::NT_PEERCAST, "フラグ設定を保存しました。");
            peercastApp->updateSettings();

        } catch (std::out_of_range &e) {
            stream.writeLineF("Flag not found: %s", flagName.c_str());
            return;
        }
    } else {
        goto ShowUsageAndReturn;
    }
}
