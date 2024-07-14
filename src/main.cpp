#include "common.h"
#include "config.h"
#include "io.h"
#include "model.h"
#include "odess.h"

template <class T>
  requires(std::is_arithmetic_v<T>)
void describe(std::vector<T> const& values) {
  std::vector<T> V = values;
  std::sort(V.begin(), V.end());
  // calculate avg
  T sum = 0;
  for (auto const& v : V) { sum += v; }
  T avg = sum / V.size();
  std::cout << std::format("num: {}, sum: {}, avg: {}\n", V.size(), sum, avg);
  // P5,P10,....P95
  for (size_t i = 5; i <= 95; i += 5) {
    size_t idx = V.size() * i / 100;
    std::cout << std::format("P{}: {}\n", i, V.at(idx));
  }
}

auto diff_bits(RawDataBlock const& lhs, RawDataBlock const& rhs) -> size_t {
  size_t diff = 0;
  for (size_t i = 0; i < kDataBlockSize; ++i) {
    diff +=
        std::popcount(static_cast<uint8_t>(lhs.data.at(i) ^ rhs.data.at(i)));
  }
  return diff;
}

auto aggregate(size_t total_blks,
               std::map<uint64_t, std::vector<size_t>> const& sf2blk) {
  std::vector<bool> blk_used;
  blk_used.reserve(total_blks);

  std::vector<ZipBlock> zipblocks;
  std::vector<size_t> pending;
  pending.reserve(kBlockPerZip);
  for (auto const& [sf, blks] : sf2blk) {
    for (auto const& blk : blks) {
      if (blk_used[blk]) { continue; }
      blk_used[blk] = true;
      pending.emplace_back(blk);
      if (pending.size() >= kBlockPerZip) {
        zipblocks.emplace_back(std::move(pending));
        pending.reserve(kBlockPerZip);
        pending.clear();
      }
    }
  }
  return zipblocks;
}

auto main(int argc, char* argv[]) -> int {
  try {
    if (argc < 2) {
      std::cout << std::format("Usage: {} <config file>\n", argv[0]);
      return 0;
    }
    std::cout << std::format("Config file: {}\n", argv[1]);
    auto config = readConfigFromFile(argv[1]);
    for (auto const& path : config.paths) {
      std::cout << std::format("Path: {}\n", path.string());
    }

    auto io_helper = std::make_unique<IOHelper<kBufferSize>>(config.paths);

    constexpr size_t feature_num = 12;
    constexpr size_t cluster_num = 4;
    constexpr size_t sample_bits = 7;
    constexpr size_t hash_shift = 8;
    Odess<feature_num, cluster_num, sample_bits, hash_shift> odess;
    std::map<uint64_t, std::vector<size_t>> sf2blk;
    for (auto const& [idx, blk] : *io_helper) {
      auto sf = odess.genSuperFeatures(blk);
      for (auto const& f : sf) { sf2blk[f].emplace_back(idx); }
    }

    auto zipblocks = aggregate(io_helper->totalBlocks(), sf2blk);

    auto zip_idx = 0;
    std::cout << "Zipblocks result:";
    std::vector<uint64_t> zip_diff;
    zip_diff.reserve(zipblocks.size());
    for (auto const& zipblock : zipblocks) {
      // for(auto const &blk : zipblock.blks) {
      // std::print("{},", blk);
      // }
      size_t diff = 0;
      for (size_t i = 1; i < zipblock.blks.size(); ++i) {
        for (size_t j = 0; j < i; ++j) {
          auto current_dif =
              diff_bits(io_helper->readBlock(zipblock.blks.at(i)),
                        io_helper->readBlock(zipblock.blks.at(j)));
          diff += current_dif;
        }
      }

      if (zip_idx % 100 == 0) {
        std::cout << std::format(
            "{}/{} th zipblock avg diff bits: {}\n", zip_idx, zipblocks.size(),
            diff / zipblock.blks.size() * (zipblock.blks.size() - 1) / 2);
      }
      zip_diff.emplace_back(diff / zipblock.blks.size() *
                            (zipblock.blks.size() - 1) / 2);
      ++zip_idx;
    }

    describe(zip_diff);

  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}