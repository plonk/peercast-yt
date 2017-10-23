#include <gtest/gtest.h>

#include <servmgr.h>

class VariableWriterFixture : public ::testing::Test {
public:
    VariableWriterFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~VariableWriterFixture()
    {
    }
};

TEST_F(VariableWriterFixture, getVariable)
{
    ASSERT_EQ("1935", servMgr->getVariable("rtmpPort"));
    ASSERT_EQ("nonexistentVariable", servMgr->getVariable("nonexistentVariable"));
}

TEST_F(VariableWriterFixture, getVariableWithLoopCounter)
{
    ASSERT_EQ("nonexistentVariable", servMgr->getVariable("nonexistentVariable", 0));
}

