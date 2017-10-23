#include <gtest/gtest.h>

#include "sstream.h"
#include "xml.h"

class XMLFixture : public ::testing::Test {
public:
};

TEST_F(XMLFixture, read)
{
    StringStream mem;
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
    StringStream mem;
    mem.writeString("<%s%s%s%s%s%s%s%s%s%s%s%s/>");
    mem.rewind();

    XML xml;
    xml.read(mem);

    ASSERT_TRUE(xml.root != NULL);
    ASSERT_STREQ("%s%s%s%s%s%s%s%s%s%s%s%s", xml.root->getName());
}

TEST_F(XMLFixture, read_yp4g)
{
    std::string input =
        "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\r\n"
        "<yp4g>\r\n"
        "\t<yp name=\"SP\" />\r\n"
        "\t<host ip=\"254.254.254.254\" port_open=\"1\" speed=\"408\" over=\"0\" />\r\n"
        "\t<uptest checkable=\"1\" remain=\"0\" />\r\n"
        "\t<uptest_srv addr=\"bayonet.ddo.jp\" port=\"444\" object=\"/sp/uptest.cgi\" post_size=\"250\" limit=\"3000\" interval=\"15\" enabled=\"1\" />\r\n"
        "</yp4g>";
    StringStream mem;
    mem.str(input);

    XML xml;
    xml.read(mem);
    ASSERT_STREQ("yp4g", xml.root->getName());
    ASSERT_STREQ("SP", xml.findNode("yp")->findAttr("name"));
    ASSERT_STREQ("408", xml.findNode("host")->findAttr("speed"));
    ASSERT_EQ(1, xml.findNode("uptest")->findAttrInt("checkable"));
    ASSERT_STREQ("1", xml.findNode("uptest_srv")->findAttr("enabled"));
}

TEST_F(XMLFixture, content)
{
    StringStream mem("<b>hoge&amp;fuga</b>");

    XML xml;
    xml.read(mem);
    ASSERT_STREQ("b", xml.root->getName());
    ASSERT_STREQ("hoge&amp;fuga", xml.root->getContent()); // 文字実体参照の解決はされない
}

TEST_F(XMLFixture, emptyContent)
{
    StringStream mem("<b></b>");

    XML xml;
    xml.read(mem);
    ASSERT_STREQ("b", xml.root->getName());
    ASSERT_EQ(nullptr, xml.root->getContent()); // 空文字列ではなくNULLが返る
}

TEST_F(XMLFixture, attribute)
{
    StringStream mem("<b attr=\"a&amp;b\"></b>");

    XML xml;
    xml.read(mem);
    ASSERT_STREQ("b", xml.root->getName());
    ASSERT_STREQ("a&amp;b", xml.root->findAttr("attr")); // 文字実体参照の解決はされない
}

TEST_F(XMLFixture, attribute_singleQuotes)
{
    StringStream mem("<b id=\'123\'></b>"); // 単一引用符は動かない

    XML xml;
    ASSERT_THROW(xml.read(mem), StreamException);
}

TEST_F(XMLFixture, node)
{
    XML::Node node("hoge");

    ASSERT_EQ(nullptr, node.contData);
    //ASSERT_EQ(nullptr, node.attrData);
    //ASSERT_EQ(nullptr, node.attr);
    ASSERT_EQ(1, node.numAttr);
    ASSERT_EQ(nullptr, node.child);
    ASSERT_EQ(nullptr, node.parent);
    ASSERT_EQ(nullptr, node.sibling);
}
