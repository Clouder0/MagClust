#include <xxhash.h>

#include <random>

#include "common.h"
#include "gear.h"
#include "model.h"

consteval auto gen_mask(size_t sample_bits) -> uint8_t {
  // NOLINTBEGIN
  switch (sample_bits) {
    case 0:
      return 0;
    case 1:
      return 0x010;
    case 2:
      return 0x101;
    case 3:
      return 0x190;
    case 4:
      return 0x033;
    case 5:
      return 0x03b;
    case 6:
      return 0x30f;
    case 7:
      return 0x7f0;
    case 8:
      return 0xf00f;
  }
  // NOLINTEND
  return 0;
}

template <size_t feature_num, size_t cluster_num, size_t sample_bits,
          size_t hash_shift>
class Odess {
  static_assert(sample_bits <= 8, "sample bits cannot exceed 8");

 private:
  std::default_random_engine random_engine_;
  std::array<int, feature_num> k_array_;
  std::array<int, feature_num> b_array_;

  static constexpr uint8_t mask = gen_mask(sample_bits);

 public:
  Odess() {
    std::uniform_int_distribution<uint64_t> distributionA;
    std::uniform_int_distribution<uint64_t> distributionB;
    constexpr uint64_t magic1l = 0x0000000000100000;
    constexpr uint64_t magic1r = 0x0000000010000000;
    constexpr uint64_t magic2l = 0x0000000000100000;
    constexpr uint64_t magic2r = 0x00000000ffffffff;
    const std::uniform_int_distribution<uint64_t>::param_type paramA(magic1l,
                                                                     magic1r);
    const std::uniform_int_distribution<uint64_t>::param_type paramB(magic2l,
                                                                     magic2r);
    distributionA.param(paramA);
    distributionB.param(paramB);
    for (size_t i = 0; i < feature_num; ++i) {
      k_array_.at(i) = distributionA(random_engine_);
      b_array_.at(i) = distributionB(random_engine_);
    }
  }

  auto genSuperFeatures(RawDataBlock const &blk)
      -> std::array<uint64_t, cluster_num> {
    std::array<uint32_t, feature_num> max_list_{};
    for (auto &val : max_list_) { val = 0; }
    std::array<uint64_t, cluster_num> res{};
    uint64_t hash_value = 0;
    for (auto cur_byte : blk.data) {
      // std::print("{}", static_cast<uint32_t>(cur_byte));
      hash_value = (hash_value << hash_shift) +
                   gearMatrix.at(static_cast<uint8_t>(cur_byte));
      if ((hash_value & mask) != 0) { continue; }
      for (size_t j = 0; j < feature_num; ++j) {
        const uint32_t trans_res =
            (hash_value * k_array_.at(j) + b_array_.at(j));
        max_list_.at(j) = std::max(max_list_.at(j), trans_res);
      }
    }

    constexpr size_t cluster_size = feature_num / cluster_num;
    static_assert(feature_num % cluster_num == 0);
    constexpr uint64_t seed = 0x7fcaf1;
    // for (const auto &val : max_list_) { std::print("{},", val); }
    // std::print("\n");
    for (size_t i = 0; i < cluster_num; ++i) {
      res.at(i) = XXH64(max_list_.data() + i * cluster_size,
                        sizeof(uint32_t) * cluster_size, seed);
    }
    return res;
  }
};
