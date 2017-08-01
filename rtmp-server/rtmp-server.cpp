#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <assert.h>

#include "sys.h"
#include "unix/usys.h"
#include "uri.h"
#include "iohelpers.h"
#include "session.h"

namespace rtmpserver
{
    void die(const std::string& message)
    {
        std::cerr << message << std::endl;
        exit(1);
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

    uint16_t rtmp_port = 1935;

    // TODO: コマンドラインから -p PORT を受け取る。

    if (argc == 1)
        die("no URL supplied");

    URI uri(argv[1]);

    Host serverHost(0 /* IGNORED */, rtmp_port);
    ClientSocket* server = sys->createSocket();
    server->bind(serverHost);
    server->setBlocking(true); // accept() がブロックするように。

    while (true)
    {
        ClientSocket* client = server->accept();

        assert(client != nullptr);

        FileStream file;
        file.openWriteReplace("out.flv");
        FLVWriter writer(file);

        Session session(client, uri, writer);
        session.run();
        client->close();
    }
    return 0;
}
