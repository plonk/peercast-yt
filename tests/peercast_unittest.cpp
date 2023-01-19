#include <gtest/gtest.h>

#include "peercast.h"
using namespace peercast;

class peercastFixture : public ::testing::Test {
public:
};

TEST_F(peercastFixture, log_escape)
{
    ASSERT_EQ("[00]", log_escape(std::string({ 0 })));
    ASSERT_EQ("[1F]", log_escape("\x1f"));
    ASSERT_EQ("", log_escape(""));
    ASSERT_EQ(" ", log_escape(" "));
    ASSERT_EQ("~", log_escape("~"));
    ASSERT_EQ("[7F]", log_escape("\x7f"));
    ASSERT_EQ("[80]", log_escape("\x80"));
    ASSERT_EQ("[FF]", log_escape("\xff"));
}
