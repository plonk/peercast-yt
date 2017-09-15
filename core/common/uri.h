#ifndef _URI_H
#define _URI_H

#include <memory>
#include <string>

// forward-declare LUrlParser::clParseURL
namespace LUrlParser { class clParseURL; }

class URI
{
public:
    URI(const std::string& uriString);
    ~URI();

    bool        isValid();
    std::string scheme();
    std::string user_info();
    std::string host();
    int         port();
    std::string path();
    std::string query();
    std::string fragment();

    std::shared_ptr<LUrlParser::clParseURL> m_uri;

    static int defaultPort(const std::string&);
};

#endif
