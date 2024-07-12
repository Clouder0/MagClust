#include "io.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gsl/util>
#include <ios>
#include <stdexcept>
#include <utility>
#include <vector>

#include "model.h"

namespace {
auto checkCacheValid(size_t idx, size_t start_idx, size_t cache_size) -> bool {
  return start_idx != invalid_head_idx && idx >= start_idx &&
         idx < start_idx + cache_size;
}
}  // namespace

template <size_t bufferSize>
IOHelper<bufferSize>::FileInfo::FileInfo(std::filesystem::path path_)
    : path(std::move(path_)),
      size(std::filesystem::file_size(path)),
      blknum(size / kDataBlockSize),
      ifs(path, std::ios::binary) {}

template <size_t bufferSize>
IOHelper<bufferSize>::IOHelper(
    std::vector<std::filesystem::path> const &input_files)
    : srcs_(input_files.size()) {
  for (auto const &path : input_files) {
    srcs_.emplace_back(path);
    total_blks_ += srcs_.back().blknum;
  }
}

template <size_t bufferSize>
auto IOHelper<bufferSize>::readBlock(size_t idx) -> RawDataBlock {
  // first find the correct source file the block belongs to
  for (auto const &src : srcs_) {
    if (idx < src.blknum) {
      // NOLINTNEXTLINE don't consider overflow
      src.ifs.seekg(gsl::narrow_cast<std::streamoff>(idx) * kDataBlockSize);
      RawDataBlock blk{};  // TODO: inspect initialization cost here
      // NOLINTNEXTLINE damn std::byte aren't supported by filestream
      src.ifs.read(reinterpret_cast<char *>(blk.data.data()), kDataBlockSize);
      return blk;
    }
    idx -= src.blknum;
  }
  throw std::out_of_range("Block index out of range");
}

template <size_t bufferSize>
void IOHelper<bufferSize>::prepareCache(size_t start_idx) {
  buffer_head_idx_ = start_idx;
  size_t passed = 0;
  for (FileInfo const &src : srcs_) {
    if (passed + src.blknum <= start_idx) {
      passed += src.blknum;
      continue;
    }
    // start reading
    if (passed < start_idx) {
      auto in_file_offset = start_idx - passed;
      src.ifs.seekg(in_file_offset * kDataBlockSize);
      size_t const bytes_to_read =
          std::min(kDataBlockSize * bufferSize,
                   src.size - kDataBlockSize * in_file_offset);
      size_t read_blocks =
          (bytes_to_read + kDataBlockSize - 1) / kDataBlockSize;

      char *start_ptr = reinterpret_cast<char *>(buffer_.data());  // NOLINT
      src.ifs.read(start_ptr, bytes_to_read);
      if (read_blocks == bufferSize) { return; }
      passed += read_blocks;
      continue;
    }
    src.ifs.seekg(0);
    auto current_idx = passed - start_idx;
    char *start_ptr =
        reinterpret_cast<char *>(buffer_.data() + current_idx);  // NOLINT
    if (src.blknum >= bufferSize - (passed - start_idx)) {
      // partial read and end
      size_t bytes_to_read = (bufferSize - current_idx) * kDataBlockSize;
      src.ifs.read(start_ptr, bytes_to_read);
      return;  // READ CACHE DONE
    }
    // read whole file
    src.ifs.read(start_ptr, src.size);
    passed += src.blknum;
  }
}

template <size_t bufferSize>
auto IOHelper<bufferSize>::readBlockBuffered(size_t idx) -> RawDataBlock {
  if (!checkCacheValid(idx, buffer_head_idx_, bufferSize)) {
    prepareCache(idx);
  }
  return buffer_[idx - buffer_head_idx_];
}

template <size_t bufferSize>
auto IOHelper<bufferSize>::readBlocksBuffered(size_t begin_idx, size_t end_idx)
    -> std::vector<RawDataBlock> {
  if (!checkCacheValid(begin_idx, buffer_head_idx_, bufferSize)) {
    prepareCache(begin_idx);
  }
  std::vector<RawDataBlock> result;
  result.reserve(end_idx - begin_idx);
  for (size_t i = begin_idx; i < end_idx; ++i) {
    result.emplace_back(buffer_[i - buffer_head_idx_]);
  }
  return result;
}

template <size_t bufferSize>
auto IOHelper<bufferSize>::begin() -> DataBlockIterator<bufferSize> {
  return DataBlockIterator<bufferSize>{0, this};
}

template <size_t bufferSize>
auto IOHelper<bufferSize>::end() -> DataBlockIterator<bufferSize> {
  return DataBlockIterator<bufferSize>{total_blks_, this};
}

template <size_t bufferSize>
auto DataBlockIterator<bufferSize>::operator++() -> DataBlockIterator & {
  ++blk_idx_;
  return *this;
}

template <size_t bufferSize>
auto DataBlockIterator<bufferSize>::operator*() -> RawDataBlock & {
  return io_->readBlockBuffered(blk_idx_);
}

template <size_t bufferSize>
auto operator==(DataBlockIterator<bufferSize> const &lhs,
                DataBlockIterator<bufferSize> const &rhs) -> bool {
  return lhs.blk_idx_ == rhs.blk_idx_;
}

template <size_t bufferSize>
auto operator!=(DataBlockIterator<bufferSize> const &lhs,
                DataBlockIterator<bufferSize> const &rhs) -> bool {
  return lhs.blk_idx_ != rhs.blk_idx_;
}
