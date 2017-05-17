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

#include <list>
#include "sys.h"
#include "stream.h"
#include "json.hpp"
#include "varwriter.h"

using json = nlohmann::json;

// HTML テンプレートシステム
class Template
{
public:

    class Scope : VariableWriter
    {
    public:
        virtual bool writeVariable(Stream &, const String &, int) = 0;
    };

    enum
    {
        TMPL_UNKNOWN,
        TMPL_LOOP,
        TMPL_IF,
        TMPL_ELSE,
        TMPL_END,
        TMPL_FRAGMENT,
        TMPL_FOREACH
    };

    Template(const char* args = NULL);
    Template(const std::string& args);
    ~Template();

    void initVariableWriters();

    Template& prependScope(Scope& scope)
    {
        m_scopes.push_front(&scope);
        return *this;
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
    void    writeGlobalVariable(Stream &, const String &, int);
    int     getIntVariable(const String &, int);
    bool    getBoolVariable(const String &, int);

    // ディレクティブの実行
    int     readCmd(Stream &, Stream *, int);
    void    readIf(Stream &, Stream *, int);
    void    readLoop(Stream &, Stream *, int);
    void    readForeach(Stream &, Stream *, int);
    void    readFragment(Stream &, Stream *, int);

    void    readVariable(Stream &, Stream *, int);
    void    readVariableJavaScript(Stream &in, Stream *outp, int loop);
    void    readVariableRaw(Stream &in, Stream *outp, int loop);
    bool    readTemplate(Stream &, Stream *, int);
    bool    writeObjectProperty(Stream& s, const String& varName, json::object_t object);
    json::array_t evaluateCollectionVariable(String& varName);

    bool    evalCondition(const std::string& cond, int loop);
    std::vector<std::string> tokenize(const std::string& input);
    std::pair<std::string,std::string> readStringLiteral(const std::string& input);
    std::string evalStringLiteral(const std::string& input);
    std::string getStringVariable(const std::string& varName, int loop);

    char * tmplArgs;
    std::string selectedFragment;
    std::string currentFragment;
    json currentElement;

    std::list<Scope*> m_scopes;
    std::map<std::string,VariableWriter*> m_variableWriters;
};

#include "http.h"

// HTTP リクエストを変数としてエクスポートするスコープ
class HTTPRequestScope : public Template::Scope
{
public:
    HTTPRequestScope(const HTTPRequest& aRequest)
        : m_request(aRequest)
    {
    }

    bool writeVariable(Stream &, const String &, int) override;

    const HTTPRequest& m_request;
};

#endif
