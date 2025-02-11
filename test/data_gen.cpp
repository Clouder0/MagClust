#include "data_gen.h"

#include <cstdint>
#include <functional>
#include <vector>

#include "common.h"
#include "model.h"

auto random_block() -> RawDataBlock {
  static auto randomHelper = RandomHelper();
  RawDataBlock blk{};
  for (size_t i = 0; i < kDataBlockSize; i++) {
    blk.data.at(i) = randomHelper.nextByte();
  }
  return blk;
}

auto random_blocks(size_t blknum) -> std::vector<RawDataBlock> {
  std::vector<RawDataBlock> blks;
  blks.reserve(blknum);
  for (size_t i = 0; i < blknum; ++i) { blks.emplace_back(random_block()); }
  return blks;
}

auto gen_blocks(size_t blknum,
                std::function<void(RawDataBlock &)> const &writer)
    -> std::vector<RawDataBlock> {
  std::vector<RawDataBlock> blks;
  blks.reserve(blknum);
  for (size_t i = 0; i < blknum; ++i) {
    blks.emplace_back();
    writer(blks.back());
  }
  return blks;
}

auto incre_blocks(size_t blknum) -> std::vector<RawDataBlock> {
  size_t idx = 0;
  return gen_blocks(blknum, [&idx](auto &blk) {
    *reinterpret_cast<uint64_t *>(blk.data.data()) = idx;
    ++idx;
  });
}

auto decre_blocks(size_t blknum) -> std::vector<RawDataBlock> {
  size_t idx = blknum - 1;
  return gen_blocks(blknum, [&idx](auto &blk) {
    *reinterpret_cast<uint64_t *>(blk.data.data()) = idx;
    --idx;
  });
}

void gen_file_increment(std::string const &path, size_t blknum) {
  std::print("generating file to {}\n", path);
  gen_file(path, incre_blocks(blknum));
}

void gen_file_decrement(std::string const &path, size_t blknum) {
  std::print("generating file to {}\n", path);
  gen_file(path, decre_blocks(blknum));
}

void gen_file(std::string const &path,
              std::vector<RawDataBlock> const &blks) {
  std::print("generating file to {}\n", path);
  std::ofstream ofs(path, std::ios::binary | std::ios::out);
  for (auto const &blk : blks) {
    ofs.write(reinterpret_cast<const char *>(blk.data.data()), kDataBlockSize);
  }
  ofs.flush();
  ofs.close();
}

auto random_file(std::string const &path,
                 size_t blknum) -> std::vector<RawDataBlock> {
  std::print("generating file to {}\n", path);
  std::vector<RawDataBlock> blks;
  blks.reserve(blknum);
  for (size_t i = 0; i < blknum; ++i) { blks.emplace_back(random_block()); }
  gen_file(path, blks);
  return blks;
}

auto random_path() -> std::string {
  static auto randomHelper = RandomHelper();
  auto path = "/tmp/magclust_test/" + std::to_string(randomHelper.nextUInt64());
  return path;
}