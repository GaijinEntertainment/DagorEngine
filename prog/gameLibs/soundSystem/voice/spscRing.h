// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <generic/dag_span.h>
#include <osApiWrappers/dag_atomic.h>
#include <math/dag_adjpow2.h>
#include <EASTL/algorithm.h>
#include <EASTL/vector.h>

namespace voicechat
{

// Lock-free ring for one producer thread and one consumer thread. write, writeWith and freeSpace belong to
// the producer; read, available, skip and discard belong to the consumer. Its counters run free: each side
// stores its own and reads the other's.
template <typename T>
class SpscRing
{
public:
  // the capacity is min_capacity rounded up to a power of two
  explicit SpscRing(uint32_t min_capacity) : storage(get_bigger_pow2(min_capacity)), mask(storage.size() - 1) {}

  uint32_t capacity() const { return mask + 1; }
  uint32_t freeSpace() const { return capacity() - (writeCount - interlocked_acquire_load(readCount)); }
  uint32_t available() const { return interlocked_acquire_load(writeCount) - readCount; }

  // writes all of count elements made by make(i), or none when they do not fit
  template <typename F>
  bool writeWith(uint32_t count, F &&make)
  {
    if (count > freeSpace())
      return false;
    const uint32_t at = writeCount;
    for (uint32_t i = 0; i < count; ++i)
      storage[(at + i) & mask] = make(i);
    interlocked_release_store(writeCount, at + count);
    return true;
  }

  bool write(dag::ConstSpan<T> data)
  {
    return writeWith(data.size(), [&data](uint32_t i) { return data[i]; });
  }

  uint32_t read(dag::Span<T> out)
  {
    const uint32_t count = eastl::min<uint32_t>(available(), out.size());
    const uint32_t at = readCount;
    for (uint32_t i = 0; i < count; ++i)
      out[i] = storage[(at + i) & mask];
    interlocked_release_store(readCount, at + count);
    return count;
  }

  void skip(uint32_t count) { interlocked_release_store(readCount, readCount + eastl::min(count, available())); }
  void discard() { interlocked_release_store(readCount, interlocked_acquire_load(writeCount)); }

private:
  eastl::vector<T> storage;
  const uint32_t mask;
  volatile uint32_t writeCount = 0;
  volatile uint32_t readCount = 0;
};

} // namespace voicechat
