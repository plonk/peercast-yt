#ifndef _BBS_H
#define _BBS_H

#include <memory>

namespace bbs {
    using namespace std;
    class Board;

    class Board
    {
    public:
        static shared_ptr<Board> tryCreate(const std::string& url);

        Board(const std::string& serverUrl, const std::string& boardName);

        std::string datUrl;
        std::string readUrl;
        std::string topUrl;
    };
    shared_ptr<Board> createBoard(const std::string& url);
}

#endif
