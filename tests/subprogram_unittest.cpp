#include <gtest/gtest.h>

#include "subprog.h"

class SubprogramFixture : public ::testing::Test {
public:
    SubprogramFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~SubprogramFixture()
    {
    }
};

#ifndef WIN32
TEST_F(SubprogramFixture, echo)
{
    Environment env;
    Subprogram prog("/bin/echo");

    ASSERT_TRUE( prog.start({"fuga"}, env) );
    auto& input = prog.inputStream();
    std::string buf;
    try {
        while (true)
        {
            buf += input.readChar();
        }
    } catch (StreamException&)
    {
    }
    int status;
    ASSERT_TRUE( prog.wait(&status) );
    EXPECT_EQ(0, status);
    EXPECT_EQ(buf, "fuga\n");
}

TEST_F(SubprogramFixture, cat)
{
    Environment env;
    Subprogram prog("/bin/cat");

    ASSERT_TRUE( prog.start({}, env) );
    prog.outputStream().writeString("hoge");
    prog.outputStream().close();
    std::string buf;
    while (!prog.inputStream().eof())
    {
        buf += prog.inputStream().readChar();
    }
    int status;
    ASSERT_TRUE( prog.wait(&status) );
    EXPECT_EQ(0, status);
    EXPECT_EQ(buf, "hoge");
}
#endif
