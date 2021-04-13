#include <gtest/gtest.h>

#include "pcp.h"
#include "atom.h"
#include "sstream.h"
#include "channel.h"

class PCPStreamFixture : public ::testing::Test {
public:
    PCPStreamFixture()
        : m_pcp(GnuID())
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~PCPStreamFixture()
    {
    }

    PCPStream m_pcp;
};

TEST_F(PCPStreamFixture, smallDataPacket)
{
    auto ch = std::make_shared<Channel>();
    StringStream mem(std::string({'d','a','t','a',2,0,0,0}) + std::string(2, 'A'));
    AtomStream atom(mem);
    BroadcastState bcs;
    int numc = 1;

    ASSERT_NO_THROW(m_pcp.readPktAtoms(ch, atom, numc, bcs));
}

TEST_F(PCPStreamFixture, bigDataPacket)
{
    // "g++.exe (Rev2, Built by MSYS2 project) 9.2.0" で uniform
    // initialization に問題があるらしく、以下のようやるとSEGVる。
    // PCPStream pcp{GnuID()};

    auto ch = std::make_shared<Channel>();
    StringStream mem(std::string({'d','a','t','a',0,0,10,0}) + std::string(160 * 1024, 'A'));
    AtomStream atom(mem);
    BroadcastState bcs;
    int numc = 1;

    ASSERT_THROW(m_pcp.readPktAtoms(ch, atom, numc, bcs), StreamException);
}
