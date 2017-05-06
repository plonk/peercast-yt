#ifndef _PUBLIC_H
#define _PUBLIC_H

#include "mapper.h"
#include "http.h"

class PublicController
{
public:
    PublicController(const std::string& documentRoot);
    HTTPResponse operator()(const HTTPRequest&, Stream&, Host&);
    std::string MIMEType(const std::string& path);
    std::string createChannelIndex();

    FileSystemMapper mapper;
};

#endif
