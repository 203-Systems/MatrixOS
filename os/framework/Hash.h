#pragma once
#include "Types.h"

const uint32_t FNV_PRIME = 16777619u;
const uint32_t FNV_OFFSET_BASIS = 2166136261u;

inline uint32_t FNV1aHash(const char* ptr, size_t length)  // Implentmation of FNV-1a Hash
{
  uint32_t hash = FNV_OFFSET_BASIS;
  for (size_t i = 0; i < length; ++i)
  {
    hash ^= *ptr++;
    hash *= FNV_PRIME;
  }
  return hash;
}

inline uint32_t FNV1aHash(const char* str)
{
  const size_t length = strlen(str) + 1;
  return FNV1aHash(str, length);
}


inline uint32_t Hash(string str) {
  return FNV1aHash(str.c_str());
}

// Static Hash: Hashing at compile time
template <uint32_t N, uint32_t I>
struct HashHelper {
  constexpr static uint32_t Calculate(const char (&str)[N]) {
    return (HashHelper<N, I - 1>::Calculate(str) ^ (str[I - 1] & 0xFF)) * FNV_PRIME;
  }
};

template <uint32_t N>
struct HashHelper<N, 1> {
  constexpr static uint32_t Calculate(const char (&str)[N]) { return (FNV_OFFSET_BASIS ^ (str[0] & 0xFF)) * FNV_PRIME; }
};

template <uint32_t N>
constexpr static uint32_t StaticHash(const char (&str)[N]) {
  return HashHelper<N, N>::Calculate(str);
}
