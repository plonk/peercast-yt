#include "varwriter.h"
#include "sstream.h"

amf0::Value VariableWriter::getState()
{
    return amf0::Value::object({});
}

static amf0::Value getProperty(const amf0::Value& v, const std::string& path)
{
    if (!v.isObject()) {
        throw std::runtime_error(str::STR("Cannot read property '", path,"' of ", v.inspect()));
    }

    auto& obj = v.object();
    if (obj.count(path)) {
        return obj.at(path);
    } else {
        auto components = str::split(path, ".");
        if (components.size() == 1)
            throw std::runtime_error(str::STR("Cannot read property '", path,"' of ", v.inspect()));
        else {
            return getProperty(obj.count(components[0]) ? obj.at(components[0]) : amf0::Value(nullptr),
                               path.substr(components[0].size() + 1));
        }
    }
}

std::string VariableWriter::getVariable(const std::string& varName, int)
{
    amf0::Value state = this->getState();
    try {
        amf0::Value value = getProperty(state, varName);
        return value.string();
    } catch (std::runtime_error& e)
    {
        return varName;
    }
}

bool VariableWriter::writeVariable(Stream& s, const String& varName, int)
{
    amf0::Value state = this->getState();
    try {
        amf0::Value value = getProperty(state, varName);
        s.writeString(value.isString() ? value.string() : value.inspect());
        return true;
    } catch (std::runtime_error& e)
    {
        return false;
    }
}

