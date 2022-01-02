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

TEST_F(UptestServiceRegistryFixture, getState)
{
    auto state = r.getState();

    ASSERT_EQ(state.object().count("providers"), 1);
    ASSERT_EQ(state.object().at("providers").strictArray().size(), 0);
}

TEST_F(UptestServiceRegistryFixture, getState2)
{
    r.addURL("http://example.com/1");

    auto state = r.getState();
    ASSERT_EQ(state.at("providers").size(), 1);

    ASSERT_EQ("http://example.com/1", state.at("providers").at(0).at("url").string());
    ASSERT_EQ("Untried", state.at("providers").at(0).at("status").string());

    ASSERT_EQ("", state.at("providers").at(0).at("speed").string());
    ASSERT_EQ("", state.at("providers").at(0).at("over").string());
    ASSERT_EQ("", state.at("providers").at(0).at("checkable").string());

    r.m_providers.at(0).m_info.speed = "100";
    r.m_providers.at(0).m_info.over = "0";
    r.m_providers.at(0).m_info.checkable = "1";

    state = r.getState();

    ASSERT_EQ("100", state.at("providers").at(0).at("speed").string());
    ASSERT_EQ("0", state.at("providers").at(0).at("over").string());
    ASSERT_EQ("1", state.at("providers").at(0).at("checkable").string());
}

TEST_F(UptestServiceRegistryFixture, deleteByIndex)
{
    r.addURL("http://example.com/1");
    r.addURL("http://example.com/2");
    r.deleteByIndex(0);
    ASSERT_EQ(1, r.m_providers.size());
    ASSERT_EQ("http://example.com/2", r.m_providers[0].url);
}
