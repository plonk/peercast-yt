#ifndef _HTML_H
#define _HTML_H

// ---------------------------------------
#include "xml.h"

// ---------------------------------------
class HTML : public XML
{
public:
	HTML(const char *);

	
	void	startNode(XML::Node *, const char * = NULL);
	void	addLink(const char *, const char *);
	void	addArgLink(const char *, const char *);
	XML::Node *startTag(const char *, const char * = NULL,...);
	XML::Node *startTagEnd(const char *, const char * = NULL,...);
	void	startSingleTagEnd(const char *,...);
	void	startTableRow(int);
	void	end();
	void	addRefresh(int);

	char	defArgs[128];
	XML::Node	*currNode,*headNode,*htmlNode;
};

#endif
