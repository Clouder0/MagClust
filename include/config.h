#include "common.h"

// Config Helper
struct Config {
  std::vector<std::filesystem::path> paths;
};

auto readConfigFromFile(std::filesystem::path const &path) -> Config;
