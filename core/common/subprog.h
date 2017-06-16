#ifndef _SUBPROG_H
#define _SUBPROG_H

#include <string>
#include "stream.h"
#include "env.h"

#ifdef WIN32
#include <windows.h>
#endif

class Subprogram
{
public:
    Subprogram(const std::string& name, Environment& env);
    ~Subprogram();

    bool    start();
    bool    wait(int* status);
    Stream& inputStream();
    int     pid();

    std::string  m_name;
    Environment& m_env;
    FileStream   m_inputStream;
    int          m_pid;

#ifdef WIN32
    HANDLE m_processHandle;
#endif
};

#endif
