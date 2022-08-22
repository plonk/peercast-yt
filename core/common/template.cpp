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

static bool isTruish(const amf0::Value& value);

// --------------------------------------
Template::Template(const std::string& args)
{
    tmplArgs = args;
    m_pageScope = new GenericScope;

    cgi::Query query(tmplArgs);
    std::map<std::string,amf0::Value> page;
    for (auto& key : query.getKeys())
    {
        page[key] = query.get(key);
    }
    m_pageScope->vars["page"] = page;
    m_scopes.push_front(m_pageScope);
}

// --------------------------------------
Template::~Template()
{
    delete m_pageScope;
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
bool Template::writeGlobalVariable(amf0::Value& out, const String &varName)
{
    bool r = false;

    if (varName == "TRUE")
    {
        out = "1";
        r = true;
    }else if (varName == "FALSE")
    {
        out = "0";
        r = true;
    }else if (varName == "true")
    {
        out = true;
        r = true;
    }else if (varName == "false")
    {
        out = false;
        r = true;
    }else if (varName == "null")
    {
        out = nullptr;
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
                // 数字で始まっていない。
                return 0;
            } catch(std::out_of_range& e)
            {
                // 値が大きすぎる、あるいは小さすぎる。
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
        return isTruish(value);
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
        throw StreamException(str::STR("Malformed string literal: ", input));

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
std::list<std::string> Template::tokenize(const string& input)
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
        }else if (str::has_prefix(s, "==") ||
                  str::has_prefix(s, "!=") ||
                  str::has_prefix(s, "=~") ||
                  str::has_prefix(s, "!~"))
        {
            tokens.push_back(s.substr(0,2));
            s.erase(0,2);
        }else if (s[0] == '(' || s[0] == ')' || s[0] == '!' || s[0] == ',' || s[0] == '=' || s[0] == '{' || s[0] == '}' || s[0] == ':')
        {
            tokens.push_back(s.substr(0, 1));
            s.erase(0, 1);
        }else if (s[0] == '\"')
        {
            string t;
            tie(t, s) = readStringLiteral(s);
            tokens.push_back(t);
        }else if (isident(s[0]))
        {
            string var;
            while (!s.empty() && isident(s[0]))
            {
                var += s[0];
                s.erase(0,1);
            };
            tokens.push_back(var);
        }else
        {
            auto c = string() + s[0];
            throw StreamException(("Unrecognized token. Error at " + str::inspect(c)).c_str());
        }
    }
    return { tokens.begin(), tokens.end() };
}

// --------------------------------------
static Regexp REG_OP("^==|=~|!=|!~$");
static Regexp REG_NOT("^!$");
static Regexp REG_IDENT("^[A-z0-9_.]+$");
static Regexp REG_LPAREN("^\\($");
static Regexp REG_RPAREN("^\\)$");
static Regexp REG_STRING("^\".*?\"$");
static Regexp REG_COMMA("^,$");
static Regexp REG_COLON("^:$");
static Regexp REG_ASSIGN("^=$");
static Regexp REG_LBRACE("^\\{$");
static Regexp REG_RBRACE("^\\}$");

amf0::Value Template::parse(std::list<std::string>& tokens)
{
    /*

      EXP := EXP2 OP EXP |                       (op exp2 exp)
             EXP2 |                   
             '!' EXP                             (! exp)
             '{' ( EXP ':' EXP ( ',' EXP )* )? '}' (object exp exp ...)

      EXP2 := IDENT '(' EXP (',' EXP)* ')' |     (ident exp ...)
              IDENT |                            ident
              STRING                             (quote STRING)

      OP := '=~' | '!~' | '==' | '!='

    */

    auto accept =
        [&](const Regexp& sym) -> shared_ptr<std::string>
        {
            if (tokens.empty())
                return nullptr;
            if (sym.matches(tokens.front()))
            {
                auto r = tokens.front();
                tokens.pop_front();
                return make_shared<std::string>(r);
            }else
                return nullptr;
        };

    auto expect =
        [&](const Regexp& sym) -> void
        {
            if (tokens.empty())
                throw GeneralException(str::STR("Premature end while expecting ", sym.m_exp));

            if (!sym.matches(tokens.front()))
                throw GeneralException(str::STR("Got ", tokens.front()," while expecting ", sym.m_exp));

            tokens.pop_front();
        };

    std::function<shared_ptr<amf0::Value>()> exp, exp2;

    exp =
        [&]() -> shared_ptr<amf0::Value>
        {
            if (auto e1 = exp2()) {
                if (auto op = accept(REG_OP)) {
                    if (auto e2 = exp()) {
                        return make_shared<amf0::Value>(amf0::Value::strictArray({ *op, *e1, *e2 }));
                    } else {
                        return nullptr;
                    }
                } else {
                    return e1;
                }
            } else if (accept(REG_NOT)) {
                if (auto r = exp()) {
                    return make_shared<amf0::Value>(amf0::Value::strictArray({ "!", *r }));
                } else
                    return nullptr;
            } else if (accept(REG_LBRACE)) {
                std::vector<amf0::Value> funcall = { "object" };
                if (accept(REG_RBRACE))
                {
                    return make_shared<amf0::Value>(amf0::Value::strictArray(funcall));
                }
                while (true) {
                    auto e = exp();
                    if (!e)
                    {
                        return nullptr;
                    }
                    funcall.push_back(*e);
                    expect(REG_COLON);
                    e = exp();
                    if (!e)
                    {
                        return nullptr;
                    }
                    funcall.push_back(*e);
                    if (accept(REG_COMMA))
                    {
                        continue;
                    }else{
                        expect(REG_RBRACE);
                        return make_shared<amf0::Value>(amf0::Value::strictArray(funcall));
                    }
                }
            } else
                return nullptr;
        };

    exp2 =
        [&]() -> shared_ptr<amf0::Value>
        {
            if (auto ident = accept(REG_IDENT)) {
                if (accept(REG_LPAREN)) {
                    std::vector<amf0::Value> funcall = { *ident };
                    if (accept(REG_RPAREN))
                    {
                        return make_shared<amf0::Value>(amf0::Value::strictArray(funcall));
                    }
                    while (true) {
                        auto e = exp();
                        if (!e)
                        {
                            return nullptr;
                        }
                        funcall.push_back(*e);
                        if (accept(REG_COMMA))
                        {
                            continue;
                        }else{
                            expect(REG_RPAREN);
                            return make_shared<amf0::Value>(amf0::Value::strictArray(funcall));
                        }
                    }
                } else {
                    return make_shared<amf0::Value>(*ident);
                }
            } else if (auto s = accept(REG_STRING)) {
                return make_shared<amf0::Value>(amf0::Value::strictArray({ "quote", evalStringLiteral(*s) }));
            } else
                return nullptr;
        };


    auto pvalue = exp();
    if (!pvalue) {
        throw GeneralException(" something weird ");
    }
    return *pvalue;
}


// --------------------------------------
static bool isTruish(const amf0::Value& value)
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
}

// --------------------------------------
amf0::Value Template::evalForm(const amf0::Value& exp)
{
    const auto& arr = exp.strictArray();

    if (arr.size() == 0) {
        throw GeneralException("empty list form");
    } else {
        const auto& name = arr.at(0).string();
        if (name == "==") {
            return evalExpression(arr.at(1)) == evalExpression(arr.at(2));
        } else if (name == "!=") {
            return evalExpression(arr.at(1)) != evalExpression(arr.at(2));
        } else if (name == "=~") {
            return Regexp(evalExpression(arr.at(2)).string()).matches(evalExpression(arr.at(1)).string());
        } else if (name == "!~") {
            return !Regexp(evalExpression(arr.at(2)).string()).matches(evalExpression(arr.at(1)).string());
        } else if (name == "!") {
            return !isTruish(evalExpression(arr.at(1)));
        } else if (name == "quote") {
            return arr.at(1);
        } else if (name == "length") {
            return evalExpression(arr.at(1)).strictArray().size();
        } else if (name == "inspect") {
            return evalExpression(arr.at(1)).inspect();
        } else if (name == "if") {
            if (arr.size() == 3) {
                return isTruish(evalExpression(arr.at(1))) ? evalExpression(arr.at(2)) : nullptr;
            }else if (arr.size() == 4) {
                return isTruish(evalExpression(arr.at(1))) ? evalExpression(arr.at(2)) : evalExpression(arr.at(3));
            }else
                throw GeneralException("malformed if form");
        } else if (name == "and") {
            size_t index = 1;
            amf0::Value v = true;
            while (index < arr.size())
            {
                v = evalExpression(arr.at(index));
                if (!isTruish(v))
                    break;
                index++;
            }
            return v;
        } else if (name == "or") {
            size_t index = 1;
            amf0::Value v = false;
            while (index < arr.size())
            {
                v = evalExpression(arr.at(index));
                if (isTruish(v))
                    break;
                index++;
            }
            return v;
        } else if (name == "object") {
            std::map<std::string,amf0::Value> object;
            if ((arr.size() - 1) % 2) // odd
            {
                throw GeneralException("object: Odd number of arguments given");
            }
            for (size_t i = 1; i < arr.size(); i += 2)
            {
                auto key = evalExpression(arr.at(i));
                if (!key.isString())
                {
                    throw GeneralException("object: Non-string key");
                }
                auto value = evalExpression(arr.at(i+1));

                object[key.string()] = value;
            }
            return object;
        } else if (name == "merge") {
            if (arr.size() - 1 < 1)
            {
                throw GeneralException("merge: Wrong number of arguments");
            }
            std::map<std::string,amf0::Value> result = evalExpression(arr[1]).object();
            for (size_t i = 2; i < arr.size(); i++)
            {
                auto evaluated = evalExpression(arr[i]);
                const auto& src = evaluated.object();
                for (auto& pair : src)
                {
                    result[pair.first] = pair.second;
                }
            }
            return result;
        } else {
            throw GeneralException(str::STR("Unknown function name or operator name ", name));
        }
    }
}

// --------------------------------------
static Regexp REG_INTEGER("^[0-9]$");

amf0::Value Template::evalExpression(const amf0::Value& exp)
{
    if (exp.isStrictArray()) {
        return evalForm(exp);
    } else if (exp.isString()) {
        if (REG_INTEGER.matches(exp.string())) {
            return atoi(exp.string().c_str());
        } else { // variable
            amf0::Value value;
            if (writeVariable(value, exp.string().c_str()))
                return value;
            else
                throw GeneralException(str::STR(exp.string(), " is not defined"));
        }
    } else
        throw GeneralException("evalExpression: unknown type of expression");
}

// --------------------------------------
amf0::Value Template::evalExpression(const string& str)
{
    std::list<std::string> tokens = tokenize(str);

    auto exp = parse(tokens);
    if (tokens.size()) {
        throw GeneralException(str::STR("Unexpected token ", tokens.front()));
    }
    return evalExpression(exp);
}

// --------------------------------------
bool    Template::evalCondition(const string& cond)
{
    return isTruish(evalExpression(cond));
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
            std::string cond = readCondition(in).c_str();
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
std::vector<std::pair<std::string,amf0::Value>> Template::parseLetSpec(std::list<std::string>& tokens)
{
    std::vector<std::pair<std::string,amf0::Value>> result;

    auto accept =
        [&](const Regexp& sym) -> shared_ptr<std::string>
        {
            if (tokens.empty())
                return nullptr;
            if (sym.matches(tokens.front()))
            {
                auto r = tokens.front();
                tokens.pop_front();
                return make_shared<std::string>(r);
            }else
                return nullptr;
        };

    auto expect =
        [&](const Regexp& sym) -> void
        {
            if (tokens.empty())
                throw GeneralException(str::STR("Premature end while expecting ", sym.m_exp));

            if (!sym.matches(tokens.front()))
                throw GeneralException(str::STR("Got ", tokens.front()," while expecting ", sym.m_exp));

            tokens.pop_front();
        };

    while (true)
    {
        if (auto ident = accept(REG_IDENT))
        {
            expect(REG_ASSIGN);
            auto exp = parse(tokens);

            result.push_back(std::pair<std::string,amf0::Value>(*ident, exp));
            if (tokens.size())
            {
                expect(REG_COMMA);
                continue;
            }else{
                break;
            }
        }else{
            throw GeneralException("Identifier expected");
        }
    }
    return result;
}

// --------------------------------------
void    Template::readLet(Stream &in, Stream *outp)
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

            std::list<std::string> tokens = tokenize(var.c_str());
            std::vector<std::pair<std::string,amf0::Value>> letspec = parseLetSpec(tokens);

            GenericScope newScope;
            prependScope(newScope);
            for (size_t i = 0; i < letspec.size(); ++i)
            {
                auto& pair = letspec[i];
                newScope.vars[pair.first] = evalExpression(pair.second);
            }
            readTemplate(in, outp);
            m_scopes.pop_front();
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
            }else if (cmd == "let")
            {
                readLet(in, outp);
                tmpl = TMPL_LET;
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
static void to_s(const amf0::Value& value, std::string& out)
{
    if (value.isString())
        out = value.string();
    else if (value.isNull())
        out = "";
    else
        out = value.inspect();
}

// --------------------------------------
void Template::readVariable_(Stream &in, Stream *outp, std::function<std::string(const std::string&)> filter)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (!inSelectedFragment() || !outp)
                return;

            auto out = evalExpression(var);
            std::string str;
            to_s(out, str);
            outp->writeString(filter(str));

            return;
        }else
        {
            var.append(c);
        }
    }
}
// --------------------------------------
void    Template::readVariable(Stream &in, Stream *outp)
{
    readVariable_(in, outp, cgi::escape_html);
}

// --------------------------------------
void    Template::readVariableJavaScript(Stream &in, Stream *outp)
{
    readVariable_(in, outp, cgi::escape_javascript);
}

// --------------------------------------
void    Template::readVariableRaw(Stream &in, Stream *outp)
{
    readVariable_(in, outp, [](const std::string& s) { return s; });
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

RootObjectScope::RootObjectScope()
    : m_objects({{"servMgr" , servMgr->getState()},
                 {"chanMgr" , chanMgr->getState()},
                 {"stats"   , stats.getState()},
                 {"notificationBuffer" , g_notificationBuffer.getState()},
                 {"sys"     , sys->getState()}})
{
}
