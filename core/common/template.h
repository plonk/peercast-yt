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
        virtual ~Scope() {}
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
    amf0::Value evalForm(const amf0::Value&);
    amf0::Value evalExpression(const amf0::Value&);
    amf0::Value evalExpression(const std::string&);
    std::vector<std::string> tokenize(const std::string& input);
    static amf0::Value parse(std::vector<std::string>& tokens_);
    std::pair<std::string,std::string> readStringLiteral(const std::string& input);
    static std::string evalStringLiteral(const std::string& input);
    std::string getStringVariable(const std::string& varName);

    std::string tmplArgs;
    std::string selectedFragment;
    std::string currentFragment;

    std::list<Scope*> m_scopes;
    class GenericScope* m_pageScope;
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

class RootObjectScope : public Template::Scope
{
public:
    RootObjectScope();

    bool writeObjectProperty(amf0::Value& out, const String& varName, amf0::Value& obj)
    {
        auto names = str::split(varName.str(), ".");

        if (names.size() == 1)
        {
            try
            {
                auto value = obj.object().at(varName.str());
                out = value;
                return true;
            }catch (std::out_of_range&)
            {
                return false;
            }
        }else{
            try
            {
                auto value = obj.object().at(names[0]);
                if (value.isArray())
                {
                    return false;
//writeObjectProperty(s, varName + strlen(names[0].c_str()) + 1, array_to_object(value));
                }else if (value.isObject())
                {
                    return writeObjectProperty(out, varName + strlen(names[0].c_str()) + 1, value);
                }else
                    return false;
            } catch (std::out_of_range&)
            {
                return false;
            }
            return true;
        }
    }

    // loop は無視。
    bool writeVariable(amf0::Value& out, const String& varName) override
    {
        const std::string v = varName;
        if (m_objects.count(v))
        {
            out =  m_objects[v];
            return true;
        }else{
            auto idx = v.find('.');
            if (idx != std::string::npos)
            {
                auto qual = v.substr(0, idx);
                if (m_objects.count(qual))
                {
                    auto obj = m_objects[qual];
                    return writeObjectProperty(out, varName + qual.size() + 1, obj);
                }
            }
            return false;
        }
    }
    std::map<std::string,amf0::Value> m_objects;
};

#endif
