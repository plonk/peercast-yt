#include "html.h"
#include <stdarg.h>

// --------------------------------------
HTML::HTML(const char *title)
{
	defArgs[0] = 0;
	htmlNode = new XML::Node("html");

	currNode = htmlNode;
	setRoot(htmlNode);

	headNode = startTag("head");
		startTagEnd("title",title);
	end();

	headNode->add(new XML::Node("meta http-equiv=\"Content-Type\" content=\"text/html; charset=iso-8859-1\""));

}
// --------------------------------------
void HTML::addRefresh(int sec)
{
	if (sec > 0)
		headNode->add(new XML::Node("meta http-equiv=\"refresh\" content=\"%d\"",sec));
}
// --------------------------------------
void HTML::startNode(XML::Node *n, const char *text)
{
	if (text)
		n->setContent(text);
	currNode->add(n);
	currNode = n;
}
// --------------------------------------
void HTML::addLink(const char *url, const char *text)
{
	startNode(new XML::Node("a href=\"%s\"",url),text);
	end();
}
// --------------------------------------
void HTML::addArgLink(const char *url, const char *text)
{
	startNode(new XML::Node("a href=\"%s&%s\"",url,defArgs),text);
	end();
}
// --------------------------------------
XML::Node *HTML::startTag(const char *tag, const char *fmt,...)
{
	XML::Node *n;
	if (fmt)
	{

		va_list ap;
  		va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(n=new XML::Node(tag),tmp);

	   	va_end(ap);	
	}else{
		startNode(n=new XML::Node(tag),NULL);
	}
	return n;
}
// --------------------------------------
XML::Node *HTML::startTagEnd(const char *tag, const char *fmt,...)
{
	XML::Node *n;
	if (fmt)
	{

		va_list ap;
  		va_start(ap, fmt);

		char tmp[512];
		vsprintf(tmp,fmt,ap);
		startNode(n=new XML::Node(tag),tmp);

	   	va_end(ap);	
	}else{
		startNode(n=new XML::Node(tag),NULL);
	}
	end();
	return n;
}
// --------------------------------------
void HTML::startSingleTagEnd(const char *fmt,...)
{
	va_list ap;
	va_start(ap, fmt);

	char tmp[512];
	vsprintf(tmp,fmt,ap);
	startNode(new XML::Node(tmp));

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
// --------------------------------------
void HTML::end()
{
	currNode = currNode->parent;
}
