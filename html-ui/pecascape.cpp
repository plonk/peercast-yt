#include <iostream>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include "../core/common/str.h"

#include <errno.h>
#include "macro.h"
#include "server.h"
#include "remote.h"

using namespace std;

std::string message_interpolate(std::string str, const std::map<std::string,std::string>& dic)
{
  std::string res;
  std::smatch match;

  while (std::regex_search(str, match, std::regex(R"(\{#([^}]*)\})"))) {
    res += std::string(str.begin(), str.begin() + match.position(0));
    auto key = match[1].str();
    if (dic.count(key))
      res += dic.at(key);
    else
      res += key;
    str = std::string(str.begin() + match.position(0) + match[0].length(), str.end());
  }
  res += str;
  return res;
}

std::string slurp(std::istream& in)
{
  std::string res;
  char c;

  while (in.get(c)) {
    res += c;
  }

  return res;
}

std::string decomment(const std::string& s)
{
  std::string res;

  // コメントを削除する。
  for (auto it = s.cbegin(); it != s.cend(); it++) {
    if (*it == '/' && it+1 != s.cend() && *(it+1) == '/') {
        while (it != s.cend() && *it != '\n')
          it++;
        if (it != s.cend()) {
          res += '\n';
        }
    } else {
      res += *it;
    }
  }

  return res;
}

       #include <sys/types.h>
       #include <sys/stat.h>
       #include <unistd.h>
bool
is_regular_file(const char* path)
{
  struct stat buf;
  stat(path, &buf);
  auto type = (buf.st_mode & S_IFMT);
  return type == S_IFREG;
}

std::string slurp(FILE *fp)
{
  std::string res;

  int c;
  while ((c = fgetc(fp)) != EOF) {
    res += (char) c;
  }
  return res;
}

void route_static(std::smatch match, const Server::Request& req, Server::Response& res)
{
  cout << "public/" + match[1].str() <<endl;
  std::string path = "public/" + match[1].str();
  if (!is_regular_file(path.c_str())) {
    res.statusCode = 403;
  } else {
    FILE *fp = fopen(path.c_str(), "rb");
    if (fp) {
      // auto type = httplib::detail::find_content_type(req.path, {});
      // if (!type)
      //   type = "application/binary";

      std::string data = slurp(fp);
      res.body = data;
      fclose(fp);
    } else if (errno == ENOENT) {
      res.statusCode = 404;
    } else if (errno == EACCES) {
      res.statusCode = 403;
    } else {
      res.statusCode = 500;
    }
  }
}

#include  "picojson.h"

std::string test(std::string str)
{
  picojson::value v;
  std::string buf;

  FILE *fp = fopen("catalogs/ja.json", "rb");
  buf = slurp(fp);
  fclose(fp);

  buf = decomment(buf);

  std::string err = picojson::parse(v, buf);
  if (!err.empty()) {
    std::cerr << err << std::endl;
    exit(1);
  }

  const auto& obj = v.get<picojson::object>();
  std::map<std::string,std::string> dic;
  for (auto pair : obj) {
    dic[pair.first] = pair.second.get<std::string>();
  }
  return message_interpolate(str, dic);
}

#include "usys.h"
#include "template2.h"

#include "sstream.h"

int main()
{
  sys = new USys();

  using namespace std;
  using namespace macro;

  Server svr;

  svr.Get(R"(/([^\.]+)\.html)",
          [](std::smatch match, const Server::Request& req, Server::Response& res) {
            auto buf = macro_expand(match[1].str() + ".html");
            buf = test(buf);

            Remote r("127.0.0.1", 8144);
            picojson::value v =
              r.call("evaluateVariables", (picojson::value)picojson::object({
                    {(std::string)"variables", (picojson::value)picojson::array({})}
                  }));

            Template2 tmpl;
            StringStream in;
            StringStream out;

            tmpl.env_ = v.get<picojson::object>();
            in.str(buf);
            tmpl.readTemplate(in, &out, 0);

            res.body = out.str();
          });
  svr.Get("/(.*)", route_static);

  // std::thread bg(update);
  svr.listen("localhost", 8888);

  return 0;
}
