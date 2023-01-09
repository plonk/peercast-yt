#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "sys.h"

#ifdef _UNIX
#include "usys.h"
#else
#include "wsys.h"
#endif

#include "uri.h"
#include "iohelpers.h"
#include "session.h"
#include "splitter.h"
#include "defer.h"

namespace rtmpserver
{
    void die(const std::string& message)
    {
        std::cerr << message << std::endl;
        exit(1);
    }

    std::shared_ptr<Stream> openUri(const std::string& spec)
    {
        URI uri(spec);

        if (uri.scheme() == "http")
        {
            auto sock = sys->createSocket();
            Host host;
            host.fromStrName(uri.host().c_str(), uri.port());
            sock->open(host);
            sock->connect();
            sock->writeLineF("POST %s?%s HTTP/1.0", uri.path().c_str(), uri.query().c_str());
            sock->writeLine("");
            return sock;
        }else if (uri.scheme() == "file")
        {
            auto file = std::make_shared<FileStream>();
            file->openWriteReplace(uri.path().c_str());
            return file;
        }else if (!uri.isValid())
        {
            auto file = std::make_shared<FileStream>();
            file->openWriteReplace(spec.c_str());
            return file;
        }else
        {
            throw std::runtime_error("Unsupported protocol " + uri.scheme());
        }
    }

    std::map<std::string,std::string>
    optparse(int* pArgc, char* argv[])
    {
        assert(*pArgc > 0);

        std::map<std::string,std::string> opts;

        int ri = 1;
        int wi = 1;
        while (argv[ri] != nullptr)
        {
            if (argv[ri] == std::string("-p"))
            {
                if (argv[ri + 1] == nullptr)
                    throw std::runtime_error("no value for option -p");
                opts[argv[ri]] = argv[ri+1];
                ri += 2;
            }else
            {
                argv[wi++] = argv[ri++];
            }
        }
        argv[wi] = argv[ri];

        *pArgc = wi;
        return opts;
    }

    void test()
    {
        {
            const char *argv[] = { "cmd", NULL };
            int argc = 1;
            auto opts = optparse(&argc, (char**)argv);
            assert(argc == 1);
            assert(opts.size() == 0);
            assert(argv[0] == std::string("cmd"));
            assert(argv[1] == NULL);
        }

        {
            const char *argv[] = { "cmd", "url1", "-p", "9999", "url2", NULL };
            int argc = 5;
            auto opts = optparse(&argc, (char**)argv);
            assert(argc == 3);
            assert(opts.size() == 1);
            assert(opts["-p"] == std::string("9999"));
            assert(argv[0] == std::string("cmd"));
            assert(argv[1] == std::string("url1"));
            assert(argv[2] == std::string("url2"));
            assert(argv[3] == NULL);
        }

        {
            const char *argv[] = { "cmd", "url1", "-p", NULL };
            int argc = 3;
            try
            {
                optparse(&argc, (char**)argv);
                assert(false);
            }catch (std::runtime_error& e)
            {
                assert(e.what() == std::string("no value for option -p"));
            }
        }
    }
}

using namespace rtmpserver;

int main(int argc, char* argv[])
{
#ifdef _UNIX
    sys = new USys();
#else // Windows
    sys = new WSys(0);
#endif

    iohelpers::test();
    Session::test();
    rtmpserver::test();

    // コマンドラインから -p PORT を受け取る。
    std::map<std::string,std::string> opts = optparse(&argc, argv);
    unsigned int rtmp_port = opts.count("-p") ? std::stoi(opts["-p"]) : 1935;

    if (argc == 1)
        die("no URL supplied");

    Host serverHost(0 /* IGNORED */, rtmp_port);
    auto server = sys->createSocket();
    server->bind(serverHost);
    server->setBlocking(true); // accept() がブロックするように。

    while (true)
    {
        // ループブロックの最後でストリームを解放する。
        std::vector<std::shared_ptr<Stream>> streams;

        auto client = server->accept();

        for (int i = 1; i < argc; ++i)
            streams.push_back(openUri(argv[i]));

        StreamSplitter splitter(streams);

        FLVWriter writer(splitter);

        Session session(client, writer);
        try {
          session.run();
        } catch (EOFException& e)
        {
          printf("EOFException: %s\n", e.what());
        }
        client->close();
    }
    return 0;
}
