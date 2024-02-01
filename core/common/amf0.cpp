
#include "amf0.h"
#include "stream.h"

#include <iomanip> // setprecision
#include <sstream> // stringstream

namespace amf0 {

bool Deserializer::readBool(Stream &in)
{
    return in.readChar() != 0;
}

int32_t Deserializer::readInt32(Stream &in)
{
    return (in.readChar() << 24) | (in.readChar() << 16) | (in.readChar() << 8) | (in.readChar());
}

int16_t Deserializer::readInt16(Stream& in)
{
    return (in.readChar() << 8) | (in.readChar());
}

std::string Deserializer::readString(Stream &in)
{
    int len = (in.readChar() << 8) | (in.readChar());
    return in.read(len);
}

double Deserializer::readDouble(Stream &in)
{
    double number;
    char* data = reinterpret_cast<char*>(&number);
    for (int i = 8; i > 0; i--) {
        char c = in.readChar();
        *(data + i - 1) = c;
    }
    return number;
}

std::vector<KeyValuePair> Deserializer::readObject(Stream &in)
{
    std::vector<KeyValuePair> list;
    while (true)
    {
        std::string key = readString(in);
        if (key == "")
            break;

        list.push_back({key, readValue(in)});
    }
    in.readChar(); // OBJECT_END
    return list;
}

Value Deserializer::readValue(Stream &in)
{
    char type = in.readChar();

    switch (type)
    {
    case AMF_NUMBER:
    {
        return Value::number(readDouble(in));
    }
    case AMF_STRING:
    {
        return Value::string(readString(in));
    }
    case AMF_OBJECT:
    {
        return Value::object(readObject(in));
    }
    case AMF_BOOL:
    {
        return Value::boolean(readBool(in));
    }
    case AMF_ARRAY:
    {
        readInt32(in); // length
        return Value::array(readObject(in));
    }
    case AMF_STRICTARRAY:
    {
        int len = readInt32(in);
        std::vector<Value> list;
        for (int i = 0; i < len; i++) {
            list.push_back(readValue(in));
        }
        return Value::strictArray(list);
    }
    case AMF_NULL:
    {
        return Value(nullptr);
    }
    case AMF_DATE:
    {
        double unixTime = readDouble(in);
        uint16_t timeZone = readInt16(in);
        return Value::date(unixTime, timeZone);
    }
    default:
        throw std::runtime_error("unknown AMF value type " + std::to_string(type));
    }
}

std::string Value::inspect() const
{
    switch (m_type)
    {
    case kNumber:
    {
        std::stringstream ss;
        ss << std::setprecision(std::numeric_limits<double>::max_digits10) << m_number;
        return ss.str();
    }
    case kObject:
    case kArray:
    {
        std::string buf = "{";
        bool first = true;
        for (auto pair : m_object)
        {
            if (!first)
                buf += ",";
            first = false;
            buf += string(pair.first).inspect() + ":" + pair.second.inspect();
        }
        buf += "}";
        return buf;
    }
    case kString:
        return str::json_inspect(m_string);
    case kNull:
        return "null";
    case kBool:
        return (m_bool) ? "true" : "false";
    case kDate:
        return "(" + std::to_string(m_date.unixTime) + ", " + std::to_string(m_date.timezone) + ")";
    case kStrictArray:
    {
        std::string buf = "[";
        bool first = true;
        for (auto elt : m_strict_array)
        {
            if (!first)
                buf += ",";
            first = false;
            buf += elt.inspect();
        }
        buf += "]";
        return buf;
    }
    default:
        throw std::runtime_error(str::format("inspect: unknown type %d", m_type));
    }
}

std::string format(const amf0::Value& value, int allowance, int indent)
{
    if (allowance <= 0 || value.inspect().size() <= allowance) {
        return value.inspect();
    } else if (value.isStrictArray()) {
        std::string out = "[\n";
        bool firstTime = true;
        for (const auto& elt : value.strictArray()) {
            if (!firstTime) {
                out += ",\n";
            }
            out += str::repeat(" ", indent + 2) + format(elt, allowance - indent, indent + 2);
            firstTime = false;
        }
        out += "\n" + str::repeat(" ", indent) + "]";
        return out;
    } else if (value.isObject() || value.isArray()) {
        std::string out = "{\n";
        bool firstTime = true;
        for (const auto& pair : value.object()) {
            if (!firstTime) {
                out += ",\n";
            }
            auto key = amf0::Value::string(pair.first).inspect();
            out += str::repeat(" ", indent + 2) + key + ": " + format(pair.second, allowance - indent - key.size() - 2 - 1, indent + 2);
            firstTime = false;
        }
        out += "\n" + str::repeat(" ", indent) + "}";
        return out;
    } else {
        return value.inspect();
    }
}

} // namespace amf0
