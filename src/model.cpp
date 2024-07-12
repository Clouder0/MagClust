#include "model.h"

#include <cstddef>

DataBlock::DataBlock(size_t const idx, RawDataBlock const& data)
    : idx(idx), data_(data), features_() {}

DataBlock::DataBlock(size_t const idx) : DataBlock(idx, RawDataBlock()) {}

auto DataBlock::getFeatures() const -> const DataBlockFeatures& {
  // TODO: maybe add lazy evaluation?
  return features_;
}
