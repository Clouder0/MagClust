#pragma once
#include "common.h"
#include "utils.h"

constexpr size_t kDataBlockSize = 4096;
// Pure Datablock of 4KB
struct RawDataBlock {
  alignas(kCachelineSize) std::array<std::byte, kDataBlockSize> data;
};
static_assert(sizeof(RawDataBlock) == kDataBlockSize, "RawDataBlockSize");
static_assert(as_offset<&RawDataBlock::data> == 0,
              "data should be the first member");

constexpr size_t kBlockPerZip = 16;
struct ZipBlock {
  std::vector<size_t> blks;
  ZipBlock() { blks.reserve(kBlockPerZip); }
  ZipBlock(std::vector<size_t>&& blks_) : blks(std::move(blks_)) {}
};

template <size_t cluster_num>
struct DataBlock {
  uint64_t idx;
  std::array<uint64_t, cluster_num> features_;
};

/*
class DataBlockMemoryChecker {
  // [0,8): idx
  // [64,4160): data
  // [4160, ...]: feature
  static_assert(as_offset<&DataBlock::idx> == 0);
  static_assert(as_offset<&DataBlock::data_> == kCachelineSize);
  static_assert(as_offset<&DataBlock::features_> ==
                kCachelineSize + sizeof(RawDataBlock));

  static_assert(alignof(DataBlock) == kCachelineSize);
  static_assert(aligned_sizeof<DataBlock> ==
                kCachelineSize + sizeof(RawDataBlock) +
                    aligned_sizeof_base<DataBlockFeatures, kCachelineSize>);
  static_assert(sizeof(DataBlockFeatures) != 1);
  static_assert(aligned_sizeof_base<DataBlockFeatures, kCachelineSize> >
                0);  // only for checking its value
};
*/
