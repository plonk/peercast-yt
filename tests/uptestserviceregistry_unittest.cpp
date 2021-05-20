#include <gtest/gtest.h>
#include "uptest.h"

class UptestServiceRegistryFixture : public ::testing::Test {
public:
    UptestServiceRegistryFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~UptestServiceRegistryFixture()
    {
    }

    UptestServiceRegistry r;
};

TEST_F(UptestServiceRegistryFixture, initialState)
{
    ASSERT_EQ(0, r.m_providers.size());
}

TEST_F(UptestServiceRegistryFixture, addURL)
{
    ASSERT_EQ(0, r.m_providers.size());
    r.addURL("http://example.com");
    ASSERT_EQ(1, r.m_providers.size());
}

TEST_F(UptestServiceRegistryFixture, getURLs)
{
    r.addURL("http://example.com/1");
    r.addURL("http://example.com/2");
    auto vec = r.getURLs();
    ASSERT_EQ("http://example.com/1", vec[0]);
    ASSERT_EQ("http://example.com/2", vec[1]);
}

TEST_F(UptestServiceRegistryFixture, clear)
{
    r.addURL("http://example.com");
    ASSERT_EQ(1, r.m_providers.size());
    r.clear();
    ASSERT_EQ(0, r.m_providers.size());
}

TEST_F(UptestServiceRegistryFixture, isIndexValid)
{
    ASSERT_FALSE(r.isIndexValid(0));
    ASSERT_FALSE(r.isIndexValid(1));

    r.addURL("http://example.com");

    ASSERT_TRUE(r.isIndexValid(0));
    ASSERT_FALSE(r.isIndexValid(1));
}

TEST_F(UptestServiceRegistryFixture, writeVariable)
{
    ASSERT_EQ("0", r.getVariable("numURLs"));
}

TEST_F(UptestServiceRegistryFixture, writeVariable_loop)
{
    r.addURL("http://example.com/1");

    ASSERT_EQ("http://example.com/1", r.getVariable("url", 0));
    ASSERT_EQ("Untried", r.getVariable("status", 0));

    ASSERT_EQ("", r.getVariable("speed", 0));
    ASSERT_EQ("", r.getVariable("over", 0));
    ASSERT_EQ("", r.getVariable("checkable", 0));

    r.m_providers[0].m_info.speed = "100";
    r.m_providers[0].m_info.over = "0";
    r.m_providers[0].m_info.checkable = "1";

    ASSERT_EQ("100", r.getVariable("speed", 0));
    ASSERT_EQ("0", r.getVariable("over", 0));
    ASSERT_EQ("1", r.getVariable("checkable", 0));
}

TEST_F(UptestServiceRegistryFixture, deleteByIndex)
{
    r.addURL("http://example.com/1");
    r.addURL("http://example.com/2");
    r.deleteByIndex(0);
    ASSERT_EQ(1, r.m_providers.size());
    ASSERT_EQ("http://example.com/2", r.m_providers[0].url);
}
