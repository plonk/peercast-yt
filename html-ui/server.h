#include <functional>
#include "http.h"
#include <regex>
#include <memory>

class Server
{
public:
  typedef HTTPRequest Request;
  typedef HTTPResponse Response;
  typedef std::function<void(const std::smatch& match, const Request& req, Response& res)> Action;

  void Get(const char* pattern, Action action);
  void listen(const char* hostname, int port);

  void process(std::shared_ptr<ClientSocket> cs);

  std::vector<std::pair<std::regex,Action>> routes_;
};
