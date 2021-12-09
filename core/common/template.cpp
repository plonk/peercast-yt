// ------------------------------------------------
// File : template.cpp
// Date: 4-apr-2002
// Author: giles
// Desc:
//      HTML templating system
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

#include <cctype>

#include "template.h"

#include "servmgr.h"
#include "stats.h"
#include "sstream.h"
#include "notif.h"
#include "str.h"
#include "jrpc.h"
#include "regexp.h"
#include "uptest.h"

#include <assert.h>

using namespace std;

// --------------------------------------
Template::Template(const std::string& args)
{
    tmplArgs = args;
}

// --------------------------------------
Template::~Template()
{
}

// --------------------------------------
bool Template::writeObjectProperty(amf0::Value& out, const String& varName, amf0::Value& obj)
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
            if (value.isObject())
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

// --------------------------------------
bool Template::writeVariable(amf0::Value& out, const String &varName)
{
    bool written;
    for (auto scope : m_scopes)
    {
        written = scope->writeVariable(out, varName);
        if (written)
            return true;
    }

    written = writeGlobalVariable(out, varName);
    if (!written)
        out = amf0::Value(); // null

    return true;
}

// --------------------------------------
bool Template::writePageVariable(amf0::Value& out, const String &varName)
{
    String v = varName+5;
    v.append('=');
    const char *a = getCGIarg(tmplArgs.c_str(), v);
    if (a)
    {
        Regexp pat("^([^&]*)");
        auto vec = pat.exec(a);
        assert(vec.size() > 0);

        out = cgi::unescape(vec[0]);
        return true;
    }

    return false;
}

// --------------------------------------
bool Template::writeGlobalVariable(amf0::Value& out, const String &varName)
{
    bool r = false;

    if (varName.startsWith("page."))
    {
        r = writePageVariable(out, varName);
    }else if (varName == "TRUE")
    {
        out = "1";
        r = true;
    }else if (varName == "FALSE")
    {
        out = "0";
        r = true;
    }

    return r;
}

// --------------------------------------
string Template::getStringVariable(const string& varName)
{
    amf0::Value value;
    bool written = writeVariable(value, varName.c_str());
    if (written)
    {
        if (value.isString())
            return value.string();
        else
            return value.inspect();
    } else {
        throw GeneralException(str::STR(varName, " is not defined"));
    }
}

// --------------------------------------
int Template::getIntVariable(const String &varName)
{
    amf0::Value value;
    bool written = writeVariable(value, varName.c_str());
    if (written)
    {
        if (value.isNumber())
            return value.number();
        else if (value.isString())
        {
            try {
                return std::stoi(value.string());
            } catch(std::invalid_argument& e)
            {
                //throw GeneralException(e.what());
                return 0;
            } catch(std::out_of_range& e)
            {
                throw GeneralException(e.what());
            }
        }
        else
            throw GeneralException(str::STR(varName, " is not a Number. Value: ", value.inspect()));
    } else {
        throw GeneralException(str::STR(varName, " is not defined"));
    }
}

// --------------------------------------
bool Template::getBoolVariable(const String &varName)
{
    amf0::Value value;
    bool written = writeVariable(value, varName.c_str());
    if (written)
    {
        if (value.isNull())
            return false;
        else if (value.isString() && (value.string() == "" || value.string() == "0"))
            return false;
        else if (value.isNumber() && value.number() == 0)
            return false;
        else if (value.isBool() && value.boolean() == false)
            return false;
        else
            return true;
    } else {
        throw GeneralException(str::STR(varName, " is not defined"));
    }
}

// --------------------------------------
void    Template::readFragment(Stream &in, Stream *outp)
{
    string fragName;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            auto outerFragment = currentFragment;
            currentFragment = fragName;
            readTemplate(in, outp);
            currentFragment = outerFragment;
            return;
        }else
        {
            fragName += c;
        }
    }
    LOG_ERROR("Premature end while processing fragment directive");
    return;
}

// --------------------------------------
string Template::evalStringLiteral(const string& input)
{
    if (input[0] != '\"' || input[input.size()-1] != '\"')
        throw StreamException("no string literal");

    string res;
    auto s = input.substr(1);

    while (!s.empty() && s[0] != '\"')
    {
        if (s[0] == '\\')
        {
            // バックスラッシュが最後の文字ではないことはわかっている
            // ので末端チェックはしない。
            res += s[0];
            res += s[1];
            s.erase(0,2);
        }else
        {
            res += s[0];
            s.erase(0,1);
        }
    }

    if (s.empty())
        throw StreamException("Premature end of string");
    return res;
}

// --------------------------------------
pair<string,string> Template::readStringLiteral(const string& input)
{
    if (input.empty())
        throw StreamException("empty input");

    if (input[0] != '\"')
        throw StreamException("no string literal");

    auto s = input;
    string res = "\"";
    s.erase(0,1);

    while (!s.empty() && s[0] != '\"')
    {
        if (s[0] == '\\')
        {
            res += s[0];
            s.erase(0,1);
            if (!s.empty())
            {
                res += s[0];
                s.erase(0,1);
            }
        }else
        {
            res += s[0];
            s.erase(0,1);
        }
    }

    if (s.empty())
        throw StreamException("Premature end of string");

    res += "\"";
    s.erase(0,1);
    return make_pair(res, s);
}

// --------------------------------------
vector<string> Template::tokenize(const string& input)
{
    using namespace std;

    auto isident =
        [](int c)
        {
            return isalpha(c) || isdigit(c) || c=='_' || c=='.';
        };

    auto s = input;
    vector<string> tokens;

    while (!s.empty())
    {
        if (isspace(s[0]))
        {
            do {
                s.erase(0,1);
            }while (isspace(s[0]));
        }else if (s[0] == '\"')
        {
            string t;
            tie(t, s) = readStringLiteral(s);
            tokens.push_back(t);
        }else if (str::has_prefix(s, "==") ||
                  str::has_prefix(s, "!=") ||
                  str::has_prefix(s, "=~") ||
                  str::has_prefix(s, "!~"))
        {
            tokens.push_back(s.substr(0,2));
            s.erase(0,2);
        }else if (isident(s[0]) || s[0] == '!')
        {
            string bangs, var;
            while (!s.empty() && s[0] == '!')
            {
                bangs += s[0];
                s.erase(0,1);
            }

            if (s.empty())
                throw StreamException("Premature end of token");

            if (!(isalpha(s[0]) || s[0]=='_' || s[0]=='.'))
                throw StreamException("Identifier expected after '!'");

            while (!s.empty() && isident(s[0]))
            {
                var += s[0];
                s.erase(0,1);
            };
            tokens.push_back(bangs + var);
        }else
        {
            auto c = string() + s[0];
            throw StreamException(("Unrecognized token. Error at " + str::inspect(c)).c_str());
        }
    }
    return tokens;
}

// --------------------------------------
bool    Template::evalCondition(const string& cond)
{
    auto tokens = tokenize(cond);
    bool res = false;

    if (tokens.size() == 3) // 二項演算
    {
        auto op = tokens[1];
        if (op == "=~" || op == "!~")
        {
            bool pred = (op == "=~");

            string lhs, rhs;

            if (tokens[0][0] == '\"')
                lhs = evalStringLiteral(tokens[0]);
            else
                lhs = getStringVariable(tokens[0].c_str());

            if (tokens[2][0] == '\"')
                rhs = evalStringLiteral(tokens[2]);
            else
                rhs = getStringVariable(tokens[2].c_str());

            res = ((!Regexp(rhs).exec(lhs).empty()) == pred);
        }else if (op == "==" || op == "!=")
        {
            bool pred = (op == "==");

            string lhs, rhs;

            if (tokens[0][0] == '\"')
                lhs = evalStringLiteral(tokens[0]);
            else
                lhs = getStringVariable(tokens[0]);

            if (tokens[2][0] == '\"')
                rhs = evalStringLiteral(tokens[2]);
            else
                rhs = getStringVariable(tokens[2]);

            res = ((lhs==rhs) == pred);
        }
        else
            throw StreamException(("Unrecognized condition operator " + op).c_str());
    }else if (tokens.size() == 1)
    {
        string varName;
        bool pred = true;

        for (auto c : tokens[0])
        {
            if (c == '!')
                pred = !pred;
            else
                varName += c;
        }

        res = getBoolVariable(varName.c_str()) == pred;
    }else
    {
        throw StreamException("Malformed condition expression");
    }
    return res;
}

// --------------------------------------
static String readCondition(Stream &in)
{
    String cond;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
            break;
        else
            cond.append(c);
    }
    return cond;
}

// --------------------------------------
void    Template::readIf(Stream &in, Stream *outp)
{
    bool hadActive = false;
    int cmd = TMPL_IF;

    while (cmd != TMPL_END)
    {
        if (cmd == TMPL_ELSE)
        {
            cmd = readTemplate(in, hadActive ? NULL : outp);
        }else if (cmd == TMPL_IF || cmd == TMPL_ELSIF)
        {
            String cond = readCondition(in);
            if (!hadActive && evalCondition(cond))
            {
                hadActive = true;
                cmd = readTemplate(in, outp);
            }else
            {
                cmd = readTemplate(in, NULL);
            }
        }
    }
    return;
}

// --------------------------------------
void    Template::readLoop(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (!inSelectedFragment() || !outp)
            {
                readTemplate(in, NULL);
                return;
            }

            int cnt = getIntVariable(var);

            if (cnt)
            {
                int spos = in.getPosition();
                for (int i=0; i<cnt; i++)
                {
                    in.seekTo(spos);
                    readTemplate(in, outp);
                }
            }else
            {
                readTemplate(in, NULL);
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
void    Template::readForeach(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (!inSelectedFragment() || !outp)
            {
                readTemplate(in, NULL);
                return;
            }

            amf0::Value value;
            bool written = false;
            for (auto scope : m_scopes)
            {
                written = scope->writeVariable(value, var);
                if (written)
                {
                    break;
                }
            }
            if (!value.isStrictArray())
                throw GeneralException(str::STR(var, " is not a strictArray. Value: ", value.inspect()));

            auto& coll = value.strictArray();

            if (coll.size() == 0)
            {
                readTemplate(in, NULL);
            }else
            {
                GenericScope loopScope;
                prependScope(loopScope);
                int start = in.getPosition();
                for (size_t i = 0; i < coll.size(); i++)
                {
                    loopScope.vars["this"] = coll[i];
                    loopScope.vars["loop.index"] = i;
                    loopScope.vars["loop.indexBaseOne"] = i + 1;
                    in.seekTo(start);
                    readTemplate(in, outp);
                }
                m_scopes.pop_front();
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
int Template::readCmd(Stream &in, Stream *outp)
{
    String cmd;

    int tmpl = TMPL_UNKNOWN;

    while (!in.eof())
    {
        char c = in.readChar();

        if (String::isWhitespace(c) || (c=='}'))
        {
            if (cmd == "loop")
            {
                readLoop(in, outp);
                tmpl = TMPL_LOOP;
            }else if (cmd == "if")
            {
                readIf(in, outp);
                tmpl = TMPL_IF;
            }else if (cmd == "elsif")
            {
                tmpl = TMPL_ELSIF;
            }else if (cmd == "fragment")
            {
                readFragment(in, outp);
                tmpl = TMPL_FRAGMENT;
            }else if (cmd == "foreach")
            {
                readForeach(in, outp);
                tmpl = TMPL_FOREACH;
            }else if (cmd == "end")
            {
                tmpl = TMPL_END;
            }
            else if (cmd == "else")
            {
                tmpl = TMPL_ELSE;
            }
            break;
        }else
        {
            cmd.append(c);
        }
    }
    return tmpl;
}

// --------------------------------------
void    Template::readVariable(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                amf0::Value out;
                std::string str;
                writeVariable(out, var);
                if (out.isString())
                    str = out.string();
                else
                    str = out.inspect();
                outp->writeString(cgi::escape_html(str));
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
void    Template::readVariableJavaScript(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                amf0::Value out;
                std::string str;
                writeVariable(out, var);
                if (out.isString())
                    str = out.string();
                else
                    str = out.inspect();
                outp->writeString(cgi::escape_javascript(str).c_str());
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
void    Template::readVariableRaw(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                amf0::Value out;
                std::string str;
                writeVariable(out, var);
                if (out.isString())
                    str = out.string();
                else
                    str = out.inspect();
                outp->writeString(str);
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
// ストリーム in の現在の位置から 1 ブロック分のテンプレートを処理し、
// outp がNULL でなければ *outp に出力する。EOF あるいは{@end} に当たっ
// た場合は TMPL_END を返し、{@else} に当たった場合は TMPL_ELSE、
// {@elsif ...} に当たった場合は TMPL_ELSIF を返す(条件式を読み込む前
// に停止する)。
int Template::readTemplate(Stream &in, Stream *outp)
{
    Stream *p = inSelectedFragment() ? outp : NULL;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '{')
        {
            c = in.readChar();
            if (c == '$')
            {
                readVariable(in, outp);
            }
            else if (c == '\\')
            {
                readVariableJavaScript(in, outp);
            }
            else if (c == '!')
            {
                readVariableRaw(in, outp);
            }
            else if (c == '@')
            {
                int t = readCmd(in, outp);
                if (t == TMPL_END || t == TMPL_ELSE || t == TMPL_ELSIF)
                    return t;
            }
            else
            {
                // テンプレートに関係のない波括弧はそのまま表示する
                if (p)
                {
                    p->writeChar('{');
                    p->writeChar(c);
                }
            }
        }else
        {
            if (p)
                p->writeChar(c);
        }
    }
    return TMPL_END;
}

// --------------------------------------
bool HTTPRequestScope::writeVariable(amf0::Value& out, const String& varName)
{
    if (varName == "request.host")
    {
        if (m_request.headers.get("Host").empty())
        {
            auto s = servMgr->getState();
            out = s.object().at("serverIP").string() + ":" + s.object().at("serverPort").string();
        }
        else
        {
            out = m_request.headers.get("Host");
        }
        return true;
    }else if (varName == "request.path") // HTTPRequest に委譲すべきか
    {
        out = m_request.path;
        return true;
    }else if (varName == "request.queryString")
    {
        out = m_request.queryString;
        return true;
    }else if (varName == "request.search")
    {
        if (m_request.queryString != "")
        {
            out = "?" + m_request.queryString;
        }else
            out = "";
        return true;
    }

    return false;
}

static bool writeObjectProperty(amf0::Value& out, const String& varName, amf0::Value& obj)
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
            if (value.isObject())
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

bool GenericScope::writeVariable(amf0::Value& out, const String& varName)
{
    if (vars.count(varName))
    {
        out = vars[varName];
        return true;
    } else {
        const std::string v = varName;
        auto idx = v.find('.');
        if (idx != std::string::npos)
        {
            auto qual = v.substr(0, idx);
            if (vars.count(qual))
            {
                auto obj = vars[qual];
                return writeObjectProperty(out, varName + qual.size() + 1, obj);
            }
        }
        return false;
    }
}
