// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dagmem_base.h"
#include <memory/dag_mem.h>
#include <debug/dag_fatal.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <util/dag_string.h>
#include <util/limBufWriter.h>
#include <memory/dag_dbgMem.h>
#include <memory/dag_memStat.h>
#include <3d/3dMemStat.h>
#include <errno.h>
#include <startup/dag_globalSettings.h>

#include <supp/_platform.h>
#if _TARGET_PC_WIN
#include <windows.h>
#include <malloc.h>
#endif

#define HAS_GET_SYSMEM_IN_USE     1
#define NO_GET_SYSMEM_PTRS_IN_USE 1

#include <perfMon/dag_daProfileMemory.h>
namespace da_profiler
{
void *unprofiled_mem_alloc(size_t n) { return dagmem_malloc_base(n); }
void unprofiled_mem_free(void *p) { return dagmem_free_base(p); }
} // namespace da_profiler

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_LINUX
#include <malloc.h>
// Note: `mallinfo` is really slow (>1ms) under some conditions, probably many freed/allocated
// small blocks, so  don't use it as we expect fast operation for this method.
// To consider: use something like RLIMIT_RSS instead?
#undef HAS_GET_SYSMEM_IN_USE
static size_t get_sysmem_in_use()
{
#if __GLIBC_PREREQ(2, 33)
  struct mallinfo2 mi = mallinfo2();
#else
  struct mallinfo mi = mallinfo();
#endif
  return mi.uordblks + mi.hblkhd;
}
#elif _TARGET_ANDROID
static size_t get_sysmem_in_use()
{
  // mallinfo on android is already 64-bit
  // uordblks includes hblkhd value on android
  struct mallinfo mi = mallinfo();
  return mi.uordblks;
}
#elif _TARGET_APPLE
#include <malloc/malloc.h>
static size_t get_sysmem_in_use() { return mstats().bytes_used; }
static size_t get_sysmem_ptrs_in_use() { return mstats().chunks_used; }
#undef NO_GET_SYSMEM_PTRS_IN_USE
#elif _TARGET_PC_WIN | _TARGET_XBOX
#include <Psapi.h>
static size_t get_sysmem_in_use()
{
  PROCESS_MEMORY_COUNTERS pmc = {sizeof(PROCESS_MEMORY_COUNTERS)};
  pmc.WorkingSetSize = 0;
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
    return pmc.WorkingSetSize;
  return 0;
}
#else
#pragma message("memory manager is inaccurate with USE_RTL_ALLOC for this target")
static size_t get_sysmem_in_use() { return 0; }
#endif

#if NO_GET_SYSMEM_PTRS_IN_USE
static unsigned get_sysmem_ptrs_in_use() { return 0; }
#endif

// memory tracker settings (work buffer is preallocated)
#define MTR_MAX_BLOCKS         (4 << 20) // max tracked blocks x 48B
#define MTR_MAX_BT_ELEMS       (600000)  // approx number of backtrace elements x 8B
#define MTR_COLLECT_BACKTRACE  1         // allow to collect backtraces (disable if slow)
#define MTR_DUMP_SIZE_PERCENTS 30        // workbuffer size (% of total memory needed to write all data)
#define MTR_PACK_DATA          0         // pack data when writing files [TODO:]


// MEMUSAGE_COUNT: 1=summary (performed always) 2=chunk_size summary 0=minimal stats

#define PROTECT_MEMORY_MANAGER 0 // breaks execution of non-protected executables
#if DAGOR_DBGLEVEL > 0 && _TARGET_STATIC_LIB && MEM_DEBUGALLOC >= 0 && !DAGOR_ADDRESS_SANITIZER
#define MEMUSAGE_COUNT        1
#define FILL_ALLOCATED_MEMORY 1
#define FILL_DELETED_MEMORY   1
#else
#if MEM_DEBUGALLOC < 0 || DAGOR_DBGLEVEL == 0
#define MEMUSAGE_COUNT 0
#else
#define MEMUSAGE_COUNT 1
#endif
#define FILL_ALLOCATED_MEMORY 0
#define FILL_DELETED_MEMORY   0
#endif
// #define MEMUSAGE_BIG_CHUNCK_THRES (12<<20)

#if (defined(_M_IX86_FP) && _M_IX86_FP == 0) || MEM_DEBUGALLOC > 0
#define MIN4K(x) min<unsigned>(x, 65536u)
#else
#define MIN4K(x) min<unsigned>(x, 4096u)
#endif

#ifndef DAGMEM_GET_ALLOCATED_USABLE_SIZE
#define DAGMEM_GET_ALLOCATED_USABLE_SIZE(PTR, AL, SZ) dagmem_malloc_usable_size(PTR)
#endif
#ifndef DAGMEM_GET_USABLE_SIZE
#define DAGMEM_GET_USABLE_SIZE(PTR, AL) dagmem_malloc_usable_size(PTR)
#endif

#ifndef DAGMEM_DECL_EFF_PTR_AND_CLRHDR
#define DAGMEM_DECL_EFF_PTR_AND_CLRHDR(VAR, PTR, MAX_AL) void *VAR = PTR;
#define DAGMEM_DECL_EFF_PTR_WITH_SIZE_AND_CLRHDR(VAR, PTR, MAX_AL, OUT_SZ) \
  void *VAR = PTR;                                                         \
  OUT_SZ = DAGMEM_GET_USABLE_SIZE(PTR, MAX_AL)
#endif
#ifndef DAGMEM_FREE_NATIVE
#define DAGMEM_FREE_NATIVE(PTR) dagmem_free_base(PTR)
#endif
#ifndef DAGMEM_FREE_ALIGNED
#define DAGMEM_FREE_ALIGNED(PTR) DAGMEM_FREE_NATIVE(PTR)
#endif

#define FILL_MEM_WITH_PATTERN(p, pattern, sz) memset_dw(p, pattern, MIN4K(sz) / sizeof(int))

#if FILL_ALLOCATED_MEMORY
#define NEW_MEMORY_FILL_PATTERN 0x7ffdcdcd
#define FILL_NEW_MEMORY(P, ASZ) FILL_MEM_WITH_PATTERN(P, NEW_MEMORY_FILL_PATTERN, ASZ)
#else
#define FILL_NEW_MEMORY(P, ASZ) ((void)0)
#endif

#if FILL_DELETED_MEMORY
#define FILL_FREED_MEMORY(P, ASZ) FILL_MEM_WITH_PATTERN(P, 0x7ffddddd, ASZ)
#else
#define FILL_FREED_MEMORY(P, ASZ) ((void)0)
#endif

#define MEASURE_EXPAND_EFF 0
#if MEASURE_EXPAND_EFF
static volatile int cnt_expand_total_16K = 0, cnt_expand_total = 0, cnt_expand_ok_16K = 0, cnt_expand_ok = 0, cnt_expand_bad_p = 0;
static volatile int cnt_expand_r_total_16K = 0, cnt_expand_r_total = 0, cnt_expand_r_ok_16K = 0, cnt_expand_r_ok = 0;
#endif

namespace muc2
{
#if MEMUSAGE_COUNT == 2
static constexpr int MEM_BINS_CNT = 64 /*step 8*/ + 120 /*step 64*/ + 62 /*step 4K*/ + 3 /*step 256K*/ + 15 /*step 1M*/ + 1 /*16M+*/;
static volatile int32_t chunkCnt[MEM_BINS_CNT] = {0}, chunkCntMax[MEM_BINS_CNT] = {0};
static volatile int64_t chunkMemUsed[MEM_BINS_CNT] = {0};

static int getChunkIdx(size_t sz)
{
  if (sz < 512)
    return sz / 8 + 0;
  else if (sz < 8192)
    return (sz - 512) / 64 + 64;
  else if (sz < (256 << 10))
    return (sz - 8192) / (4 << 10) + 64 + 120;
  else if (sz < (1 << 20))
    return (sz - (256 << 10)) / (256 << 10) + 64 + 120 + 62;
  else if (sz < (16 << 20))
    return (sz - (1 << 20)) / (1 << 20) + 64 + 120 + 62 + 3;
  return MEM_BINS_CNT - 1;
}
static void getChunkSz(int idx, int &min, int &max)
{
  if (idx < 64)
  {
    min = idx * 8;
    max = min + 8;
  }
  else if (idx < 64 + 120)
  {
    min = (idx - 64) * 64 + 512;
    max = min + 64;
  }
  else if (idx < 64 + 120 + 62)
  {
    min = (idx - (64 + 120)) * (4 << 10) + 8192;
    max = min + (4 << 10);
  }
  else if (idx < 64 + 120 + 62 + 3)
  {
    min = (idx - (64 + 120 + 62)) * (256 << 10) + (256 << 10);
    max = min + (256 << 10);
  }
  else if (idx < 64 + 120 + 62 + 3 + 15)
  {
    min = (idx - (64 + 120 + 62 + 3)) * (1 << 20) + (1 << 20);
    max = min + (1 << 20);
  }
  else
  {
    min = 16 << 20;
    max = (1 << 30);
  }
}

static void measureAlloc(void *p, ptrdiff_t sz)
{
  int idx = getChunkIdx(sz);
  interlocked_add(chunkMemUsed[idx], sz);
  int32_t cur_ptrs = interlocked_increment(chunkCnt[idx]);
  int32_t max_ptrs = interlocked_acquire_load(chunkCntMax[idx]);
  while (cur_ptrs > max_ptrs)
    if (interlocked_compare_exchange(chunkCntMax[idx], cur_ptrs, max_ptrs) == max_ptrs)
      break;
    else
      max_ptrs = interlocked_acquire_load(chunkCntMax[idx]);
  G_UNUSED(p);
#ifdef MEMUSAGE_BIG_CHUNCK_THRES
  if (sz >= MEMUSAGE_BIG_CHUNCK_THRES)
  {
    char buf[128];
    LimitedBufferWriter lbw(buf, sizeof(buf));
    lbw.aprintf("alloc_big(%d)=%p", sz, p);
    debug_dump_stack(buf);
  }
#endif
}
static void measureFree(void *p, ptrdiff_t sz)
{
  int idx = getChunkIdx(sz);
  interlocked_add(chunkMemUsed[idx], -sz);
  interlocked_decrement(chunkCnt[idx]);
  G_UNUSED(p);
#ifdef MEMUSAGE_BIG_CHUNCK_THRES
  if (sz >= MEMUSAGE_BIG_CHUNCK_THRES)
  {
    char buf[128];
    LimitedBufferWriter lbw(buf, sizeof(buf));
    lbw.aprintf("free_big(%p, %d)", p, sz);
    debug_dump_stack(buf);
  }
#endif
}

static inline int bytes_to_compact_number(int32_t b)
{
  if (b <= 8192)
    return b;
  if (b < (2 << 20))
    return b >> 10;
  if (b < (1 << 30))
    return b >> 20;
  return b >> 30;
}
static inline const char *bytes_to_compact_suffix(int32_t b)
{
  if (b <= 8192)
    return " ";
  if (b < (2 << 20))
    return "K";
  if (b < (1 << 30))
    return "M";
  return "G";
}

static void measurePrintStat()
{
  DEBUG_CTX("per-chunksize memory usage statistics:");
  int64_t total_mem = 0;
  int32_t total_ptrs = 0;
  for (int i = 0; i < sizeof(chunkCnt) / sizeof(chunkCnt[0]); i++)
    if (chunkCnt[i] || chunkCntMax[i] >= 8)
    {
      total_mem += chunkMemUsed[i];
      total_ptrs += chunkCnt[i];
      int min, max;
      getChunkSz(i, min, max);
      debug("  %6d [%6d max]  sz[%4d%s..%4d%s),  total=%6dK", chunkCnt[i], chunkCntMax[i], bytes_to_compact_number(min),
        bytes_to_compact_suffix(min), bytes_to_compact_number(max), bytes_to_compact_suffix(max), chunkMemUsed[i] >> 10);
    }
  debug("%dM total in %d ptrs", int(total_mem >> 20), total_ptrs);
}
#elif MEMUSAGE_COUNT || !(DAGMEM_SHOULD_USE_TOTAL_ALLOC_FROM_CRT && HAS_GET_SYSMEM_IN_USE)
static void measureAlloc(void *, ptrdiff_t) {}
static void measureFree(void *, ptrdiff_t) {}
#endif
} // namespace muc2

#if MEMUSAGE_COUNT
struct MemUsageStats
{
  volatile int64_t malloc_cnt, free_cnt, realloc_cnt;
#if _TARGET_64BIT
  volatile int64_t max_alloc, total_alloc;
#else
  volatile int max_alloc, total_alloc;
#endif
  volatile int ptr_in_use, max_ptr_in_use;

  void copyFrom(const MemUsageStats &_m)
  {
#define COPY_FIELD(X) X = interlocked_acquire_load(_m.X)
    COPY_FIELD(malloc_cnt);
    COPY_FIELD(free_cnt);
    COPY_FIELD(realloc_cnt);
    COPY_FIELD(max_alloc);
    COPY_FIELD(total_alloc);
    COPY_FIELD(ptr_in_use);
    COPY_FIELD(max_ptr_in_use);
#undef COPY_FIELD
  }
  void on_alloc(void *ptr, ptrdiff_t asize)
  {
    if (!ptr)
      return;

    interlocked_increment(malloc_cnt);

    ptrdiff_t cur_mem = interlocked_add(total_alloc, asize);
    ptrdiff_t max_mem = interlocked_acquire_load(max_alloc);
    while (cur_mem > max_mem)
      if (interlocked_compare_exchange(max_alloc, cur_mem, max_mem) == max_mem)
        break;
      else
        max_mem = interlocked_acquire_load(max_alloc);

    int cur_ptr = interlocked_increment(ptr_in_use);
    int max_ptr = interlocked_acquire_load(max_ptr_in_use);
    while (cur_ptr > max_ptr)
      if (interlocked_compare_exchange(max_ptr_in_use, cur_ptr, max_ptr) == max_ptr)
        break;
      else
        max_ptr = interlocked_acquire_load(max_ptr_in_use);
    muc2::measureAlloc(ptr, asize);
  }

  void on_free(void *ptr, ptrdiff_t asize)
  {
    if (!ptr)
      return;

    interlocked_increment(free_cnt);

    interlocked_add(total_alloc, -asize);
    interlocked_decrement(ptr_in_use);
    muc2::measureFree(ptr, asize);
  }
  void on_realloc(bool new_mem)
  {
    interlocked_increment(realloc_cnt);
    if (!new_mem)
      interlocked_decrement(malloc_cnt);
  }
};

static MemUsageStats muc = {0, 0, 0, 0, 0, 0, 0};

int64_t dagor_memory_stat::get_malloc_call_count()
{
  return interlocked_acquire_load(muc.malloc_cnt) + interlocked_acquire_load(muc.free_cnt) + interlocked_acquire_load(muc.realloc_cnt);
}
size_t dagor_memory_stat::get_memchunk_count([[maybe_unused]] bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (unsigned ptrs_in_use = get_sysmem_ptrs_in_use())
      return ptrs_in_use;
#endif
  return interlocked_acquire_load(muc.ptr_in_use);
}
size_t dagor_memory_stat::get_memory_allocated([[maybe_unused]] bool _crt)
{
  size_t total_alloc = interlocked_acquire_load(muc.total_alloc);
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (size_t mem_in_use = get_sysmem_in_use())
      return max(mem_in_use, total_alloc);
#endif
  return total_alloc;
}

#elif DAGMEM_SHOULD_USE_TOTAL_ALLOC_FROM_CRT && HAS_GET_SYSMEM_IN_USE
#define MUC_DECL_ASIZE(VAR, PTR, AL, SZ)
#define MUC_DECL_MSIZE(VAR, PTR, AL)
#define MUC_DECL_MSIZE_AND_EFF_PTR(VAR_SZ, VAR_PTR, PTR, MAX_AL) DAGMEM_DECL_EFF_PTR_AND_CLRHDR(VAR_PTR, PTR, MAX_AL)

#define MUC_ON_MEM_ALLOC(M, PTR, ASZ)
#define MUC_ON_MEM_REALLOC(M, NEW_MEM)
#define MUC_ON_MEM_FREE(M, PTR, ASZ)

int64_t dagor_memory_stat::get_malloc_call_count() { return 0; }

size_t dagor_memory_stat::get_memchunk_count([[maybe_unused]] bool _crt)
{
  if (_crt)
    if (unsigned ptrs_in_use = get_sysmem_ptrs_in_use())
      return ptrs_in_use;
  return 0;
}
size_t dagor_memory_stat::get_memory_allocated([[maybe_unused]] bool _crt) { return get_sysmem_in_use(); }

#else

struct MemUsageStats0
{
#if _TARGET_64BIT
  volatile int64_t total_alloc_x16, ptr_in_use;
#else
  volatile int total_alloc_x16, ptr_in_use;
#endif

  void on_alloc(void *ptr, ptrdiff_t _asize)
  {
    if (!ptr)
      return;

    int asize = int((_asize + 15) / 16);
    interlocked_add(total_alloc_x16, asize);
    interlocked_increment(ptr_in_use);
    muc2::measureAlloc(ptr, asize);
  }
  void on_free(void *ptr, ptrdiff_t _asize)
  {
    if (!ptr)
      return;

    int asize = int((_asize + 15) / 16);
    interlocked_add(total_alloc_x16, -asize);
    interlocked_decrement(ptr_in_use);
    muc2::measureFree(ptr, asize);
  }
  void on_realloc(bool) {}
};

static MemUsageStats0 muc = {0, 0};

int64_t dagor_memory_stat::get_malloc_call_count() { return 0; }

size_t dagor_memory_stat::get_memchunk_count([[maybe_unused]] bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (unsigned ptrs_in_use = get_sysmem_ptrs_in_use())
      return ptrs_in_use;
#endif
  return interlocked_acquire_load(muc.ptr_in_use);
}
size_t dagor_memory_stat::get_memory_allocated([[maybe_unused]] bool _crt)
{
  size_t total_alloc = 16 * (size_t(interlocked_acquire_load(muc.total_alloc_x16)));
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (size_t mem_in_use = get_sysmem_in_use())
      return max(mem_in_use, total_alloc);
#endif
  return total_alloc;
}

#endif

int dagor_memory_stat::get_memory_allocated_kb(bool with_crt) { return int(dagor_memory_stat::get_memory_allocated(with_crt) >> 10); }

#ifndef MUC_ON_MEM_ALLOC
#define MUC_DECL_ASIZE(VAR, PTR, AL, SZ) size_t VAR = DAGMEM_GET_ALLOCATED_USABLE_SIZE(PTR, AL, SZ);
#define MUC_DECL_MSIZE(VAR, PTR, AL)     size_t VAR = DAGMEM_GET_USABLE_SIZE(PTR, AL)
#define MUC_DECL_MSIZE_AND_EFF_PTR(VAR_SZ, VAR_PTR, PTR, MAX_AL) \
  size_t VAR_SZ = 0;                                             \
  DAGMEM_DECL_EFF_PTR_WITH_SIZE_AND_CLRHDR(VAR_PTR, PTR, MAX_AL, VAR_SZ)

#define MUC_ON_MEM_ALLOC(M, PTR, ASZ)  M.on_alloc(PTR, ASZ)
#define MUC_ON_MEM_REALLOC(M, NEW_MEM) M.on_realloc(NEW_MEM)
#define MUC_ON_MEM_FREE(M, PTR, ASZ)   M.on_free(PTR, ASZ)
#endif

#if DAGOR_ADDRESS_SANITIZER
bool g_destroy_init_on_demand_in_dtor = true;
#else
bool g_destroy_init_on_demand_in_dtor = false;
#endif

bool (*dgs_on_out_of_memory)(size_t sz) = NULL;

#define LAST_RESORT_ON_OOM(SZ, ALLOC_EXPR)                 \
  while (dgs_on_out_of_memory && dgs_on_out_of_memory(SZ)) \
    if (void *retry_ptr = ALLOC_EXPR)                      \
      return retry_ptr;

static int mem_mgr_inited = 0;

void (*d3d_dump_memory_stat)(LimitedBufferWriter &wr) = NULL;

#if !USE_DLMALLOC
extern "C" size_t get_memory_committed() { return 0; }
extern "C" size_t get_memory_committed_max() { return 0; }
#if _TARGET_PC_WIN
extern "C" size_t mmap_allocated_bytes, mmap_allocated_chunks;
size_t mmap_allocated_bytes = 0, mmap_allocated_chunks = 0;
#endif
#endif


inline void *dagmem_malloc(size_t bytes)
{
  void *mem = dagmem_malloc_base(bytes);

  MUC_DECL_ASIZE(asz, mem, 16, bytes);
  MUC_ON_MEM_ALLOC(muc, mem, asz);
  FILL_NEW_MEMORY(mem, asz);
  return mem;
}

inline void *dagmem_calloc(size_t const count, size_t const size)
{
  void *mem = dagmem_calloc_base(count, size);

  MUC_DECL_ASIZE(asz, mem, MALLOC_MIN_ALIGNMENT, count * size);
  MUC_ON_MEM_ALLOC(muc, mem, asz);
  return mem;
}

inline void *dagmem_memalign(size_t alignment, size_t bytes)
{
  void *mem = alignment ? dagmem_memalign_base(alignment, bytes) : dagmem_valloc_base(bytes);

  MUC_DECL_ASIZE(asz, mem, alignment ? alignment : 4096, bytes);
  MUC_ON_MEM_ALLOC(muc, mem, asz);
#if FILL_ALLOCATED_MEMORY
  FILL_MEM_WITH_PATTERN(mem, NEW_MEMORY_FILL_PATTERN, asz);
#endif
  return mem;
}

inline void dagmem_free(void *mem)
{
  MUC_DECL_MSIZE_AND_EFF_PTR(msz, native_ptr, mem, 16);
  FILL_FREED_MEMORY(mem, msz);
  MUC_ON_MEM_FREE(muc, mem, msz);

  DAGMEM_FREE_NATIVE(native_ptr);
}
inline void dagmem_free_aligned(void *mem)
{
  MUC_DECL_MSIZE_AND_EFF_PTR(msz, native_ptr, mem, 4096);
  FILL_FREED_MEMORY(mem, msz);
  MUC_ON_MEM_FREE(muc, mem, msz);

  DAGMEM_FREE_ALIGNED(native_ptr);
}

inline void *dagmem_realloc(void *mem, size_t bytes)
{
  MUC_DECL_MSIZE(prevMemSize, mem, 16);
  MUC_ON_MEM_FREE(muc, mem, prevMemSize);

  void *new_mem = dagmem_realloc_base(mem, bytes);

  MUC_DECL_ASIZE(asz, new_mem, 16, bytes);
  MUC_ON_MEM_ALLOC(muc, new_mem, asz);
  MUC_ON_MEM_REALLOC(muc, new_mem);

#if FILL_ALLOCATED_MEMORY
  ptrdiff_t memDiff = asz - prevMemSize;
  if (memDiff > 0)
    FILL_MEM_WITH_PATTERN((char *)new_mem + prevMemSize, NEW_MEMORY_FILL_PATTERN, memDiff);
#endif
  return new_mem;
}

static inline void *dagmem_recalloc_base(void *p, size_t elem_count, size_t elem_size)
{
  size_t old_size = dagmem_malloc_usable_size(p);
  size_t new_size = elem_count * elem_size;

  void *new_p = dagmem_realloc_base(p, new_size);

  // If the reallocation succeeded and the new block is larger, zero-fill the new bytes:
  if (new_p && old_size < new_size)
    memset(static_cast<char *>(new_p) + old_size, 0, new_size - old_size);
  return new_p;
}

inline void *dagmem_recalloc(void *mem, size_t elem_count, size_t elem_size)
{
  MUC_DECL_MSIZE(prevsz, mem, MALLOC_MIN_ALIGNMENT);
  MUC_ON_MEM_FREE(muc, mem, prevsz);

  void *new_mem = dagmem_recalloc_base(mem, elem_count, elem_size);

  MUC_DECL_ASIZE(asz, new_mem, MALLOC_MIN_ALIGNMENT, elem_count * elem_size);
  MUC_ON_MEM_ALLOC(muc, new_mem, asz);
  MUC_ON_MEM_REALLOC(muc, new_mem);
  return new_mem;
}

inline bool dagmem_expand(void *p, size_t sz)
{
  if (DAGOR_UNLIKELY(!p || !sz))
  {
#if MEASURE_EXPAND_EFF
    interlocked_increment(cnt_expand_bad_p);
#endif
    return sz == 0;
  }
  MUC_DECL_MSIZE(prevsz, p, 16);
  MUC_ON_MEM_FREE(muc, p, prevsz);

  void *pn = dagmem_expand_base(p, sz);

  MUC_DECL_ASIZE(asz, pn ? pn : p, 16, pn ? sz : prevsz);
  MUC_ON_MEM_ALLOC(muc, p, asz);

#if MEASURE_EXPAND_EFF
  if (prevsz >= 2048)
  {
    interlocked_increment(prevsz <= (16 << 10) ? cnt_expand_total_16K : cnt_expand_total);
    if (pn)
      interlocked_increment(prevsz <= (16 << 10) ? cnt_expand_ok_16K : cnt_expand_ok);
  }
#endif
  return pn != nullptr;
}

class DagStdMemoryAllocator final : public IMemAlloc
{
public:
  DagStdMemoryAllocator(const char *) {}
  ~DagStdMemoryAllocator() {}

  // IMemAlloc interface implementation
  void destroy() override { delete this; }
  bool isEmpty() override { return false; }
  void *alloc(size_t sz) override
  {
    void *p = dagmem_malloc(sz);
    if (!p)
    {
      LAST_RESORT_ON_OOM(sz, dagmem_malloc(sz));
      dumpUsedMem(), DAG_FATAL("Not enough memory to alloc %llu bytes", sz);
    }
    return p;
  }
  void *tryAlloc(size_t sz) override { return dagmem_malloc(sz); }
  void *allocAligned(size_t sz, size_t alignment) override
  {
    void *p = dagmem_memalign(alignment, sz);
    if (!p)
    {
      LAST_RESORT_ON_OOM(sz, dagmem_memalign(alignment, sz));
      dumpUsedMem(), DAG_FATAL("Not enough memory to alloc %llu bytes (alignment: %u)", sz, alignment);
    }
    return p;
  }
  void free(void *p) override { dagmem_free(p); }
  void freeAligned(void *p) override { dagmem_free_aligned(p); }
  size_t getSize(void *p) override { return p ? dagmem_malloc_usable_size(p) : 0; }
  bool resizeInplace(void *p, size_t sz) override
  {
    bool res = dagmem_expand(p, sz);
    return res;
  }
  void *realloc(void *p, size_t sz) override
  {
#if MEASURE_EXPAND_EFF
    size_t asz = dagmem_malloc_usable_size(p);
    if (asz > 2048 && sz > asz)
    {
      interlocked_decrement(asz <= (16 << 10) ? cnt_expand_total_16K : cnt_expand_total);
      interlocked_increment(asz <= (16 << 10) ? cnt_expand_r_total_16K : cnt_expand_r_total);
      if (dagmem_expand(p, sz))
      {
        interlocked_decrement(asz <= (16 << 10) ? cnt_expand_ok_16K : cnt_expand_ok);
        interlocked_increment(asz <= (16 << 10) ? cnt_expand_r_ok_16K : cnt_expand_r_ok);
        return p;
      }
    }
#endif
    void *np = dagmem_realloc(p, sz);
    if (!np && sz)
    {
      LAST_RESORT_ON_OOM(sz, dagmem_realloc(p, sz));
      dumpUsedMem(), DAG_FATAL("Not enough memory in realloc(%p,%llu) call", p, sz);
    }
    return np;
  }

protected:
  void dumpUsedMem()
  {
#if DAGOR_DBGLEVEL > 0
    static char buf[2048];
    get_memory_stats(buf, 2048);
    debug("on mem finished: %s", buf);
#endif
  }
};

#define DECL_MEM(NM) IMemAlloc *NM = nullptr //-V1003

DECL_MEM(stdmem);
DECL_MEM(tmpmem);
DECL_MEM(inimem);
DECL_MEM(midmem);
DECL_MEM(strmem);
DECL_MEM(defaultmem);
DECL_MEM(uimem);
DECL_MEM(scriptmem);
DECL_MEM(globmem);

#undef DECL_MEM

char *str_dup(const char *s, IMemAlloc *a)
{
  // dbg( "%s : %i  (%s)\n", __FILE__, __LINE__, __FUNCTION__ );
  if (!s)
    return NULL;
  size_t n = strlen(s) + 1;
  void *p = a->alloc(n);
  if (!p)
    return (char *)p;
  memcpy(p, s, n);
  return (char *)p;
}

void get_memory_stats(char *buf, int buflen)
{
  LimitedBufferWriter lbw(buf, buflen);
  lbw.append("--- Mem stats ---\n");
#if MEASURE_EXPAND_EFF
  lbw.aprintf("expand:    (%d/%d less 16K %d/%d others) bad(ptr)=%d\n", cnt_expand_ok_16K, cnt_expand_total_16K, cnt_expand_ok,
    cnt_expand_total, cnt_expand_bad_p);
  lbw.aprintf("expand(r): (%d/%d less 16K, %d/%d others)\n", cnt_expand_r_ok_16K, cnt_expand_r_total_16K, cnt_expand_r_ok,
    cnt_expand_r_total);
#endif
#if MEMUSAGE_COUNT
  MemUsageStats m;
  m.copyFrom(::muc);
  lbw.aprintf("  calls(malloc/realloc/free):  %9d/%9d/%9d\n", //
    int(m.malloc_cnt & 0xFFFFFFFF), int(m.realloc_cnt & 0xFFFFFFFF), int(m.free_cnt & 0xFFFFFFFF));
  lbw.aprintf("  memory(max/now):             %9llu/%9llu  (%6d/%6d K)\n", //
    m.max_alloc, m.total_alloc, m.max_alloc >> 10, m.total_alloc >> 10);
#if USE_DLMALLOC && !NO_MALLINFO && !DEBUG
  mallinfo ma = dlmallinfo();
  lbw.aprintf("  dlmalloc allocated(used/total/free):  %6dK/%6dK/%6dK\n", ma.uordblks >> 10, dlmalloc_footprint() >> 10,
    ma.fordblks >> 10);
#endif
  lbw.aprintf("  pointers(max/now):           %9d/%9d\n", m.max_ptr_in_use, m.ptr_in_use);

#if USE_DLMALLOC
  lbw.aprintf("  commited(max/now):           %6d/%6d K\n", get_memory_committed_max() >> 10, get_memory_committed() >> 10);
#endif
  lbw.aprintf("  sysmem:                      %7d K\n", get_sysmem_in_use() >> 10);

  if (d3d_dump_memory_stat)
    d3d_dump_memory_stat(lbw);

#else
  lbw.aprintf("%dK in %d ptrs (+CRT: %dK)\n", dagor_memory_stat::get_memory_allocated(false),
    dagor_memory_stat::get_memchunk_count(false),
    dagor_memory_stat::get_memory_allocated(true) - dagor_memory_stat::get_memory_allocated(false));
#endif

#if MEMUSAGE_COUNT == 2
  muc2::measurePrintStat();
#endif
}

size_t get_max_mem_used() { return get_memory_committed_max(); }

static void win_enable_low_fragmentation_heap() // https://msdn.microsoft.com/en-US/library/aa366750.aspx
{
#if _TARGET_PC_WIN && !USE_IMPORTED_ALLOC
  unsigned numHeaps = GetProcessHeaps(0, NULL); // ~ 4 in practice
  if (!numHeaps)
    return;
  HANDLE *heaps = (HANDLE *)alloca(sizeof(HANDLE) * numHeaps);
  if (!GetProcessHeaps(numHeaps, heaps))
    return;
  ULONG lfhFlag = 2;
  for (int i = 0; i < numHeaps; ++i)
    HeapSetInformation(heaps[i], HeapCompatibilityInformation, &lfhFlag, sizeof(lfhFlag));
#endif
}

#define DECLARE_MEM_STORAGE(NM_STOR, CLS) alignas(CLS) static char NM_STOR[sizeof(CLS)]

#if MEM_DEBUGALLOC <= 0
// normal (non-debug) memory allocator
#define FINAL_ALLOC(M) reinterpret_cast<DagStdMemoryAllocator *>(std_mem_storage)

DECLARE_MEM_STORAGE(std_mem_storage, DagStdMemoryAllocator);

static void real_init_memory()
{
  if (mem_mgr_inited)
    return;

  win_enable_low_fragmentation_heap();

  stdmem = new (std_mem_storage, _NEW_INPLACE) DagStdMemoryAllocator("StdMem");
  tmpmem = stdmem;
  inimem = stdmem;
  midmem = stdmem;
  strmem = stdmem;
  uimem = stdmem;
  globmem = stdmem;
  scriptmem = stdmem;
  defaultmem = tmpmem;

  mem_mgr_inited = 1;
}

void memfree_anywhere(void *p)
{
  // measureFree(p) is not called
  dagmem_free(p);
}

IMemAllocExtAPI *stdmem_extapi = nullptr;
namespace DagDbgMem
{
const char *get_ptr_allocation_call_stack(void *ptr) { return stdmem_extapi ? stdmem_extapi->getPtrAllocationCallstack(ptr) : "n/a"; }
bool check_memory(bool /*only_cur_gen*/) { return true; }
bool enable_stack_fill(bool /*enable*/) { return false; }
da_profiler::profile_mem_data_t set_forced_profile_callstack()
{
  return stdmem_extapi ? stdmem_extapi->setForcedProfileCallstack() : da_profiler::invalid_memory_profile;
}
void unset_forced_profile_callstack(da_profiler::profile_mem_data_t prev_handle)
{
  if (stdmem_extapi)
    stdmem_extapi->unsetForcedProfileCallstack(prev_handle);
}
void set_allow_fill_stack(bool) {}
} // namespace DagDbgMem

#else
// debug memory allocator (can detect leaks and overruns)
#define FINAL_ALLOC(M) reinterpret_cast<DagDebugMemAllocator *>(M ? (void *)M : (void *)std_mem_storage)

#include "dagDbgMem.inc.cpp"
struct DagorDebugMemAllocExtAPI : public IMemAllocExtAPI
{
public:
  da_profiler::profile_mem_data_t setForcedProfileCallstack() override { return DagDbgMem::set_forced_profile_callstack(); }
  void unsetForcedProfileCallstack(da_profiler::profile_mem_data_t h) override { DagDbgMem::unset_forced_profile_callstack(h); }
  const char *getPtrAllocationCallstack(void *ptr) override { return DagDbgMem::get_ptr_allocation_call_stack(ptr); }
};

static DagorDebugMemAllocExtAPI stdmem_extapi_impl;
IMemAllocExtAPI *stdmem_extapi = &stdmem_extapi_impl;

DECLARE_MEM_STORAGE(source_mem_storage, DagStdMemoryAllocator);
DECLARE_MEM_STORAGE(std_mem_storage, DagDebugMemAllocator);

#undef DECLARE_MEM_STORAGE

static void real_init_memory()
{
  if (mem_mgr_inited)
    return;

  win_enable_low_fragmentation_heap();

  sourcemem = new (source_mem_storage, _NEW_INPLACE) DagStdMemoryAllocator("SourceMem");

  stdmem = new (std_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("StdMem");
  tmpmem = stdmem;
  inimem = stdmem;
  midmem = stdmem;
  strmem = stdmem;
  uimem = stdmem;
  globmem = stdmem;
  scriptmem = stdmem;
  defaultmem = tmpmem;

  mem_mgr_inited = true;
}

void memfree_anywhere(void *p) { static_cast<DagDebugMemAllocator *>(stdmem)->memFreeAnywhere(p); }

#endif

extern "C" void *memalloc_default(size_t sz) { return defaultmem->alloc(sz); }
extern "C" void memfree_default(void *p) { defaultmem->free(p); }
extern "C" void *memalloc_aligned_default(size_t sz, size_t al) { return defaultmem->allocAligned(sz, al); }
extern "C" void memfree_aligned_default(void *p) { defaultmem->freeAligned(p); }
extern "C" void *memrealloc_default(void *p, size_t sz) { return defaultmem->realloc(p, sz); }
extern "C" int memresizeinplace_default(void *p, size_t sz) { return int(defaultmem->resizeInplace(p, sz)); }

extern "C" void *memcalloc_default(size_t n_elem, size_t elem_sz)
{
  size_t sz = elem_sz * n_elem;
  void *ptr = defaultmem->alloc(sz);
  memset(ptr, 0, sz);
  return ptr;
}

#if USE_MIMALLOC
#include <osApiWrappers/dag_miscApi.h>
static void memcollect_cur_thread() { mi_collect(/*force*/ !is_main_thread()); }
#endif

void (*get_memcollect_cur_thread_cb())()
{
#if USE_MIMALLOC
  return &memcollect_cur_thread;
#else
  return NULL;
#endif
}

#if !DAGOR_ADDRESS_SANITIZER
#include "stubRtlAlloc.inc.cpp"
#else
static inline void init_acrt_heap() {}
#endif

#if defined(__clang__)
void *operator new(std::size_t s) DAG_THROW_BAD_ALLOC { return FINAL_ALLOC(defaultmem)->alloc(s); }
void *operator new(std::size_t s, const std::nothrow_t &) DAG_NOEXCEPT { return FINAL_ALLOC(defaultmem)->tryAlloc(s); }
void *operator new[](std::size_t s) DAG_THROW_BAD_ALLOC { return FINAL_ALLOC(defaultmem)->alloc(s); }
void *operator new[](std::size_t s, const std::nothrow_t &) DAG_NOEXCEPT { return FINAL_ALLOC(defaultmem)->tryAlloc(s); }
void operator delete(void *p) DAG_NOEXCEPT { memfree_anywhere(p); }
void operator delete[](void *p) DAG_NOEXCEPT { memfree_anywhere(p); }
void operator delete(void *p, size_t) DAG_NOEXCEPT { memfree_anywhere(p); }
void operator delete[](void *p, size_t) DAG_NOEXCEPT { memfree_anywhere(p); }
#if (__cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703)) && !__has_feature(address_sanitizer)
void *operator new(std::size_t sz, std::align_val_t al) { return FINAL_ALLOC(defaultmem)->allocAligned(sz, (size_t)al); }
void *operator new[](std::size_t sz, std::align_val_t al) { return FINAL_ALLOC(defaultmem)->allocAligned(sz, (size_t)al); }
void operator delete(void *p, std::align_val_t) noexcept { FINAL_ALLOC(defaultmem)->freeAligned(p); }
void operator delete[](void *p, std::align_val_t) noexcept { FINAL_ALLOC(defaultmem)->freeAligned(p); }
#endif
#endif

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | defined(__clang__)
void *memalloc(size_t sz) { return FINAL_ALLOC(defaultmem)->alloc(sz); }
#endif

typedef int(__cdecl *StaticInitFuncPtrT)(void);
extern "C" StaticInitFuncPtrT __c_dagor_memory_real_init;
#if !USE_IMPORTED_ALLOC
#if defined(__GNUC__) || defined(__clang__)
static class __MemMgrInit
{
public:
  __MemMgrInit()
  {
    init_acrt_heap();
    real_init_memory();
#ifdef DAGMEM_POST_INIT_ACTION
    DAGMEM_POST_INIT_ACTION();
#endif
  }
} __mem_init __attribute__((init_priority(101)));

#else
extern "C" int __cdecl dagor_memory_real_init(void)
{
  init_acrt_heap();
  real_init_memory();
  return !__c_dagor_memory_real_init;
}

#pragma warning(disable : 4074)
#pragma init_seg(compiler)
static int __cpp_dagor_memory_real_init = dagor_memory_real_init();

#endif

#else // USE_IMPORTED_ALLOC
static IMemAlloc *dagmem_imported_init();
class FirstJumpAllocator final : public IMemAlloc
{
public:
  void *alloc(size_t sz) override { return prepareMem()->alloc(sz); }
  void *tryAlloc(size_t sz) override { return prepareMem()->tryAlloc(sz); }
  void *allocAligned(size_t sz, size_t alignment) override { return prepareMem()->allocAligned(sz, alignment); }
  void free(void *p) override { prepareMem()->free(p); }
  void freeAligned(void *p) override { prepareMem()->freeAligned(p); }
  size_t getSize(void *p) override { return p ? prepareMem()->getSize(p) : 0; }
  bool resizeInplace(void *p, size_t sz) override { return prepareMem()->resizeInplace(p, sz); }
  void *realloc(void *p, size_t sz) override { return prepareMem()->realloc(p, sz); }

  void destroy() override {}
  bool isEmpty() override { return false; }
  static IMemAlloc *prepareMem() { return imported_mem == &a ? dagmem_imported_init() : imported_mem; }
  static FirstJumpAllocator a;
};
FirstJumpAllocator FirstJumpAllocator::a;

IMemAlloc *imported_mem = &FirstJumpAllocator::a;
static IMemAlloc *init_mem_imported(void *ptr, void *ptr_extapi)
{
  if (ptr)
    imported_mem = *(IMemAlloc **)ptr;
  if (ptr_extapi)
    stdmem_extapi = *(IMemAllocExtAPI **)ptr_extapi;
  real_init_memory();
  return imported_mem;
}

#if _TARGET_PC_WIN | _TARGET_XBOX
#include <windows.h>
#define RESOLVE_SYM(NM) GetProcAddress(GetModuleHandle(NULL), NM)
extern "C" int __cdecl dagor_memory_real_init() { return dagmem_imported_init() && __c_dagor_memory_real_init ? 0 : -1; }

#elif _TARGET_APPLE
#include <malloc/malloc.h>
#include <dlfcn.h>
#define RESOLVE_SYM(NM) dlsym(RTLD_MAIN_ONLY, NM)
__attribute__((constructor(101))) static void dagor_memory_real_init() { dagmem_imported_init(); }

#elif _TARGET_PC_LINUX
#include <dlfcn.h>
#define RESOLVE_SYM(NM) dlsym(RTLD_DEFAULT, NM)
__attribute__((constructor(101))) static void dagor_memory_real_init() { dagmem_imported_init(); }

#endif

#ifdef RESOLVE_SYM
IMemAlloc *dagmem_imported_init() { return init_mem_imported(RESOLVE_SYM("shared_stdmem"), RESOLVE_SYM("shared_stdmem_extapi")); }
#undef RESOLVE_SYM

#elif _TARGET_C1 | _TARGET_C2


#endif
#endif

#define EXPORT_PULL dll_pull_memory_dagmem
#include <supp/exportPull.h>
