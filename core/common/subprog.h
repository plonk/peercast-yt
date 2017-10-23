#ifndef _SUBPROG_H
#define _SUBPROG_H

#include <initializer_list>
#include <string>
#include "stream.h"
#include "env.h"

#ifdef WIN32
#include <windows.h>
#endif

class Subprogram
{
public:
    Subprogram(const std::string& name, bool receiveData = true, bool feedData = true);
    ~Subprogram();

    bool    start(std::initializer_list<std::string> arguments, Environment& env);
    bool    wait(int* status);
    Stream& inputStream();
    Stream& outputStream();
    int     pid();
    bool    isAlive();
    void    terminate();

    bool m_receiveData;
    bool m_feedData;

    std::string  m_name;
    FileStream   m_inputStream;
    FileStream   m_outputStream;
    int          m_pid;

#ifdef WIN32
    HANDLE m_processHandle;
#endif
};

#endif
