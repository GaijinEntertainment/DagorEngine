//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <osApiWrappers/dag_cpuJobs.h>
#include <atomic>
#include <osApiWrappers/dag_miscApi.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <math/dag_adjpow2.h>

namespace cpujobs
{
// Single or Multi Producer Multiple Consumers Queue, based on
// http://www.codeproject.com/Articles/153898/Yet-another-implementation-of-a-lock-free-circular

template <typename T, bool MULTI_PRODUCER>
struct JobQueue
{
// In case of multi_producer, each index is multiplied by 2
// LSB means that other thread is writing right now
// keep counters apart from each other to avoid false sharing
#ifndef CACHE_LINE_SIZE
  static constexpr int CACHE_LINE_SIZE = 128; // std::hardware_destructive_interference_size is c++17!
#endif
  std::atomic<uint32_t> writeIndex = {0}; // LSB - partial write in progress. we can't read from it
  uint32_t queueSizeMinusOne = ~0u;       // pow2
  T *queue = nullptr;
  char paddingToAvoidFalseSharing[CACHE_LINE_SIZE - sizeof(void *) - sizeof(uint32_t) * 2]; //-V730
  std::atomic<uint32_t> readIndex = {0};                                                    // LSB is always 0

  void initQueue(T *mem, unsigned sz)
  {
    G_ASSERT(!sz || is_pow_of2(sz));
    queue = mem;
    queueSizeMinusOne = sz - 1;
  }

  void allocQueue(unsigned sz) { initQueue(new T[sz], sz); }

  void freeQueue()
  {
    delete[] queue;
    queue = NULL;
    queueSizeMinusOne = ~0u;
  }

  uint32_t countToIndex(uint32_t i) const { return i / (MULTI_PRODUCER ? 2 : 1) & queueSizeMinusOne; }

  uint32_t getCurrentReadIndex() const { return readIndex.load(); }

  uint32_t push(T t, void (*on_queue_full_cb)(void *), void *param)
  {
    G_ASSERTF(queue, "attempt to push to empty queue");

    for (;;)
    {
      uint32_t currentWriteIndex;

      for (int i = 0;; ++i)
      {
        currentWriteIndex = writeIndex.load();
        if (countToIndex(MULTI_PRODUCER ? (currentWriteIndex & ~1) + 2 : currentWriteIndex + 1) == countToIndex(readIndex.load()))
        {
          if (!MULTI_PRODUCER || currentWriteIndex == writeIndex.load()) // All threads agree that the queue is full and no one writes.
          {
            if (i == 0 && on_queue_full_cb)
            {
              on_queue_full_cb(param);
              debug("queue is full!");
            }
            sleep_msec(0); // queue is full - wait some time for free slots
          }
        }
        else
          break;
      }

      if (MULTI_PRODUCER)
      {
        uint32_t allocIndex = currentWriteIndex & ~1;
        if (writeIndex.compare_exchange_strong(allocIndex, (allocIndex + 1))) // we allocate one, and all other writing threads have to
                                                                              // spinlock (go for next index), until queue is fully
                                                                              // written
        {
          queue[countToIndex(currentWriteIndex)] = t;
          writeIndex.fetch_add(1); // finalize writing
          return currentWriteIndex;
        }
      }
      else
      {
        queue[countToIndex(currentWriteIndex)] = t;

        // interlocked_increment((volatile int&)writeIndex);
        //  No need to increment write index atomically. It is a
        //  a requierement of this queue that only one thred can push stuff in
        writeIndex++;
        return currentWriteIndex;
      }
    }
  }

  T pop()
  {
    uint32_t currentMaximumReadIndex;
    uint32_t currentReadIndex;
    T ret;

    for (;;)
    {
      currentReadIndex = readIndex.load();
      currentMaximumReadIndex = writeIndex.load();

      if (countToIndex(currentReadIndex) == countToIndex(currentMaximumReadIndex))
        return T(); // return empty object

      // retrieve the data from the queue
      ret = queue[countToIndex(currentReadIndex)];
      if (readIndex.compare_exchange_strong(currentReadIndex, (currentReadIndex + (MULTI_PRODUCER ? 2 : 1))))
        return ret;
    }
  }

  uint32_t size()
  {
    uint32_t currentWriteIndex = writeIndex.load() / (MULTI_PRODUCER ? 2 : 1);
    uint32_t currentReadIndex = readIndex.load() / (MULTI_PRODUCER ? 2 : 1);

    if (currentWriteIndex >= currentReadIndex)
      return currentWriteIndex - currentReadIndex;
    else
      return (queueSizeMinusOne + 1) + currentWriteIndex - currentReadIndex;
  }
};
} // namespace cpujobs
