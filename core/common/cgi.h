#ifndef _CGI_H
#define _CGI_H

#include <string>
#include <vector>
#include <map>

namespace cgi {

std::string escape(const std::string&);
std::string unescape(const std::string&);

class Query
{
public:
    Query(const std::string& queryString);

    bool hasKey(const std::string&);
    std::string get(const std::string&);
    std::vector<std::string> getAll(const std::string&);

    std::map<std::string,std::vector<std::string> > m_dict;
};

}

#endif
