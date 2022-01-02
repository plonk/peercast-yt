#include <gtest/gtest.h>

#include "public.h"

class PublicControllerFixture : public ::testing::Test {
public:
    PublicControllerFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~PublicControllerFixture()
    {
    }
};

TEST_F(PublicControllerFixture, formatUptime)
{
    ASSERT_STREQ("00:00", PublicController::formatUptime(0).c_str());
    ASSERT_STREQ("00:00", PublicController::formatUptime(1).c_str());
    ASSERT_STREQ("00:00", PublicController::formatUptime(59).c_str());
    ASSERT_STREQ("00:01", PublicController::formatUptime(60).c_str());
    ASSERT_STREQ("00:59", PublicController::formatUptime(3600 - 1).c_str());
    ASSERT_STREQ("01:00", PublicController::formatUptime(3600).c_str());
    ASSERT_STREQ("99:59", PublicController::formatUptime(100 * 3600 - 1).c_str());
    ASSERT_STREQ("100:00", PublicController::formatUptime(100 * 3600).c_str());
}

TEST_F(PublicControllerFixture, acceptableLanguages_nullCase)
{
    std::vector<std::string> langs;
    langs = PublicController::acceptableLanguages("");
    ASSERT_EQ(0, langs.size());
}

TEST_F(PublicControllerFixture, acceptableLanguages1)
{
    std::vector<std::string> langs;
    langs = PublicController::acceptableLanguages("fr-CA");
    ASSERT_EQ(1, langs.size());
    ASSERT_EQ("fr-CA", langs[0]);
}

TEST_F(PublicControllerFixture, acceptableLanguages2)
{
    std::vector<std::string> langs;
    langs = PublicController::acceptableLanguages("ja,en;q=0.5");
    ASSERT_EQ(2, langs.size());
    ASSERT_EQ("ja", langs[0]);
    ASSERT_EQ("en", langs[1]);
}

TEST_F(PublicControllerFixture, acceptableLanguages3)
{
    std::vector<std::string> langs;
    langs = PublicController::acceptableLanguages("ja;q=0.5,en");
    ASSERT_EQ(2, langs.size());
    ASSERT_EQ("en", langs[0]);
    ASSERT_EQ("ja", langs[1]);
}

#include "json.hpp"
extern amf0::Value jsonToAmf(const nlohmann::json& j);
TEST_F(PublicControllerFixture, jsonToAmf)
{
    ASSERT_EQ(jsonToAmf(1).number(), 1.0);
    ASSERT_EQ(jsonToAmf(1.2).number(), 1.2);
    ASSERT_EQ(jsonToAmf("abc").string(), "abc");
    auto sa = jsonToAmf({ 1, 2, 3}).strictArray();
    ASSERT_EQ(sa.size(), 3);
    auto obj = jsonToAmf({ {"a", 1}, {"b", 2}, {"c", 3}}).object();
    ASSERT_EQ(obj.size(), 3);
    ASSERT_TRUE(jsonToAmf(nullptr).isNull());
}
