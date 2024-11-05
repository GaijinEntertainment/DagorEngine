// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>

namespace da_profiler
{

// simple single-producer,single-consumer ring buffer. multiple consumers is easy extension, but we dont need it
template <typename T, size_t buffer_shift = 8>
class RingBuffer
{
public:
  static constexpr size_t buffer_size = 1 << buffer_shift;
  static constexpr size_t buffer_mask = buffer_size - 1;
  RingBuffer() : head(0), tail(0) {}
  void clear() { tail.store(head.load(std::memory_order_relaxed), std::memory_order_relaxed); }

  bool empty() const { return readAvailable() == 0; }
  bool isFull() const { return writeAvailable() == 0; }
  uint32_t readAvailable() const { return head.load(std::memory_order_acquire) - tail.load(std::memory_order_relaxed); }
  uint32_t writeAvailable() const
  {
    return buffer_size - (head.load(std::memory_order_relaxed) - tail.load(std::memory_order_acquire));
  }

  bool push_back(const T &v)
  {
    uint32_t tmpHead = head.load(std::memory_order_relaxed); // relaxed is fine as we write in one thread only (producer)

    if ((tmpHead - tail.load(std::memory_order_acquire)) == buffer_size)
      return false;
    else
    {
      data[tmpHead & buffer_mask] = v; // we use tmp head, instead of head++, as to be sure head is increased only after data is
                                       // written
      std::atomic_signal_fence(std::memory_order_release); // if we can't be sure that T is atomic type, we cant use interlocked_store
      head.store(tmpHead + 1, std::memory_order_release);
    }
    return true;
  }

  bool pop_front(T &to)
  {
    uint32_t tmpTail = tail.load(std::memory_order_relaxed); // as we read from one thread only (consumer)

    if (tmpTail == head.load(std::memory_order_acquire))
      return false;
    else
    {
      to = data[tmpTail & buffer_mask]; // we use tmp tail, instead of tail++, as to be sure tail is increased only after data is read
      std::atomic_signal_fence(std::memory_order_release); // if we can't be sure that T is atomic type, we cant use interlocked_store
      tail.store(tmpTail + 1, std::memory_order_release);
    }
    return true;
  }

  size_t read_front(T *buf, size_t buf_size)
  {
    uint32_t tmpTail = tail.load(std::memory_order_relaxed); // since we are single-consumer

    const uint32_t available = head.load(std::memory_order_acquire) - tmpTail;
    const size_t toRead = (available < buf_size) ? available : buf_size;

    // todo:we can divide into two memcpys. doesn't worth it
    for (size_t i = 0; i < toRead; i++)
      buf[i] = data[tmpTail++ & buffer_mask];

    std::atomic_signal_fence(std::memory_order_release);
    tail.store(tmpTail, std::memory_order_release);

    return toRead;
  }

private:
  static constexpr size_t cacheline_size = 64;
  alignas(cacheline_size) std::atomic<uint32_t> head;
  alignas(cacheline_size) std::atomic<uint32_t> tail;
  alignas(cacheline_size) T data[buffer_size];
};

} // namespace da_profiler