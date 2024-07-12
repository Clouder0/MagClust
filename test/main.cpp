#include <cstring>

#include "common.h"
#include "data_gen.h"
#include "io.h"
#include "model.h"
#include "ut.h"

using namespace boost::ut;

auto compare_block(RawDataBlock const &lhs, RawDataBlock const &rhs) -> bool {
  return memcmp(lhs.data.data(), rhs.data.data(), kDataBlockSize) == 0;
}

auto main() -> int {
  "sequence_readblock"_test = [] {
    constexpr size_t kTestBlocks = 4197;
    auto path = random_path();
    gen_file(path, kTestBlocks);
    std::vector<std::filesystem::path> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (size_t i = 0; i < kTestBlocks; ++i) {
      auto blk = helper.readBlock(i);
      auto res = *reinterpret_cast<uint64_t *>(blk.data.data());
      boost::ut::expect(eq(i, res));
    }
  };

  "sequence_readblock_buffered"_test = [] {
    constexpr size_t kTestBlocks = 4197;
    auto path = random_path();
    gen_file(path, kTestBlocks);
    std::vector<std::filesystem::path> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (size_t i = 0; i < kTestBlocks; ++i) {
      auto blk = helper.readBlockBuffered(i);
      auto res = *reinterpret_cast<uint64_t *>(blk.data.data());
      boost::ut::expect(eq(i, res));
    }
  };

  "sequence_random_read"_test = [] {
    constexpr size_t kTestBlocks = 1025;
    auto path = random_path();
    auto blks = random_file(path, kTestBlocks);
    std::vector<std::filesystem::path> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (size_t i = 0; i < kTestBlocks; ++i) {
      auto blk = helper.readBlock(i);
      for (size_t j = 0; j < kDataBlockSize / sizeof(uint64_t); ++j) {
        auto res = *reinterpret_cast<uint64_t *>(blk.data.data() + j);
        auto actual = *reinterpret_cast<uint64_t *>(blks[i].data.data() + j);
        expect(eq(actual, res));
        if (res != actual) {
          std::print("{}\n{}\n", res, actual);
          std::print("ERR! {}\n", j);
        }
      }
    }
  };

  "ranged-based-forloop-random"_test = [] {
    constexpr size_t kTestBlocks = 1025;
    auto path = random_path();
    auto blks = random_file(path, kTestBlocks);
    std::vector<std::filesystem::path> paths{path};
    IOHelper<kBufferSize> helper(paths);
    size_t idx = 0;
    for (auto const &blk : helper) {
      compare_block(blk, blks[idx]);
      idx++;
    }
  };

  "multiple_files"_test = [] {
    constexpr size_t kTestBlocks = 4782;
    auto path1 = random_path();
    auto path2 = random_path();
    auto blks1 = random_file(path1, kTestBlocks);
    auto blks2 = random_file(path2, kTestBlocks);
    std::vector<std::filesystem::path> paths{path1, path2};
    IOHelper<kBufferSize> helper(paths);
    size_t idx = 0;
    for (auto const &blk : helper) {
      if (idx < kTestBlocks) {
        compare_block(blk, blks1[idx]);
      } else {
        compare_block(blk, blks2[idx - kTestBlocks]);
      }
      idx++;
    }
  };

  return 0;
}