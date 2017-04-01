#ifndef _CGI_H
#define _CGI_H

#include <string>

namespace cgi {

std::string escape(const std::string&);
std::string unescape(const std::string&);

}

#endif
