#include "common.h"
#include "socket.h"
#include "dmstream.h"

class MockClientSocket : public ClientSocket
{
public:
    void            open(Host &) override
    {
    }

    void            bind(Host &) override
    {
    }

    void            connect() override
    {
    }

    bool            active() override
    {
        return false;
    }

    ClientSocket    *accept() override
    {
        return NULL;
    }

    Host            getLocalHost() override
    {
        Host host;
        host.fromStrIP("127.0.0.1", 7144);
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

    DynamicMemoryStream incoming, outgoing;
};
