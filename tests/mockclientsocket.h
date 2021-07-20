#include "common.h"
#include "socket.h"
#include "sstream.h"

class MockClientSocket : public ClientSocket
{
public:
    MockClientSocket()
        : localHostString("127.0.0.1")
    {}

    void            open(const Host &) override
    {
    }

    void            bind(const Host &) override
    {
    }

    void            connect() override
    {
    }

    bool            active() override
    {
        return false;
    }

    std::shared_ptr<ClientSocket> accept() override
    {
        return NULL;
    }

    std::string localHostString;
    Host            getLocalHost() override
    {
        Host host;
        host.fromStrName(localHostString.c_str(), 0);
        return host;
    }

    void    setBlocking(bool) override
    {
    }

    // virtuals from Stream
    int read(void *p, int len) override
    {
        return incoming.read(p, len);
    }

    void write(const void *p, int len) override
    {
        outgoing.write(p, len);
    }

    StringStream incoming, outgoing;
};
