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
#include "stream.h"
#include "varwriter.h"
#include <functional>

// HTML テンプレートシステム
class Template
{
public:

    class Scope
    {
    public:
        virtual bool writeVariable(amf0::Value& out, const String &) = 0;
    };

    enum
    {
        TMPL_UNKNOWN,
        TMPL_LOOP,
        TMPL_IF,
        TMPL_ELSE,
        TMPL_ELSIF,
        TMPL_END,
        TMPL_FRAGMENT,
        TMPL_FOREACH
    };

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
    bool    writeVariable(amf0::Value&, const String &);
    bool    writeGlobalVariable(amf0::Value&, const String &);
    bool    writePageVariable(amf0::Value&, const String &varName);
    int     getIntVariable(const String &);
    bool    getBoolVariable(const String &);

    // ディレクティブの実行
    int     readCmd(Stream &, Stream *);
    void    readIf(Stream &, Stream *);
    void    readLoop(Stream &, Stream *);
    void    readForeach(Stream &, Stream *);
    void    readFragment(Stream &, Stream *);

    void    readVariable_(Stream &in, Stream *outp, std::function<std::string(const std::string&)> filter);
    void    readVariable(Stream &, Stream *);
    void    readVariableJavaScript(Stream &in, Stream *outp);
    void    readVariableRaw(Stream &in, Stream *outp);
    int     readTemplate(Stream &, Stream *);
    bool    writeObjectProperty(amf0::Value& out, const String& varName, amf0::Value& obj);

    bool    evalCondition(const std::string& cond);
    std::vector<std::string> tokenize(const std::string& input);
    std::pair<std::string,std::string> readStringLiteral(const std::string& input);
    std::string evalStringLiteral(const std::string& input);
    std::string getStringVariable(const std::string& varName);

    std::string tmplArgs;
    std::string selectedFragment;
    std::string currentFragment;

    std::list<Scope*> m_scopes;
};

class GenericScope : public Template::Scope
{
public:
    bool writeVariable(amf0::Value& out, const String& varName) override;
    std::map<std::string,amf0::Value> vars;
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

    bool writeVariable(amf0::Value& out, const String &) override;

    const HTTPRequest& m_request;
};

#endif
