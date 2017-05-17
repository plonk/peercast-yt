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
#include "dmstream.h"
#include "notif.h"
#include "str.h"
#include "jrpc.h"

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
    m_variableWriters["notificationBufer"] = &g_notificationBuffer;
}

// --------------------------------------
Template::Template(const char* args)
    : currentElement(json::object({}))
{
    if (args)
        tmplArgs = strdup(args);
    else
        tmplArgs = NULL;
    initVariableWriters();
}

// --------------------------------------
Template::Template(const std::string& args)
    : currentElement(json::object({}))
{
    tmplArgs = strdup(args.c_str());
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

    if (varName.startsWith("sys."))
    {
        if (varName == "sys.log.dumpHTML")
        {
            sys->logBuf->dumpHTML(s);
            r = true;
        }else if (varName == "sys.time")
        {
            s.writeString(to_string(sys->getTime()).c_str());
            r = true;
        }
    }
    else if (varName.startsWith("loop."))
    {
        if (varName.startsWith("loop.channel."))
        {
            Channel *ch = chanMgr->findChannelByIndex(loop);
            if (ch)
                r = ch->writeVariable(s, varName+13, loop);
        }else if (varName.startsWith("loop.servent."))
        {
            Servent *sv = servMgr->findServentByIndex(loop);
            if (sv)
                r = sv->writeVariable(s, varName+13);
        }else if (varName.startsWith("loop.filter."))
        {
            ServFilter *sf = &servMgr->filters[loop];
            r = sf->writeVariable(s, varName+12);
        }else if (varName.startsWith("loop.bcid."))
        {
            BCID *bcid = servMgr->findValidBCID(loop);
            if (bcid)
                r = bcid->writeVariable(s, varName+10);
        }else if (varName == "loop.indexEven")
        {
            s.writeStringF("%d", (loop&1)==0);
            r = true;
        }else if (varName == "loop.index")
        {
            s.writeStringF("%d", loop);
            r = true;
        }else if (varName == "loop.indexBaseOne")
        {
            s.writeStringF("%d", loop + 1);
            r = true;
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
                                r = ch->writeVariable(s, varName+9);
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
            r = servMgr->channelDirectory.writeVariable(s, varName + strlen("loop."), loop);
        }else if (varName.startsWith("loop.channelFeed."))
        {
            r = servMgr->channelDirectory.writeVariable(s, varName + strlen("loop."), loop);
        }else if (varName.startsWith("loop.notification."))
        {
            r = g_notificationBuffer.writeVariable(s, varName + strlen("loop."), loop);
        }
    }else if (varName.startsWith("this."))
    {
        r = writeObjectProperty(s, varName + strlen("this."), currentElement);
    }else if (varName.startsWith("page."))
    {
        if (varName == "page.channel.exist")
        {
            const char *idstr = getCGIarg(tmplArgs, "id=");
            if (idstr)
            {
                GnuID id;
                id.fromStr(idstr);
                Channel *ch = chanMgr->findChannelByID(id);
                if (ch)
                    s.writeString("1");
                else
                    s.writeString("0");
                r = true;
            }
        }if (varName.startsWith("page.channel."))
        {
            const char *idstr = getCGIarg(tmplArgs, "id=");
            if (idstr)
            {
                GnuID id;
                id.fromStr(idstr);
                Channel *ch = chanMgr->findChannelByID(id);
                if (ch)
                    r = ch->writeVariable(s, varName+13, loop);
            }
        }else
        {
            String v = varName+5;
            v.append('=');
            const char *a = getCGIarg(tmplArgs, v);
            if (a)
            {
                s.writeString(a);
                r = true;
            }
        }
    }else if (varName == "TRUE")
    {
        s.writeString("1");
        r = true;
    }else if (varName == "FALSE")
    {
        s.writeString("0");
        r = true;
    }

End:
    if (!r)
        s.writeString(varName);
}

// --------------------------------------
string Template::getStringVariable(const string& varName, int loop)
{
    DynamicMemoryStream mem;

    writeVariable(mem, varName.c_str(), loop);

    return mem.str();
}

// --------------------------------------
int Template::getIntVariable(const String &varName, int loop)
{
    DynamicMemoryStream mem;

    writeVariable(mem, varName, loop);

    return atoi(mem.str().c_str());
}

// --------------------------------------
bool Template::getBoolVariable(const String &varName, int loop)
{
    DynamicMemoryStream mem;

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
            bool elsefound = false;
            elsefound = readTemplate(in, outp, loop);
            if (elsefound)
                LOG_ERROR("Stray else in fragment block");
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
        }else if (str::is_prefix_of("==", s)
                  || str::is_prefix_of("!=", s))
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

    if (tokens.size() == 3)
    {
        auto op = tokens[1];
        if (op != "==" && op != "!=")
            throw StreamException(("Unrecognized condition operator " + op).c_str());

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
void    Template::readIf(Stream &in, Stream *outp, int loop)
{
    String cond;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (evalCondition(cond, loop))
            {
                if (readTemplate(in, outp, loop))
                    readTemplate(in, NULL, loop);
            }else
            {
                if (readTemplate(in, NULL, loop))
                    readTemplate(in, outp, loop);
            }
            return;
        }else
        {
            cond.append(c);
        }
    }
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
        return api.getChannelsFound({});
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
    }else if (varName == "publicExternalChannels")
    {
        auto channels = JrpcApi().getYPChannelsInternal({});
        auto end = std::remove_if(channels.begin(), channels.end(),
                                  [&] (json c)
                                  { return !c["isPublic"]; });
        return json::array_t(channels.begin(), end);
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
                DynamicMemoryStream mem;

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
                DynamicMemoryStream mem;

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

// in の現在の位置から 1 ブロック分のテンプレートを処理し、outp が
// NULL でなければ *outp に出力する。{@loop} 内を処理している場合は、0
// から始まるループカウンターの値が loop に設定される。EOF あるいは
// {@end} に当たった場合は false を返し、{@else} に当たった場合は true
// を返す。
bool Template::readTemplate(Stream &in, Stream *outp, int loop)
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
                if (t == TMPL_END)
                    return false;
                else if (t == TMPL_ELSE)
                    return true;
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
    return false;
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
            return true;
        }
    }

    return false;
}
