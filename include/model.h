#pragma once
#include "common.h"
#include "utils.h"

constexpr size_t kDataBlockSize = 4096;
// Pure Datablock of 4KB
struct RawDataBlock {
    alignas(kCachelineSize) std::array<std::byte, kDataBlockSize> data;
};
static_assert(sizeof(RawDataBlock) == kDataBlockSize, "RawDataBlockSize");
static_assert(offsetof(RawDataBlock, data) == 0, "data should be the first member");


constexpr uint32_t kFeaturePerBlock = 16;
using FeatureType = uint64_t;
using DataBlockFeatures = std::array<FeatureType, kFeaturePerBlock>;
class DataBlock {
 private:
  public:
  size_t const idx;
  private:
  alignas(kCachelineSize) RawDataBlock data_;
  alignas(kCachelineSize) DataBlockFeatures features_;
  // memory layout guarantee
  friend class DataBlockMemoryChecker;

 public:
  DataBlock(size_t idx, RawDataBlock const &data);
  DataBlock(size_t idx);
  DataBlock(const DataBlock &) = delete;
  DataBlock(DataBlock &&) = delete;
  auto operator=(const DataBlock &) -> DataBlock & = delete;
  auto operator=(DataBlock &&) -> DataBlock & = delete;
  ~DataBlock() = default;
  [[nodiscard]] auto getFeatures() const -> const DataBlockFeatures &;
};

constexpr size_t ZipBlockSize = 16;
class ZipBlock {
  public:
    size_t const idx;
  
  private:
    alignas(kCachelineSize) std::array<gsl::not_null<DataBlock*>, ZipBlockSize> data_;
  
  public:
    ZipBlock(size_t idx);
    ZipBlock(const ZipBlock &) = delete;
    ZipBlock(ZipBlock &&) = delete;
    auto operator=(const ZipBlock &) -> ZipBlock & = delete;
    auto operator=(ZipBlock &&) -> ZipBlock & = delete;
    ~ZipBlock() = default;
    [[nodiscard]] auto getBlock(size_t idx) const -> const DataBlock &;
    [[nodiscard]] auto getBlock(size_t idx) -> DataBlock &;
};

class DataBlockMemoryChecker {
  // [0,8): idx
  // [64,4160): data
  // [4160, ...]: feature
  static_assert(as_offset<&DataBlock::idx> == 0);
  static_assert(as_offset<&DataBlock::data_> == kCachelineSize);
  static_assert(as_offset<&DataBlock::features_> == kCachelineSize + sizeof(RawDataBlock));
  
  static_assert(alignof(DataBlock) == kCachelineSize);
  static_assert(aligned_sizeof<DataBlock> == kCachelineSize + sizeof(RawDataBlock) + aligned_sizeof_base<DataBlockFeatures, kCachelineSize>);
  static_assert(sizeof(DataBlockFeatures) != 1);
  static_assert(aligned_sizeof_base<DataBlockFeatures, kCachelineSize> > 0); // only for checking its value
};
