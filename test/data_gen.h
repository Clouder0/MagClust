#include <functional>
#include <random>

#include "common.h"
#include "model.h"

struct RandomHelper {
  std::random_device r;
  std::default_random_engine e;
  RandomHelper() : e(r()) {}
  auto nextUInt64() -> uint64_t {
    static std::uniform_int_distribution<uint64_t> gen(0, UINT64_MAX);
    return gen(e);
  }
  auto nextByte() -> std::byte {
    static std::uniform_int_distribution<uint8_t> gen(0, UINT8_MAX);
    return static_cast<std::byte>(gen(e));
  }
};

void gen_file_increment(std::filesystem::path const &path, size_t blknum);
void gen_file_decrement(std::filesystem::path const &path, size_t blknum);

void gen_file(std::filesystem::path const &path,
              std::vector<RawDataBlock> const &blks);

auto random_block() -> RawDataBlock;
auto random_blocks(size_t blknum) -> std::vector<RawDataBlock>;
auto gen_blocks(size_t blknum,
                std::function<void(RawDataBlock &)> const &writer)
    -> std::vector<RawDataBlock>;
auto incre_blocks(size_t blknum) -> std::vector<RawDataBlock>;
auto decre_blocks(size_t blknum) -> std::vector<RawDataBlock>;

auto random_file(std::filesystem::path const &path,
                 size_t blknum) -> std::vector<RawDataBlock>;

auto random_path() -> std::filesystem::path;
