#include <gtest/gtest.h>

#include "env.h"

class EnvironmentFixture : public ::testing::Test {
public:
    EnvironmentFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~EnvironmentFixture()
    {
    }

    Environment env;
};

TEST_F(EnvironmentFixture, initialState)
{
    ASSERT_EQ(0, env.keys().size());
    ASSERT_TRUE(env.env());
}

TEST_F(EnvironmentFixture, constructor)
{
    extern char** environ; // Ç±ÇÃÉvÉçÉZÉXÇÃä¬ã´
    Environment e(environ);

    EXPECT_NE(0, e.size());
}

TEST_F(EnvironmentFixture, hasKey)
{
    ASSERT_FALSE(env.hasKey("my-key"));

    env.set("my-key", "");
    ASSERT_TRUE(env.hasKey("my-key"));
}

TEST_F(EnvironmentFixture, hasKey_nullCase)
{
    ASSERT_FALSE(env.hasKey(""));
}

TEST_F(EnvironmentFixture, setAndGet)
{
    ASSERT_EQ("", env.get("my-key"));

    env.set("my-key", "my-value");
    ASSERT_EQ("my-value", env.get("my-key"));
}

TEST_F(EnvironmentFixture, env_nullCase)
{
    ASSERT_EQ(NULL, env.env()[0]);
}

TEST_F(EnvironmentFixture, env)
{
    env.set("key1", "value1");
    env.set("key2", "value2");
    env.set("key3", "value3");

    char const ** e = env.env();
    ASSERT_STREQ("key1=value1", e[0]);
    ASSERT_STREQ("key2=value2", e[1]);
    ASSERT_STREQ("key3=value3", e[2]);
}

TEST_F(EnvironmentFixture, keys)
{
    env.set("key1", "value1");
    env.set("key2", "value2");
    env.set("key3", "value3");

    auto keys = env.keys();
    ASSERT_EQ(3, keys.size());
    ASSERT_EQ("key1", keys[0]);
    ASSERT_EQ("key2", keys[1]);
    ASSERT_EQ("key3", keys[2]);
}

TEST_F(EnvironmentFixture, size)
{
    env.set("key1", "value1");
    env.set("key2", "value2");
    env.set("key3", "value3");

    ASSERT_EQ(3, env.size());
}

TEST_F(EnvironmentFixture, unset)
{
    env.set("key1", "value1");
    env.set("key2", "value2");
    env.set("key3", "value3");

    ASSERT_EQ(3, env.size());

    env.unset("key2");
    ASSERT_EQ(2, env.size());
}
