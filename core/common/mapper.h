#ifndef _MAPPER_H
#define _MAPPER_H

#include <string>

class FileSystemMapper
{
public:
    FileSystemMapper(const std::string&, const std::string&);

    std::string toLocalFilePath(const std::string&);

    std::string virtualPath;
    std::string documentRoot;
};

#endif
