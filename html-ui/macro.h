#include <map>
#include <list>
#include <string>

namespace macro
{

typedef std::map<std::string,std::string> Env;

std::string eval_instance(Env& env, const std::string& str);
std::string eval_toks(Env& env, std::list<std::string>& toks);
std::string slurp(FILE *fp);
std::string slurp(const std::string& path);
std::string strip(std::string str);
std::string process_tag(Env& env, std::string stmt, std::list<std::string>& toks);
std::string eval_toks(Env& env, std::list<std::string>& toks);
std::list<std::string> tokenize(std::string str);
std::string eval_instance(Env& env, const std::string& str);
std::string macro_expand(const std::string& name);

}; // namespace macro
