#include "mapper.h"
#include "str.h"

FileSystemMapper::FileSystemMapper(const std::string& aVirtualPath, const std::string& aDocumentRoot)
    : virtualPath(aVirtualPath)
    , documentRoot(aDocumentRoot)
{
}

// TODO: ちゃんとディレクトリトラバーサルを防ぐ。
std::string FileSystemMapper::toLocalFilePath(const std::string& vpath)
{
    return str::replace_prefix(vpath, virtualPath, documentRoot);
}
