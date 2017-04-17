// ------------------------------------------------
// File : template.h
// Date: 4-apr-2002
// Author: giles
// Desc:
//
// (c) 2002 peercast.org
// ------------------------------------------------
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// ------------------------------------------------

#ifndef _TEMPLATE_H
#define _TEMPLATE_H

#include "sys.h"
#include "stream.h"

// HTML テンプレートシステム
class Template
{
    enum
    {
        TMPL_UNKNOWN,
        TMPL_LOOP,
        TMPL_IF,
        TMPL_ELSE,
        TMPL_END,
        TMPL_FRAGMENT
    };

public:
    Template(const char* args = NULL)
    {
        if (args)
            tmplArgs = strdup(args);
        else
            tmplArgs = NULL;
    }
    ~Template()
    {
        if (tmplArgs)
            free(tmplArgs);
    }

    bool inSelectedFragment()
    {
        if (selectedFragment.empty())
            return true;
        else
            return selectedFragment == currentFragment;
    }

    // 変数
    void    writeVariable(Stream &, const String &, int);
    int     getIntVariable(const String &, int);
    bool    getBoolVariable(const String &, int);

    // ディレクティブの実行
    int     readCmd(Stream &, Stream *, int);
    void    readIf(Stream &, Stream *, int);
    void    readLoop(Stream &, Stream *, int);
    void    readFragment(Stream &, Stream *, int);

    void    readVariable(Stream &, Stream *, int);
    void    readVariableRaw(Stream &in, Stream *outp, int loop);
    bool    readTemplate(Stream &, Stream *, int);

    char * tmplArgs;
    std::string selectedFragment;
    std::string currentFragment;
};

#endif
