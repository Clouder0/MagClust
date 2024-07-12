#include "model.h"
#include <cstdint>
#include <iterator>
#include <fstream>
#include <filesystem>
#include <vector>
#include <gsl/pointers>


template<size_t bufferSize>
class DataBlockIterator;

constexpr size_t invalid_head_idx = UINT64_MAX;
// IO layer for processing, support stream reading and indexing
template<size_t bufferSize>
class IOHelper {
    struct FileInfo {
        size_t size, blknum;
        std::filesystem::path path;
        std::ifstream ifs;
        FileInfo(std::filesystem::path path);
    };
    size_t total_blks_{0};
    std::vector<FileInfo> srcs_;
    
    static_assert(std::is_same_v<size_t, uint64_t>, "Ensure size_t is 64bit");
    size_t buffer_head_idx_ = invalid_head_idx; // use max to indicate invalid
    std::array<RawDataBlock, bufferSize> buffer_;
    
  public:
    IOHelper(std::vector<std::filesystem::path> const &input_files);
    IOHelper(IOHelper&&) = delete;
    IOHelper(IOHelper const &) = delete;
    auto operator=(IOHelper&&) -> IOHelper& = delete;
    auto operator=(IOHelper const &) -> IOHelper& = delete;
    ~IOHelper() = default;
    
    /// NOTICE: IO operation! expensive
    [[nodiscard]] auto readBlock(size_t idx) -> RawDataBlock;

    [[nodiscard]] auto readBlockBuffered(size_t idx) -> RawDataBlock;
    
    [[nodiscard]] auto readBlocksBuffered(size_t begin_idx, size_t end_idx) -> std::vector<RawDataBlock>;
    
    // generally, streamed buffered read should be used.
  private:
    friend class DataBlockIterator<bufferSize>;
    void prepareCache(size_t start_idx);
  public:
    auto begin() -> DataBlockIterator<bufferSize>;
    auto end() -> DataBlockIterator<bufferSize>;
};

template<size_t bufferSize>
class DataBlockIterator {
 private:
  size_t blk_idx_;
  gsl::not_null<IOHelper<bufferSize> *> io_;

 public:
  using iterator_catetory = std::input_iterator_tag;
  using value_type = RawDataBlock;
  using pointer = RawDataBlock *;
  using reference = RawDataBlock &;

  auto operator*() -> reference;
  auto operator->() -> pointer;
  auto operator++() -> DataBlockIterator<bufferSize>&;
  friend auto operator==(const DataBlockIterator<bufferSize> &lhs, const DataBlockIterator<bufferSize> &rhs) -> bool;
  friend auto operator!=(const DataBlockIterator<bufferSize> &lhs, const DataBlockIterator<bufferSize> &rhs) -> bool;
};
