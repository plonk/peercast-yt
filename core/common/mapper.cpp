#include <limits.h>
#include <stdlib.h>

#include "common.h"
#include "mapper.h"
#include "str.h"

FileSystemMapper::FileSystemMapper(const std::string& aVirtualPath, const std::string& aDocumentRoot)
    : virtualPath(aVirtualPath)
{
    char *dr = realpath(aDocumentRoot.c_str(), NULL);
    if (!dr)
        LOG_ERROR("Document root %s inaccessible", aDocumentRoot.c_str());
    documentRoot = dr;
    free(dr);
}

std::string FileSystemMapper::toLocalFilePath(const std::string& vpath)
{
    if (virtualPath == vpath ||
        !str::is_prefix_of(virtualPath + "/", vpath))
        return "";

    auto filePath = str::replace_prefix(vpath, virtualPath, documentRoot);

    char resolvedPath[PATH_MAX];

    char* p;
    p = realpath(filePath.c_str(), resolvedPath);

    if (p == NULL)
    {
        LOG_ERROR("Cannot resolve path %s", filePath.c_str());
        return "";
    }

    // ディレクトリトラバーサルチェック
    if (documentRoot == resolvedPath ||
        !str::is_prefix_of(documentRoot, resolvedPath))
        return "";

    return resolvedPath;
}
