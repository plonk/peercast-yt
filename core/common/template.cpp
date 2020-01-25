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

using json = nlohmann::json;
using namespace std;

// --------------------------------------
static json::object_t array_to_object(json::array_t arr)
{
    json::object_t obj;
    for (size_t i = 0; i < arr.size(); i++)
    {
        obj[to_string(i)] = arr[i];
    }
    return obj;
}

// --------------------------------------
void Template::initVariableWriters()
{
    m_variableWriters["servMgr"] = servMgr;
    m_variableWriters["chanMgr"] = chanMgr;
    m_variableWriters["stats"]   = &stats;
    m_variableWriters["notificationBuffer"] = &g_notificationBuffer;
    m_variableWriters["sys"]     = sys;
}

// --------------------------------------
Template::Template(const char* args)
    : currentElement(json::object({}))
{
    if (args)
        tmplArgs = Sys::strdup(args);
    else
        tmplArgs = NULL;
    initVariableWriters();
}

// --------------------------------------
Template::Template(const std::string& args)
    : currentElement(json::object({}))
{
    tmplArgs = Sys::strdup(args.c_str());
    initVariableWriters();
}

// --------------------------------------
Template::~Template()
{
    if (tmplArgs)
        free(tmplArgs);
}

// --------------------------------------
bool Template::writeObjectProperty(Stream& s, const String& varName, json::object_t object)
{
    // LOG_DEBUG("writeObjectProperty %s", varName.str().c_str());
    auto names = str::split(varName.str(), ".");

    if (names.size() == 1)
    {
        try
        {
            json value = object.at(varName.str());
            if (value.is_string())
            {
                string str = value;
                s.writeString(str.c_str());
            }else
                s.writeString(value.dump().c_str());
        }catch (out_of_range&)
        {
            return false;
        }
        return true;
    }else{
        try
        {
            json value = object.at(names[0]);
            if (value.is_null() || value.is_string() || value.is_number())
            {
                return false;
            }else if (value.is_array())
            {
                return writeObjectProperty(s, varName + strlen(names[0].c_str()) + 1, array_to_object(value));
            }else if (value.is_object())
            {
                return writeObjectProperty(s, varName + strlen(names[0].c_str()) + 1, value);
            }
        } catch (out_of_range&)
        {
            return false;
        }
        return true;
    }
}

// --------------------------------------
void Template::writeVariable(Stream &s, const String &varName, int loop)
{
    bool written;
    for (auto* scope : m_scopes)
    {
        written = scope->writeVariable(s, varName, loop);
        if (written)
            return;
    }

    writeGlobalVariable(s, varName, loop);
}

// --------------------------------------
bool Template::writeLoopVariable(Stream &s, const String &varName, int loop)
{
    if (varName.startsWith("loop.channel."))
    {
        auto ch = chanMgr->findChannelByIndex(loop);
        if (ch)
            return ch->writeVariable(s, varName+13);
    }else if (varName.startsWith("loop.servent."))
    {
        Servent *sv = servMgr->findServentByIndex(loop);
        if (sv)
            return sv->writeVariable(s, varName+13);
    }else if (varName.startsWith("loop.filter."))
    {
        ServFilter *sf = &servMgr->filters[loop];
        return sf->writeVariable(s, varName+12);
    }else if (varName == "loop.indexEven")
    {
        s.writeStringF("%d", (loop&1)==0);
        return true;
    }else if (varName == "loop.index")
    {
        s.writeStringF("%d", loop);
        return true;
    }else if (varName == "loop.indexBaseOne")
    {
        s.writeStringF("%d", loop + 1);
        return true;
    }else if (varName.startsWith("loop.hit."))
    {
        const char *idstr = getCGIarg(tmplArgs, "id=");
        if (idstr)
        {
            GnuID id;
            id.fromStr(idstr);
            ChanHitList *chl = chanMgr->findHitListByID(id);
            if (chl)
            {
                int cnt=0;
                ChanHit *ch = chl->hit;
                while (ch)
                {
                    if (ch->host.ip && !ch->dead)
                    {
                        if (cnt == loop)
                        {
                            return ch->writeVariable(s, varName+9);
                            break;
                        }
                        cnt++;
                    }
                    ch=ch->next;
                }
            }
        }
    }else if (varName.startsWith("loop.externalChannel."))
    {
        return servMgr->channelDirectory->writeVariable(s, varName + strlen("loop."), loop);
    }else if (varName.startsWith("loop.channelFeed."))
    {
        return servMgr->channelDirectory->writeVariable(s, varName + strlen("loop."), loop);
    }else if (varName.startsWith("loop.notification."))
    {
        return g_notificationBuffer.writeVariable(s, varName + strlen("loop."), loop);
    }else if (varName.startsWith("loop.uptestServiceRegistry."))
    {
        return servMgr->uptestServiceRegistry->writeVariable(s, varName + strlen("loop.uptestServiceRegistry."), loop);
    }

    return false;
}

// --------------------------------------
bool Template::writePageVariable(Stream &s, const String &varName, int loop)
{
    if (varName.startsWith("page.channel."))
    {
        const char *idstr = getCGIarg(tmplArgs, "id=");
        if (idstr)
        {
            GnuID id;
            id.fromStr(idstr);
            auto ch = chanMgr->findChannelByID(id);
            if (varName == "page.channel.exist")
            {
                if (ch)
                    s.writeString("1");
                else
                    s.writeString("0");
                return true;
            }else
            {
                if (ch)
                    return ch->writeVariable(s, varName+13);
            }
        }
    }else
    {
        String v = varName+5;
        v.append('=');
        const char *a = getCGIarg(tmplArgs, v);
        if (a)
        {
            Regexp pat("\\A([^&]*)");
            auto vec = pat.exec(a);
            assert(vec.size() > 0);

            s.writeString(cgi::unescape(vec[0]));
            return true;
        }
    }

    return false;
}

// --------------------------------------
void Template::writeGlobalVariable(Stream &s, const String &varName, int loop)
{
    bool r = false;

    const std::string v = varName;
    if (v.find('.') != std::string::npos)
    {
        const auto qual = v.substr(0, v.find('.'));
        if (m_variableWriters.count(qual))
        {
            r = m_variableWriters[qual]->writeVariable(s, varName + qual.size() + 1);
            goto End;
        }
    }

    if (varName.startsWith("loop."))
    {
        r = writeLoopVariable(s, varName, loop);
    }else if (varName.startsWith("this."))
    {
        r = writeObjectProperty(s, varName + strlen("this."), currentElement);
    }else if (varName.startsWith("page."))
    {
        r = writePageVariable(s, varName, loop);
    }else if (varName == "TRUE")
    {
        s.writeString("1");
        r = true;
    }else if (varName == "FALSE")
    {
        s.writeString("0");
        r = true;
    }

    // 変数が見付からなかった場合は変数名を書き出す
End:
    if (!r)
        s.writeString(varName);
}

// --------------------------------------
string Template::getStringVariable(const string& varName, int loop)
{
    StringStream mem;

    writeVariable(mem, varName.c_str(), loop);

    return mem.str();
}

// --------------------------------------
int Template::getIntVariable(const String &varName, int loop)
{
    StringStream mem;

    writeVariable(mem, varName, loop);

    return atoi(mem.str().c_str());
}

// --------------------------------------
bool Template::getBoolVariable(const String &varName, int loop)
{
    StringStream mem;

    writeVariable(mem, varName, loop);

    const string val = mem.str();

    // integer
    if ((val[0] >= '0') && (val[0] <= '9'))
        return atoi(val.c_str()) != 0;

    // string
    if (val[0] != 0)
        return true;

    return false;
}

// --------------------------------------
void    Template::readFragment(Stream &in, Stream *outp, int loop)
{
    string fragName;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            auto outerFragment = currentFragment;
            currentFragment = fragName;
            readTemplate(in, outp, loop);
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
bool    Template::evalCondition(const string& cond, int loop)
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
                lhs = getStringVariable(tokens[0].c_str(), loop);

            if (tokens[2][0] == '\"')
                rhs = evalStringLiteral(tokens[2]);
            else
                rhs = getStringVariable(tokens[2].c_str(), loop);

            res = ((!Regexp(rhs).exec(lhs).empty()) == pred);
        }else if (op == "==" || op == "!=")
        {
            bool pred = (op == "==");

            string lhs, rhs;

            if (tokens[0][0] == '\"')
                lhs = evalStringLiteral(tokens[0]);
            else
                lhs = getStringVariable(tokens[0].c_str(), loop);

            if (tokens[2][0] == '\"')
                rhs = evalStringLiteral(tokens[2]);
            else
                rhs = getStringVariable(tokens[2].c_str(), loop);

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

        res = getBoolVariable(varName.c_str(), loop) == pred;
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
void    Template::readIf(Stream &in, Stream *outp, int loop)
{
    bool hadActive = false;
    int cmd = TMPL_IF;

    while (cmd != TMPL_END)
    {
        if (cmd == TMPL_ELSE)
        {
            cmd = readTemplate(in, hadActive ? NULL : outp, loop);
        }else if (cmd == TMPL_IF || cmd == TMPL_ELSIF)
        {
            String cond = readCondition(in);
            if (!hadActive && evalCondition(cond, loop))
            {
                hadActive = true;
                cmd = readTemplate(in, outp, loop);
            }else
            {
                cmd = readTemplate(in, NULL, loop);
            }
        }
    }
    return;
}

// --------------------------------------
void    Template::readLoop(Stream &in, Stream *outp, int loop)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (!inSelectedFragment() || !outp)
            {
                readTemplate(in, NULL, 0);
                return;
            }

            int cnt = getIntVariable(var, loop);

            if (cnt)
            {
                int spos = in.getPosition();
                for (int i=0; i<cnt; i++)
                {
                    in.seekTo(spos);
                    readTemplate(in, outp, i);
                }
            }else
            {
                readTemplate(in, NULL, 0);
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
json::array_t Template::evaluateCollectionVariable(String& varName)
{
    if (varName == "channelsFound")
    {
        JrpcApi api;
        LOG_DEBUG("%s", api.getChannelsFound({}).dump().c_str());
        json::array_t cs = api.getChannelsFound({});
        // ジャンル接頭辞で始まらないチャンネルは掲載しない。
        cs.erase(std::remove_if(cs.begin(), cs.end(),
                                [] (json c) { return !str::is_prefix_of(servMgr->genrePrefix, c["genre"]); }),
                 cs.end());
        return cs;
    }else if (varName == "broadcastingChannels")
    {
        // このサーバーから配信中のチャンネルをリスナー数降順でソート。
        JrpcApi api;
        json::array_t channels = api.getChannels({});
        auto newend = std::remove_if(channels.begin(), channels.end(),
                                  [] (json channel)
                                  { return !channel["status"]["isBroadcasting"]; });
        std::sort(channels.begin(), newend,
                  [] (json a, json b)
                  {
                      return a["status"]["totalDirects"] < b["status"]["totalDirects"];
                  });

        return json::array_t(channels.begin(), newend);
    }else if (varName == "externalChannels")
    {
        auto channels = JrpcApi().getYPChannelsInternal({});
        return channels;
    }else
    {
        return {};
    }
}

// --------------------------------------
void    Template::readForeach(Stream &in, Stream *outp, int loop)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (!inSelectedFragment() || !outp)
            {
                readTemplate(in, NULL, loop);
                return;
            }

            auto coll = evaluateCollectionVariable(var);

            if (coll.size() == 0)
            {
                readTemplate(in, NULL, loop);
            }else
            {
                auto outer = currentElement;
                int start = in.getPosition();
                for (size_t i = 0; i < coll.size(); i++)
                {
                    in.seekTo(start);
                    currentElement = coll[i];
                    readTemplate(in, outp, i); // loop
                }
                currentElement = outer;
            }
            return;
        }else
        {
            var.append(c);
        }
    }
}

// --------------------------------------
int Template::readCmd(Stream &in, Stream *outp, int loop)
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
                readLoop(in, outp, loop);
                tmpl = TMPL_LOOP;
            }else if (cmd == "if")
            {
                readIf(in, outp, loop);
                tmpl = TMPL_IF;
            }else if (cmd == "elsif")
            {
                tmpl = TMPL_ELSIF;
            }else if (cmd == "fragment")
            {
                readFragment(in, outp, loop);
                tmpl = TMPL_FRAGMENT;
            }else if (cmd == "foreach")
            {
                readForeach(in, outp, loop);
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
void    Template::readVariable(Stream &in, Stream *outp, int loop)
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

                writeVariable(mem, var, loop);
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
void    Template::readVariableJavaScript(Stream &in, Stream *outp, int loop)
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

                writeVariable(mem, var, loop);
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
void    Template::readVariableRaw(Stream &in, Stream *outp, int loop)
{
    String var;
    while (!in.eof())
    {
        char c = in.readChar();
        if (c == '}')
        {
            if (inSelectedFragment() && outp)
            {
                writeVariable(*outp, var, loop);
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
int Template::readTemplate(Stream &in, Stream *outp, int loop)
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
                readVariable(in, outp, loop);
            }
            else if (c == '\\')
            {
                readVariableJavaScript(in, outp, loop);
            }
            else if (c == '!')
            {
                readVariableRaw(in, outp, loop);
            }
            else if (c == '@')
            {
                int t = readCmd(in, outp, loop);
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
bool HTTPRequestScope::writeVariable(Stream& s, const String& varName, int loop)
{
    if (varName == "request.host")
    {
        if (m_request.headers.get("Host").empty())
        {
            servMgr->writeVariable(s, "serverIP");
            s.writeString(":");
            servMgr->writeVariable(s, "serverPort");
        }
        else
        {
            s.writeString(m_request.headers.get("Host"));
        }
        return true;
    }else if (varName == "request.path") // HTTPRequest に委譲すべきか
    {
        s.writeString(m_request.path);
        return true;
    }

    return false;
}
