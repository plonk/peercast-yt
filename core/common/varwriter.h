#ifndef _VARWRITER_H
#define _VARWRITER_H

#include <string>
#include "amf0.h"

class String;
class Stream;
// ----------------------------------
class VariableWriter
{
public:
    // virtual bool writeVariable(Stream&, const String&)
    // {
    //     return false;
    // }

    // std::string getVariable(const std::string& name);
//    std::string getVariable(const std::string& name, int loop);

    virtual amf0::Value getState();

    virtual ~VariableWriter() {};
};

#endif
