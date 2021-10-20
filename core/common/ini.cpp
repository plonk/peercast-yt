#include "ini.h"
#include "str.h"
#include "regexp.h"
#include <cctype>

static std::string trim(std::string str)
{
    int i = 0;
    while (std::isspace(str[i]))
        i++;

    int j = str.size();
    while (std::isspace(str[j - 1]) && j > 0)
        j--;

    if (j < i)
        return "";
    else
        return std::string(str.begin() + i, str.begin() + j);
}

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

Section::Section(const std::string& aName,
                const std::vector<Key>& aKeys,
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

Document parse(const std::string& str)
{
    static Regexp kRegexpSection("^\\s*\\[(\\w+)\\]\\s*$");
    static Regexp kRegexpAssignment("\\s*(\\w+)\\s*=\\s*(.*)$");
    Document doc;

    auto lines = str::to_lines(str);
    Section section;
    size_t i = 0;
    while (i < lines.size()) {
        std::vector<std::string> v;
        auto line_i = trim(lines[i]);

        if (line_i == "") {
            i++;
        } else if ((v = kRegexpSection.exec(line_i)).size()) {
            if (v[1] == "End") {
                if (section.name == "") {
                    auto msg = str::format("Syntax error in line %lu: Stray 'End'", i + 1);
                    throw FormatException(msg.c_str());
                } else {
                    section.endTag = "End";
                    doc.push_back(section);
                    section = {};
                }
                i++;
                break;
            } else {
                if (section.name != "")
                    doc.push_back(section);

                std::string sectionName = v[1];
                std::vector<Key> keys;

                for (size_t j = i + 1; j < lines.size(); j++) {
                    auto line_j = trim(lines[j]);

                    if ((v = kRegexpAssignment.exec(line_j)).size()) {
                        keys.emplace_back(trim(v[1]), trim(v[2]));
                    } else {
                        i = j;
                        break;
                    }
                }
                section = Section(sectionName, keys);
            }
        } else {
            auto msg = str::format("Syntax error in line %lu", i + 1);
            throw GeneralException(msg.c_str());
        }
    }

    if (section.name != "")
        doc.push_back(section);

    return doc;
}

} // namespace ini
