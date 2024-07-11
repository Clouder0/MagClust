#pragma once
#include "common.h"
#include <array>
#include <cstdint>

constexpr size_t kDataBlockSize = 4096;
// Pure Datablock of 4KB
struct RawDataBlock {
    alignas(kCachelineSize) std::array<std::byte, kDataBlockSize> data;
};


constexpr uint32_t kFeaturePerBlock = 12;
using FeatureType = uint64_t;
using DataBlockFeatures = std::array<FeatureType, kFeaturePerBlock>;
// be careful about memory layouts
class DataBlock {
 public:
  size_t const idx;

 private:
  alignas(kCachelineSize) RawDataBlock data;
  alignas(kCachelineSize) DataBlockFeatures features;

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
