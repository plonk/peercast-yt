#include <limits.h>
#include <stdlib.h>

#include "common.h"
#include "mapper.h"
#include "str.h"
#include "_string.h"

FileSystemMapper::FileSystemMapper(const std::string& aVirtualPath, const std::string& aDocumentRoot)
    : virtualPath(aVirtualPath)
{
    char *dr = realpath(aDocumentRoot.c_str(), NULL);
    if (!dr)
    {
        throw GeneralException(String::format("Document root `%s` inaccessible", aDocumentRoot.c_str()));
    }
    documentRoot = dr;
    free(dr);
}

std::string FileSystemMapper::toLocalFilePath(const std::string& vpath)
{
    return toLocalFilePath(vpath, {}).first;
}

std::pair<std::string,std::string> FileSystemMapper::resolvePath(const std::string& rawPath, const std::vector<std::string>& langs)
{
    // if there's a language neutral version, return it
    if (realPath(rawPath) != "")
        return std::make_pair(rawPath, "");

    // otherwise, try each of the extensions
    for (auto ext : langs)
    {
        auto r = realPath(rawPath + "." + ext);
        if (r != "")
            return std::make_pair(r, ext);
    }

    // default to the English version if there is one
    auto r = realPath(rawPath + ".en");
    if (r != "")
        return std::make_pair(r, "en");

    return std::make_pair("", "");
}

std::string FileSystemMapper::realPath(const std::string& path)
{
    char resolvedPath[PATH_MAX];
    char *p = realpath(path.c_str(), resolvedPath);

    if (!p)
        return "";
    else
        return resolvedPath;
}

std::pair<std::string,std::string> FileSystemMapper::toLocalFilePath(const std::string& vpath, const std::vector<std::string>& langs)
{
    if (virtualPath == vpath || !str::is_prefix_of(virtualPath + "/", vpath))
        return std::make_pair<std::string,std::string>("", "");

    auto filePath = str::replace_prefix(vpath, virtualPath, documentRoot);

    std::string resolvedPath, resolvedLang;
    tie(resolvedPath, resolvedLang) = resolvePath(filePath, langs);

    if (resolvedPath == "")
    {
        LOG_ERROR("Cannot resolve path %s", filePath.c_str());
        return std::make_pair<std::string,std::string>("", "");
    }

    // ディレクトリトラバーサルチェック
    if (documentRoot == resolvedPath || !str::is_prefix_of(documentRoot, resolvedPath))
    {
        LOG_ERROR("Possible directory traversal attack!");
        return std::make_pair<std::string,std::string>("", "");
    }

    return std::make_pair<std::string,std::string>(std::string(resolvedPath), std::string(resolvedLang));
}
