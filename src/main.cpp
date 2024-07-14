#include <cstdint>

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
  std::cout << "num: " << V.size() << ", sum: " << sum << ", avg: " << avg
            << "\n";
  // P5,P10,....P95
  for (size_t i = 5; i <= 95; i += 5) {
    size_t idx = V.size() * i / 100;
    std::cout << "P" << i << ": " << V.at(idx) << "\n";
  }
}

inline static auto popcount(uint64_t x) -> size_t {
  size_t res = 0;
  while (x > 0) { res += x & 1, x >>= 1; }
  return res;
}

auto diff_bits(RawDataBlock const& lhs, RawDataBlock const& rhs) -> size_t {
  size_t diff = 0;
  for (size_t i = 0; i < kDataBlockSize; ++i) {
    diff += popcount(static_cast<uint8_t>(lhs.data.at(i) ^ rhs.data.at(i)));
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
  if (pending.size() > 0) { zipblocks.emplace_back(std::move(pending)); }
  return zipblocks;
}

auto main(int argc, char* argv[]) -> int {
  try {
    if (argc < 2) {
      std::cout << "Usage: " << argv[0] << " <config file>\n";
      return 0;
    }
    std::cout << "Config file: " << argv[1] << "\n";
    auto config = readConfigFromFile(argv[1]);
    for (auto const& path : config.paths) {
      std::cout << "Path: " << path << "\n";
    }

    auto io_helper = std::make_unique<IOHelper<kBufferSize>>(config.paths);

    constexpr size_t feature_num = 12;
    constexpr size_t cluster_num = 4;
    constexpr size_t sample_bits = 7;
    constexpr size_t hash_shift = 8;
    Odess<feature_num, cluster_num, sample_bits, hash_shift> odess;
    std::map<uint64_t, std::vector<size_t>> sf2blk;
    // multithread here to accelerate
    constexpr size_t thread_num = 64;
    size_t per_thread = io_helper->totalBlocks() / thread_num;
    size_t now_thread_begin = 0;
    size_t now_thread_end = per_thread;
    std::vector<std::thread> threads;
    std::vector<DataBlock<cluster_num>> all_blocks;
    all_blocks.resize(io_helper->totalBlocks());
    for (size_t t = 0; t < thread_num; ++t) {
      threads.emplace_back(
          [now_thread_begin, now_thread_end, &all_blocks, &config, &odess]() {
            auto my_io_helper =
                std::make_unique<IOHelper<kBufferSize>>(config.paths);
            for (auto const& [idx, raw] :
                 my_io_helper->ranged(now_thread_begin, now_thread_end)) {
              auto& blk = all_blocks.at(idx);
              blk.idx = idx;
              blk.features_ = odess.genSuperFeatures(raw);
              if (idx % 100 == 0) {
                std::cout << "Thread " << std::this_thread::get_id()
                          << " processed " << idx - now_thread_begin << "/"
                          << now_thread_end - now_thread_begin << "\n";
              }
            }
          });
      now_thread_begin = now_thread_end;
      now_thread_end = (t == (thread_num - 1)) ? io_helper->totalBlocks()
                                               : now_thread_end + per_thread;
    }

    for (size_t t = 0; t < thread_num; ++t) { threads[t].join(); }

    for (auto const& blk : all_blocks) {
      for (auto const& f : blk.features_) { sf2blk[f].emplace_back(blk.idx); }
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

      std::cout << zip_idx << "/" << zipblocks.size()
                << " th zipblock avg diff bits: "
                << (diff / zipblock.blks.size() * (zipblock.blks.size() - 1) /
                    2)
                << "\n";
      zip_diff.emplace_back(diff / zipblock.blks.size() *
                            (zipblock.blks.size() - 1) / 2);
      ++zip_idx;
    }

    describe(zip_diff);

  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}