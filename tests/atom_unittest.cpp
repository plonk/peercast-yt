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
    Atom atom(PCP_HELO_PORT, (short) 7144);

    ASSERT_EQ(std::string({'p','o','r','t',2,0,0,0,(char)0xe8,0x1b}), atom.serialize());
}
