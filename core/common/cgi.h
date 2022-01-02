#ifndef _CGI_H
#define _CGI_H

#include <string>
#include <vector>
#include <map>
#include <ctime>

namespace cgi {

std::string escape(const std::string&);
std::string unescape(const std::string&);
std::string rfc1123Time(time_t t);
time_t parseHttpDate(const std::string& str);
std::string unescape_html(const std::string& input);
std::string escape_html(const std::string& input);
std::string escape_javascript(const std::string& input);

class Query
{
public:
    Query(const std::string& queryString = "");

    bool hasKey(const std::string&);
    std::string get(const std::string&);
    std::vector<std::string> getAll(const std::string&);
    void add(const std::string&, const std::string&);
    std::string str();

    std::map<std::string,std::vector<std::string> > m_dict;
};

}

#endif
