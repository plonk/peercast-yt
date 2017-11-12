#ifndef _AMF0_H
#define _AMF0_H
// ------------------------------------------------
// File: amf0.h
// Desc:
//      AMF0 serialization and deserialization.  This file is based on
//      flv.h.
//
// (c) 2002-3 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#include <string>
#include <vector>
#include <map>
#include "stream.h"
#include <stdexcept>
#include "str.h"

namespace amf0
{
    static const int AMF_NUMBER      = 0x00;
    static const int AMF_BOOL        = 0x01;
    static const int AMF_STRING      = 0x02;
    static const int AMF_OBJECT      = 0x03;
    static const int AMF_MOVIECLIP   = 0x04;
    static const int AMF_NULL        = 0x05;
    static const int AMF_UNDEFINED   = 0x06;
    static const int AMF_REFERENCE   = 0x07;
    static const int AMF_ARRAY       = 0x08;
    static const int AMF_OBJECT_END  = 0x09;
    static const int AMF_STRICTARRAY = 0x0a;
    static const int AMF_DATE        = 0x0b;
    static const int AMF_LONG_STRING = 0x0c;

    class Value;
    typedef std::pair<std::string,Value> KeyValuePair;

    struct Date
    {
        Date()
            : unixTime(0), timezone(0) {}
        Date(double aUnixTime, uint16_t aTimezone)
            : unixTime(aUnixTime), timezone(aTimezone) {}

        bool operator == (const Date& rhs) const
        {
            return unixTime == rhs.unixTime && timezone == rhs.timezone;
        }

        double unixTime;
        uint16_t timezone;
    };

    class Value
    {
    public:
        enum {
            kNumber,
            kObject,
            kString,
            kBool,
            kArray,
            kStrictArray,
            kNull,
            kDate,
        };

        Value() : m_type(kNull), m_number(0.0), m_bool(false) {}
        Value(std::nullptr_t p) : Value() {}
        Value(double d) : m_type(kNumber), m_number(d), m_bool(false) {}
        Value(const char* s) : m_type(kString), m_number(0.0), m_bool(false), m_string(s) {}
        Value(bool b) : m_type(kBool), m_number(0.0), m_bool(b) {}
        Value(int n) : m_type(kNumber), m_number(n), m_bool(false) {}
        Value(std::initializer_list<KeyValuePair> l) : m_type(kObject), m_number(0.0), m_bool(false)
        {
            for (auto pair: l)
                m_object[pair.first] = pair.second;
        }
        Value(const Date& d) : m_type(kDate), m_number(0.0), m_bool(false), m_date(d) {}

        static Value number(double d)
        { Value v; v.m_type = kNumber; v.m_number = d; return v; }

        static Value string(const std::string& s)
        { Value v; v.m_type = kString; v.m_string = s; return v; }

        static Value boolean(bool b)
        { Value v; v.m_type = kBool; v.m_bool = b; return v; }

        // object
        static Value object(std::initializer_list<KeyValuePair> l)
        {
            Value v;
            v.m_type = kObject;
            for (auto pair: l)
                v.m_object[pair.first] = pair.second;
            return v;
        }
        static Value object(const std::vector<KeyValuePair>& l)
        {
            Value v;
            v.m_type = kObject;
            for (auto pair: l)
                v.m_object[pair.first] = pair.second;
            return v;
        }

        // ECMA array
        static Value array(std::initializer_list<KeyValuePair> l)
        {
            Value v;
            v.m_type = kArray;
            for (auto pair: l)
                v.m_object[pair.first] = pair.second;
            return v;
        }
        static Value array(const std::vector<KeyValuePair>& l)
        {
            Value v;
            v.m_type = kArray;
            for (auto pair: l)
                v.m_object[pair.first] = pair.second;
            return v;
        }

        // strict array
        static Value strictArray(std::initializer_list<Value> l)
        { Value v; v.m_type = kStrictArray; v.m_strict_array = l; return v; }
        static Value strictArray(const std::vector<Value>& l)
        { Value v; v.m_type = kStrictArray; v.m_strict_array = l; return v; }

        static Value date(const Date& d)
        { return Value(d); }
        static Value date(double unixTime, uint16_t timezone = 0)
        { Value v; v.m_type = kDate; v.m_date = Date(unixTime, timezone); return v; }

        bool isNumber() { return m_type == kNumber; }
        bool isObject() { return m_type == kObject; }
        bool isString() { return m_type == kString; }
        bool isBool() { return m_type == kBool; }
        bool isArray() { return m_type == kArray; }
        bool isStrictArray() { return m_type == kStrictArray; }
        bool isNull() { return m_type == kNull; }
        bool isDate() { return m_type == kDate; }

        bool operator == (const Value& rhs) const
        {
            if (this->m_type != rhs.m_type)
                return false;

            switch (this->m_type)
            {
            case kNumber:
                return this->m_number == rhs.m_number;
            case kObject:
            case kArray:
                return this->m_object == rhs.m_object;
            case kString:
                return this->m_string == rhs.m_string;
            case kStrictArray:
                return this->m_strict_array == rhs.m_strict_array;
            case kDate:
                return this->m_date == rhs.m_date;
            default:
                throw std::runtime_error("unknown AMF value type " + std::to_string(m_type));
            }
        }

        bool operator != (const Value& rhs) const
        { return !(*this == rhs); }

        std::string serialize() const
        {
            std::string b;
            switch (m_type)
            {
            case kNumber:
            {
                b += AMF_NUMBER;
                char* p = (char*) &m_number;
                for (int i = 7; i >= 0; i--)
                    b += p[i];
                return b;
            }
            case kObject:
            {
                b += AMF_OBJECT;
                for (auto pair : m_object)
                {
                    b += string(pair.first).serialize().substr(1);
                    b += pair.second.serialize();
                }
                b += '\0';
                b += '\0';
                b += AMF_OBJECT_END;
                return b;
            }
            case kString:
            {
                b += AMF_STRING;
                if (m_string.size() >= 65536)
                    throw std::runtime_error("string too long");
                b += (0xff00 & m_string.size()) >> 8;
                b += 0xff & m_string.size();
                b += m_string;
                return b;
            }
            case kNull:
            {
                b += AMF_NULL;
                return b;
            }
            case kDate:
            {
                b += AMF_DATE;
                auto p = reinterpret_cast<const char*>(&m_date.unixTime);
                for (int i = 7; i >= 0; i--)
                    b += p[i];
                p = reinterpret_cast<const char*>(&m_date.timezone);
                for (int i = 1; i >= 0; i--)
                    b += p[i];
                return b;
            }
            default:
                throw std::runtime_error("serialize: unknown type");
            }
        }

        std::string inspect()
        {
            switch (m_type)
            {
            case kNumber:
                return std::to_string(m_number);
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
                return str::inspect(m_string);
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

        double number()
        {
            if (!isNumber()) throw std::runtime_error("not a number");
            return m_number;
        }

        std::string string()
        {
            if (!isString()) throw std::runtime_error("not a string");
            return m_string;
        }

        bool boolean()
        {
            if (!isBool()) throw std::runtime_error("not a boolean");
            return m_bool;
        }

        std::map<std::string,Value>& object()
        {
            if (!isObject() && !isArray()) throw std::runtime_error("not an object or an array (%d)");
            return m_object;
        }

        std::vector<Value>& strictArray()
        {
            if (!isStrictArray()) throw std::runtime_error("not a strict array");
            return m_strict_array;
        }

        std::nullptr_t null()
        {
            if (!isNull()) throw std::runtime_error("not null");
            return nullptr;
        }

        Date date()
        {
            if (!isDate()) throw std::runtime_error("not a date");
            return m_date;
        }

        int                         m_type;
        double                      m_number;
        bool                        m_bool;
        std::string                 m_string;
        std::map<std::string,Value> m_object;
        std::vector<Value>          m_strict_array;
        Date                        m_date;
    };

    class Deserializer
    {
    public:
        bool readBool(Stream &in)
        {
            return in.readChar() != 0;
        }

        int32_t readInt32(Stream &in)
        {
            return (in.readChar() << 24) | (in.readChar() << 16) | (in.readChar() << 8) | (in.readChar());
        }

        int16_t readInt16(Stream& in)
        {
            return (in.readChar() << 8) | (in.readChar());
        }

        std::string readString(Stream &in)
        {
            int len = (in.readChar() << 8) | (in.readChar());
            return in.read(len);
        }

        double readDouble(Stream &in)
        {
            double number;
            char* data = reinterpret_cast<char*>(&number);
            for (int i = 8; i > 0; i--) {
                char c = in.readChar();
                *(data + i - 1) = c;
            }
            return number;
        }

        std::vector<KeyValuePair> readObject(Stream &in)
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

        Value readValue(Stream &in)
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
                int len = readInt32(in); // length
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
    };
} // namespace amf
#endif
