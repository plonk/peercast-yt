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

#include "template.h"

#include "servmgr.h"
#include "stats.h"
#include "dmstream.h"
#include "notif.h"
#include "str.h"
#include "jrpc.h"

using json = nlohmann::json;

// --------------------------------------
static json::object_t array_to_object(json::array_t arr)
{
    json::object_t obj;
    for (int i = 0; i < arr.size(); i++)
    {
        obj[std::to_string(i)] = arr[i];
    }
    return obj;
}

// --------------------------------------
bool Template::writeObjectProperty(Stream& s, const String& varName, json::object_t object)
{
    LOG_DEBUG("writeObjectProperty %s", varName.str().c_str());
    auto names = str::split(varName.str(), ".");

    if (names.size() == 1)
    {
        try
        {
            json value = object.at(varName.str());
            if (value.is_string())
            {
                std::string str = value;
                s.writeString(str.c_str());
            }else
                s.writeString(value.dump().c_str());
        }catch (std::out_of_range&)
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
        } catch (std::out_of_range&)
        {
            return false;
        }
        return true;
    }
}

// --------------------------------------
void Template::writeVariable(Stream &s, const String &varName, int loop)
{
    bool r = false;
    if (varName.startsWith("servMgr."))
        r = servMgr->writeVariable(s, varName+8);
    else if (varName.startsWith("chanMgr."))
        r = chanMgr->writeVariable(s, varName+8, loop);
    else if (varName.startsWith("stats."))
        r = stats.writeVariable(s, varName+6);
    else if (varName.startsWith("sys."))
    {
        if (varName == "sys.log.dumpHTML")
        {
            sys->logBuf->dumpHTML(s);
            r = true;
        }else if (varName == "sys.time")
        {
            s.writeString(std::to_string(sys->getTime()).c_str());
            r = true;
        }
    }
    else if (varName.startsWith("notificationBuffer."))
        r = g_notificationBuffer.writeVariable(s, varName + strlen("notificationBuffer."));
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
        }else if (varName.startsWith("loop.this."))
        {
            r = writeObjectProperty(s, varName + strlen("loop.this."), currentElement);
        }
    }
    else if (varName.startsWith("page."))
    {
        if (varName.startsWith("page.channel."))
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

    if (!r)
        s.writeString(varName);
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

    const std::string val = mem.str();

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
    std::string fragName;

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
void    Template::readIf(Stream &in, Stream *outp, int loop)
{
    String var;
    bool varCond=true;

    while (!in.eof())
    {
        char c = in.readChar();

        if (c == '}')
        {
            if (getBoolVariable(var, loop)==varCond)
            {
                if (readTemplate(in, outp, loop))
                    readTemplate(in, NULL, loop);
            }else{
                if (readTemplate(in, NULL, loop))
                    readTemplate(in, outp, loop);
            }
            return;
        }else if (c == '!')
        {
            varCond = !varCond;
        }else
        {
            var.append(c);
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
    if (varName == "testCollection")
    {
        json::array_t res;

        res.push_back(json::object({ { "a", 1 } }));
        res.push_back(json::object({ { "a", 2 } }));
        res.push_back(json::object({ { "a", 3 } }));
        return res;
    // }else if (varName == "channelsFound")
    // {
    //     JrpcApi api;
    //     LOG_DEBUG("%s", api.getChannelsFound({}).dump().c_str());
    //     return api.getChannelsFound({});
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
            auto coll = evaluateCollectionVariable(var);

            if (coll.size() == 0)
            {
                readTemplate(in, NULL, loop);
            }else
            {
                auto outer = currentElement;
                int start = in.getPosition();
                for (int i = 0; i < coll.size(); i++)
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
