#include <list>
#include "macro.h"
#include <regex>
#include "str.h"

namespace macro
{

std::string slurp(FILE *fp)
{
  std::string res;

  int c;
  while ((c = fgetc(fp)) != EOF) {
    res += (char) c;
  }
  return res;
}

std::string slurp(const std::string& path)
{
  FILE *fp = fopen(path.c_str(), "rb");
  if (!fp)
    return "";

  std::string res = slurp(fp);
  fclose(fp);
  return res;
}

std::string process_tag(Env& env, std::string stmt, std::list<std::string>& toks)
{
  std::smatch match;

  // 毎回正規表現コンパイルしちゃう…
  if (std::regex_match(stmt, match, std::regex(R"(^define\s+(\w+)\s*$)"))) {
    env[match[1].str()] = eval_toks(env, toks);
    return "";
  } else if (std::regex_match(stmt, match, std::regex(R"(^define\s+(\w+)\s+(\S+)\s*$)"))) {
    env[match[1].str()] = match[2].str();
    return "";
  } else if (std::regex_match(stmt, match, std::regex(R"(^include\s+(\S+)\s*$)"))) {
    // ここよくない。
    std::string fn = "Templates/" + match[1].str();
    FILE *fp = fopen(fn.c_str(), "rb");
    if (!fp)
      return "Cannot open file: " + match[1].str();
    auto res = eval_instance(env, slurp(fp));
    fclose(fp);
    return res;
  } else {
    auto key = strip(stmt);

    if (env.find(key) == env.end()) {
      return "undefined variable: " + key;
    } else {
      return env[key];
    }
  }
}

std::string strip(std::string str)
{
  int beg = 0, end = str.size()-1;

  while (beg <= end && isspace(str[beg]))
    beg++;
  while (end > beg && isspace(str[end]))
    end--;

  return str.substr(beg, end - beg + 1);
}

std::string eval_toks(Env& env, std::list<std::string>& toks)
{
  std::string res;

  while (!toks.empty()) {
    auto tok = toks.front();
    toks.pop_front();

    if (tok.size() > 1 && tok[0] == '{' && tok[1] == '^') {
      std::string content = strip(tok.substr(2, tok.size() - 3));
      if (content == "end") {
        break;
      } else {
        res += process_tag(env, content, toks);
      }
    } else {
      res += tok;
    }
  }
  return res;
}

std::list<std::string> tokenize(std::string str)
{
  std::list<std::string> res;

  while (str != "") {
    if (str[0] == '{') {
      // マクロタグ。
      if (str.size() > 1 && str[1] == '^') {
        std::string tmp = "{^";
        size_t i = 2;
        while (i < str.size()) {
          tmp += str[i];
          if (str[i] == '}')
            break;
          i++;
        }
        if (tmp[tmp.size()-1] == '}') {
          res.push_back(tmp);
          str = str.substr(tmp.size());
          continue;
        }
      }

      // 関係ない波括弧。
      res.push_back("{");
      str = str.substr(1);
    } else {
      // 普通のテキスト。
      std::string tmp;

      size_t i = 0;
      while (i < str.size() && str[i] != '{') {
        tmp += str[i];
        i++;
      }
      res.push_back(tmp);
      str = str.substr(i);
    }
  }
  return res;
}

// なんでinstanceなんて名前付けたんだ？
std::string eval_instance(Env& env, const std::string& str)
{
  auto toks = tokenize(str);
  return eval_toks(env, toks);
}

std::string macro_expand(const std::string& name)
{
  Env env;

  eval_instance(env, slurp("Templates/defs.html"));
  auto html = eval_instance(env, slurp(str::STR("views/", name)));

  if (env.find("LAYOUT") != env.end()) {
    env["yield"] = html;
    html = eval_instance(env, slurp(str::STR("Templates/", env["LAYOUT"])));
  }
  return html;
}

}; // namespace macro
