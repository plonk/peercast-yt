
#include "amf0.h"
#include "stream.h"

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

} // namespace amf0
