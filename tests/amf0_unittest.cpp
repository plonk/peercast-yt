#include "amf0.h"
#include "sstream.h"

#include <gtest/gtest.h>

using namespace amf0;

class amf0Fixture : public ::testing::Test {
public:
    amf0Fixture()
    {
    }

    void SetUp()
    {
    }

    void TearDown()
    {
    }

    ~amf0Fixture()
    {
    }
};

TEST_F(amf0Fixture, compile)
{
    ASSERT_TRUE(true);
}

TEST_F(amf0Fixture, Number)
{
    auto v = Value::number(1.5);
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_TRUE(v.isNumber());
    ASSERT_EQ("1.500000", v.inspect());
    ASSERT_EQ(1.5, v.number());
}

TEST_F(amf0Fixture, String)
{
    Value s = "hoge";
    ASSERT_EQ(Value::kString, s.m_type);
    ASSERT_TRUE(s.isString());
    ASSERT_FALSE(s.isObject());
    ASSERT_EQ("\"hoge\"", s.inspect());
    ASSERT_EQ("hoge", s.string());
}

TEST_F(amf0Fixture, Object)
{
    auto o = Value::object({
            { "name",  "Mike" },
            { "age",   "30" },
            { "alias", "Mike" }
        });
    ASSERT_FALSE(o.isString());
    ASSERT_TRUE(o.isObject());

    ASSERT_EQ("{\"age\":\"30\",\"alias\":\"Mike\",\"name\":\"Mike\"}", o.inspect());
}

TEST_F(amf0Fixture, Deserializer_String)
{
    amf0::Deserializer d;
    char data[7] = { 0x02, 0x00, 0x04, 'h', 'o', 'g', 'e' };
    StringStream mem;
    mem.str(std::string(data, data + sizeof(data)));
    Value v = d.readValue(mem);
    ASSERT_EQ("\"hoge\"", v.inspect());
}

TEST_F(amf0Fixture, Deserializer_Object)
{
    amf0::Deserializer d;
    char data[45] = { 0x03, 0x00, 0x04, 0x6e, 0x61, 0x6d, 0x65, 0x02, 0x00, 0x04, 0x4d, 0x69, 0x6b, 0x65, 0x00,
                      0x03, 0x61, 0x67, 0x65, 0x00, 0x40, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05,
                      0x61, 0x6c, 0x69, 0x61, 0x73, 0x02, 0x00, 0x04, 0x4d, 0x69, 0x6b, 0x65, 0x00, 0x00, 0x09 };
    StringStream mem;
    mem.str(std::string(data, data + sizeof(data)));
    Value v = d.readValue(mem);
    ASSERT_EQ("{\"age\":30.000000,\"alias\":\"Mike\",\"name\":\"Mike\"}", v.inspect());
}

TEST_F(amf0Fixture, compare_Number)
{
    ASSERT_EQ(Value::number(1.234), Value::number(1.234));
    ASSERT_NE(Value::number(1.234), Value::number(1.235));
}

TEST_F(amf0Fixture, serialize_Number)
{
    auto n = Value::number(1.234);
    StringStream mem(n.serialize());
    ASSERT_EQ(Value::number(1.234), Deserializer().readValue(mem));
}

TEST_F(amf0Fixture, compare_String)
{
    ASSERT_EQ(Value::string("hoge"), Value::string("hoge"));
    ASSERT_NE(Value::string("fuga"), Value::string("hoge"));
}

TEST_F(amf0Fixture, serialize_String)
{
    auto s = Value::string("hoge");
    ASSERT_EQ(Value::kString, s.m_type);
    StringStream mem(s.serialize());
    ASSERT_EQ(Value::string("hoge"), Deserializer().readValue(mem));
}

TEST_F(amf0Fixture, serialize_Object)
{
    Value o = {{"hoge", 123}};
    StringStream mem(o.serialize());
    ASSERT_EQ(Value::object({{"hoge", Value::number(123)}}), Deserializer().readValue(mem));
}
