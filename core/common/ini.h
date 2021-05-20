#ifndef _INI_H
#define _INI_H

#include <vector>
#include <string>
#include <atomic>
#include "_string.h"

namespace ini {

struct Section;
struct Value;

typedef std::vector<Section> Document;
typedef std::pair<std::string, Value> Key;

Document parse(const std::string&);
std::string dump(const Document&);

struct Value {
    Value(const char* v);
    Value(bool);
    Value(const std::atomic_bool&);
    Value(const std::string&);
    Value(const String&);
    Value(int);
    Value(unsigned int);

    bool getBool() const;
    std::string getString() const;
    int getInt() const;

    std::string dump() const;

    bool operator==(const Value& rhs) const
    {
        return this->m_representation == rhs.m_representation;
    }
    bool operator!=(const Value& rhs) const
    {
        return !(*this == rhs);
    }
private:
    std::string m_representation;
};

struct Section {
    Section(const std::string& name = "",
            const std::initializer_list<Key>& keys = {},
            const std::string& endTag = "");
    Section(const std::string& name,
            const std::vector<Key>& keys,
            const std::string& endTag = "");
    std::string dump() const;

    std::string name;
    std::vector<Key> keys;
    std::string endTag;

    bool operator==(const Section& rhs) const
    {
        return (this->name == rhs.name &&
                this->keys == rhs.keys &&
                this->endTag == rhs.endTag);
    }
    bool operator!=(const Section& rhs) const
    {
        return !(*this == rhs);
    }
};

} // namespace ini

#endif
