// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <atomic>
#include <debug/dag_assert.h>
#include <eastl/array.h>
#include <math/dag_intrin.h>
#include <new>
#include <util/dag_compilerDefs.h>
#include <windows.h>


class EventsPool
{
  static constexpr uint8_t MAX_SLOT_COUNT = 16u;
  static constexpr uint16_t DEFAULT_FREE_SLOT_MASK = (1u << MAX_SLOT_COUNT) - 1;

  eastl::array<HANDLE, MAX_SLOT_COUNT> slots = {};
  // Atomic writes don't invalidate readonly fields' cache lines.
  alignas(std::hardware_constructive_interference_size)
    // If the Nth bit in mask is set then the Nth slot is available.
    std::atomic<uint16_t> freeSlotMask = DEFAULT_FREE_SLOT_MASK;

public:
  class Event
  {
    union
    {
      EventsPool *const eventsPool;
      const HANDLE handle;
    };
    const int8_t slotIndex;

    friend class EventsPool;
    Event(EventsPool *events_pool, int8_t slot_index) : eventsPool{events_pool}, slotIndex{slot_index} {};
    Event(HANDLE handle) : handle{handle}, slotIndex{-1} {};

  public:
    Event(const Event &) = delete;
    Event &operator=(const Event &) = delete;
    Event(Event &&) = delete;
    Event &operator=(Event &&) = delete;
    ~Event();

    HANDLE operator*() const { return DAGOR_LIKELY(slotIndex >= 0) ? eventsPool->slots[slotIndex] : handle; }
  };

  Event acquireEvent();
  ~EventsPool();

private:
  Event GenerateEvent(int8_t slot_index);
};

inline EventsPool::Event EventsPool::acquireEvent()
{
  for (auto observedMask = freeSlotMask.load(std::memory_order_acquire); DAGOR_LIKELY(observedMask != 0);)
  {
    const int8_t slotIndex = __ctz_unsafe(observedMask);
    if (freeSlotMask.compare_exchange_weak(observedMask, observedMask ^ (1u << slotIndex), std::memory_order_acquire,
          std::memory_order_relaxed))
    {
      if (DAGOR_UNLIKELY(!slots[slotIndex]))
      {
        return GenerateEvent(slotIndex);
      }
      return {this, slotIndex};
    }
  }

  return GenerateEvent(-1);
}

inline EventsPool::Event::~Event()
{
  if (DAGOR_UNLIKELY(slotIndex < 0))
  {
    CloseHandle(handle);
  }
  else
  {
    eventsPool->freeSlotMask.fetch_or(1u << slotIndex, std::memory_order_release);
  }
}

inline EventsPool::~EventsPool()
{
  G_ASSERT(freeSlotMask.load(std::memory_order_relaxed) == DEFAULT_FREE_SLOT_MASK);
  for (auto &handle : slots)
  {
    if (handle)
    {
      CloseHandle(handle);
    }
  }
}

DAGOR_NOINLINE
inline EventsPool::Event EventsPool::GenerateEvent(int8_t slot_index)
{
  const auto handle = CreateEvent(nullptr, FALSE, FALSE, nullptr);
  G_ASSERT(handle != INVALID_HANDLE_VALUE);

  if (slot_index == -1)
  {
    return {handle};
  }
  else
  {
    slots[slot_index] = handle;
    return {this, slot_index};
  }
}