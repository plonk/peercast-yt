#include "ini.h"

namespace ini {

std::string dump(const Document& doc)
{
    std::string buf;

    for (auto sec = doc.begin(); sec != doc.end(); ++sec) {
        buf += sec->dump();
    }
    return buf;
}

Section::Section(const std::string& aName,
                const std::initializer_list<Key>& aKeys,
                const std::string& aEndTag)
    : name(aName),
      keys(aKeys),
      endTag(aEndTag)
{
}

std::string Section::dump() const
{
    std::string buf;

    buf += "\n[" + name + "]\n";

    for (auto it = keys.begin();
         it != keys.end();
         ++it) {
        buf += it->first + " = " + it->second.dump() + "\n";
    }

    if (!endTag.empty())
        buf += "[" + endTag + "]\n";

    return buf;
}

Value::Value(bool v)
{
    m_representation = (v) ? "Yes" : "No";
}

Value::Value(const std::atomic_bool& v)
{
    m_representation = (v.load()) ? "Yes" : "No";
}

Value::Value(const std::string &v)
{
    m_representation = v;
}

Value::Value(const String &v)
{
    m_representation = v.c_str();
}

Value::Value(const char* v)
{
    m_representation = v;
}

Value::Value(int v)
{
    m_representation = std::to_string(v);
}

Value::Value(unsigned int v)
{
    m_representation = std::to_string(v);
}

std::string Value::dump() const
{
    // バックスラッシュエスケープを実装してもよい。
    return m_representation;
}

} // namespace ini
