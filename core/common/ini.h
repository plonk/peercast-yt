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

struct Section {
    Section(const std::string& name, const std::initializer_list<Key>& keys,
          const std::string& endTag = "");
    std::string dump() const;

    std::string name;
    std::vector<Key> keys;
private:
    std::string endTag;
};

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
private:
    std::string m_representation;
};

} // namespace ini

#endif
