#include <cstdio>
#include <exception>
#include <iostream>
#include <print>

#include "common.h"
#include "model.h"

auto main() -> int {
  try {
    std::print("Hello C++23!!!! {}\n", getResult());
    std::cout << "Hello meson again again again !" << '\n';
    std::cout << getResult() << '\n';
    RawDataBlock blk{};
    for (size_t i = 0; i < kDataBlockSize; i++) {
      std::print("{}", static_cast<char>(blk.data.at(i)));
    }
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}