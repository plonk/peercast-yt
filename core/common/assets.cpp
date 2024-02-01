#include "sstream.h"
#include "assets.h"

using namespace std;

// ------------------------------------------------------------
static string MIMEType(const string& path)
{
    using namespace str;

    if (has_suffix(path, ".htm") || has_suffix(path, ".html"))
    {
        return MIME_HTML;
    }else if (has_suffix(path, ".css"))
    {
        return MIME_CSS;
    }else if (has_suffix(path, ".jpg"))
    {
        return MIME_JPEG;
    }else if (has_suffix(path, ".gif"))
    {
        return MIME_GIF;
    }else if (has_suffix(path, ".png"))
    {
        return MIME_PNG;
    }else if (has_suffix(path, ".js"))
    {
        return MIME_JS;
    }else if (has_suffix(path, ".svg"))
    {
        return "image/svg+xml";
    }else if (has_suffix(path, ".ico"))
    {
        return "image/vnd.microsoft.icon";
    }else
    {
        return "application/octet-stream";
    }
}

// ------------------------------------------------------------
AssetsController::AssetsController(const std::string& documentRoot)
    : mapper("/assets", documentRoot)
{
}

// --------------------------------------
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

static time_t mtime(const char *path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        return -1;
    else
        return st.st_mtime;
}

// ------------------------------------------------------------
#include "cgi.h"
HTTPResponse AssetsController::operator()(const HTTPRequest& req, Stream& stream, Host& remoteHost)
{
    auto path = mapper.toLocalFilePath(req.path);

    if (path.empty())
        return HTTPResponse::notFound();

    StringStream mem;
    FileStream   file;

    time_t last_modified = mtime(path.c_str());
    time_t if_modified_since = -1;

    if (req.headers.get("If-Modified-Since").size())
        if_modified_since = cgi::parseHttpDate(req.headers.get("If-Modified-Since"));

    if (last_modified != -1 && if_modified_since != -1)
        if (last_modified <= if_modified_since)
            return HTTPResponse::notModified({{ "Last-Modified", cgi::rfc1123Time(last_modified) }});

    file.openReadOnly(path.c_str());
    file.writeTo(mem, file.length());

    string body = mem.str();
    map<string,string> headers = {
        {"Content-Type",MIMEType(path)},
        {"Content-Length",to_string(body.size())}
    };

    if (last_modified != -1)
        headers["Last-Modified"] = cgi::rfc1123Time(last_modified);

    return HTTPResponse::ok(headers, body);
}
