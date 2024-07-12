#pragma once

#include <cstddef>

auto getResult() -> int;

constexpr size_t kCachelineSize = 64;

template<typename T, typename U> consteval auto offsetOf(U T::*member) -> size_t
{
    return (char*)&((T*)nullptr->*member) - (char*)nullptr;
}
