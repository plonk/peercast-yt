#include "public.h"
#include "str.h"
#include "dmstream.h"
#include "template.h"

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
        return HTTPResponse::ok({{"Content-Type","text/plain"}}, "hello");
    }else if (strcmp(fn, "/public/index.html") == 0)
    {
        FileStream file;
        DynamicMemoryStream mem;
        file.openReadOnly((mapper.documentRoot + "/index.html").c_str());
        HTTPRequestScope scope(request);

        Template(request.queryString).prependScope(scope).readTemplate(file, &mem, 0);
        return HTTPResponse::ok({{"Content-Type","text/html"}}, mem.str());
    }else
    {
        auto path = mapper.toLocalFilePath(fn);
        if (path.empty())
            return HTTPResponse::notFound();
        else
        {
            auto type = MIMEType(path);

            DynamicMemoryStream mem;
            FileStream file;
            std::map<std::string,std::string> additionalHeaders;

            try
            {
                file.openReadOnly(path.c_str());
                file.writeTo(mem, file.length());
            }catch (StreamException &)
            {
                LOG_DEBUG("StreamException in %s", __FUNCTION__);
            }
            file.close();

            return HTTPResponse::ok({{"Content-Type",type}}, mem.str());
        }
    }
}
