#include <gtest/gtest.h>

#include "template.h"
#include "http.h"

class HTTPRequestScopeFixture : public ::testing::Test {
public:
    HTTPRequestScopeFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~HTTPRequestScopeFixture()
    {
    }
};

TEST_F(HTTPRequestScopeFixture, getVariable_path)
{
    HTTPRequest req("GET", "/hoge", "HTTP/1.0", { {"Host","254.254.254.254"} });
    HTTPRequestScope scope(req);

    ASSERT_EQ("/hoge", scope.getVariable("request.path", 0));
}

TEST_F(HTTPRequestScopeFixture, getVariable_withHost)
{
    HTTPRequest req("GET", "/hoge", "HTTP/1.0", { {"Host","254.254.254.254"} });
    HTTPRequestScope scope(req);
    ASSERT_EQ("254.254.254.254", scope.getVariable("request.host", 0));
}

TEST_F(HTTPRequestScopeFixture, getVariable_withoutHost)
{
    HTTPRequest req("GET", "/hoge", "HTTP/1.0", {});
    HTTPRequestScope scope(req);

    ASSERT_EQ("127.0.0.1:7144", scope.getVariable("request.host", 0));
}
