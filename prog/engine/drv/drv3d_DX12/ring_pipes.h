// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "constants.h"
#include <atomic>
#include <osApiWrappers/dag_events.h>
#include <osApiWrappers/dag_spinlock.h>

#if !defined(_M_ARM64)
#include <emmintrin.h>
#endif


constexpr uint32_t default_event_wait_time = 1;

// runs until check returns true, after some busy loop tries it starts to check the event state
template <typename T>
inline void try_and_wait_with_os_event(os_event_t &event, uint32_t spin_count, T check, uint32_t idle_time = default_event_wait_time)
{
  for (uint32_t i = 0; i < spin_count; ++i)
  {
    if (check())
      return;
#if !defined(_M_ARM64)
    _mm_pause();
#endif
  }
  for (;;)
  {
    if (check())
      return;
    os_event_wait(&event, idle_time);
  }
}

template <typename T, size_t N>
class ForwardRing
{
  T elements[N]{};
  T *selectedElement = &elements[0];

  ptrdiff_t getIndex(const T *pos) const { return pos - &elements[0]; }

  T *advancePosition(T *pos) { return &elements[(getIndex(pos) + 1) % N]; }

public:
  T &get() { return *selectedElement; }
  const T &get() const { return *selectedElement; }
  T &operator*() { return *selectedElement; }
  const T &operator*() const { return *selectedElement; }
  T *operator->() { return selectedElement; }
  const T *operator->() const { return selectedElement; }

  constexpr size_t size() const { return N; }

  void advance() { selectedElement = advancePosition(selectedElement); }

  // Iterates all elements of the ring in a unspecified order
  template <typename T>
  void iterate(T clb)
  {
    for (auto &&v : elements)
    {
      clb(v);
    }
  }

  // Iterates all elements from last to the current element
  template <typename T>
  void walkAll(T clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N; ++i)
    {
      clb(*at);
      at = advancePosition(at);
    }
  }

  // Iterates all elements from last to the element before the current element
  template <typename T>
  void walkUnitlCurrent(T clb)
  {
    auto at = advancePosition(selectedElement);
    for (size_t i = 0; i < N - 1; ++i)
    {
      clb(at);
      at = advancePosition(at);
    }
  }
};

// Ring buffer acting as a thread safe, non blocking, single producer, single consumer pipe
template <typename T, size_t N>
class ConcurrentRingPipe
{
  T pipe[N]{};
  alignas(64) std::atomic<size_t> readPos{0};
  alignas(64) std::atomic<size_t> writePos{0};

public:
  T *tryAcquireRead()
  {
    auto rp = readPos.load(std::memory_order_relaxed);
    if (rp < writePos.load(std::memory_order_acquire))
    {
      return &pipe[rp % N];
    }
    return nullptr;
  }
  void releaseRead() { ++readPos; }
  T *tryAcquireWrite()
  {
    auto wp = writePos.load(std::memory_order_relaxed);
    if (wp - readPos.load(std::memory_order_acquire) < N)
      return &pipe[wp % N];
    return nullptr;
  }
  void releaseWrite() { ++writePos; }
  bool canAcquireWrite() const { return (writePos.load(std::memory_order_relaxed) - readPos.load(std::memory_order_acquire)) < N; }
};

// Specialization with only one entry, this is used for builds that only support
// concurrent execution mode and don't need more than one pipe element.
// No call will block, return null or fail.
template <typename T>
class ConcurrentRingPipe<T, 1>
{
  T pipe{};

public:
  T *tryAcquireRead() { return &pipe; }
  void releaseRead() {}
  T *tryAcquireWrite() { return &pipe; }
  void releaseWrite() {}
  constexpr bool canAcquireWrite() const { return true; }
};

// Extended ConcurrentRingPipe that implements blocking read/write
template <typename T, size_t N>
class WaitableConcurrentRingPipe : private ConcurrentRingPipe<T, N>
{
  using BaseType = ConcurrentRingPipe<T, N>;

  os_event_t itemsToRead;
  os_event_t itemsToWrite;

public:
  WaitableConcurrentRingPipe()
  {
    os_event_create(&itemsToRead, "WCRP:itemsToRead");
    os_event_create(&itemsToWrite, "WCRP:itemsToWrite");
  }
  ~WaitableConcurrentRingPipe()
  {
    os_event_destroy(&itemsToRead);
    os_event_destroy(&itemsToWrite);
  }
  using BaseType::tryAcquireRead;
  void releaseRead()
  {
    BaseType::releaseRead();
    os_event_set(&itemsToWrite);
  }
  using BaseType::tryAcquireWrite;
  void releaseWrite()
  {
    BaseType::releaseWrite();
    os_event_set(&itemsToRead);
  }
  T &acquireRead()
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToRead, drv3d_dx12::DEFAULT_WAIT_SPINS,
      [this, &result]() //
      {
        result = tryAcquireRead();
        return result != nullptr;
      });
    return *result;
  }
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireRead(U &&clb)
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToRead, drv3d_dx12::DEFAULT_WAIT_SPINS,
      [this, &result, &clb]() //
      {
        if (!clb())
          return true;
        result = tryAcquireRead();
        return result != nullptr;
      });
    return result;
  }
  T &acquireWrite()
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToWrite, drv3d_dx12::DEFAULT_WAIT_SPINS,
      [this, &result]() //
      {
        result = tryAcquireWrite();
        return result != nullptr;
      });
    return *result;
  }
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireWrite(U &&clb)
  {
    T *result = nullptr;
    try_and_wait_with_os_event(itemsToWrite, drv3d_dx12::DEFAULT_WAIT_SPINS,
      [this, &result, &clb]() //
      {
        if (!clb())
          return true;
        result = tryAcquireWrite();
        return result != nullptr;
      });
    return result;
  }
  using BaseType::canAcquireWrite;
  void waitForWriteItem()
  {
    try_and_wait_with_os_event(itemsToWrite, drv3d_dx12::DEFAULT_WAIT_SPINS, [this]() { return this->canAcquireWrite(); });
  }
};

// Specialization with only one entry, this is used for builds that only support
// concurrent execution mode and don't need more than one pipe element.
// No call will block, return null or fail.
template <typename T>
class WaitableConcurrentRingPipe<T, 1> : private ConcurrentRingPipe<T, 1>
{
  using BaseType = ConcurrentRingPipe<T, 1>;

public:
  using BaseType::releaseRead;
  using BaseType::releaseWrite;
  using BaseType::tryAcquireRead;
  using BaseType::tryAcquireWrite;
  T &acquireRead() { return *tryAcquireRead(); } // -V522
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireRead(U &&)
  {
    return acquireRead();
  }
  T &acquireWrite() { return *tryAcquireWrite(); } // -V522
  // repeatedly call clb until it either returns false or a T could be acquired
  // return value of acquireRead can be nullptr if clb returned false
  template <typename U>
  T *acquireWrite(U &&)
  {
    return acquireWrite();
  }
  void waitForWriteItem() {}
  using BaseType::canAcquireWrite;
};
