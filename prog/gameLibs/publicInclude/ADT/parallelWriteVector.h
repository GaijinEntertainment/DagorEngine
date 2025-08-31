//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/algorithm.h>
#include <EASTL/memory.h>
#include <dag/dag_relocatable.h>
#include <generic/dag_span.h>
#include <osApiWrappers/dag_miscApi.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_assert.h>


// Vector-like structure, that allows pushing items from multiple threads in parallel.
// Other interface is similar to normal vector and is not thread-safe.
// In addition it also has a thread-safe swap
//
// NOTE: While writing from multiple threads, methods other than writeNext/push_back/swap
// with multiple_writers parameter set to true, are NOT safe to call!
//
// If AllowGrowOnWrite is false, then it will do only 1 atomic add on write instead of at least 2,
// but will not be able to grow on write, and must be reserved prior to writing
template <typename T, bool AllowGrowOnWrite = true>
struct ParallelWriteVector
{
  using value_type = T;
  using iterator = T *;
  using const_iterator = const T *;

  ParallelWriteVector() = default;
  EA_NON_COPYABLE(ParallelWriteVector);
  ParallelWriteVector(ParallelWriteVector &&rhs) :
    writePosAndLock(eastl::exchange(rhs.writePosAndLock, 0u)),
    ptr(eastl::exchange(rhs.ptr, nullptr)),
    allocatedSize(eastl::exchange(rhs.allocatedSize, 0u))
  {
    validateNotLocked("move constructor");
  }
  ParallelWriteVector &operator=(ParallelWriteVector &&rhs)
  {
    reallocateBuffer(0);
    rhs.validateNotLocked("move assignment");
    writePosAndLock = eastl::exchange(rhs.writePosAndLock, 0u);
    ptr = eastl::exchange(rhs.ptr, nullptr);
    allocatedSize = eastl::exchange(rhs.allocatedSize, 0u);
    return *this;
  }
  ~ParallelWriteVector() { reallocateBuffer(0); }

  // normal vector stuff

  uint32_t size() const { return uint32_t(writePosAndLock >> WRITE_POS_SHIFT); }
  bool empty() const { return size() == 0; }
  uint32_t capacity() const { return allocatedSize; }
  const T *begin() const
  {
    validateNotLocked("cbegin()");
    return ptr;
  }
  const T *end() const { return ptr + size(); }
  T *begin()
  {
    validateNotLocked("begin()");
    return ptr;
  }
  T *end() { return ptr + size(); }
  const T *data() const { return ptr; }
  T *data() { return ptr; }
  T &operator[](int idx)
  {
    validateNotLocked("operator[]");
    return ptr[idx];
  }
  const T &operator[](int idx) const
  {
    validateNotLocked("operator[]");
    return ptr[idx];
  }

  void clear()
  {
    validateNotLocked("clear()");
    eastl::destruct(begin(), end());
    writePosAndLock = 0u;
  }
  void reallocateBuffer(uint32_t new_size)
  {
    validateNotLocked("reallocateInternalBuffer()");
    if (new_size == capacity())
      return;
    resizeDataImpl(size(), new_size);
    if (new_size < size())
      writePosAndLock = uint64_t(new_size) << WRITE_POS_SHIFT;
  }
  void shrink_to_fit() { reallocateBuffer(size()); }
  void clear_and_shrink() { reallocateBuffer(0); }
  void reserve(uint32_t new_capacity)
  {
    validateNotLocked("reserve()");
    if (new_capacity > capacity())
      reallocateBuffer(new_capacity);
  }

  T *erase(const T *first_, const T *last_)
  {
    T *first = const_cast<T *>(first_);
    T *last = const_cast<T *>(last_);
    validateNotLocked("erase(first, last)");
    if (first != last)
    {
      G_ASSERT_RETURN(first < last, first);
      G_ASSERT(first >= begin() && first < end());
      G_ASSERT(last > begin() && last <= end());
      if constexpr (dag::is_type_relocatable<T>::value)
      {
        eastl::destruct(first, last);
        memmove(first, last, reinterpret_cast<char *>(end()) - reinterpret_cast<char *>(last));
      }
      else
      {
        T *const position = eastl::move(last, end(), first);
        eastl::destruct(position, end());
      }
      writePosAndLock -= uint64_t(last - first) << WRITE_POS_SHIFT;
    }
    return first;
  }

  T *erase(const T *at) { return erase(at, at + 1); }

  // normal push_back, but can be called in parallel with other push_backs
  void push_back(const T &item)
  {
    writeNext(true, 1, [&](dag::Span<T> span) { new (span.data(), _NEW_INPLACE) T(item); });
  }

  // normal push_back, but can be called in parallel with other push_backs
  void push_back(T &&item)
  {
    writeNext(true, 1, [&](dag::Span<T> span) { new (span.data(), _NEW_INPLACE) T(eastl::move(item)); });
  }

  template <typename F>
  void writeNext(bool multiple_writers, F &&cb)
  {
    writeNext(multiple_writers, 1, [&](dag::Span<T> span) { cb(*span.data()); });
  }

#if 1
  // Allocate next item_count contiguous items, do not construct them,
  // pass span to cb. In case multiple_writers is true, can be safely called in parallel
  // from multiple threads
  //
  // IMPORTANT: cb MUST construct all items in the given span,
  // as they will be destructed later using ~T()
  template <typename F>
  void writeNext(bool multiple_writers, uint32_t item_count, F &&cb)
  {
    G_ASSERT_RETURN(item_count > 0, );
    uint32_t idx = writerLockPushAndIncPos(multiple_writers, item_count);
    if (DAGOR_UNLIKELY(idx + item_count > allocatedSize))
    {
      G_ASSERT_AND_DO(AllowGrowOnWrite, {
        writerUnlockPush(multiple_writers);
        return;
      });
      idx = writerReLockRealloc(multiple_writers, /* push_locked */ true, item_count);
      if (DAGOR_LIKELY(idx + item_count > allocatedSize))
        resizeDataImpl(idx, eastl::max<uint32_t>(idx + item_count + 8u, allocatedSize * 2u));
      writerUnlockRealloc(multiple_writers, /* push_locked */ true);
    }
    cb(dag::Span<T>(ptr + idx, item_count));
    writerUnlockPush(multiple_writers);
  }
#else // for multi-threading write validation
  template <typename F>
  void writeNext(bool multiple_writers, uint32_t item_count, F &&cb)
  {
    uint32_t idx = writerLockPushAndIncPos(multiple_writers, item_count);
    G_ASSERT(interlocked_acquire_load(dbgRealloc) == 0);
    interlocked_increment(dbgPush);
    if (DAGOR_UNLIKELY(idx + item_count > allocatedSize))
    {
      G_ASSERT_AND_DO(AllowGrowOnWrite, {
        writerUnlockPush(multiple_writers);
        return;
      });
      TIME_PROFILE(ParallelWriteVector__realloc)
      interlocked_decrement(dbgPush);
      G_ASSERT(interlocked_acquire_load(dbgRealloc) == 0);
      idx = writerReLockRealloc(multiple_writers, /* push_locked */ true, item_count);
      G_ASSERT(interlocked_increment(dbgPush) == 1);
      G_ASSERT(interlocked_increment(dbgRealloc) == 1);
      if (DAGOR_LIKELY(idx + item_count > allocatedSize))
        resizeDataImpl(idx, eastl::max<uint32_t>(idx + item_count, allocatedSize + 1)); // more reallocations
      G_ASSERT(interlocked_decrement(dbgRealloc) == 0);
      writerUnlockRealloc(multiple_writers, /* push_locked */ true);
    }
    cb(dag::Span<T>(ptr + idx, item_count));
    G_ASSERT(interlocked_acquire_load(dbgRealloc) == 0);
    interlocked_decrement(dbgPush);
    writerUnlockPush(multiple_writers);
  }
  int dbgPush = 0, dbgRealloc = 0;
#endif

  // Swaps two vectors. In case multiple_writers is true, it is safe
  // to call it in parallel with other swaps and writeNext
  // TODO: this was not properly tested
  void swap(bool multiple_writers, ParallelWriteVector &rhs)
  {
    G_ASSERT_RETURN(AllowGrowOnWrite, );
    if (&rhs == this)
      return;
    // lock both and acquire actual sizes (those cant change, while realloc is locked)
    const uint32_t thisSize = writerReLockRealloc(multiple_writers, false, 0);
    const uint32_t rhsSize = rhs.writerReLockRealloc(multiple_writers, false, 0);
    // swap sizes by atomically adding their differences
    interlocked_add(writePosAndLock, uint64_t(rhsSize - thisSize) << WRITE_POS_SHIFT);
    interlocked_add(rhs.writePosAndLock, uint64_t(thisSize - rhsSize) << WRITE_POS_SHIFT);
    // swap other
    eastl::swap(allocatedSize, rhs.allocatedSize);
    eastl::swap(ptr, rhs.ptr);
    // unlock
    rhs.writerUnlockRealloc(multiple_writers, false);
    writerUnlockRealloc(multiple_writers, false);
  }

private:
  static constexpr uint64_t WRITE_POS_SHIFT = 16ull;
  static constexpr uint64_t LOCK_BIT_MASK = (1ull << WRITE_POS_SHIFT) - 1ull;
  static constexpr uint64_t REALLOC_LOCKED = 256ull; // push lock adds 1, realloc lock adds this value

  void validateNotLocked(const char *call) const
  {
    (void)call;
    G_ASSERTF((writePosAndLock & LOCK_BIT_MASK) == 0, "parallel write vector is locked during %s", call);
  }

  // locks for push (shared lock), and acquires position to write next allocate_items items
  EASTL_FORCE_INLINE uint32_t writerLockPushAndIncPos(bool multiple_writers, uint64_t allocate_items)
  {
    const uint64_t writePosInc = allocate_items << WRITE_POS_SHIFT;
    if (!multiple_writers)
    {
      const uint64_t i = writePosAndLock;
      writePosAndLock += writePosInc;
      return uint32_t(i >> WRITE_POS_SHIFT);
    }

    if constexpr (!AllowGrowOnWrite)
    {
      // don't lock anything, because realloc is disabled, just increment pos
      const uint64_t i = interlocked_add(writePosAndLock, writePosInc);
      return uint32_t((i >> WRITE_POS_SHIFT) - allocate_items);
    }

    // try acquire push lock and increment write pos
    const uint64_t writePosIncAndPushLock = writePosInc + /* +1 to lock push */ 1ull;
    uint64_t posAndLock = interlocked_add(writePosAndLock, writePosIncAndPushLock);
    while (DAGOR_UNLIKELY((posAndLock & LOCK_BIT_MASK) >= REALLOC_LOCKED))
    {
      interlocked_add(writePosAndLock, uint64_t(-int64_t(writePosIncAndPushLock)));
      while ((posAndLock & LOCK_BIT_MASK) >= REALLOC_LOCKED)
      {
        cpu_yield();
        posAndLock = interlocked_acquire_load(writePosAndLock);
      }
      posAndLock = interlocked_add(writePosAndLock, writePosIncAndPushLock);
    }
    return uint32_t(uint64_t(posAndLock >> WRITE_POS_SHIFT) - allocate_items);
  }

  EASTL_FORCE_INLINE void writerUnlockPush(bool multiple_writers)
  {
    if constexpr (!AllowGrowOnWrite)
      return;
    if (!multiple_writers)
      return;
    interlocked_decrement(writePosAndLock);
  }

  // locks for reallocation (non-shared lock), and acquires write position again
  uint32_t writerReLockRealloc(bool multiple_writers, bool push_locked, uint64_t allocate_items)
  {
    const uint64_t writePosInc = allocate_items << WRITE_POS_SHIFT;
    const uint64_t writePosDec = uint64_t(-int64_t(writePosInc));
    if (!multiple_writers)
      return uint32_t((writePosAndLock >> WRITE_POS_SHIFT) - allocate_items);

    // release push lock & decrement write pos back, and try to take reallocation lock
    const uint64_t reLockAndDec = writePosDec /* lock realloc */ + REALLOC_LOCKED /* unlock push */ - (push_locked ? 1ull : 0ull);
    uint64_t l = interlocked_add(writePosAndLock, reLockAndDec) & LOCK_BIT_MASK;
    while (DAGOR_UNLIKELY(l >= REALLOC_LOCKED * 2ull))
    {
      interlocked_add(writePosAndLock, uint64_t(-int64_t(REALLOC_LOCKED)));
      while (l >= REALLOC_LOCKED)
      {
        cpu_yield();
        l = interlocked_acquire_load(writePosAndLock) & LOCK_BIT_MASK;
      }
      l = interlocked_add(writePosAndLock, REALLOC_LOCKED) & LOCK_BIT_MASK;
    }
    // after realloc lock is acquired, increment write pos again
    interlocked_add(writePosAndLock, writePosInc);

    // now wait, until only this thread owns push lock,
    // other threads will not lock anything, since lock value is >= REALLOC_LOCKED
    while (true)
    {
      const uint64_t posAndLock = interlocked_acquire_load(writePosAndLock);
      G_ASSERT((posAndLock & LOCK_BIT_MASK) >= REALLOC_LOCKED);
      // if push is not locked, we've acquired current write pos
      if (DAGOR_LIKELY((posAndLock & LOCK_BIT_MASK) == REALLOC_LOCKED))
        return uint32_t((posAndLock >> WRITE_POS_SHIFT) - allocate_items);
      cpu_yield();
    }
  }

  void writerUnlockRealloc(bool multiple_writers, bool push_locked)
  {
    if (!multiple_writers)
      return;
    interlocked_add(writePosAndLock, uint64_t(-int64_t(REALLOC_LOCKED - (push_locked ? 1ull : 0ull))));
  }

  void resizeDataImpl(uint32_t item_count, uint32_t new_size)
  {
    if constexpr (dag::is_type_relocatable<T>::value)
    {
      if (allocatedSize > 0)
      {
        if (item_count > new_size)
          eastl::destruct(ptr + new_size, ptr + item_count);
        ptr = static_cast<T *>(midmem->realloc(ptr, sizeof(T) * new_size));
        allocatedSize = new_size;
        return;
      }
    }
    T *newData = new_size > 0 ? static_cast<T *>(midmem->alloc(sizeof(T) * new_size)) : nullptr;
    if (allocatedSize > 0)
    {
      G_ASSERT(ptr);
      if (new_size > 0)
        eastl::uninitialized_move(ptr, ptr + eastl::min<uint32_t>(item_count, new_size), newData);
      eastl::destruct(ptr, ptr + item_count);
      midmem->free(ptr);
    }
    ptr = newData;
    allocatedSize = new_size;
  }

  // upper bits are used as write pos, lower bits - as rw spin lock
  uint64_t writePosAndLock = 0;

  T *ptr = nullptr;
  uint32_t allocatedSize = 0;
};
