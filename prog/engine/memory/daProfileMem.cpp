// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <perfMon/dag_daProfileMemory.h>
#include <dag/dag_vectorMap.h>
#include <osApiWrappers/dag_ttas_spinlock.h>
#include <osApiWrappers/dag_rwSpinLock.h>
#include <util/dag_hashedKeyMap.h>
#include <util/dag_stack_compress.h>
#include <osApiWrappers/dag_stackHlp.h> // for bp()
#include <debug/dag_debug.h>

namespace da_profiler
{

struct unprofiled_allocator
{
  unprofiled_allocator(const char * = NULL) {}
  unprofiled_allocator(const unprofiled_allocator &) = default;
  unprofiled_allocator(const unprofiled_allocator &, const char *) {}
  unprofiled_allocator &operator=(const unprofiled_allocator &) = default;
  static void *allocate(size_t n, int = 0) { return unprofiled_mem_alloc(n); }
  static void *allocate(size_t n, size_t, size_t, int = 0) { return unprofiled_mem_alloc(n); }
  static void deallocate(void *b, size_t) { return unprofiled_mem_free(b); }
  const char *get_name() const { return "unprofiled_allocator"; }
  void set_name(const char *) {}
};

struct CallStack
{
  uint32_t allocatedSize = 0, allocations = 0;
};

typedef uint64_t mem_hash_t;

template <class Ptr>
static inline bool atomic_capture_fetch_or(Ptr **slot, Ptr *&capture)
{
  uintptr_t old = __atomic_fetch_or((uintptr_t *)slot, 1, __ATOMIC_ACQ_REL);
  if (old & 1)
    return false;
  // We successfully set the bit -> we own it
  capture = (Ptr *)(old & ~(uintptr_t)1);
  return true;
}

static CallStack *atomic_capture_ownership_safe_spin(CallStack **callStack)
{
  CallStack *capturedStack;
  // however, we never expect any contention in this code path
  // in fact, only two legal ways it is possible:
  // * dump memmap while freeing/reallocing memory
  // * two threads are (re)allocing/freeing memory allocated with same call stack
  // latter option is the most often in real cases, but should still be rather rare
  spin_wait_no_profile([&]() { return !atomic_capture_fetch_or(callStack, capturedStack); });
  return capturedStack;
}

// Simple release: just mark as not owned (keep same pointer)
template <class Ptr>
static inline void atomic_release_keep(Ptr **atomic_slot)
{
  // Clear the owned bit (LSB)
  __atomic_fetch_and((uintptr_t *)atomic_slot, ~(uintptr_t)1, __ATOMIC_RELEASE);
}

// Release and replace with new pointer (or nullptr)
template <class Ptr>
static inline void atomic_release_replace(Ptr **atomic_slot, const Ptr *new_ptr)
{
  __atomic_store_n((uintptr_t *)atomic_slot, uintptr_t(new_ptr), __ATOMIC_RELEASE);
}

static __forceinline void decompress_call_stack(uintptr_t *cStack, uint8_t stack_len, const uint8_t *compressed_stack)
{
  compress_call_stack::decompress(cStack, stack_len, compressed_stack, uintptr_t(stackhlp_get_bp()));
}
// potentional perf optimization:
// we can add partitioning based on thread if

// beware! we never allocate this structure (constructor and destructor are never called!)
// this is to prevent "order-of-constructor" pitfall, as some of static constructors could and would allocate memory
// so, in order to make this work, we rely on static data to be initialized with 0 by default
// everything in this GlobalMemHashMap has to be 0-initializable and should not require destruction
struct GlobalMemHashMap
{
  uint32_t allocatedEntries = 0;
  static constexpr uint32_t max_stack_len = 64, min_hash_map_capacity = 128;

  // OSReadWriteLock is probably better, but would require initialization before everything else
  // spinlock rwlock is initalized with zero, so nothing needed!
  SpinLockReadWriteLock rwLock;
  volatile int isDumping = 0;

  typedef CallStack *callstack_t;
  typedef HashedKeyMap<mem_hash_t, callstack_t, 0, oa_hashmap_util::NoHash<mem_hash_t>, unprofiled_allocator> hashmap_t;
  hashmap_t hashes;
  void changeStackTrace(mem_hash_t hash, uint32_t old_sz, uint32_t new_sz)
  {
    if (!hash || old_sz == new_sz)
      return;
    rwLock.lockRead();
    // if hashes.size() != 0 should not be needed in normal situaion, it is UB case (realloc of free mem)
    callstack_t *it;
    if (hashes.size() && (it = hashes.findVal(hash)))
    {
      CallStack *capturedStack = atomic_capture_ownership_safe_spin(it);
      if (capturedStack)
        capturedStack->allocatedSize += int(new_sz - old_sz);
      atomic_release_keep(it);
    }
    rwLock.unlockRead();
  }

  void shrink_to_fit(uint32_t reserve_to)
  {
    hashmap_t nextHashMap;
    // do memory allocation before locking spinlock
    if (reserve_to)
      nextHashMap.reserve(reserve_to);

    rwLock.lockWrite();
    const auto allocated = interlocked_acquire_load(allocatedEntries);

    // ensure that capacity is enough, so we don't allocate memory under writeLock. Life is hard even without it
    if (!allocated || hashmap_t::get_capacity(allocated) <= nextHashMap.bucket_count())
    {
      hashes.iterate([&](const auto hash, const auto cs) {
        if (cs)
          nextHashMap.emplace_new(hash, cs);
      });
      eastl::swap(nextHashMap, hashes);
    }
    rwLock.unlockWrite();
  }

  void removeStackTrace(mem_hash_t hash, uint32_t allocation_size)
  {
    if (!hash)
      return;
    size_t hashmapSize;
    rwLock.lockRead();
    hashmapSize = hashes.size();
    CallStack *capturedStack = nullptr;
    // if hashes.size() != 0 should not be needed in normal situaion, it is double-free case
    callstack_t *it;
    if (hashes.size() && (it = hashes.findVal(hash)))
    {
      capturedStack = atomic_capture_ownership_safe_spin(it);
      if (capturedStack)
      {
        capturedStack->allocatedSize -= allocation_size;
        if (--capturedStack->allocations == 0)
          atomic_release_replace(it, (CallStack *)nullptr);
        else
          capturedStack = nullptr;
      }
      if (!capturedStack)
        atomic_release_keep(it);
    }
    rwLock.unlockRead();

    if (capturedStack)
    {
      unprofiled_allocator::deallocate(capturedStack, 0);
      auto entries = interlocked_decrement(allocatedEntries);
      if ((entries << 2) <= hashmapSize) // 25% or less is used
        shrink_to_fit(entries << 1);
    }
  }

  static inline callstack_t create_callstack_data(const void *const *stack, uint8_t stack_size, uint32_t alloc_size)
  {
#if _TARGET_PC_WIN
    // first two call stacks are always pointing to kernel call functions
    stack_size = stack_size > 2 ? stack_size - 2 : 0;
#endif
    uint8_t compressedCS[max_stack_len * compress_call_stack::bytes_per_frame_bound];
    uint32_t compressedSize =
      compress_call_stack::compress(compressedCS, (const uintptr_t *)stack, stack_size, uintptr_t(stackhlp_get_bp()));
    G_FAST_ASSERT(compressedSize <= countof(compressedCS));
    CallStack *cs = (CallStack *)unprofiled_allocator::allocate(compressedSize + 1 + sizeof(CallStack));
    cs->allocatedSize = alloc_size;
    cs->allocations = 1;

    uint8_t *callstack = (uint8_t *)(cs + 1);
    memcpy(callstack, &stack_size, sizeof(uint8_t));
    memcpy(callstack + 1, compressedCS, compressedSize);
    return cs;
  }

  __forceinline mem_hash_t addStackTrace(mem_hash_t hash, uint32_t allocation_size)
  {
    rwLock.lockRead();

    callstack_t *it;
    if (hashes.size() && (it = hashes.findVal(hash)))
    {
      CallStack *capturedStack = atomic_capture_ownership_safe_spin(it);

      if (capturedStack)
      {
        capturedStack->allocatedSize += allocation_size;
        capturedStack->allocations++;
        atomic_release_keep(it);
      }
      else
        hash = 0;
      rwLock.unlockRead();
      return hash;
    }
    rwLock.unlockRead();
    return 0;
  }
  mem_hash_t addStackTrace(const void *const *stack, unsigned stack_size_, uint32_t allocation_size)
  {
    const uint32_t stack_size = eastl::min<uint32_t>(stack_size_, max_stack_len);
    mem_hash_t hash = wyhash(stack, stack_size * sizeof(void *), 1);
    if (!hash)
      hash |= (1ULL << 63UL);

    rwLock.lockRead();

    callstack_t *it;
    if (hashes.size() && (it = hashes.findVal(hash)))
    {
      CallStack *capturedStack = atomic_capture_ownership_safe_spin(it);

      if (capturedStack)
      {
        capturedStack->allocatedSize += allocation_size;
        capturedStack->allocations++;
        atomic_release_keep(it);
      }
      else
      {
        // fixme: better unlock lock here and relock again with created callstack
        interlocked_increment(allocatedEntries);
        atomic_release_replace(it, create_callstack_data(stack, stack_size, allocation_size));
      }
      rwLock.unlockRead();
      return hash;
    }
    const size_t used = hashes.size(), capacity = hashes.bucket_count(), nextCapacity = hashes.get_capacity(used + 1);
    rwLock.unlockRead();

    auto created = create_callstack_data(stack, stack_size, allocation_size);

    hashmap_t nextHashMap;
    if (nextCapacity > capacity)
    {
      // to decrease amount of allocations under writeLock
      nextHashMap.reserve(min_hash_map_capacity > used ? min_hash_map_capacity : used + 1);
    }

    rwLock.lockWrite();
    if (nextCapacity > capacity && nextHashMap.bucket_count() > hashes.bucket_count())
    {
      if (hashes.size() != 0)
        hashes.iterate([&nextHashMap](const auto hash, const auto cs) { nextHashMap.emplace_new(hash, cs); });
      eastl::swap(hashes, nextHashMap);
      rwLock.unlockWrite();
      nextHashMap = hashmap_t();
      rwLock.lockWrite();
    }
    auto ret = hashes.emplace_if_missing(hash);

    if (!ret.second && *ret.first)
    {
      // was inserted between locks
      (*ret.first)->allocatedSize += allocation_size;
      (*ret.first)->allocations++;
    }
    else
    {
      (*ret.first) = created;
      created = nullptr;
      interlocked_increment(allocatedEntries);
    }
    rwLock.unlockWrite();
    if (created)
      unprofiled_allocator::deallocate(created, 0); // fixme: size
    return hash;
  }

  void memory_allocations(size_t &allocated_chunks, size_t &unique_entries)
  {
    rwLock.lockRead();
    unique_entries = interlocked_acquire_load(allocatedEntries);
    allocated_chunks = unique_entries + (hashes.bucket_count() > 0 ? 2 : 0);
    rwLock.unlockRead();
  }

  uint32_t getEntry(mem_hash_t hash, uintptr_t *stack, uint32_t max_stack, size_t &allocation_size, size_t &allocations)
  {
    allocation_size = allocations = 0;
    if (!hash)
      return 0;
    uint8_t cStackLen = 0;
    callstack_t *it = nullptr;
    rwLock.lockRead();
    if (hashes.size() && (it = hashes.findVal(hash)))
    {
      if (auto capturedStack = atomic_capture_ownership_safe_spin(it))
      {
        allocation_size = capturedStack->allocatedSize;
        allocations = capturedStack->allocations;
        auto cst = (const uint8_t *)(capturedStack + 1);
        decompress_call_stack(stack, cStackLen = eastl::min<uint32_t>(max_stack, *cst), cst + 1);
      }
      atomic_release_keep(it);
    }
    rwLock.unlockRead();
    return cStackLen;
  }
  // relatively slow function, will iterate all entries to gather allocated memory
  void memory_allocated(size_t &profiler_chunks, size_t &profiled_entries, size_t &profiled_memory, size_t &max_entry,
    size_t &profiled_allocations)
  {
    size_t allocatedSize = 0, allocations = 0, callstacks = 0, maxEntry = 0;
    max_entry = 0;

    rwLock.lockRead();
    hashes.iterate([&](mem_hash_t, auto &cs) {
      CallStack *capturedStack = atomic_capture_ownership_safe_spin(&cs);
      if (capturedStack)
      {
        allocations += capturedStack->allocations;
        auto asz = capturedStack->allocatedSize;
        allocatedSize += asz;
        maxEntry = maxEntry < asz ? asz : maxEntry;
        callstacks++;
      }
      atomic_release_keep(&cs);
    });
    profiler_chunks = callstacks + size_t(hashes.bucket_count() > 0);
    rwLock.unlockRead();
    profiled_allocations = allocations;
    profiled_entries = callstacks; // should be exactly same as interlocked_release_load(allocatedEntries)
    profiled_memory = allocatedSize;
    max_entry = maxEntry;
  }

  void dump(MemDumpCb &cb)
  {
    size_t from = 0, end = 0;
    size_t allocatedSize = 0, allocations = 0, callstacks = 0, callstackFrames = 0;
    mem_hash_t lastUniqueId = 0;
    do
    {
      mem_hash_t unique_id = 0;
      size_t cAllocations = 0, cAllocatedSize = 0;
      uint32_t cStackLen = 0;
      uintptr_t cStack[max_stack_len];

      rwLock.lockRead();

      end = hashes.bucket_count();
      from = hashes.iterate_until(
               [&](const mem_hash_t &h, auto &cs) {
                 if (h != lastUniqueId)
                 {
                   CallStack *capturedStack = atomic_capture_ownership_safe_spin(&cs);
                   if (capturedStack)
                   {
                     unique_id = h;
                     cAllocations = capturedStack->allocations;
                     cAllocatedSize = capturedStack->allocatedSize;
                     auto cst = (const uint8_t *)(capturedStack + 1);
                     decompress_call_stack(cStack, cStackLen = eastl::min<uint32_t>(max_stack_len, *cst), cst + 1);
                   }
                   atomic_release_keep(&cs);
                   return capturedStack != nullptr;
                 }
                 return false;
               },
               from) +
             1;

      rwLock.unlockRead();
      if (from > end) // we found nothing
        break;
      allocatedSize += cAllocatedSize;
      allocations += cAllocations;
      callstackFrames += cStackLen;
      callstacks++;
      if (ttas_spinlock_trylock(isDumping))
      {
        cb.dump(lastUniqueId = unique_id, cStack, cStackLen, cAllocatedSize, cAllocations); // -V614
        ttas_spinlock_unlock(isDumping);
      }
    } while (from < end);
  }
};

// everything in this GlobalMemHashMap has to be 0-initializable and should not require destruction
// so we can use hash_map_container initialized with zeroes
alignas(GlobalMemHashMap) static char hash_map_container[sizeof(GlobalMemHashMap)];

GlobalMemHashMap &get_hash_map() // we can add partitioning based on thread if
{
  return (GlobalMemHashMap &)hash_map_container;
}

void profile_deallocation(size_t n, profile_mem_data_t node) { get_hash_map().removeStackTrace((mem_hash_t)node, n); }

void profile_reallocation(size_t old_sz, size_t new_sz, profile_mem_data_t node)
{
  get_hash_map().changeStackTrace((mem_hash_t)node, old_sz, new_sz);
}

profile_mem_data_t profile_allocation(size_t n, const void *const *stack, unsigned stack_size)
{
  return (profile_mem_data_t)get_hash_map().addStackTrace(stack, stack_size, n);
}

profile_mem_data_t profile_allocation(size_t n, profile_mem_data_t p)
{
  return (profile_mem_data_t)get_hash_map().addStackTrace(p, n);
}

void dump_memory_map(MemDumpCb &cb) { get_hash_map().dump(cb); }

// doesn't return amount of memory allocated by profiler, but is very fast function
void profile_memory_profiler_allocations(size_t &allocated_chunks, size_t &unique_entries)
{
  get_hash_map().memory_allocations(allocated_chunks, unique_entries);
}

// relatively slow function, will iterate all entries to gather allocated memory
void profile_memory_allocated(size_t &profiler_chunks, size_t &profiled_entries, size_t &profiled_memory, size_t &max_entry)
{
  size_t allocations;
  get_hash_map().memory_allocated(profiler_chunks, profiled_entries, profiled_memory, max_entry, allocations);
}

uint32_t profile_get_entry(profile_mem_data_t hash, uintptr_t *stack, uint32_t max_stack_size, size_t &allocations, size_t &allocated)
{
  return get_hash_map().getEntry(hash, stack, max_stack_size, allocations, allocated);
}

}; // namespace da_profiler

#define EXPORT_PULL dll_pull_memory_profile
#include <supp/exportPull.h>
