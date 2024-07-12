#include "common.h"

// NOLINTBEGIN
using size_t = decltype(sizeof(0));

#pragma pack(push, 1)
template <auto, class, size_t N>
union member_at;
template <class T, class U, T U::*M, class B, size_t N>
union member_at<M, B, N> {
  std::array<char, sizeof(B)> data;
  struct {
    std::array<char, N> at;
    T member;
  };
  B parent;
  U base;
};
#pragma pack(pop)

template <auto M, class B, size_t Lo = 0, size_t Hi = sizeof(B)>
constexpr auto find_offset_of() -> size_t {
  constexpr size_t Mid = Lo + (Hi - Lo) / 2;
  if constexpr (Lo == Mid) {
    return Mid;
  } else {
    constexpr member_at<M, B, Mid> test{};

    if constexpr (&(test.parent.*M) < &test.member) {
      return find_offset_of<M, B, Lo, Mid>();
    } else {
      return find_offset_of<M, B, Mid, Hi>();
    }
  }
}

template <auto M, class B = decltype(member_at<M, int, 1>::base)>
constexpr size_t as_offset = find_offset_of<M, B>();

template <class T, size_t alignment>
constexpr size_t aligned_sizeof_base =
    (sizeof(T) + alignment - 1) & ~(alignment - 1);

template <class T>
constexpr size_t aligned_sizeof = aligned_sizeof_base<T, alignof(T)>;

// NOLINTEND