// ------------------------------------------------
// File : html.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		HTML protocol handling
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


#include <stdarg.h>
#include <stdlib.h>
#include "html.h"
#include "http.h"
#include "stream.h"
#include "gnutella.h"
#include "servmgr.h"
#include "channel.h"
#include "stats.h"
#include "version2.h"


// --------------------------------------
HTML::HTML(const char *t, Stream &o)
{
	out = &o;
	out->writeCRLF = false;

	title.set(t);
	tagLevel = 0;
	refresh = 0;
}

// --------------------------------------
void HTML::writeOK(const char *content)
{
	out->writeLine(HTTP_SC_OK);
	out->writeLineF("%s %s",HTTP_HS_SERVER,PCX_AGENT);
	//out->writeLine("%s %s",HTTP_HS_CACHE,"no-cache");
	out->writeLineF("%s %s",HTTP_HS_CONNECTION,"close");
    out->writeLineF("%s %s",HTTP_HS_CONTENT,content);
	out->writeLine("");
}
// --------------------------------------
void HTML::writeVariable(Stream &s,const String &varName, int loop)
{
	bool r=false;
	if (varName.startsWith("servMgr."))
		r=servMgr->writeVariable(s,varName+8);
	else if (varName.startsWith("chanMgr."))
		r=chanMgr->writeVariable(s,varName+8,loop);
	else if (varName.startsWith("stats."))
		r=stats.writeVariable(s,varName+6);
	else if (varName.startsWith("sys."))
	{
		if (varName == "sys.log.dumpHTML")
		{
			sys->logBuf->dumpHTML(s);
			r=true;
		}
	}
	else if (varName.startsWith("loop."))
	{
		if (varName.startsWith("loop.channel."))
		{
			Channel *ch = chanMgr->findChannelByIndex(loop);
			if (ch)
				r = ch->writeVariable(s,varName+13,loop);
		}else if (varName.startsWith("loop.servent."))
		{
			Servent *sv = servMgr->findServentByIndex(loop);
			if (sv)
				r = sv->writeVariable(s,varName+13);
		}else if (varName.startsWith("loop.filter."))
		{
			ServFilter *sf = &servMgr->filters[loop];
			r = sf->writeVariable(s,varName+12);

		}else if (varName.startsWith("loop.bcid."))
		{
			BCID *bcid = servMgr->findValidBCID(loop);
			if (bcid)
				r = bcid->writeVariable(s,varName+10);
		
		}else if (varName == "loop.indexEven")
		{
			s.writeStringF("%d",(loop&1)==0);
			r = true;
		}else if (varName == "loop.index")
		{
			s.writeStringF("%d",loop);
			r = true;
		}else if (varName.startsWith("loop.hit."))
		{
			char *idstr = getCGIarg(tmplArgs,"id=");
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
								r = ch->writeVariable(s,varName+9);
								break;
							}
							cnt++;
						}
						ch=ch->next;
					}

				}
			}
		}

	}
	else if (varName.startsWith("page."))
	{
		if (varName.startsWith("page.channel."))
		{
			char *idstr = getCGIarg(tmplArgs,"id=");
			if (idstr)
			{
				GnuID id;
				id.fromStr(idstr);
				Channel *ch = chanMgr->findChannelByID(id);
				if (ch)
					r = ch->writeVariable(s,varName+13,loop);
			}
		}else
		{

			String v = varName+5;
			v.append('=');
			char *a = getCGIarg(tmplArgs,v);
			if (a)
			{
				s.writeString(a);		
				r=true;
			}
		}
	}


	if (!r)
		s.writeString(varName);
}
// --------------------------------------
int HTML::getIntVariable(const String &varName,int loop)
{
	String val;
	MemoryStream mem(val.cstr(),String::MAX_LEN);

	writeVariable(mem,varName,loop);

	return atoi(val.cstr());
}
// --------------------------------------
bool HTML::getBoolVariable(const String &varName,int loop)
{
	String val;
	MemoryStream mem(val.cstr(),String::MAX_LEN);

	writeVariable(mem,varName,loop);

	// integer
	if ((val[0] >= '0') && (val[0] <= '9'))
		return atoi(val.cstr()) != 0;	

	// string
	if (val[0]!=0)
		return true;	

	return false;
}

// --------------------------------------
void	HTML::readIf(Stream &in,Stream *outp,int loop)
{
	String var;
	bool varCond=true;

	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '}')
		{
			if (getBoolVariable(var,loop)==varCond)
			{
				if (readTemplate(in,outp,loop))
					readTemplate(in,NULL,loop);
			}else{
				if (readTemplate(in,NULL,loop))
					readTemplate(in,outp,loop);
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
void	HTML::readLoop(Stream &in,Stream *outp,int loop)
{
	String var;
	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '}')
		{
			int cnt = getIntVariable(var,loop);

			if (cnt)
			{
				int spos = in.getPosition();
				for(int i=0; i<cnt; i++)
				{
					in.seekTo(spos);
					readTemplate(in,outp,i);
				}
			}else
			{
				readTemplate(in,NULL,0);
			}
			return;

		}else
		{
			var.append(c);
		}
	}

}

// --------------------------------------
int HTML::readCmd(Stream &in,Stream *outp,int loop)
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
				readLoop(in,outp,loop);
				tmpl = TMPL_LOOP;
			}else if (cmd == "if")
			{
				readIf(in,outp,loop);
				tmpl = TMPL_IF;
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
void	HTML::readVariable(Stream &in,Stream *outp,int loop)
{
	String var;
	while (!in.eof())
	{
		char c = in.readChar();
		if (c == '}')
		{
			if (outp)
				writeVariable(*outp,var,loop);
			return;
		}else
		{
			var.append(c);
		}
	}

}
// --------------------------------------
bool HTML::readTemplate(Stream &in,Stream *outp,int loop)
{
	while (!in.eof())
	{
		char c = in.readChar();

		if (c == '{')
		{
			c = in.readChar();
			if (c == '$')
			{
				readVariable(in,outp,loop);
			}
			else if (c == '@')
			{
				int t = readCmd(in,outp,loop);
				if (t == TMPL_END)
					return false;
				else if (t == TMPL_ELSE)
					return true;
			}
			else
				throw StreamException("Unknown template escape");
		}else
		{
			if (outp)
				outp->writeChar(c);
		}
	}
	return false;
}

// --------------------------------------
void HTML::writeTemplate(const char *fileName, const char *args)
{
	FileStream file;
	try
	{
		file.openReadOnly(fileName);

		tmplArgs = args;
		readTemplate(file,out,0);

	}catch(StreamException &e)
	{
		out->writeString(e.msg);
		out->writeString(" : ");
		out->writeString(fileName);
	}

	file.close();
}
// --------------------------------------
void HTML::writeRawFile(const char *fileName)
{
	FileStream file;
	try
	{
		file.openReadOnly(fileName);

		file.writeTo(*out,file.length());

	}catch(StreamException &)
	{
	}

	file.close();
}

// --------------------------------------
void HTML::locateTo(const char *url)
{
	out->writeLine(HTTP_SC_FOUND);
	out->writeLineF("Location: %s",url);
	out->writeLine("");
}
// --------------------------------------
void HTML::startHTML()
{
	startNode("html");
}
// --------------------------------------
void HTML::startBody()
{
	startNode("body");
}
// --------------------------------------
void HTML::addHead()
{
	char buf[512];
		startNode("head");
			startTagEnd("title",title.cstr());
			startTagEnd("meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\"");

			if (!refreshURL.isEmpty())
			{
				sprintf(buf,"meta http-equiv=\"refresh\" content=\"%d;URL=%s\"",refresh,refreshURL.cstr());
				startTagEnd(buf);
			}else if (refresh)
			{
				sprintf(buf,"meta http-equiv=\"refresh\" content=\"%d\"",refresh);
				startTagEnd(buf);
			}


		end();
}
// --------------------------------------
void HTML::addContent(const char *s)
{
	out->writeString(s);
}
// --------------------------------------
void HTML::startNode(const char *tag, const char *data)
{
	const char *p = tag;
	char *o = &currTag[tagLevel][0];

	int i;
	for(i=0; i<MAX_TAGLEN-1; i++)
	{
		char c = *p++;
		if ((c==0) || (c==' '))
			break;
		else
			*o++ = c;
	}
	*o = 0;

	out->writeString("<");
	out->writeString(tag);
	out->writeString(">");
	if (data)
		out->writeString(data);

	tagLevel++;
	if (tagLevel >= MAX_TAGLEVEL)
		throw StreamException("HTML too deep!");
}
// --------------------------------------
void HTML::end()
{
	tagLevel--;
	if (tagLevel < 0)
		throw StreamException("HTML premature end!");

	out->writeString("</");
	out->writeString(&currTag[tagLevel][0]);
	out->writeString(">");
}
// --------------------------------------
void HTML::addLink(const char *url, const char *text, bool toblank)
{
	char buf[1024];

	sprintf(buf,"a href=\"%s\" %s",url,toblank?"target=\"_blank\"":"");
	startNode(buf,text);
	end();
}
// --------------------------------------
void HTML::startTag(const char *tag, const char *fmt,...)
{
	if (fmt)
	{

		va_list ap;
  		va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(tag,tmp);

	   	va_end(ap);	
	}else{
		startNode(tag,NULL);
	}
}
// --------------------------------------
void HTML::startTagEnd(const char *tag, const char *fmt,...)
{
	if (fmt)
	{

		va_list ap;
  		va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(tag,tmp);

	   	va_end(ap);	
	}else{
		startNode(tag,NULL);
	}
	end();
}
// --------------------------------------
void HTML::startSingleTagEnd(const char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);

	char tmp[512];
	vsprintf(tmp,fmt,ap);
	startNode(tmp);

	va_end(ap);	
	end();
}

// --------------------------------------
void HTML::startTableRow(int i)
{
	if (i & 1)
		startTag("tr bgcolor=\"#dddddd\" align=\"left\"");
	else
		startTag("tr bgcolor=\"#eeeeee\" align=\"left\"");
}
