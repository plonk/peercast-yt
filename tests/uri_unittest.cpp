// boost::network::uri::uri クラスのテスト。

#include <boost/network/protocol/http/client.hpp>
#include <gtest/gtest.h>

using namespace boost::network::uri;

class uriFixture : public ::testing::Test {
public:
    uriFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~uriFixture()
    {
    }
};

TEST_F(uriFixture, httpScheme)
{
    uri u("http://www.example.com");

    ASSERT_TRUE(u.is_valid());
    ASSERT_STREQ("http", u.scheme().c_str());
    ASSERT_STREQ("www.example.com", u.host().c_str());
    ASSERT_STREQ("", u.port().c_str()); // 自動的に "80" にはならない。
    ASSERT_STREQ("", u.path().c_str()); // パスが省略されている場合は "/" にはならない。
}

TEST_F(uriFixture, httpSchemeWithPortQueryAndFragment)
{
    uri u("http://localhost:7144/html/en/index.html?name=%E4%BA%88%E5%AE%9A%E5%9C%B0#top");

    ASSERT_TRUE(u.is_valid());
    ASSERT_STREQ("http", u.scheme().c_str());
    ASSERT_STREQ("localhost", u.host().c_str());
    ASSERT_STREQ("7144", u.port().c_str());
    ASSERT_STREQ("/html/en/index.html", u.path().c_str());
    ASSERT_STREQ("name=%E4%BA%88%E5%AE%9A%E5%9C%B0", u.query().c_str()); // 自動的に unescape はされない。
    ASSERT_STREQ("top", u.fragment().c_str());
}

TEST_F(uriFixture, ftpScheme)
{
    uri u("ftp://user:pass@localhost/pub/file.bin");
    ASSERT_TRUE(u.is_valid());
    ASSERT_STREQ("ftp", u.scheme().c_str());
    ASSERT_STREQ("user:pass", u.user_info().c_str());
    ASSERT_STREQ("localhost", u.host().c_str());
    ASSERT_STREQ("/pub/file.bin", u.path().c_str());
}

TEST_F(uriFixture, invalidUri)
{
    uri u("hoge");

    ASSERT_FALSE(u.is_valid());
    EXPECT_STREQ("hoge", u.scheme().c_str()); // おかしいけど、こうなる。
}

TEST_F(uriFixture, emptyUri)
{
    ASSERT_NO_THROW(uri(""));

    uri u("");
    ASSERT_FALSE(u.is_valid());
}

TEST_F(uriFixture, mailtoScheme)
{
    uri u("mailto:webmaster@example.com");
    ASSERT_TRUE(u.is_valid());
    ASSERT_STREQ("mailto", u.scheme().c_str());
    ASSERT_STREQ("webmaster@example.com", u.path().c_str());
    ASSERT_STREQ("", u.host().c_str());
}
