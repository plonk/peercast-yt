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

    virtual ~VariableWriter() {};
};

#endif
