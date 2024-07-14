#include <sys/types.h>

#include <cstring>

#include "common.h"
#include "data_gen.h"
#include "io.h"
#include "model.h"
#include "odess.h"
#include "ut.h"
// #include "origin_odess.h"

using namespace boost::ut;

auto compare_block(RawDataBlock const &lhs, RawDataBlock const &rhs) -> bool {
  return memcmp(lhs.data.data(), rhs.data.data(), kDataBlockSize) == 0;
}

auto diff_bits(RawDataBlock const &lhs, RawDataBlock const &rhs) -> size_t {
  size_t diff = 0;
  for (size_t i = 0; i < kDataBlockSize; ++i) {
    diff +=
        std::popcount(static_cast<uint8_t>(lhs.data.at(i) ^ rhs.data.at(i)));
  }
  return diff;
}

namespace IOTEST {
void sequence_readblock() {
  "sequence_readblock"_test = [] {
    constexpr size_t kTestBlocks = 4197;
    auto path = random_path();
    gen_file_increment(path, kTestBlocks);
    const std::vector<std::string> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (size_t i = 0; i < kTestBlocks; ++i) {
      auto blk = helper.readBlock(i);
      auto res = *reinterpret_cast<uint64_t *>(blk.data.data());
      boost::ut::expect(eq(i, res));
    }
  };
}
void sequence_readblock_buffered() {
  "sequence_readblock_buffered"_test = [] {
    constexpr size_t kTestBlocks = 4197;
    auto path = random_path();
    gen_file_increment(path, kTestBlocks);
    const std::vector<std::string> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (size_t i = 0; i < kTestBlocks; ++i) {
      auto blk = helper.readBlockBuffered(i);
      auto res = *reinterpret_cast<uint64_t *>(blk.data.data());
      boost::ut::expect(eq(i, res));
    }
  };
}
void multiple_files() {
  "multiple_files"_test = [] {
    constexpr size_t kTestBlocks = 4782;
    auto path1 = random_path();
    auto path2 = random_path();
    auto blks1 = random_file(path1, kTestBlocks);
    auto blks2 = random_file(path2, kTestBlocks);
    const std::vector<std::string> paths{path1, path2};
    IOHelper<kBufferSize> helper(paths);
    for (auto const &[idx, blk] : helper) {
      if (idx < kTestBlocks) {
        compare_block(blk, blks1[idx]);
      } else {
        compare_block(blk, blks2[idx - kTestBlocks]);
      }
    }
  };
}

void sequence_random_read() {
  "sequence_random_read"_test = [] {
    constexpr size_t kTestBlocks = 1025;
    auto path = random_path();
    auto blks = random_file(path, kTestBlocks);
    const std::vector<std::string> paths{path};
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
}

void range_based_forloop() {
  "ranged-based-forloop-random"_test = [] {
    constexpr size_t kTestBlocks = 1025;
    auto path = random_path();
    auto blks = random_file(path, kTestBlocks);
    const std::vector<std::string> paths{path};
    IOHelper<kBufferSize> helper(paths);
    for (auto const &[idx, blk] : helper) { compare_block(blk, blks[idx]); }
  };
}

void IO_tests() {
  const suite<"IOHelper"> io_helper = [] {
    sequence_readblock();
    sequence_readblock_buffered();
    multiple_files();
    sequence_random_read();
    range_based_forloop();
  };
}
}  // namespace IOTEST

namespace SF_TEST {
void totally_different() {
  constexpr size_t feature_num = 12;
  constexpr size_t cluster_num = 4;
  constexpr size_t sample_bits = 7;
  constexpr size_t hash_shift = 8;
  "totally_different"_test = [] {
    RawDataBlock blk1{};
    RawDataBlock blk2{};
    for (size_t i = 0; i < kDataBlockSize / sizeof(uint8_t); ++i) {
      *reinterpret_cast<uint8_t *>(blk1.data.data() + i) = i % UINT8_MAX;
      *reinterpret_cast<uint8_t *>(blk2.data.data() + i) =
          UINT8_MAX - (i % UINT8_MAX);
    }
    Odess<feature_num, cluster_num, sample_bits, hash_shift> odess;  // NOLINT
    auto sf1 = odess.genSuperFeatures(blk1);
    auto sf2 = odess.genSuperFeatures(blk2);
    for (size_t i = 0; i < cluster_num; ++i) {
      expect(neq(sf1.at(i), sf2.at(i)));
    }
  };

  "totally_same"_test = [] {
    RawDataBlock blk1{};
    RawDataBlock blk2{};
    for (size_t i = 0; i < kDataBlockSize / sizeof(uint8_t); ++i) {
      *reinterpret_cast<uint8_t *>(blk1.data.data() + i) = i % UINT8_MAX;
      *reinterpret_cast<uint8_t *>(blk2.data.data() + i) = i % UINT8_MAX;
    }
    Odess<feature_num, cluster_num, sample_bits, hash_shift> odess;  // NOLINT
    auto sf1 = odess.genSuperFeatures(blk1);
    auto sf2 = odess.genSuperFeatures(blk2);
    for (size_t i = 0; i < cluster_num; ++i) {
      expect(eq(sf1.at(i), sf2.at(i)));
    }
  };

  "similarity"_test = [] {
    auto test_similarity = [](size_t dif_num) {
      auto blk1 = random_block();
      auto blk2 = blk1;
      auto randomHelper = RandomHelper();
      for (size_t i = 0; i < dif_num; ++i) {
        blk2.data.at(randomHelper.nextUInt64() % kDataBlockSize) =
            randomHelper.nextByte();
      }

      Odess<feature_num, cluster_num, sample_bits, hash_shift> odess;
      auto sf1 = odess.genSuperFeatures(blk1);
      auto sf2 = odess.genSuperFeatures(blk2);
      // for (size_t i = 0; i < cluster_num; ++i) {
      // std::print("{}:{}\n", sf1.at(i), sf2.at(i));
      // }
      std::print("bit diff: {}/{}\n", diff_bits(blk1, blk2),
                 kDataBlockSize * 8);
      auto same_feature = 0;
      for (size_t i = 0; i < cluster_num; ++i) {
        if (sf1.at(i) == sf2.at(i)) {
          ++same_feature;
          break;
        }
      }
      std::print("same SF: {}\n", same_feature);
      return same_feature;
    };
    auto same_times = 0;
    constexpr auto test_times = 10000;
    constexpr auto dif_num = 200;
    for (int i = 0; i < test_times; ++i) {
      same_times += test_similarity(dif_num);
    }
    std::print("avg {}/4096 modified feature equal rate: {}/{}, {}%\n", dif_num,
               same_times, test_times, 1.0 * same_times / test_times * 100);
  };
}
void sf_tests() {
  const suite<"Odess"> odess = [] { totally_different(); };
}
}  // namespace SF_TEST

auto main() -> int {
  IOTEST::IO_tests();
  SF_TEST::sf_tests();
  return 0;
}