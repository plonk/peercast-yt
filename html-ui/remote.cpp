#include "remote.h"
#include "http.h"
#include "sys.h"
#include "socket.h"

Remote::Remote(const std::string& hostname, int port)
{
  hostname_ = hostname;
  port_ = port;
}

picojson::value Remote::call(const std::string& name, const picojson::value& args)
{
  using namespace std;

  HTTPRequest req("POST", "/api/1", "HTTP/1.0", {{"X-Requested-With", "XMLHttpRequest"}});

  req.body = str::format(R"({"jsonrpc":"2.0","method":%s,"params":%s,"id":1})",
                         ((picojson::value)name).serialize().c_str(),
                         args.serialize().c_str());
  cout << req.body <<endl;
  auto sock = sys->createSocket();
  Host h(hostname_, port_);
  cout << h.str() <<endl;
  sock->open(h);
  sock->connect();
  HTTP http(*sock);
  req.headers.set("Content-Length", std::to_string(req.body.size()));
  auto res = http.send(req);
  picojson::value v;
  picojson::parse(v, res.body);
  return v.get<picojson::object>() ["result"];
}

