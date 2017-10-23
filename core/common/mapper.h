#ifndef _MAPPER_H
#define _MAPPER_H

#include <string>
#include <vector>
#include <tuple>

class FileSystemMapper
{
public:
    FileSystemMapper(const std::string& vpath, const std::string& droot);

    std::string toLocalFilePath(const std::string&);
    std::pair<std::string,std::string> toLocalFilePath(const std::string&, const std::vector<std::string>& langExts);
    static std::pair<std::string,std::string> resolvePath(const std::string& rawPath, const std::vector<std::string>& langs);
    static std::string realPath(const std::string& path);

    std::string virtualPath;
    std::string documentRoot;
};

#endif
