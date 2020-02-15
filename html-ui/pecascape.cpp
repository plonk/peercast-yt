#include <iostream>
#include <thread>
#include <stdio.h>
#include <unistd.h>
#include "../core/common/str.h"

#include <errno.h>
#include "macro.h"
#include "server.h"
#include "remote.h"

#include "usys.h"
#include "template2.h"

#include "sstream.h"
#include <algorithm>

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

#include "servent.h"

void route_static(std::smatch match, const Server::Request& req, Server::Response& res)
{
  cout << "public/" + match[1].str() <<endl;
  std::string path = "public/" + match[1].str();
  if (!is_regular_file(path.c_str())) {
    res.statusCode = 403;
  } else {
    FILE *fp = fopen(path.c_str(), "rb");
    if (fp) {
      const char* type = Servent::fileNameToMimeType(String(path.c_str()));
      if (!type)
        type = "application/binary";

      std::string data = slurp(fp);
      res.headers.set("Content-Type", type);
      res.body = data;
      res.statusCode = 200;
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

std::string message_interpolate(std::string str, const std::string& lang)
{
  picojson::value v;
  std::string buf;

  FILE *fp = fopen(str::format("catalogs/%s.json", lang.c_str()).c_str(), "rb");
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

static picojson::object query_to_object(cgi::Query& query)
{
  picojson::object obj;

  for (auto& pair : query.m_dict) {
    if (pair.second.size() >= 1)
      obj[pair.first] = (picojson::value) pair.second[0];
  }
  return obj;
}


int main()
{
  sys = new USys();

  using namespace std;
  using namespace macro;

  Server svr;

  Remote r("127.0.0.1", 8144);

  svr.Get(R"(/play\.html)",
          [&](std::smatch match, const Server::Request& req, Server::Response& res) {
            Template2 tmpl;
            StringStream in;
            StringStream out;

            cout << match[0].str() << endl;

            auto buf = macro_expand(match[1].str() + ".html");
            buf = message_interpolate(buf, "ja");
            auto v = r.call("evaluateVariables",
                            (picojson::value)picojson::object({
                                {"variables", (picojson::value)picojson::array({})}
                              }));

            cgi::Query query(req.queryString);

            tmpl.selectedFragment = query.get("fragment");
            tmpl.env_ = v.get<picojson::object>();
            in.str(buf);

            tmpl.env_["page"] = (picojson::value) query_to_object(query);
            auto& channels = tmpl.env_["chanMgr"].get<picojson::object>()["channels"].get<picojson::array>();
            auto it = std::find_if(channels.begin(), channels.end(), [&query](picojson::value& ch) { return ch.get<picojson::object>()["id"].get<std::string>() == query.get("id"); });
            if (it == channels.end()) {
              res.statusCode = 404;
              res.body = "Channel not found";
              return;
            }
            tmpl.env_["channel"] = *it;

            try{
              tmpl.readTemplate(in, &out);
            }catch (TemplateError& e) {
              res.statusCode = 500;
              res.body = e.what();
              return;
            }

            res.body = out.str();
            res.headers.set("Content-Type", "text/html; charset=UTF-8");
            res.statusCode = 200;
          });

  svr.Get(R"(/([^\.]+)\.html)",
          [&](std::smatch match, const Server::Request& req, Server::Response& res) {
            Template2 tmpl;
            StringStream in;
            StringStream out;

            cout << match[0].str() << endl;

            auto buf = macro_expand(match[1].str() + ".html");
            buf = message_interpolate(buf, "ja");
            auto v = r.call("evaluateVariables",
                            (picojson::value)picojson::object({
                                {(std::string)"variables", (picojson::value)picojson::array({})}
                              }));

            cgi::Query query(req.queryString);

            tmpl.selectedFragment = query.get("fragment");
            tmpl.env_ = v.get<picojson::object>();
            in.str(buf);

            try{
              tmpl.readTemplate(in, &out);
            }catch (TemplateError& e) {
              res.statusCode = 500;
              res.body = e.what();
              return;
            }

            res.body = out.str();
            res.headers.set("Content-Type", "text/html; charset=UTF-8");
            res.statusCode = 200;
          });
  svr.Get("/(.*)", route_static);

  // std::thread bg(update);
  svr.listen("localhost", 8888);

  return 0;
}
