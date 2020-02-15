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

#include "template2.h"
#include "sstream.h"
#include "str.h"
#include "jrpc.h"

#include <assert.h>

using namespace std;

// --------------------------------------
static picojson::object array_to_object(picojson::array arr)
{
    picojson::object obj;
    for (size_t i = 0; i < arr.size(); i++)
    {
        obj[to_string(i)] = arr[i];
    }
    return obj;
}


// --------------------------------------
Template2::Template2()
    : env_({})
{
}

// --------------------------------------
Template2::~Template2()
{
}

// --------------------------------------
picojson::value Template2::getObjectProperty(const String& varName, picojson::object& object)
{
    auto names = str::split(varName.str(), ".");

    if (names.size() == 1)
    {
        try {
            return object.at(varName.str());
        } catch(out_of_range&)
        {
            throw TemplateError("lookup error");
        }
    }else{
        picojson::value value;
        try {
            value = object.at(names[0]);
        } catch(out_of_range&)
        {
            throw TemplateError("lookup error");
        }

        if (value.is<picojson::null>() || value.is<std::string>() || value.is<double>() || value.is<picojson::array>())
        {
            throw TemplateError("lookup error");
        }else if (value.is<picojson::object>())
        {
            return getObjectProperty(varName + strlen(names[0].c_str()) + 1, value.get<picojson::object>());
        }
    }
}

// --------------------------------------
static bool isTruish(const picojson::value& v)
{
    return v.evaluate_as_boolean();
}

// --------------------------------------
bool Template2::writeObjectProperty(Stream& s, const String& varName, const picojson::object& object)
{
    auto names = str::split(varName.str(), ".");

    if (names.size() == 1)
    {
        try
        {
            picojson::value value = object.at(varName.str());
            if (value.is<std::string>())
            {
                string str = value.get<std::string>();
                s.writeString(str.c_str());
            }else
                s.writeString(value.serialize().c_str());
        }catch (out_of_range&)
        {
            return false;
        }
        return true;
    }else{
        try
        {
            picojson::value value = object.at(names[0]);
            if (value.is<picojson::null>() || value.is<std::string>() || value.is<double>())
            {
                return false;
            }else if (value.is<picojson::array>())
            {
                return writeObjectProperty(s, varName + strlen(names[0].c_str()) + 1, array_to_object(value.get<picojson::array>()));
            }else if (value.is<picojson::object>())
            {
                return writeObjectProperty(s, varName + strlen(names[0].c_str()) + 1, value.get<picojson::object>());
            }
        } catch (out_of_range&)
        {
            return false;
        }
        return true;
    }
}

// --------------------------------------
void Template2::writeVariable(Stream &s, const String &varName)
{
    std::string key = varName.str();

    if (env_.find(key) != env_.end()) {
        picojson::value v = env_[key];

        if (v.is<std::string>())
            s.writeString(v.get<std::string>());
        else
            s.writeString(v.serialize());
    } else {
        s.writeString(key);
    }
}

// --------------------------------------
string Template2::getStringVariable(const string& varName)
{
    StringStream mem;

    writeVariable(mem, varName.c_str());

    return mem.str();
}

// --------------------------------------
void    Template2::readFragment(Stream &in, Stream *outp)
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
string Template2::evalStringLiteral(const string& input)
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
pair<string,string> Template2::readStringLiteral(const string& input)
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
vector<string> Template2::tokenize(const string& input)
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
                  str::has_prefix(s, "!="))
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
bool    Template2::evalCondition(const string& cond)
{
    auto tokens = tokenize(cond);
    bool res = false;

    if (tokens.size() == 3) // 二項演算
    {
        auto op = tokens[1];
        if (op == "==" || op == "!=")
        {
            bool pred = (op == "==");

            string lhs, rhs;

            if (tokens[0][0] == '\"')
                lhs = evalStringLiteral(tokens[0]);
            else
                lhs = getStringVariable(tokens[0].c_str());

            if (tokens[2][0] == '\"')
                rhs = evalStringLiteral(tokens[2]);
            else
                rhs = getStringVariable(tokens[2].c_str());

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

        try
        {
            res = isTruish(getObjectProperty(varName.c_str(), env_)) == pred;
        }catch(TemplateError&)
        {
            LOG_ERROR("Template error");
            res = false;
        }
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
void    Template2::readIf(Stream &in, Stream *outp)
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
picojson::array Template2::evaluateCollectionVariable(const String& varName,
                                                      picojson::object& object)
{
    auto names = str::split(varName.str(), ".");

    if (names.size() == 1)
    {
        try
        {
            picojson::value value = object.at(varName.str());
            if (value.is<picojson::array>())
            {
                return value.get<picojson::array>();
            }else
                return {};
        }catch (out_of_range&)
        {
            return {};
        }
    }else{
        try
        {
            picojson::value value = object.at(names[0]);
            if (value.is<picojson::null>() || value.is<std::string>() || value.is<double>() || value.is<picojson::array>())
            {
                return {};
            }else if (value.is<picojson::object>())
            {
                return evaluateCollectionVariable(varName + strlen(names[0].c_str()) + 1, value.get<picojson::object>());
            }
        } catch (out_of_range&)
        {
            return {};
        }
    }
}

// --------------------------------------
void    Template2::readForeach(Stream &in, Stream *outp)
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

            auto coll = evaluateCollectionVariable(var, env_);

            if (coll.size() == 0)
            {
                readTemplate(in, NULL);
            }else
            {
                picojson::value old = env_["it"];
                picojson::value oldIndex = env_["index"];
                picojson::value oldIndexPlusOne = env_["indexPlusOne"];

                int start = in.getPosition();
                for (size_t i = 0; i < coll.size(); i++)
                {
                    in.seekTo(start);
                    env_["it"] = coll[i];
                    env_["index"].set<double>(i);
                    env_["indexPlusOne"].set<double>(i + 1);
                    readTemplate(in, outp);
                }

                env_["it"] = old;
                env_["index"] = oldIndex;
                env_["indexPlusOne"] = oldIndexPlusOne;
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
int Template2::readCmd(Stream &in, Stream *outp)
{
    String cmd;

    int tmpl = TMPL_UNKNOWN;

    while (!in.eof())
    {
        char c = in.readChar();

        if (String::isWhitespace(c) || (c=='}'))
        {
            if (cmd == "if")
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
            }else if (cmd == "else")
            {
                tmpl = TMPL_ELSE;
            }else
            {
                throw TemplateError(str::format("unknown command: %s", cmd.c_str()).c_str());
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
void    Template2::readVariable(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                StringStream mem;

                writeObjectProperty(mem, var, env_);
                outp->writeString(cgi::escape_html(mem.str()).c_str());
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
void    Template2::readVariableJavaScript(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                StringStream mem;

                writeVariable(mem, var);
                outp->writeString(cgi::escape_javascript(mem.str()).c_str());
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
void    Template2::readVariableRaw(Stream &in, Stream *outp)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                writeObjectProperty(*outp, var, env_);
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
int Template2::readTemplate(Stream &in, Stream *outp)
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
