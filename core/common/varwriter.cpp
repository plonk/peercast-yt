#include "varwriter.h"
#include "sstream.h"

std::string VariableWriter::getVariable(const std::string& name)
{
    StringStream mem;
    bool written = writeVariable(mem, name.c_str());
    if (written)
        return mem.str();
    else
        return name.c_str();
}

std::string VariableWriter::getVariable(const std::string& name, int loop)
{
    StringStream mem;
    bool written = writeVariable(mem, name.c_str(), loop);
    if (written)
        return mem.str();
    else
        return name.c_str();
}
