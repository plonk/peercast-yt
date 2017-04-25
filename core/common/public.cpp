#include "public.h"
#include "str.h"

PublicController::PublicController(const std::string& documentRoot)
    : mapper("/public", documentRoot)
{
}

std::string PublicController::MIMEType(const std::string& path)
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

HTTPResponse PublicController::operator()(const HTTPRequest& request, Stream& stream, Host& remoteHost)
{
    auto fn = request.url.c_str();

    if (strcmp(fn, "/public/") == 0)
    {
        return HTTPResponse::redirectTo("/public/index.html");
    }else if (strcmp(fn, "/public/index.txt") == 0)
    {
        return HTTPResponse::ok(MIME_TEXT, {{"Content-Type","text/plain"}}, "hello");
    }else
    {
        String tmp = fn + 1;
        auto path = mapper.toLocalFilePath(tmp);
        if (path.empty())
            return HTTPResponse::notFound();
        else
        {
            //handshakeLocalFile(path.c_str());
            auto type = MIMEType(path);
            return HTTPResponse::ok(type, {{"Content-Type","text/plain"}}, "hoge");
        }
    }
}
