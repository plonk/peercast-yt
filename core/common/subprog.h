#ifndef _SUBPROG_H
#define _SUBPROG_H

#include <string>
#include "stream.h"

class Subprogram
{
public:
    Subprogram(const std::string& name, const char **);
    ~Subprogram();

    bool    start();
    bool    wait(int* status);
    Stream& inputStream();
    int     pid();

    std::string m_name;
    char**      m_env;
    FileStream  m_inputStream;
    int         m_pid;
};

#endif
