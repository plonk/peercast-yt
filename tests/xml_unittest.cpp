#include <gtest/gtest.h>

#include "dmstream.h"
#include "xml.h"

class XMLFixture : public ::testing::Test {
public:
    XMLFixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~XMLFixture()
    {
    }
};

TEST_F(XMLFixture, read)
{
    DynamicMemoryStream mem;
    mem.writeString("<br/>");
    mem.rewind();

    XML xml;
    ASSERT_TRUE(xml.root == NULL);

    xml.read(mem);
    ASSERT_TRUE(xml.root != NULL);

    ASSERT_STREQ("br", xml.root->getName());
}

// タグ名はフォーマット文字列として解釈されてはならない。
TEST_F(XMLFixture, readCrash)
{
    DynamicMemoryStream mem;
    mem.writeString("<%s%s%s%s%s%s%s%s%s%s%s%s/>");
    mem.rewind();

    XML xml;
    xml.read(mem);

    ASSERT_TRUE(xml.root != NULL);
    ASSERT_STREQ("%s%s%s%s%s%s%s%s%s%s%s%s", xml.root->getName());
}
