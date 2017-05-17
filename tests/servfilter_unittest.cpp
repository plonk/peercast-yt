#include <gtest/gtest.h>

#include "sstream.h"
#include "servmgr.h"

class ServFilterFixture : public ::testing::Test {
public:
    ServFilter filter;
    StringStream mem;
};

TEST_F(ServFilterFixture, initialState)
{
    ASSERT_EQ(0, filter.flags);
    ASSERT_EQ(0, filter.host.ip);
}

TEST_F(ServFilterFixture, writeVariable)
{
    mem.str("");
    filter.writeVariable(mem, "network");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "private");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "direct");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "banned");
    ASSERT_STREQ("0", mem.str().c_str());

    mem.str("");
    filter.writeVariable(mem, "ip");
    ASSERT_STREQ("0.0.0.0", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableNetwork)
{
    filter.flags |= ServFilter::F_NETWORK;
    filter.writeVariable(mem, "network");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariablePrivate)
{
    filter.flags |= ServFilter::F_PRIVATE;
    filter.writeVariable(mem, "private");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableDirect)
{
    filter.flags |= ServFilter::F_DIRECT;
    filter.writeVariable(mem, "direct");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableBanned)
{
    filter.flags |= ServFilter::F_BAN;
    filter.writeVariable(mem, "banned");
    ASSERT_STREQ("1", mem.str().c_str());
}

TEST_F(ServFilterFixture, writeVariableIP)
{
    filter.host.ip = (127<<24)|1;
    filter.writeVariable(mem, "ip");
    ASSERT_STREQ("127.0.0.1", mem.str().c_str());
}
