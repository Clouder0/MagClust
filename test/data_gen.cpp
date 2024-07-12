#include "data_gen.h"

#include <cstdint>
#include <filesystem>
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

void gen_file(std::filesystem::path const &path, size_t blknum) {
  std::print("generating file to {}\n", path.string());
  std::ofstream ofs(path, std::ios::binary | std::ios::out);
  RawDataBlock blk{};
  for (size_t i = 0; i < blknum; ++i) {
    *reinterpret_cast<uint64_t *>(blk.data.data()) = i;
    ofs.write(reinterpret_cast<char *>(blk.data.data()), kDataBlockSize);
  }
  ofs.flush();
  ofs.close();
}

auto random_file(std::filesystem::path const &path,
                 size_t blknum) -> std::vector<RawDataBlock> {
  std::print("generating file to {}\n", path.string());
  std::vector<RawDataBlock> blks;
  blks.reserve(blknum);
  for (size_t i = 0; i < blknum; ++i) { blks.emplace_back(random_block()); }
  std::ofstream ofs(path, std::ios::binary | std::ios::out);
  for (size_t i = 0; i < blknum; ++i) {
    ofs.write(reinterpret_cast<char *>(blks[i].data.data()), kDataBlockSize);
  }
  return blks;
}

auto random_path() -> std::filesystem::path {
  static auto randomHelper = RandomHelper();
  auto path = std::filesystem::temp_directory_path() / "magclust_test" / 
         std::to_string(randomHelper.nextUInt64());
  std::filesystem::create_directory(path.parent_path());
  return path;
}