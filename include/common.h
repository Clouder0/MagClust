#pragma once

#include <cstddef>
#include <array>
#include <cstdint>

auto getResult() -> int;

constexpr size_t kCachelineSize = 64;
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
  size_t idx;
  alignas(kCachelineSize) RawDataBlock data;
  alignas(kCachelineSize) DataBlockFeatures features;

 public:
  [[nodiscard]] auto getFeatures() const -> const DataBlockFeatures&;
};
