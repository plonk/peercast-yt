#include <gtest/gtest.h>
#include "socket.h"

class ClientSocketFixture : public ::testing::Test {
public:
};

TEST_F(ClientSocketFixture, getIP)
{
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("localhost"));
    EXPECT_EQ((127<<24 | 1), ClientSocket::getIP("127.0.0.1"));
}
