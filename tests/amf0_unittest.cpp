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

TEST_F(amf0Fixture, Null)
{
    auto v = Value::null(nullptr);
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_FALSE(v.isNumber());
    ASSERT_TRUE(v.isNull());
    ASSERT_EQ("null", v.inspect());
    ASSERT_EQ(v.null(), nullptr);

    v = Value();
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_FALSE(v.isNumber());
    ASSERT_TRUE(v.isNull());
    ASSERT_EQ("null", v.inspect());
    ASSERT_EQ(v.null(), nullptr);
}

TEST_F(amf0Fixture, Number)
{
    auto v = Value::number(1.5);
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_TRUE(v.isNumber());
    ASSERT_EQ("1.5", v.inspect());
    ASSERT_EQ(1.5, v.number());
}

TEST_F(amf0Fixture, Bool_true)
{
    auto v = Value::boolean(true);
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_FALSE(v.isNumber());
    ASSERT_TRUE(v.isBool());
    ASSERT_EQ("true", v.inspect());
    ASSERT_EQ(true, v.boolean());
}

TEST_F(amf0Fixture, Bool_false)
{
    auto v = Value::boolean(false);
    ASSERT_FALSE(v.isString());
    ASSERT_FALSE(v.isObject());
    ASSERT_FALSE(v.isNumber());
    ASSERT_TRUE(v.isBool());
    ASSERT_EQ("false", v.inspect());
    ASSERT_EQ(false, v.boolean());
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

TEST_F(amf0Fixture, String_escapePrintableCharacters)
{
    Value s = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";
    // double quote and blakslash, and only these, must be escaped (see RFC 4627)
    ASSERT_EQ("\" !\\\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\"", s.inspect());
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

TEST_F(amf0Fixture, Array)
{
    auto o = Value::array({
            { "name",  "Mike" },
            { "age",   "30" },
            { "alias", "Mike" }
        });
    ASSERT_FALSE(o.isString());
    ASSERT_FALSE(o.isObject());
    ASSERT_TRUE(o.isArray());

    ASSERT_EQ("{\"age\":\"30\",\"alias\":\"Mike\",\"name\":\"Mike\"}", o.inspect());
}

TEST_F(amf0Fixture, Date)
{
    auto d = Value::date(0, 0);
    ASSERT_FALSE(d.isString());
    ASSERT_FALSE(d.isObject());
    ASSERT_FALSE(d.isArray());
    ASSERT_TRUE(d.isDate());

    ASSERT_EQ("(0.000000, 0)",
              d.inspect());
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

TEST_F(amf0Fixture, Deserializer_String_empty)
{
    amf0::Deserializer d;
    char data[3] = { 0x02, 0x00, 0x00 };
    StringStream mem;
    mem.str(std::string(data, data + sizeof(data)));
    Value v = d.readValue(mem);
    ASSERT_EQ("\"\"", v.inspect());
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
    ASSERT_EQ("{\"age\":30,\"alias\":\"Mike\",\"name\":\"Mike\"}", v.inspect());
}

TEST_F(amf0Fixture, compare_Null)
{
    ASSERT_EQ(Value::null(nullptr), Value::null(nullptr));
    ASSERT_EQ(Value::null(nullptr), nullptr);
    ASSERT_NE(Value::null(nullptr), Value::string(""));
    ASSERT_NE(Value::null(nullptr), Value::number(0));
}

TEST_F(amf0Fixture, compare_Number)
{
    ASSERT_EQ(Value::number(1.234), Value::number(1.234));
    ASSERT_EQ(Value::number(1.234), 1.234);
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
    ASSERT_EQ(Value::string("hoge"), "hoge");
    ASSERT_NE(Value::string("fuga"), Value::string("hoge"));
}

TEST_F(amf0Fixture, compare_Boolean)
{
    ASSERT_EQ(Value::boolean(true), Value::boolean(true));
    ASSERT_EQ(Value::boolean(false), Value::boolean(false));
    ASSERT_NE(Value::boolean(true), Value::boolean(false));
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

TEST_F(amf0Fixture, compare_Date)
{
    ASSERT_EQ(Value::date(0, 0), Value::date(0, 0));
    ASSERT_NE(Value::date(0, 1), Value::date(0, 0)); // timezone difference
    ASSERT_NE(Value::date(0, 0), Value::date(0.001, 0)); // one millisecond after the epoch
}

TEST_F(amf0Fixture, serialize_Date)
{
    Value d = amf0::Date(12, 34);
    StringStream mem(d.serialize());
    ASSERT_EQ(Value::date(12, 34), Deserializer().readValue(mem));
}

TEST_F(amf0Fixture, Array_ctor_initializer_list)
{
    Value arr = Value::array({ { "a", 1 }, {"b", "2"} });

    ASSERT_TRUE(arr.isArray());
    ASSERT_FALSE(arr.isStrictArray());

    ASSERT_EQ(arr.object().size(), 2);
}

TEST_F(amf0Fixture, Array_ctor_vector)
{
    std::vector<amf0::KeyValuePair> v = { { "a", 1 }, {"b", "2"} };
    Value arr = Value::array(v);
    
    ASSERT_TRUE(arr.isArray());
    ASSERT_FALSE(arr.isStrictArray());

    ASSERT_EQ(arr.object().size(), 2);
}

TEST_F(amf0Fixture, Array_size)
{
    Value arr = Value::array({ { "a", 1 }, {"b", "2"} });
    ASSERT_EQ(arr.object().size(), 2);
}

TEST_F(amf0Fixture, Array_at)
{
    Value arr = Value::array({ { "a", 1 }, {"b", "2"} });

    ASSERT_NO_THROW(arr.at("a"));
    ASSERT_THROW(arr.at("z"), std::out_of_range);

    ASSERT_EQ(arr.at("a"), 1);
    ASSERT_EQ(arr.at("b"), "2");
    ASSERT_NE(arr.at("b"), 2);
}

TEST_F(amf0Fixture, Array_count)
{
    Value v = Value::array({ {"name","joe"} });

    ASSERT_TRUE(v.isArray());
    ASSERT_FALSE(v.isObject());
    ASSERT_EQ(v.count("name"), 1);
    ASSERT_EQ(v.count("age"), 0);
}

TEST_F(amf0Fixture, Object_at)
{
    Value v({ {"name","joe"} });

    ASSERT_TRUE(v.isObject());
    ASSERT_EQ(v.at("name"), "joe");
    ASSERT_THROW(v.at("age"), std::out_of_range);
}

TEST_F(amf0Fixture, Object_count)
{
    Value v({ {"name","joe"} });

    ASSERT_TRUE(v.isObject());
    ASSERT_EQ(v.count("name"), 1);
    ASSERT_EQ(v.count("age"), 0);
}

TEST_F(amf0Fixture, StrictArray_at_size)
{
    Value v({ "one", "two", 3 });

    ASSERT_TRUE(v.isStrictArray());
    ASSERT_EQ(v.size(), 3);
    ASSERT_EQ(v.at(0), "one");
    ASSERT_EQ(v.at(1), "two");
    ASSERT_EQ(v.at(2), 3);
    ASSERT_THROW(v.at(3), std::out_of_range);
}

TEST_F(amf0Fixture, Number_at_size_count)
{
    Value v(1);

    ASSERT_THROW(v.at(0), std::runtime_error);
    ASSERT_THROW(v.at("a"), std::runtime_error);
    ASSERT_THROW(v.count("a"), std::runtime_error);
    ASSERT_THROW(v.size(), std::runtime_error);
}

TEST_F(amf0Fixture, String_at_size_count)
{
    Value v("x");

    ASSERT_THROW(v.at(0), std::runtime_error);
    ASSERT_THROW(v.at("a"), std::runtime_error);
    ASSERT_THROW(v.count("a"), std::runtime_error);
    ASSERT_THROW(v.size(), std::runtime_error);
}

TEST_F(amf0Fixture, Null_at_size_count)
{
    Value v;

    ASSERT_THROW(v.at(0), std::runtime_error);
    ASSERT_THROW(v.at("a"), std::runtime_error);
    ASSERT_THROW(v.count("a"), std::runtime_error);
    ASSERT_THROW(v.size(), std::runtime_error);
}
