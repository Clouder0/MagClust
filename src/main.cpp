#include "common.h"
#include "model.h"

auto main() -> int {
  try {
    std::cout << "Hello meson again again again !" << '\n';
    RawDataBlock blk{};
    for (size_t i = 0; i < kDataBlockSize; i++) {
      std::print("{}", static_cast<char>(blk.data.at(i)));
    }
  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}