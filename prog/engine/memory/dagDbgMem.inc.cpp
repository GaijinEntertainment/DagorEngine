//-- to be included from dagmem.cpp --
//--  cannot be compiled separately --
#include <debug/dag_log.h>
#include <stdio.h>
#include <osApiWrappers/dag_fastStackCapture.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <generic/dag_sort.h>
#include <generic/dag_staticTab.h>
#include <util/dag_stlqsort.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <stdio.h>
#include <perfMon/dag_daProfileMemory.h>
#include <osApiWrappers/dag_ttas_spinlock.h>

static thread_local bool allow_fill_stack = true;

static DagStdMemoryAllocator *sourcemem = NULL;

#if (MEM_DEBUGALLOC >= 2)
#define MEM_VERBOSE_PRINT(...) out_debug_str_fmt(__VA_ARGS__)
#else
#define MEM_VERBOSE_PRINT(...)
#endif

// virtually all of allocators on the planet do have at least some headerin each allocated chunk
// most of overhead is 32, in rare cases it is 16
// so, we at least allocate 16
static constexpr uint32_t est_overhead = 16;
#if _TARGET_64BIT
static bool use_fast_stackhlp = true;
#else
static constexpr bool use_fast_stackhlp = false;
#endif
static thread_local da_profiler::profile_mem_data_t forced_memory_stack = da_profiler::invalid_memory_profile;

class DagDebugMemAllocator final : public IMemAlloc
{
  struct DebugChunk
  {
    da_profiler::profile_mem_data_t profile = da_profiler::invalid_memory_profile;

    static size_t totalSize(size_t sz) { return sizeof(DebugChunk) + sz; }
  };

public:
  static __forceinline da_profiler::profile_mem_data_t get_profile_stack(size_t sz)
  {
    void *localStack[64];
    const int stackLen = !use_fast_stackhlp || ::is_debugger_present() ? stackhlp_fill_stack(localStack, countof(localStack), 0)
                                                                       : fast_stackhlp_fill_stack_rbp(localStack, countof(localStack));
    const int skip = 1;
    const unsigned len = max<int>(stackLen - skip, 0);
    return da_profiler::profile_allocation(sz + est_overhead - sizeof(DebugChunk), localStack + skip, len);
  }

private:
  static __forceinline void *initChunk(void *ptr)
  {
    const size_t sz = sourcemem->getSize(ptr);
    G_FAST_ASSERT(sz >= sizeof(DebugChunk));
    da_profiler::profile_mem_data_t prof = da_profiler::invalid_memory_profile;
    if (allow_fill_stack)
    {
      if (forced_memory_stack)
        prof = da_profiler::profile_allocation(sz, forced_memory_stack);

      if (!prof)
        prof = get_profile_stack(sz);
    }
    getChunk(ptr, sz)->profile = prof;

    return ptr;
  }
  static inline void termChunk(void *p)
  {
    const size_t sz = sourcemem->getSize(p);
    da_profiler::profile_deallocation(sz + est_overhead - sizeof(DebugChunk), getChunk(p, sz)->profile);
  }

  static DebugChunk *getChunk(void *ptr, size_t sz)
  {
    G_FAST_ASSERT(sz >= sizeof(DebugChunk));
    return (DebugChunk *)((char *)ptr + sz - sizeof(DebugChunk));
  }
  static DebugChunk *getChunk(void *ptr) { return getChunk(ptr, sourcemem->getSize(ptr)); }

public:
  DagDebugMemAllocator() = default;
  DagDebugMemAllocator(const char * /*pool_name*/) : DagDebugMemAllocator() {}
  ~DagDebugMemAllocator() {}

  // IMemAlloc interface implementation
  virtual void destroy() { delete this; }
  virtual bool isEmpty() { return false; }
  virtual void *alloc(size_t sz)
  {
    MEM_VERBOSE_PRINT("alloc %llu", sz);

    return initChunk(sourcemem->alloc(DebugChunk::totalSize(sz)));
  }
  void *tryAlloc(size_t sz) override
  {
    MEM_VERBOSE_PRINT("tryAlloc %llu", sz);

    void *ptr = sourcemem->tryAlloc(DebugChunk::totalSize(sz));
    return ptr ? initChunk(ptr) : nullptr;
  }
  void *allocAligned(size_t sz, size_t alignment) override
  {
    MEM_VERBOSE_PRINT("allocAligned %llu, a=%d", sz, alignment);
    return initChunk(sourcemem->allocAligned(DebugChunk::totalSize(sz), alignment));
  }
  void free(void *p) override
  {
    MEM_VERBOSE_PRINT("free %p", p);
    if (!p)
      return;
    termChunk(p);
    sourcemem->free(p);
  }
  void freeAligned(void *p) override
  {
    MEM_VERBOSE_PRINT("freeAligned %p", p);
    if (!p)
      return;
    termChunk(p);
    sourcemem->freeAligned(p);
  }
  size_t getSize(void *p) override
  {
    MEM_VERBOSE_PRINT("getSize %p", p);

    return p ? sourcemem->getSize(p) - sizeof(DebugChunk) : 0;
  }
  bool resizeInplace(void *p, size_t sz) override
  {
    if (!p)
      return (sz == 0);
    MEM_VERBOSE_PRINT("resizeInplace %p, %llu", p, sz);
    const size_t oldSize = sourcemem->getSize(p);
    const size_t reqSz = DebugChunk::totalSize(sz);
    if (oldSize >= reqSz)
      return p;
    DebugChunk copy = *getChunk(p, oldSize);
    if (sourcemem->resizeInplace(p, reqSz))
    {
      const size_t newSize = sourcemem->getSize(p);
      *getChunk(p, newSize) = copy;
      da_profiler::profile_reallocation(oldSize, newSize, copy.profile);
      return true;
    }
    return false;
  }
  void *realloc(void *p, size_t sz) override
  {
    if (!p)
      return DagDebugMemAllocator::alloc(sz);
    MEM_VERBOSE_PRINT("realloc %p, %llu", p, sz);
    if (!sz)
    {
      DagDebugMemAllocator::free(p);
      return NULL;
    }

    const size_t oldSize = sourcemem->getSize(p);
    const size_t reqSz = DebugChunk::totalSize(sz);
    if (oldSize >= reqSz)
      return p;
    DebugChunk copy = *getChunk(p, oldSize);
    p = sourcemem->realloc(p, reqSz);
    const size_t newSize = sourcemem->getSize(p);
    *getChunk(p, newSize) = copy;
    da_profiler::profile_reallocation(oldSize, newSize, copy.profile);
    return p;
  }

  void memFreeAnywhere(void *p)
  {
    MEM_VERBOSE_PRINT("free_any %p", p);

    DagDebugMemAllocator::free(p);
  }

  static const char *getPtrAllocCallStack(void *p)
  {
    auto profile = getChunk(p)->profile;
    size_t allocations, allocated;
    void *callstack[64];
    const uint32_t stackLen =
      profile != da_profiler::invalid_memory_profile
        ? da_profiler::profile_get_entry(profile, (uintptr_t *)callstack, countof(callstack), allocations, allocated)
        : 0;
    static char stk[8192];
    return stackLen ? stackhlp_get_call_stack(stk, sizeof(stk), callstack, stackLen) : "empty-call-stack"; // -V614
  }

protected:
}; // class DagDebugMemAllocator


da_profiler::profile_mem_data_t DagDbgMem::set_forced_profile_callstack()
{
  auto prev = forced_memory_stack;
  forced_memory_stack = DagDebugMemAllocator::get_profile_stack(1);
  return prev;
}

void DagDbgMem::unset_forced_profile_callstack(da_profiler::profile_mem_data_t prev_handle)
{
  if (forced_memory_stack != da_profiler::invalid_memory_profile)
    da_profiler::profile_deallocation(1, forced_memory_stack);
  forced_memory_stack = prev_handle;
}

void DagDbgMem::set_allow_fill_stack(bool d) { allow_fill_stack = d; }


//
// public interface implementation
//
const char *DagDbgMem::get_ptr_allocation_call_stack(void *ptr) { return DagDebugMemAllocator::getPtrAllocCallStack(ptr); }

bool DagDbgMem::enable_stack_fill(bool enable)
{
  bool res = allow_fill_stack;
  allow_fill_stack = enable;
  return res;
}
