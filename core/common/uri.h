#ifndef _URI_H
#define _URI_H

#include <string>

// forward-declare boost::network::uri::uri
namespace boost { namespace network { namespace uri { class uri; } } }

class URI
{
public:
    URI(const std::string& uriString);
    ~URI();

    bool        isValid();
    std::string scheme();
    std::string host();
    int         port();
    std::string path();
    std::string fragment();

    boost::network::uri::uri* m_uri;

    static int defaultPort(const std::string&);
};

#endif
