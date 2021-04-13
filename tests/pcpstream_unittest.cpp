#include <gtest/gtest.h>

class PCPStreamFixture : public ::testing::Test {
public:
    PCPStreamFixture()
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
};

#include "pcp.h"
#include "atom.h"
#include "sstream.h"
#include "channel.h"

TEST_F(PCPStreamFixture, bigPacket)
{
    PCPStream pcp{GnuID()};
    auto ch = std::make_shared<Channel>();
    StringStream mem(std::string({'d','a','t','a',0,0,10,0,0,0,0,0}) + std::string(160 * 1024, 'A'));
    AtomStream atom(mem);
    BroadcastState bcs;
    int numc = 1;

    try {
        pcp.readPktAtoms(ch, atom, numc, bcs);
    } catch (GeneralException& e ) {
    }
    ASSERT_TRUE(true); // とりあえずクラッシュしなければいい。
}
