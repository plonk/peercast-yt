// ------------------------------------------------
// File : xml.cpp
// Date: 4-apr-2002
// Author: giles
// Desc: 
//		Basic XML parsing/creation 
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

#include "xml.h"
#include "stream.h"
#include <stdlib.h>
#include <stdarg.h>

// ----------------------------------
void XML::Node::add(Node *n)
{
	if (!n)
		return;

	n->parent = this;

    if (child)
    {
    	// has children, add to last sibling
        Node *s = child;
        while (s->sibling)
        	s = s->sibling;
        s->sibling = n;

    }else{
    	// no children yet
        child = n;
    }
}
// ---------------------------------
inline char nibsToByte(char n1, char n2)
{
	if (n1 >= 'A') n1 = n1-'A'+10;
	else n1 = n1-'0';
	if (n2 >= 'A') n2 = n2-'A'+10;
	else n2 = n2-'0';

    return ((n2&0xf)<<4)|(n1&0xf);
}

// ----------------------------------
int XML::Node::getBinaryContent(void *ptr, int size)
{
	char *in = contData;
    char *out = (char *)ptr;

    int i=0;
    while (*in)
    {
    	if (isWhiteSpace(*in))
        {
        	in++;
        }else
        {
        	if (i >= size)
            	throw StreamException("Too much binary data");
	    	out[i++] = nibsToByte(in[0],in[1]);
	        in+=2;
        }
    }
    return i;
}
// ----------------------------------
void XML::Node::setBinaryContent(void *ptr, int size)
{
	const char hexTable[] = "0123456789ABCDEF";
    const int lineWidth = 1023;

	contData = new char[size*2+1+(size/lineWidth)];

    char *bp = (char *)ptr;
    register char *ap = contData;

    for(register int i=0; i<size; i++)
    {
    	register char c = bp[i];
    	*ap++ = hexTable[c&0xf];
    	*ap++ = hexTable[(c>>4)&0xf];
        if ((i&lineWidth)==lineWidth)
	    	*ap++ = '\n';
    }
    ap[0] = 0;
}


// ----------------------------------
void XML::Node::setContent(const char *n)
{
	contData = strdup(n);
}
// ----------------------------------
void XML::Node::setAttributes(const char *n)
{
    char c;

	attrData = strdup(n);

    // count maximum amount of attributes
    int maxAttr = 1;		// 1 for tag name
    bool inQ = false;
    int i=0;
    while ((c=attrData[i++])!=0)
    {
    	if (c=='\"')
			inQ ^= true;

		if (!inQ)
			if (c=='=')
				maxAttr++;
    }


    attr = new Attribute[maxAttr];

    attr[0].namePos = 0;
    attr[0].valuePos = 0;

    numAttr=1;

    i=0;

    // skip until whitespace
    while (c=attrData[i++])
    	if (isWhiteSpace(c))
        	break;

    if (!c) return;	// no values

    attrData[i-1]=0;


    while ((c=attrData[i])!=0)
    {
    	if (!isWhiteSpace(c))
        {
			if (numAttr>=maxAttr)
				throw StreamException("Too many attributes");

			// get start of tag name
        	attr[numAttr].namePos = i;


			// skip whitespaces until next '='
			// terminate name on next whitespace or '='
            while (attrData[i])
			{
				c = attrData[i++];

				if ((c == '=') || isWhiteSpace(c))
				{
					attrData[i-1] = 0;	// null term. name
					if (c == '=')
                		break;
				}
			}

			// skip whitespaces
            while (attrData[i])
			{
	            if (isWhiteSpace(attrData[i]))
					i++;
				else
					break;
			}

			// check for valid start of attribute value - '"'
			if (attrData[i++] != '\"')
            	throw StreamException("Bad tag value");

            attr[numAttr++].valuePos = i;

			// terminate attribute value at next '"'

            while (attrData[i])
				if (attrData[i++] == '\"')
                	break;

			attrData[i-1] = 0;	// null term. value


        }else{
	        i++;
    	}
    }
}
// ----------------------------------
XML::Node::Node(const char *fmt,...)
{
	va_list ap;
  	va_start(ap, fmt);

	char tmp[8192];
	vsprintf(tmp,fmt,ap);
	setAttributes(tmp);

   	va_end(ap);	
	init();
}

// ----------------------------------
void XML::Node::init()
{
    parent = sibling = child = NULL;
    contData = NULL;
    userPtr = NULL;
}	
// ----------------------------------
int XML::Node::findAttrInt(const char *name)
{
	char *v = findAttr(name);
    if (!v) return 0;
    return atoi(v);
}
// ----------------------------------
int XML::Node::findAttrID(const char *name)
{
	char *v = findAttr(name);
    if (!v) return 0;
    return strToID(v);
}
// ----------------------------------
char *XML::Node::findAttr(const char *name)
{
	int nlen = strlen(name);
	for(int i=1; i<numAttr; i++)
    {
    	char *an = getAttrName(i);
    	if (strnicmp(an,name,nlen)==0)
        	return getAttrValue(i);
    }
    return NULL;
}
// ----------------------------------
void XML::Node::write(Stream &out, int level)
{
    int i;
#if 0
    char tabs[64];

    for(i=0; i<level; i++)
    	tabs[i] = ' ';
    tabs[i] = '\0';


    if (level)
	    out.write(tabs,i);
#endif
    char *name = getAttrValue(0);

    out.write("<",1);
    out.write(name,strlen(name));

    for(i=1; i<numAttr; i++)
    {
	    out.write(" ",1);
    	char *at = getAttrName(i);
	    out.write(at,strlen(at));

	    out.write("=\"",2);
        char *av = getAttrValue(i);
	    out.write(av,strlen(av));
	    out.write("\"",1);
    }

	if ((!contData) && (!child))
	{
	    out.write("/>\n",3);
	}else
	{
	    out.write(">\n",2);

	    if (contData)
		    out.write(contData,strlen(contData));

		if (child)
	    	child->write(out,level+1);
#if 0
	    if (level)
		    out.write(tabs,strlen(tabs));
#endif
	    out.write("</",2);
	    out.write(name,strlen(name));
	    out.write(">\n",2);
	}

    if (sibling)
    	sibling->write(out,level);
}
// ----------------------------------
XML::Node::~Node()
{
//	LOG("delete %s",getName());

	if (contData)
    	delete [] contData;
    if (attrData)
    	delete [] attrData;
    if (attr)
    	delete [] attr;

    Node *n = child;
    while (n)
    {
    	Node *nn = n->sibling;
    	delete n;
    	n = nn;
    }
}


// ----------------------------------
XML::~XML()
{
	if (root)
    	delete root;
}

// ----------------------------------
void XML::write(Stream &out)
{
	if (!root)
    	throw StreamException("No XML root");

	out.writeLine("<?xml version=\"1.0\" encoding=\"utf-8\" ?>");
    root->write(out,1);

}
// ----------------------------------
void XML::writeCompact(Stream &out)
{
	if (!root)
    	throw StreamException("No XML root");

	out.writeLine("<?xml ?>");
    root->write(out,1);

}
// ----------------------------------
void XML::writeHTML(Stream &out)
{
	if (!root)
    	throw StreamException("No XML root");

    root->write(out,1);
}

// ----------------------------------
void XML::setRoot(Node *n)
{
	root=n;
}


// ----------------------------------
XML::Node *XML::findNode(const char *n)
{
	if (root)
      	return root->findNode(n);
	else
		return NULL;
}


// ----------------------------------
XML::Node *XML::Node::findNode(const char *name)
{
   	if (stricmp(getName(),name)==0)
    	return this;

	XML::Node *c = child;

	while (c)
	{
		XML::Node *fn = c->findNode(name);
		if (fn)
			return fn;
		c=c->sibling;
	}

	return NULL;
}

// ----------------------------------
void XML::read(Stream &in)
{
	const int BUFFER_LEN = 100*1024;
	static char buf[BUFFER_LEN];

    Node *currNode=NULL;
    int tp=0;

    while (!in.eof())
    {
    	char c = in.readChar();

        if (c == '<')
        {
        	if (tp && currNode)	// check for content
            {
            	buf[tp] = 0;
            	currNode->setContent(buf);
            }
            tp = 0;

        	// read to next '>'
            while (!in.eof())
            {
            	c = in.readChar();
                if (c == '>')
                	break;

	        	if (tp >= BUFFER_LEN)
    	        	throw StreamException("Tag too long");

                buf[tp++] = c;
            }
            buf[tp]=0;

            if (buf[0] == '!')					// comment
            {
             	// do nothing
            }else if (buf[0] == '?')			// doc type
            {
            	if (strnicmp(&buf[1],"xml ",4))
                	throw StreamException("Not XML document");
            }else if (buf[0] == '/')			// end tag
            {
            	if (!currNode)
                	throw StreamException("Unexpected end tag");
            	currNode = currNode->parent;
            }else 	// new tag
            {
	            //LOG("tag: %s",buf);

            	bool singleTag = false;

                if (buf[tp-1] == '/')		   	// check for single tag
                {
                	singleTag = true;
                    buf[tp-1] = 0;
                }

				// only add valid tags
				if (strlen(buf))
				{
					Node *n = new Node(buf);

					if (currNode)
                		currNode->add(n);
					else
                		setRoot(n);

					if (!singleTag)
						currNode = n;
				}
            }

            tp = 0;
        } else {

        	if (tp >= BUFFER_LEN)
            	throw StreamException("Content too big");

        	buf[tp++] = c;

        }
    }
}

