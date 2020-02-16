#ifndef _VARWRITER_H
#define _VARWRITER_H

#include <string>

class String;
class Stream;
// ----------------------------------
class VariableWriter
{
public:
    virtual bool writeVariable(Stream&, const String&)
    {
        return false;
    }

    // for collection variables
    virtual bool writeVariable(Stream&, const String&, int)
    {
        return false;
    }

    std::string getVariable(const std::string& name);
    std::string getVariable(const std::string& name, int loop);

    virtual ~VariableWriter() {};
};

#endif
