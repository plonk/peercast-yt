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
    virtual amf0::Value getState();
    virtual std::string getVariable(const std::string&, int = 0);
    virtual bool writeVariable(Stream&, const String&, int = 0);

    virtual ~VariableWriter() {};
};

#endif
