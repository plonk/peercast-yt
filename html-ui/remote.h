#include "picojson.h"

class Remote
{
public:
  Remote(const std::string& hostname, int port);

  picojson::value call(const std::string& name, const picojson::value& args);

private:
  std::string hostname_;
  int port_;
};
