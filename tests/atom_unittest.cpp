#include <gtest/gtest.h>
#include "atom2.h"
#include "pcp.h"
#include "version2.h"

class AtomFixture : public ::testing::Test {
public:
    AtomFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~AtomFixture()
    {
    }
};

TEST_F(AtomFixture, test)
{
    bool sendPort = true;
    bool sendPing = true;
    bool sendBCID = true;
    GnuID sessionID("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    uint16_t port = 7144;
    GnuID broadcastID("ffffffffffffffffffffffffffffffff");

    Atom atom(PCP_HELO, {
        { PCP_HELO_AGENT, PCX_AGENT },
        { PCP_HELO_VERSION, PCP_CLIENT_VERSION },
        { PCP_HELO_SESSIONID, sessionID },
        (sendPort) ? Atom(PCP_HELO_PORT, port) : Atom(),
        (sendPing) ? Atom(PCP_HELO_PING, port) : Atom(),
        (sendBCID) ? Atom(PCP_HELO_BCID, broadcastID) : Atom()
    });

    ASSERT_EQ("hoge", atom.serialize());
}
