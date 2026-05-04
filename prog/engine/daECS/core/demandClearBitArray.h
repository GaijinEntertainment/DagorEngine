// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <debug/dag_assert.h>
#include <string.h>

// Bitarray with on-demand clearing: only memsets the word range actually touched.
// Templated on Allocator (e.g. framemem_allocator) which must provide
// static allocate(size_t), resizeInplace(void*, size_t), realloc(void*, size_t).
template <typename Allocator>
struct DemandClearBitArray
{
  uint32_t *bits = nullptr;
  uint32_t capacity = 0;
  uint32_t minWord = ~0u;
  uint32_t maxWord = 0;

  explicit DemandClearBitArray(uint32_t bit_capacity) :
    bits{(uint32_t *)Allocator::allocate(((bit_capacity + 31) >> 5) * sizeof(uint32_t))}, capacity{bit_capacity}
  {}
  ~DemandClearBitArray() { Allocator::deallocate(bits, ((capacity + 31) >> 5) * sizeof(uint32_t)); }
  DemandClearBitArray(const DemandClearBitArray &) = delete;
  DemandClearBitArray &operator=(const DemandClearBitArray &) = delete;

  bool testAndSet(uint32_t idx)
  {
    if (DAGOR_UNLIKELY(idx >= capacity))
      grow(idx + 1);
    const uint32_t word = idx >> 5;
    const uint32_t bit = 1u << (idx & 31);
    if (DAGOR_UNLIKELY(minWord > maxWord)) // first insert - no range initialized yet
    {
      bits[word] = bit;
      minWord = maxWord = word;
      return true;
    }
    if (word < minWord)
    {
      memset(bits + word, 0, (minWord - word) * sizeof(uint32_t));
      minWord = word;
    }
    else if (word > maxWord)
    {
      memset(bits + maxWord + 1, 0, (word - maxWord) * sizeof(uint32_t));
      maxWord = word;
    }
    if (bits[word] & bit)
      return false;
    bits[word] |= bit;
    return true;
  }

private:
  void grow(uint32_t new_capacity)
  {
    const uint32_t newBytes = ((new_capacity + 31) >> 5) * sizeof(uint32_t);
    if (!Allocator::resizeInplace(bits, newBytes))
      bits = (uint32_t *)Allocator::realloc(bits, newBytes);
    capacity = new_capacity;
  }
};
