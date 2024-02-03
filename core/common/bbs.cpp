#include "bbs.h"
#include "str.h"
#include "regexp.h"
#include "common.h"

namespace bbs {

    shared_ptr<Board> Board::tryCreate(const std::string& url)
    {
        Regexp NICHAN_THREAD_URL_PATTERN("^https?://[a-zA-z\\-\\.]+(?::\\d+)?/test/read\\.cgi\\/(\\w+)/(\\d+)/?$");
        auto caps = NICHAN_THREAD_URL_PATTERN.exec(url);
        if (caps.size() == 0)
            return nullptr;
        return std::make_shared<Board>(url, caps[1]);
    }

    Board::Board(const std::string& url, const std::string& boardName)
    {
        Regexp NICHAN_THREAD_URL_PATTERN("^(https?)://([a-zA-z\\-\\.]+(?::\\d+)?)/test/read\\.cgi\\/(\\w+)/(\\d+)/?$");

        auto caps = NICHAN_THREAD_URL_PATTERN.exec(url);
        if (caps.size() != 5)
            throw GeneralException("hoge");

        datUrl = str::STR(caps[1], "://", caps[2], "/", caps[3], "/dat/", caps[4], ".dat");
    }
//    shared_ptr<Board> createBoard(const std::string& url);

} // namespace bbs
