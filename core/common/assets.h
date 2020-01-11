#ifndef _ASSETS_H
#define _ASSETS_H

#include "mapper.h"
#include "http.h"

class AssetsController
{
public:
    AssetsController(const std::string& documentRoot);
    HTTPResponse operator()(const HTTPRequest&, Stream&, Host&);

    FileSystemMapper mapper;
};

#endif
