#include "config.h"

auto readConfigFromFile(const std::filesystem::path &path) -> Config {
  Config config;
  std::ifstream file(path);
  if (!file.is_open()) { throw std::runtime_error("Failed to open file"); }
  std::string line;
  while (std::getline(file, line)) { config.paths.emplace_back(line); }
  return config;
}
