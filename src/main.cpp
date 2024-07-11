#include <cstdio>
#include <exception>
#include <iostream>
#include <print>

#include "common.h"

auto main() -> int {
  try {
    std::print("Hello C++23!!!! {}\n", getResult());
    std::cout << "Hello meson again again again !" << '\n';
    std::cout << getResult() << '\n';
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}