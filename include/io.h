#include "common.h"
#include "gsl/pointers"
#include "model.h"

constexpr size_t invalid_head_idx = UINT64_MAX;

namespace {
auto checkCacheValid(size_t idx, size_t start_idx, size_t cache_size) -> bool {
  return start_idx != invalid_head_idx && idx >= start_idx &&
         idx < start_idx + cache_size;
}
}  // namespace

template <size_t bufferSize>
class DataBlockIterator;

// IO layer for processing, support stream reading and indexing
template <size_t bufferSize>
class IOHelper {
  struct FileInfo {
    std::string path;
    size_t size, blknum;
    std::ifstream ifs;
    FileInfo(const std::string &path_)
        : path(path_),
          size(filesize(path_)),
          blknum((static_cast<size_t>(filesize(path_)) + kDataBlockSize - 1) /
                 kDataBlockSize),
          ifs(path_, std::ios::binary) {}
  };
  size_t total_blks_{0};
  std::vector<FileInfo> srcs_;

  static_assert(std::is_same_v<size_t, uint64_t>, "Ensure size_t is 64bit");
  size_t buffer_head_idx_ = invalid_head_idx;  // use max to indicate invalid
  std::array<RawDataBlock, bufferSize> buffer_;

 public:
  IOHelper(std::vector<std::string> const &input_files) {
    srcs_.reserve(input_files.size());
    for (auto const &path : input_files) {
      srcs_.emplace_back(path);
      total_blks_ += srcs_.back().blknum;
    }
  }

  IOHelper(IOHelper &&) = delete;
  IOHelper(IOHelper const &) = delete;
  auto operator=(IOHelper &&) -> IOHelper & = delete;
  auto operator=(IOHelper const &) -> IOHelper & = delete;
  ~IOHelper() = default;

  /// NOTICE: IO operation! expensive
  [[nodiscard]] auto readBlock(size_t idx) -> RawDataBlock {
    // first find the correct source file the block belongs to
    for (auto &src : srcs_) {
      if (idx < src.blknum) {
        // NOLINTNEXTLINE don't consider overflow
        src.ifs.seekg(gsl::narrow_cast<std::streamoff>(idx) * kDataBlockSize);
        RawDataBlock blk{};  // TODO: inspect initialization cost here
        src.ifs.read(reinterpret_cast<char *>(blk.data.data()), kDataBlockSize);
        return blk;
      }
      idx -= src.blknum;
    }
    throw std::out_of_range("Block index out of range");
  }

  [[nodiscard]] auto readBlockBuffered(size_t idx) -> RawDataBlock & {
    if (!checkCacheValid(idx, buffer_head_idx_, bufferSize)) {
      prepareCache(idx);
    }
    return buffer_.at(idx - buffer_head_idx_);
  }

  [[nodiscard]] auto readBlocksBuffered(size_t begin_idx, size_t end_idx)
      -> std::vector<gsl::not_null<RawDataBlock *>> {
    if (!checkCacheValid(begin_idx, buffer_head_idx_, bufferSize)) {
      prepareCache(begin_idx);
    }
    std::vector<gsl::not_null<RawDataBlock *>> result;
    result.reserve(end_idx - begin_idx);
    for (size_t i = begin_idx; i < end_idx; ++i) {
      result.emplace_back(&buffer_[i - buffer_head_idx_]);
    }
    return result;
  }

  [[nodiscard]] auto totalBlocks() -> size_t { return total_blks_; }

  // generally, streamed buffered read should be used.
 private:
  friend class DataBlockIterator<bufferSize>;
  void prepareCache(size_t start_idx) {
    buffer_head_idx_ = start_idx;
    size_t passed = 0;
    for (FileInfo &src : srcs_) {
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
        const size_t read_blocks =
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
        const size_t bytes_to_read =
            (bufferSize - current_idx) * kDataBlockSize;
        src.ifs.read(start_ptr, bytes_to_read);
        return;  // READ CACHE DONE
      }
      // read whole file
      src.ifs.read(start_ptr, src.size);
      passed += src.blknum;
    }
  }

 public:
  auto begin() -> DataBlockIterator<bufferSize> {
    return DataBlockIterator<bufferSize>{0, this};
  }
  auto end() -> DataBlockIterator<bufferSize> {
    printf("total blocks %lu\n", total_blks_);
    return DataBlockIterator<bufferSize>{total_blks_, this};
  }
};

template <size_t bufferSize>
class DataBlockIterator {
 private:
  size_t blk_idx_;
  gsl::not_null<IOHelper<bufferSize> *> io_;

 public:
  using iterator_catetory = std::input_iterator_tag;
  using value_type = std::tuple<size_t, RawDataBlock &>;
  // using reference = std::tuple<size_t, RawDataBlock &>;

  DataBlockIterator(size_t blk_idx, gsl::not_null<IOHelper<bufferSize> *> io_)
      : blk_idx_(blk_idx), io_(io_) {}

  auto operator*() -> value_type {
    return std::make_tuple(blk_idx_,
                           std::ref(io_->readBlockBuffered(blk_idx_)));
  }
  auto operator++() -> DataBlockIterator<bufferSize> & {
    ++blk_idx_;
    return *this;
  }
  friend auto operator==(const DataBlockIterator<bufferSize> &lhs,
                         const DataBlockIterator<bufferSize> &rhs) -> bool {
    return lhs.blk_idx_ == rhs.blk_idx_;
  }
  friend auto operator!=(const DataBlockIterator<bufferSize> &lhs,
                         const DataBlockIterator<bufferSize> &rhs) -> bool {
    return lhs.blk_idx_ != rhs.blk_idx_;
  }
};

constexpr size_t kBufferSize = 7;  // 1024*4KB = 4MB
