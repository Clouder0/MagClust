#include "common.h"

// Config Helper
struct Config {
  std::vector<std::string> paths;
};

auto readConfigFromFile(std::string const &path) -> Config;
