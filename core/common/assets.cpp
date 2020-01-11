#include "sstream.h"
#include "assets.h"

using namespace std;

// ------------------------------------------------------------
static string MIMEType(const string& path)
{
    using namespace str;

    if (contains(path, ".htm"))
    {
        return MIME_HTML;
    }else if (contains(path, ".css"))
    {
        return MIME_CSS;
    }else if (contains(path, ".jpg"))
    {
        return MIME_JPEG;
    }else if (contains(path, ".gif"))
    {
        return MIME_GIF;
    }else if (contains(path, ".png"))
    {
        return MIME_PNG;
    }else if (contains(path, ".js"))
    {
        return MIME_JS;
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

// ------------------------------------------------------------
HTTPResponse AssetsController::operator()(const HTTPRequest& req, Stream& stream, Host& remoteHost)
{
    auto path = mapper.toLocalFilePath(req.path);

    if (path.empty())
        return HTTPResponse::notFound();

    StringStream mem;
    FileStream   file;

    file.openReadOnly(path.c_str());
    file.writeTo(mem, file.length());

    string body = mem.str();
    map<string,string> headers = {
        {"Content-Type",MIMEType(path)},
        {"Content-Length",to_string(body.size())}
    };
    return HTTPResponse::ok(headers, body);
}
