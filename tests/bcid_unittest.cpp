#include <gtest/gtest.h>

#include "servmgr.h"
#include "sstream.h"

class BCIDFixture : public ::testing::Test {
public:
    BCIDFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~BCIDFixture()
    {
    }

    StringStream mem;
    BCID bcid;
};

TEST_F(BCIDFixture, initialState)
{
    ASSERT_STREQ("00000000000000000000000000000000", bcid.id.str().c_str());
    ASSERT_STREQ("", bcid.name.cstr());
    ASSERT_STREQ("", bcid.email.cstr());
    ASSERT_STREQ("", bcid.url.cstr());
    ASSERT_EQ(true, bcid.valid);
    ASSERT_EQ(nullptr, bcid.next);
}

TEST_F(BCIDFixture, writeVariable)
{
    bcid.writeVariable(mem, "id");
    ASSERT_STREQ("00000000000000000000000000000000", mem.str().c_str());
}

TEST_F(BCIDFixture, writeVariable_name)
{
    bool written = bcid.writeVariable(mem, "name");
    ASSERT_TRUE(written);
    ASSERT_STREQ("", mem.str().c_str());
}

TEST_F(BCIDFixture, writeVariable_email)
{
    bool written = bcid.writeVariable(mem, "email");
    ASSERT_TRUE(written);
    ASSERT_STREQ("", mem.str().c_str());
}

TEST_F(BCIDFixture, writeVariable_url)
{
    bool written = bcid.writeVariable(mem, "url");
    ASSERT_TRUE(written);
    ASSERT_STREQ("", mem.str().c_str());
}

TEST_F(BCIDFixture, writeVariable_valid)
{
    bcid.writeVariable(mem, "valid");
    ASSERT_STREQ("Yes", mem.str().c_str());
}
