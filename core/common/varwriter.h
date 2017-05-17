#ifndef _VARWRITER_H
#define _VARWRITER_H

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
};

#endif
