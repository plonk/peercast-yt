#include <gtest/gtest.h>
#include "usocket.h"

class ClientSocketFixture : public ::testing::Test {
public:
    ClientSocketFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~ClientSocketFixture()
    {
    }
};

TEST_F(ClientSocketFixture, getIP)
{
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("localhost"));
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("127.0.0.1"));
}
