#include "server.h"
#include <memory>
#include "socket.h"
#include "usocket.h"
#include <iostream>

using namespace std;

void Server::Get(const char* pattern, Action action)
{
  routes_.push_back({ std::regex(pattern), action });
}

void Server::process(shared_ptr<ClientSocket> cs)
{
  HTTP http(*cs);
  try {
    auto line = cs->readLine(2048);
    http.initRequest(line.c_str());
    http.readHeaders();
  } catch (EOFException& e) {
    return;
  }

  std::smatch match;
  auto req = http.getRequest();
  Server::Response res(500, { {"Connection", "close"} });
  for (auto& pair : routes_) {
    if (std::regex_match(req.path, match, pair.first)) {
      pair.second(match, req, res);
      break;
    }
  }

  try {
    http.send(res);
  } catch (SockException& e) {
    return;
  }
}

void Server::listen(const char* hostname, int port)
{
  Host host(hostname, port);
  shared_ptr<ClientSocket> sock = make_shared<UClientSocket>();
  sock->bind(host);
  sock->setBlocking(true);

  while (true) {
    shared_ptr<ClientSocket> cs(sock->accept());
    if (!cs) {
      cout << "accept failed" << endl;
      return;
    }
    process(cs);
  }
}
