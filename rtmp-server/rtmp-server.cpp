#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "sys.h"
#include "unix/usys.h"
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

}

using namespace rtmpserver;

Stream* openUri(const std::string& spec)
{
    URI uri(spec);

    if (uri.scheme() == "http")
    {
        ClientSocket* sock = sys->createSocket();
        Host host;
        host.fromStrName(uri.host().c_str(), uri.port());
        sock->open(host);
        sock->connect();
        sock->writeLineF("POST %s?%s HTTP/1.0", uri.path().c_str(), uri.query().c_str());
        sock->writeLine("");
        return sock;
    }else if (uri.scheme() == "file")
    {
        FileStream *file = new FileStream();
        file->openWriteReplace(uri.path().c_str());
        return file;
    }else if (!uri.isValid())
    {
        FileStream *file = new FileStream();
        file->openWriteReplace(spec.c_str());
        return file;
    }else
    {
        throw std::runtime_error("Unsupported protocol " + uri.scheme());
    }
}

int main(int argc, char* argv[])
{
#ifdef _UNIX
    sys = new USys();
#else // Windows
    sys = new WSys(0);
#endif

    iohelpers::test();
    Session::test();

    uint16_t rtmp_port = 1935;

    // TODO: コマンドラインから -p PORT を受け取る。

    if (argc == 1)
        die("no URL supplied");

    Host serverHost(0 /* IGNORED */, rtmp_port);
    ClientSocket* server = sys->createSocket();
    server->bind(serverHost);
    server->setBlocking(true); // accept() がブロックするように。

    while (true)
    {
        // ループブロックの最後でストリームを解放する。
        std::vector<Stream*> streams;
        Defer cb([&]() { for (auto s : streams) delete s; });

        ClientSocket* client = server->accept();

        for (int i = 1; i < argc; ++i)
            streams.push_back(openUri(argv[i]));

        StreamSplitter splitter(streams);

        FLVWriter writer(splitter);

        Session session(client, writer);
        session.run();
        client->close();
    }
    return 0;
}
