#include <gtest/gtest.h>

#include "template.h"
#include "dmstream.h"

class TemplateFixture : public ::testing::Test {
public:
    TemplateFixture()
        : temp("")
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~TemplateFixture()
    {
    }

    Template temp;
};

TEST_F(TemplateFixture, readVariable)
{
    DynamicMemoryStream in, out;

    in.writeString("servMgr.version}");
    in.rewind();
    temp.readVariable(in, &out, 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariable)
{
    DynamicMemoryStream out;

    temp.writeVariable(out, "servMgr.version", 0);
    ASSERT_STREQ("v0.1218", out.str().substr(0,7).c_str());
}

TEST_F(TemplateFixture, writeVariableNonExistentVariable)
{
    DynamicMemoryStream out;

    temp.writeVariable(out, "hoge.fuga.piyo", 0);
    ASSERT_STREQ("hoge.fuga.piyo", out.str().c_str());
}

TEST_F(TemplateFixture, getIntVariable)
{
    ASSERT_EQ(0, temp.getIntVariable("servMgr.version", 0));
}

TEST_F(TemplateFixture, getBoolVariable)
{
    ASSERT_EQ(false, temp.getIntVariable("servMgr.version", 0));
}


TEST_F(TemplateFixture, readTemplate)
{
    DynamicMemoryStream in, out;
    bool res;

    in.writeString("hoge");
    in.rewind();
    res = temp.readTemplate(in, &out, 0);
    ASSERT_FALSE(res);
    ASSERT_STREQ("hoge", out.str().c_str());
}
