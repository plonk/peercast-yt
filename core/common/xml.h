// ------------------------------------------------
// File : xml.h
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

#ifndef _XML_H
#define _XML_H

//-----------------------
#include "common.h"
#include <stdarg.h>

//-----------------------
class Stream;

//-----------------------
class XML
{
public:
	class Node
    {
    public:
    	class Attribute
        {
        	public:
                int namePos,valuePos;
        };

		Node(const char *,...);

		void	init();

        ~Node();
        
	    void	add(Node *);
        void	write(Stream &,int); 	// output, level
        char 	*getName() {return getAttrName(0);}

        char 	*getAttrValue(int i) {return &attrData[attr[i].valuePos];}
        char 	*getAttrName(int i) {return &attrData[attr[i].namePos];}
        char	*getContent()  {return contData; }
        int		getBinaryContent(void *, int);

        void	setAttributes(const char *);
        void	setContent(const char *);
        void	setBinaryContent(void *, int);

	    Node	*findNode(const char *);
        char	*findAttr(const char *);
        int		findAttrInt(const char *);
        int		findAttrID(const char *);

        char *contData,*attrData;

        Attribute	*attr;
        int	numAttr;

    	Node *child,*parent,*sibling;
        void *userPtr;
    };

    XML()
    {
    	root = NULL;
    }

    ~XML();

    void	setRoot(Node *n);
    void	write(Stream &);
    void	writeCompact(Stream &);
    void	writeHTML(Stream &);
    void	read(Stream &);
    Node	*findNode(const char *n);

    Node *root;
};
#endif
