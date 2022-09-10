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

static const char* data = R"%(<?xml version="1.0" encoding="utf-8"?>
<root version="100">
	<item enable="true" name="プログラミング" favorite="true" download="false" ignore="false">
		<base>
			<search>プログラ|ハッカソン</search>
			<source/>
			<normal channelName="false" genre="true" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</base>
		<and enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="true" detail="true" comment="true" artist="true" title="true" album="true"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</and>
		<not enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="false" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</not>
		<option icase="false" width="false"/>
		<bitrate enableMin="false" min="0" enableMax="false" max="0"/>
		<listener enableMin="false" min="0" enableMax="false" max="0"/>
		<relay enableMin="false" min="0" enableMax="false" max="0"/>
		<time enableMin="false" min="0" enableMax="false" max="0"/>
		<new new="false" notNew="false"/>
		<play play="false" notPlay="false" listener="false" notListener="false" relay="false" notRelay="false"/>
		<common sort="true" enableText="false" text="0" enableBg="true" bg="8454016"/>
		<favorite tab="true" all="true" exclusive="true" player="false"/>
		<download execute="true" observe="true"/>
		<ignore show="true"/>
		<notify notify="true" enableAlarm="false" alarm=""/>
	</item>
	<item enable="true" name="アスカ配信" favorite="true" download="false" ignore="false">
		<base>
			<search>アスカ|裏白|白蛇|asuka|罰白</search>
			<source/>
			<normal channelName="true" genre="true" detail="true" comment="true" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</base>
		<and enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="true" detail="true" comment="true" artist="true" title="true" album="true"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</and>
		<not enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="false" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</not>
		<option icase="true" width="true"/>
		<bitrate enableMin="false" min="0" enableMax="false" max="0"/>
		<listener enableMin="false" min="0" enableMax="false" max="0"/>
		<relay enableMin="false" min="0" enableMax="false" max="0"/>
		<time enableMin="false" min="0" enableMax="false" max="0"/>
		<new new="false" notNew="false"/>
		<play play="false" notPlay="false" listener="false" notListener="false" relay="false" notRelay="false"/>
		<common sort="true" enableText="false" text="0" enableBg="true" bg="12961023"/>
		<favorite tab="true" all="true" exclusive="true" player="false"/>
		<download execute="true" observe="true"/>
		<ignore show="true"/>
		<notify notify="true" enableAlarm="false" alarm=""/>
	</item>
	<item enable="true" name="きゃすけっと" favorite="true" download="false" ignore="false">
		<base>
			<search>きゃすけ</search>
			<source/>
			<normal channelName="true" genre="true" detail="true" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</base>
		<and enable="false">
			<search/>
			<source/>
			<normal channelName="true" genre="false" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</and>
		<not enable="false">
			<search/>
			<source/>
			<normal channelName="true" genre="false" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</not>
		<option icase="true" width="true"/>
		<bitrate enableMin="false" min="0" enableMax="false" max="0"/>
		<listener enableMin="false" min="0" enableMax="false" max="0"/>
		<relay enableMin="false" min="0" enableMax="false" max="0"/>
		<time enableMin="false" min="0" enableMax="false" max="0"/>
		<new new="false" notNew="false"/>
		<play play="false" notPlay="false" listener="false" notListener="false" relay="false" notRelay="false"/>
		<common sort="true" enableText="false" text="0" enableBg="false" bg="16777215"/>
		<favorite tab="true" all="true" exclusive="true" player="false"/>
		<download execute="true" observe="true"/>
		<ignore show="true"/>
		<notify notify="true" enableAlarm="false" alarm=""/>
	</item>
	<item enable="true" name="Peercast Gateway" favorite="true" download="false" ignore="false">
		<base>
			<search>Peercast Gateway</search>
			<source/>
			<normal channelName="false" genre="false" detail="false" comment="false" artist="true" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="true"/>
		</base>
		<and enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="true" detail="true" comment="true" artist="true" title="true" album="true"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</and>
		<not enable="false">
			<search/>
			<source/>
			<normal channelName="false" genre="false" detail="false" comment="false" artist="false" title="false" album="false"/>
			<extra ypName="false" ypUrl="false" contactUrl="false" trackContactUrl="false" type="false" status="false" id="false" tip="false"/>
		</not>
		<option icase="false" width="false"/>
		<bitrate enableMin="false" min="0" enableMax="false" max="0"/>
		<listener enableMin="false" min="0" enableMax="false" max="0"/>
		<relay enableMin="false" min="0" enableMax="false" max="0"/>
		<time enableMin="false" min="0" enableMax="false" max="0"/>
		<new new="false" notNew="false"/>
		<play play="false" notPlay="false" listener="false" notListener="false" relay="false" notRelay="false"/>
		<common sort="true" enableText="false" text="0" enableBg="true" bg="6736588"/>
		<favorite tab="true" all="true" exclusive="true" player="false"/>
		<download execute="true" observe="true"/>
		<ignore show="true"/>
		<notify notify="true" enableAlarm="false" alarm=""/>
	</item>
</root>
)%";

#include "amf0.h"

static amf0::Value convert(XML::Node *node)
{
    std::map<std::string,amf0::Value> object;

    for (int i = 1; i < node->numAttr; i++)
    {
        object[str::STR("@", node->getAttrName(i))] = node->getAttrValue(i);
    }

    std::map<std::string,std::vector<amf0::Value>> dict;
    XML::Node* child = node->child;
    while (child)
    {
        dict[child->getName()].push_back(convert(child));
        child = child->sibling;
    }

    for (auto& pair : dict)
    {
        if (pair.second.size() == 1)
            object[pair.first] = pair.second[0];
        else
            object[pair.first] = pair.second;
    }
    object["#text"] = node->getContent() ? node->getContent() : "";

    return object;
}

TEST_F(XMLFixture, read2)
{
    StringStream mem;
    mem.writeString(data);
    mem.rewind();

    XML xml;
    ASSERT_TRUE(xml.root == NULL);

    xml.read(mem);
    ASSERT_TRUE(xml.root != NULL);

    ASSERT_STREQ("root", xml.root->getName());
    ASSERT_STREQ("item", xml.root->child->getName());

    // この仕様やべえな。
    ASSERT_STREQ("item", xml.root->child->getAttrName(0));
    ASSERT_STREQ("item", xml.root->child->getAttrValue(0));

    ASSERT_STREQ("enable", xml.root->child->getAttrName(1));
    ASSERT_STREQ("true", xml.root->child->getAttrValue(1));

    ASSERT_STREQ("name", xml.root->child->getAttrName(2));
    ASSERT_STREQ("プログラミング", xml.root->child->getAttrValue(2));

    ASSERT_THROW(xml.root->child->getAttrName(1000), StreamException);
    ASSERT_THROW(xml.root->child->getAttrValue(1000), StreamException);

    //printf("%s", amf0::Value::object( { { xml.root->getName(), convert(xml.root) } }).inspect().c_str());
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
