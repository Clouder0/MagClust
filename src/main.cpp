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
  for (size_t i = 0; i <= 100; i += 5) {
    size_t idx = (V.size() - 1) * i / 100;
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

auto aggregate_compare(size_t total_blks,
                       std::map<uint64_t, std::vector<size_t>> const& sf2blk) {
  std::vector<uint64_t> all_blocks;
  for (auto const& [sf, blks] : sf2blk) {
    for (auto const& blk : blks) { all_blocks.emplace_back(blk); }
  }
  // random shuffle
  std::shuffle(all_blocks.begin(), all_blocks.end(),
               std::mt19937(std::random_device()()));
  // divide to zipblocks
  std::vector<ZipBlock> zipblocks;
  std::vector<size_t> pending;
  pending.reserve(kBlockPerZip);
  for (auto const& blk : all_blocks) {
    pending.emplace_back(blk);
    if (pending.size() >= kBlockPerZip) {
      zipblocks.emplace_back(pending);
      pending.clear();
    }
  }
  if (pending.size() > 0) { zipblocks.emplace_back(std::move(pending)); }
  return zipblocks;
}

auto aggregate(size_t total_blks,
               std::map<uint64_t, std::vector<size_t>> const& sf2blk) {
  std::vector<bool> blk_used;
  blk_used.resize(total_blks);

  std::vector<ZipBlock> zipblocks;
  std::vector<size_t> pending;
  // pending.reserve(kBlockPerZip);
  for (auto const& [sf, blks] : sf2blk) {
    for (auto const& blk : blks) {
      if (blk_used.at(blk)) { continue; }
      blk_used.at(blk) = true;
      pending.emplace_back(blk);
      if (pending.size() >= kBlockPerZip) {
        zipblocks.emplace_back(pending);
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
                std::cout << " processed [" + std::to_string(now_thread_begin) +
                                 "," + std::to_string(idx) + "," +
                                 std::to_string(now_thread_end) + "]\n";
              }
            }
          });
      now_thread_begin = now_thread_end;
      now_thread_end = (t == (thread_num - 1)) ? io_helper->totalBlocks()
                                               : now_thread_end + per_thread;
    }

    for (size_t t = 0; t < thread_num; ++t) { threads[t].join(); }

    std::cout << "Done feature calculating.\n";

    for (auto const& blk : all_blocks) {
      for (auto const& f : blk.features_) { sf2blk[f].emplace_back(blk.idx); }
    }

    std::cout << "Done feature mapping.\n";

    auto zipblocks = aggregate(io_helper->totalBlocks(), sf2blk);

    std::cout << "Zipblocks result:";
    std::vector<uint64_t> zip_diff;
    zip_diff.resize(zipblocks.size());
    threads.clear();
    per_thread = zipblocks.size() / thread_num;
    now_thread_begin = 0;
    now_thread_end = per_thread;

    for (size_t t = 0; t < thread_num; ++t) {
      threads.emplace_back([now_thread_begin, now_thread_end, &zipblocks,
                            &zip_diff, &config]() {
        auto my_io_helper =
            std::make_unique<IOHelper<kBufferSize>>(config.paths);
        for (size_t i = now_thread_begin; i < now_thread_end; ++i) {
          auto& zipblock = zipblocks.at(i);
          size_t diff = 0;
          for (size_t k = 1; k < zipblock.blks.size(); ++k) {
            for (size_t j = 0; j < k; ++j) {
              auto current_dif =
                  diff_bits(my_io_helper->readBlock(zipblock.blks.at(k)),
                            my_io_helper->readBlock(zipblock.blks.at(j)));
              diff += current_dif;
            }
          }
          auto avg_diff =
              diff / zipblock.blks.size() / (zipblock.blks.size() - 1) * 2;
          zip_diff.at(i) = avg_diff;
          std::cout << std::to_string(i - now_thread_begin) + "/" +
                           std::to_string(now_thread_end - now_thread_begin) +
                           " processed, diff bits:" + std::to_string(avg_diff) +
                           "\n";
        }
      });
      now_thread_begin = now_thread_end;
      now_thread_end = (t == (thread_num - 1)) ? zipblocks.size()
                                               : now_thread_end + per_thread;
    }

    for (size_t t = 0; t < thread_num; ++t) { threads[t].join(); }

    std::cout << "Done.\n";
    describe(zip_diff);

  } catch (const std::exception& e) { std::cerr << e.what() << '\n'; }
  return 0;
}