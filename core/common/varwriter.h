#ifndef _VARWRITER_H
#define _VARWRITER_H

#include "stream.h"

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
};

#endif
