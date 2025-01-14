// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "stdRtlMemUsage.h"
#if !defined(_STD_RTL_MEMORY) || defined(_DLMALLOC_MSPACE)
#include "dlmalloc-2.8.4.h"
#endif
#include <memory/dag_mem.h>
#include <debug/dag_fatal.h>
#include <generic/dag_initOnDemand.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_atomic.h>
#include <debug/dag_debug.h>
#include <string.h>
#include <util/dag_string.h>
#include <util/limBufWriter.h>
#include <memory/dag_dbgMem.h>
#include <memory/dag_memStat.h>
#include "allocStep.h"
#include <3d/3dMemStat.h>
#include <errno.h>
#include <startup/dag_globalSettings.h>
#include "macMemInit.h"

#include <supp/_platform.h>
#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_WIN
#include <windows.h>
#include <malloc.h>
#endif

#define HAS_GET_SYSMEM_IN_USE     1
#define NO_GET_SYSMEM_PTRS_IN_USE 1

#if _TARGET_C1 | _TARGET_C2

#elif _TARGET_PC_LINUX
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
#pragma message("memory manager is inaccurate with _STD_RTL_MEMORY for this target")
static size_t get_sysmem_in_use() { return 0; }
#endif

#if NO_GET_SYSMEM_PTRS_IN_USE
static unsigned get_sysmem_ptrs_in_use() { return 0; }
#endif

#if !defined(__SANITIZE_ADDRESS__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define __SANITIZE_ADDRESS__ 1
#endif
#endif

// memory tracker settings (work buffer is preallocated)
#define MTR_MAX_BLOCKS         (4 << 20) // max tracked blocks x 48B
#define MTR_MAX_BT_ELEMS       (600000)  // approx number of backtrace elements x 8B
#define MTR_COLLECT_BACKTRACE  1         // allow to collect backtraces (disable if slow)
#define MTR_DUMP_SIZE_PERCENTS 30        // workbuffer size (% of total memory needed to write all data)
#define MTR_PACK_DATA          0         // pack data when writing files [TODO:]


// MEMUSAGE_COUNT: 1=summary (performed always) 2=chunk_size summary 3=tracing
#ifndef MEMORY_TRACKER
#define MEMORY_TRACKER 0 // set 1 to enable memory tracker, use -sUseMemTracker=yes
#endif

#define PROTECT_MEMORY_MANAGER 0 // breaks execution of non-protected executables
#if DAGOR_DBGLEVEL > 0 && _TARGET_STATIC_LIB && MEM_DEBUGALLOC >= 0 && !defined(__SANITIZE_ADDRESS__)
#if MEMORY_TRACKER > 0
#define MEMUSAGE_COUNT 3
#else
#define MEMUSAGE_COUNT 1
#endif
#define FILL_ALLOCATED_MEMORY 1
#define FILL_DELETED_MEMORY   1
#else
#define MEMUSAGE_COUNT        0
#define FILL_ALLOCATED_MEMORY 0
#define FILL_DELETED_MEMORY   0
#endif
// #define MEMUSAGE_BIG_CHUNCK_THRES (12<<20)

#define GET_MEM_SIZE(p) sys_malloc_usable_size(p)

#if (defined(_M_IX86_FP) && _M_IX86_FP == 0) || MEM_DEBUGALLOC > 0
#define MIN4K(x) x
#else
#define MIN4K(x) min<unsigned>(x, 4096u)
#endif

#define FILL_MEM_WITH_PATTERN(ptr, pattern, sz)                                                                                    \
  do                                                                                                                               \
  {                                                                                                                                \
    size_t blockSize = sz;                                                                                                         \
    G_FAST_ASSERT(blockSize < (size_t)-0x10000); /*_msize returns -1 for a bad block, but _aligned_msize subtracts the alignment*/ \
    for (int *i = (int *)(ptr), *e = i + MIN4K(blockSize / sizeof(int)); i < e;)                                                   \
      *i++ = pattern;                                                                                                              \
  } while (0)
#define FILL_PTR_WITH_PATTERN(p, pattern) FILL_MEM_WITH_PATTERN(p, pattern, GET_MEM_SIZE(p))

#if FILL_ALLOCATED_MEMORY
#define NEW_MEMORY_FILL_PATTERN 0x7ffdcdcd
#define FILL_NEW_MEMORY(p)      FILL_PTR_WITH_PATTERN(p, NEW_MEMORY_FILL_PATTERN)
#else
#define FILL_NEW_MEMORY(p) ((void)0)
#endif

#if FILL_DELETED_MEMORY
#define FILL_FREED_MEMORY(p) FILL_PTR_WITH_PATTERN(p, 0x7ffddddd)
#else
#define FILL_FREED_MEMORY(p) ((void)0)
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
  static constexpr int MEM_BINS_CNT = 64 /*step 8*/ + 120 /*step 64*/ + 62 /*step 4K*/ + 3 /*step 256K*/ + 15 /*step 1M*/ + 1 /*16M+*/;
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
#else
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
  void on_alloc(void *ptr, bool aligned = false)
  {
    if (!ptr)
      return;

    interlocked_increment(malloc_cnt);

    size_t asize = aligned ? dlmalloc_usable_size_aligned(ptr) : sys_malloc_usable_size(ptr);

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

  void on_free(void *ptr, bool aligned = false)
  {
    if (!ptr)
      return;

    interlocked_increment(free_cnt);

    ptrdiff_t asize = aligned ? dlmalloc_usable_size_aligned(ptr) : sys_malloc_usable_size(ptr);

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
static MemUsageStats muc_crt = {0, 0, 0, 0, 0, 0, 0};

int64_t dagor_memory_stat::get_malloc_call_count()
{
  return interlocked_acquire_load(muc.malloc_cnt) + interlocked_acquire_load(muc.free_cnt) + interlocked_acquire_load(muc.realloc_cnt);
}
size_t dagor_memory_stat::get_memchunk_count(bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (unsigned ptrs_in_use = get_sysmem_ptrs_in_use())
      return ptrs_in_use;
#endif
  return interlocked_acquire_load(muc.ptr_in_use) + (_crt ? interlocked_acquire_load(muc_crt.ptr_in_use) : 0);
}
size_t dagor_memory_stat::get_memory_allocated(bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (size_t mem_in_use = get_sysmem_in_use())
      return mem_in_use;
#endif
  return interlocked_acquire_load(muc.total_alloc) + (_crt ? interlocked_acquire_load(muc_crt.total_alloc) : 0);
}

#else

struct MemUsageStats0
{
  volatile int total_alloc_x16, ptr_in_use;

  void on_alloc(void *ptr, bool aligned = false)
  {
    if (!ptr)
      return;

    int asize = int(((aligned ? dlmalloc_usable_size_aligned(ptr) : sys_malloc_usable_size(ptr)) + 15) / 16);
    interlocked_add(total_alloc_x16, asize);
    interlocked_increment(ptr_in_use);
    muc2::measureAlloc(ptr, asize);
  }
  void on_free(void *ptr, bool aligned = false)
  {
    if (!ptr)
      return;

    int asize = int(((aligned ? dlmalloc_usable_size_aligned(ptr) : sys_malloc_usable_size(ptr)) + 15) / 16);
    interlocked_add(total_alloc_x16, -asize);
    interlocked_decrement(ptr_in_use);
    muc2::measureFree(ptr, asize);
  }
  void on_realloc(bool) {}
};

static MemUsageStats0 muc = {0, 0}, muc_crt = {0, 0};

int64_t dagor_memory_stat::get_malloc_call_count() { return 0; }

size_t dagor_memory_stat::get_memchunk_count(bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (unsigned ptrs_in_use = get_sysmem_ptrs_in_use())
      return ptrs_in_use;
#endif
  return interlocked_acquire_load(muc.ptr_in_use) + (_crt ? interlocked_acquire_load(muc_crt.ptr_in_use) : 0);
}
size_t dagor_memory_stat::get_memory_allocated(bool _crt)
{
#if HAS_GET_SYSMEM_IN_USE
  if (_crt)
    if (size_t mem_in_use = get_sysmem_in_use())
      return mem_in_use;
#endif
  return 16 * (size_t(interlocked_acquire_load(muc.total_alloc_x16)) + (_crt ? interlocked_acquire_load(muc_crt.total_alloc_x16) : 0));
}

#endif

int dagor_memory_stat::get_memory_allocated_kb(bool with_crt) { return int(dagor_memory_stat::get_memory_allocated(with_crt) >> 10); }


#define MUC_ON_MEM_ALLOC(m, ptr)         m.on_alloc(ptr)
#define MUC_ON_MEM_REALLOC(m, new_mem)   m.on_realloc(new_mem)
#define MUC_ON_MEM_FREE(m, ptr)          m.on_free(ptr)
#define MUC_ON_MEM_ALLOC_ALIGNED(m, ptr) m.on_alloc(ptr, true)
#define MUC_ON_MEM_FREE_ALIGNED(m, ptr)  m.on_free(ptr, true)

#if !_STD_RTL_MEMORY || (MEM_DEBUGALLOC > 0) // debug allocator requires crit section for changing internal debug structures
#define MEM_LOCKS 1
#else
#define MEM_LOCKS 0
#endif

#if MEM_LOCKS
static bool _dlmalloc_threadsafe = false; // can NOT be true by default
static bool _dlmalloc_critsec_inited = false;
static CritSecStorage _dlmalloc_critsec;
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

#if MEM_LOCKS
static int memory_thread_safe_ref = 0;
#endif

void (*d3d_dump_memory_stat)(LimitedBufferWriter &wr) = NULL;

#if defined(_STD_RTL_MEMORY)
extern "C" size_t get_memory_committed() { return 0; }
extern "C" size_t get_memory_committed_max() { return 0; }
#endif

#define INSIDE_ENGINE 1
#include "memoryTracker.h"
using namespace memorytracker;

#if MEMUSAGE_COUNT == 3
MemoryTracker memory_tracker;
#else
MemoryTrackerNull memory_tracker;
#endif

namespace memorytracker
{
MemoryTracker *get_memtracker_access()
{
#if MEMUSAGE_COUNT == 3
  return &memory_tracker;
#else
  return nullptr;
#endif
}
}; // namespace memorytracker

static void set_memory_thread_safe(bool is_safe)
{
#if MEM_LOCKS
  // critical-sectioon must be created BEFORE _dlmalloc_threadsafe is set,
  // it uses dlmalloc
  if (is_safe && !_dlmalloc_critsec_inited)
  {
    create_critical_section(_dlmalloc_critsec, "dlmalloc");
    _dlmalloc_critsec_inited = true;
  }

  enter_critical_section_raw(_dlmalloc_critsec);

  if (is_safe)
    memory_thread_safe_ref++;
  else
  {
    G_ASSERT(memory_thread_safe_ref > 0);
    memory_thread_safe_ref--;
  }
  is_safe = (memory_thread_safe_ref > 0);

  if (is_safe != _dlmalloc_threadsafe)
  {
    _dlmalloc_threadsafe = is_safe;
    // debug("=== switched to %sthead-safe mem management (%d refs)", _dlmalloc_threadsafe ? "" : "NON ", memory_thread_safe_ref);
  }
  leave_critical_section(_dlmalloc_critsec);
#else
  (void)is_safe;
#endif
}

static void enter_memory_critical_section()
{
#if MEM_LOCKS
  if (_dlmalloc_critsec_inited)
    enter_critical_section_raw(_dlmalloc_critsec);
#endif
}

static void leave_memory_critical_section()
{
#if MEM_LOCKS
  if (_dlmalloc_critsec_inited)
    leave_critical_section(_dlmalloc_critsec);
#endif
}


//
//  thread-safe wrappers on dlmalloc functions
//
inline void *mt_dlmalloc(size_t bytes)
{
  enter_memory_critical_section();

  void *mem = dlmalloc(bytes);

  MUC_ON_MEM_ALLOC(muc, mem);

  leave_memory_critical_section();
  FILL_NEW_MEMORY(mem);

  return mem;
}

inline void *mt_dlmemalign(size_t bytes, size_t alignment)
{
  enter_memory_critical_section();

  void *mem;
  if (alignment)
    mem = dlmemalign(alignment, bytes);
  else
    mem = dlvalloc(bytes);

  MUC_ON_MEM_ALLOC_ALIGNED(muc, mem);

  leave_memory_critical_section();

#if FILL_ALLOCATED_MEMORY
  FILL_MEM_WITH_PATTERN(mem, NEW_MEMORY_FILL_PATTERN, dlmalloc_usable_size_aligned(mem));
#endif

  return mem;
}

inline void mt_dlfree(void *mem)
{
  FILL_FREED_MEMORY(mem);

#if DAGOR_DBGLEVEL < 1 && (_TARGET_C1 | _TARGET_C2)
  if (cpujobs::abortJobsRequested)
    return;
#endif

  enter_memory_critical_section();

  MUC_ON_MEM_FREE(muc, mem);

  dlfree(mem);

  leave_memory_critical_section();
}

inline void mt_dlfree_aligned(void *mem)
{
#if FILL_DELETED_MEMORY
  FILL_MEM_WITH_PATTERN(mem, 0x7ffddddd, dlmalloc_usable_size_aligned(mem));
#endif
  enter_memory_critical_section();
  MUC_ON_MEM_FREE_ALIGNED(muc, mem);
  dlfree_aligned(mem);
  leave_memory_critical_section();
}

inline void *mt_dlrealloc(void *mem, size_t bytes)
{
  enter_memory_critical_section();

  MUC_ON_MEM_FREE(muc, mem);
#if FILL_ALLOCATED_MEMORY
  size_t prevMemSize = mem ? GET_MEM_SIZE(mem) : 0;
#endif

  void *new_mem = dlrealloc(mem, bytes);

  MUC_ON_MEM_ALLOC(muc, new_mem);
  MUC_ON_MEM_REALLOC(muc, new_mem);

  leave_memory_critical_section();

#if FILL_ALLOCATED_MEMORY
  ptrdiff_t memDiff = GET_MEM_SIZE(new_mem) - prevMemSize;
  if (memDiff > 0)
    FILL_MEM_WITH_PATTERN((char *)new_mem + prevMemSize, NEW_MEMORY_FILL_PATTERN, memDiff);
#endif

  return new_mem;
}

inline bool mt_expand(void *p, size_t sz)
{
  if (DAGOR_UNLIKELY(!p || !sz))
  {
#if MEASURE_EXPAND_EFF
    interlocked_increment(cnt_expand_bad_p);
#endif
    return sz == 0;
  }
#if MEASURE_EXPAND_EFF || (defined(_STD_RTL_MEMORY) && !((_TARGET_PC_WIN | _TARGET_XBOX) && (_TARGET_64BIT | USE_MIMALLOC)))
  size_t asz = sys_malloc_usable_size(p);
#endif

  MUC_ON_MEM_FREE(muc, p);
#if defined(_STD_RTL_MEMORY) && (_TARGET_PC_WIN | _TARGET_XBOX) && (_TARGET_64BIT | USE_MIMALLOC)
  void *pn = ::_expand(p, sz);
#elif !defined(_STD_RTL_MEMORY)
  enter_memory_critical_section();
  void *pn = ::dlresize_inplace(p, sz);
  leave_memory_critical_section();
#else
  void *pn = sz <= asz ? p : nullptr;
#endif
  MUC_ON_MEM_ALLOC(muc, p);

#if MEASURE_EXPAND_EFF
  if (asz >= 2048)
  {
    interlocked_increment(asz <= (16 << 10) ? cnt_expand_total_16K : cnt_expand_total);
    if (pn)
      interlocked_increment(asz <= (16 << 10) ? cnt_expand_ok_16K : cnt_expand_ok);
  }
#endif
  return pn != nullptr;
}

#ifndef _STD_RTL_MEMORY
static CritSecStorage _dlmalloc_critsec_crt;
static void *_mspace_crt = NULL;
static bool _mspace_crt_begin_fatal = false;
static constexpr int START_CRT_POOL_SZ = 64 << 10;

#if _TARGET_STATIC_LIB
#define CRT_PREFIX static inline
#else
#define CRT_PREFIX extern "C" __declspec(dllexport)
#endif

static inline void enter_dlmalloc_critsec_crt()
{
  if (!_mspace_crt)
  {
    _mspace_crt = create_mspace(START_CRT_POOL_SZ, false);
    create_critical_section(_dlmalloc_critsec_crt);
    mspace_footprint(_mspace_crt);
  }
  enter_critical_section_raw(_dlmalloc_critsec_crt);
}

CRT_PREFIX void *mt_dlmalloc_crt(size_t bytes)
{
  enter_dlmalloc_critsec_crt();

  void *mem = mspace_malloc(_mspace_crt, bytes);
  MUC_ON_MEM_ALLOC(muc_crt, mem);
  if (mspace_footprint(_mspace_crt) > dagormem_max_crt_pool_sz && !_mspace_crt_begin_fatal)
  {
    _mspace_crt_begin_fatal = true;
    debug("Too large footprint for CRT mempool: %llu, last alloc:(%llu)", mspace_footprint(_mspace_crt), bytes);
  }

  leave_critical_section(_dlmalloc_critsec_crt);

  return mem;
}

CRT_PREFIX void *mt_dlmemalign_crt(size_t bytes, size_t al)
{
  enter_dlmalloc_critsec_crt();

  void *mem = mspace_memalign(_mspace_crt, al, bytes);
  MUC_ON_MEM_ALLOC(muc_crt, mem);
  if (mspace_footprint(_mspace_crt) > dagormem_max_crt_pool_sz && !_mspace_crt_begin_fatal)
  {
    _mspace_crt_begin_fatal = true;
    debug("Too large footprint for CRT mempool: %llu, last alloc:(%llu, align=%d)", mspace_footprint(_mspace_crt), bytes, al);
  }

  leave_critical_section(_dlmalloc_critsec_crt);
  return mem;
}

CRT_PREFIX void mt_dlfree_crt(void *mem)
{
  enter_critical_section_raw(_dlmalloc_critsec_crt);

  MUC_ON_MEM_FREE(muc_crt, mem);
  mspace_free(_mspace_crt, mem);

  leave_critical_section(_dlmalloc_critsec_crt);
}

CRT_PREFIX void *mt_dlrealloc_crt(void *mem, size_t bytes)
{
  enter_dlmalloc_critsec_crt();

  MUC_ON_MEM_FREE(muc_crt, mem);
  void *new_mem = mspace_realloc(_mspace_crt, mem, bytes);
  if (mspace_footprint(_mspace_crt) > dagormem_max_crt_pool_sz && !_mspace_crt_begin_fatal)
  {
    _mspace_crt_begin_fatal = true;
    debug("Too large footprint for CRT mempool: %llu, last realloc(%p, %llu)", mspace_footprint(_mspace_crt), mem, bytes);
  }

  MUC_ON_MEM_ALLOC(muc_crt, new_mem);
  MUC_ON_MEM_REALLOC(muc_crt, new_mem);

  leave_critical_section(_dlmalloc_critsec_crt);

  return new_mem;
}
#undef CRT_PREFIX
#endif

class StdDlmallocAllocator final : public IMemAlloc
{
public:
  StdDlmallocAllocator(const char *) {}
  ~StdDlmallocAllocator() {}

  // IMemAlloc interface implementation
  void destroy() override { delete this; }
  bool isEmpty() override { return false; }
  void *alloc(size_t sz) override
  {
    void *p = mt_dlmalloc(sz);
    if (!p)
    {
      LAST_RESORT_ON_OOM(sz, mt_dlmalloc(sz));
      dumpUsedMem(), DAG_FATAL("Not enough memory to alloc %llu bytes", sz);
    }
    memory_tracker.addBlock(this, p, sz);
    return p;
  }
  void *tryAlloc(size_t sz) override
  {
    void *p = mt_dlmalloc(sz);
    memory_tracker.addBlock(this, p, sz);
    return p;
  }
  void *allocAligned(size_t sz, size_t alignment) override
  {
    void *p = mt_dlmemalign(sz, alignment);
    if (!p)
    {
      LAST_RESORT_ON_OOM(sz, mt_dlmemalign(sz, alignment));
      dumpUsedMem(), DAG_FATAL("Not enough memory to alloc %llu bytes (alignment: %u)", sz, alignment);
    }
    memory_tracker.addBlock(this, p, sz);
    return p;
  }
  void free(void *p) override
  {
    memory_tracker.removeBlock(this, p);
    mt_dlfree(p);
  }
  void freeAligned(void *p) override
  {
    memory_tracker.removeBlock(this, p);
    mt_dlfree_aligned(p);
  }
  size_t getSize(void *p) override { return p ? sys_malloc_usable_size(p) : 0; }
  bool resizeInplace(void *p, size_t sz) override
  {
    memory_tracker.removeBlock(this, p);
    bool res = mt_expand(p, sz);
    memory_tracker.addBlock(this, p, sz);
    return res;
  }
  void *realloc(void *p, size_t sz) override
  {
    memory_tracker.removeBlock(this, p);
#if MEASURE_EXPAND_EFF
    size_t asz = sys_malloc_usable_size(p);
    if (asz > 2048 && sz > asz)
    {
      interlocked_decrement(asz <= (16 << 10) ? cnt_expand_total_16K : cnt_expand_total);
      interlocked_increment(asz <= (16 << 10) ? cnt_expand_r_total_16K : cnt_expand_r_total);
      if (mt_expand(p, sz))
      {
        interlocked_decrement(asz <= (16 << 10) ? cnt_expand_ok_16K : cnt_expand_ok);
        interlocked_increment(asz <= (16 << 10) ? cnt_expand_r_ok_16K : cnt_expand_r_ok);
        memory_tracker.addBlock(this, p, sz);
        return p;
      }
    }
#endif
    void *np = mt_dlrealloc(p, sz);
    if (!np && sz)
    {
      LAST_RESORT_ON_OOM(sz, mt_dlrealloc(p, sz));
      dumpUsedMem(), DAG_FATAL("Not enough memory in realloc(%p,%llu) call", p, sz);
    }
    memory_tracker.addBlock(this, np, sz);
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

void memset_dw(void *p, int dw, int dword_num)
{
  if (dw == 0)
    memset(p, 0, dword_num * 4);
  else
  {
    int *ip = (int *)p;
    while (dword_num > 0)
    {
      *ip++ = dw;
      dword_num--;
    }
  }
}

void get_memory_stats(char *buf, int buflen)
{
  enter_memory_critical_section();

  LimitedBufferWriter lbw(buf, buflen);
  lbw.append("--- Mem stats ---\n");
#if MEASURE_EXPAND_EFF
  lbw.aprintf("expand:    (%d/%d less 16K %d/%d others) bad(ptr)=%d\n", cnt_expand_ok_16K, cnt_expand_total_16K, cnt_expand_ok,
    cnt_expand_total, cnt_expand_bad_p);
  lbw.aprintf("expand(r): (%d/%d less 16K, %d/%d others)\n", cnt_expand_r_ok_16K, cnt_expand_r_total_16K, cnt_expand_r_ok,
    cnt_expand_r_total);
#endif
#if MEMUSAGE_COUNT
  MemUsageStats _muc, _muc_crt;
  _muc.copyFrom(::muc), _muc_crt.copyFrom(::muc_crt);
  for (int i = 0; i < 2; ++i)
  {
    MemUsageStats &m = i ? _muc_crt : _muc;
    lbw.aprintf("  %scalls(malloc/realloc/free):  %9d/%9d/%9d\n", i ? "crt " : "",
      (int)(interlocked_acquire_load(m.malloc_cnt) & 0xFFFFFFFF), (int)(interlocked_acquire_load(m.realloc_cnt) & 0xFFFFFFFF),
      (int)(interlocked_acquire_load(m.free_cnt) & 0xFFFFFFFF));
    lbw.aprintf("  %smemory(max/now):             %9llu/%9llu  (%6d/%6d K)\n", i ? "crt " : "", interlocked_acquire_load(m.max_alloc),
      interlocked_acquire_load(m.total_alloc), interlocked_acquire_load(m.max_alloc) >> 10,
      interlocked_acquire_load(m.total_alloc) >> 10);
  }
#if !defined(_STD_RTL_MEMORY) && !NO_MALLINFO && !DEBUG
  mallinfo ma = dlmallinfo();
  lbw.aprintf("  dlmalloc allocated(used/total/free):  %6dK/%6dK/%6dK\n", ma.uordblks >> 10, dlmalloc_footprint() >> 10,
    ma.fordblks >> 10);
#endif
  lbw.aprintf("  pointers(max/now):           %9d/%9d\n", interlocked_acquire_load(_muc.max_ptr_in_use),
    interlocked_acquire_load(_muc.ptr_in_use));

#if !(_TARGET_C1 | _TARGET_C2)
  lbw.aprintf("  commited(max/now):           %6d/%6d K\n", get_memory_committed_max() >> 10, get_memory_committed() >> 10);
#endif

  if (d3d_dump_memory_stat)
    d3d_dump_memory_stat(lbw);

#else
  lbw.aprintf("%dK in %d ptrs (CRT: %dK in %d)\n", size_t(interlocked_acquire_load(::muc.total_alloc_x16)) * 16 / 1024,
    interlocked_acquire_load(::muc.ptr_in_use), size_t(interlocked_acquire_load(::muc_crt.total_alloc_x16)) * 16 / 1024,
    interlocked_acquire_load(::muc_crt.ptr_in_use));
#endif

#if MEMUSAGE_COUNT == 2
  muc2::measurePrintStat();

#elif MEMUSAGE_COUNT == 3
  memory_tracker.printStats();
#endif

  leave_memory_critical_section();
}

size_t get_max_mem_used() { return get_memory_committed_max(); }

static void win_enable_low_fragmentation_heap() // https://msdn.microsoft.com/en-US/library/aa366750.aspx
{
#if _TARGET_PC_WIN
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
#define FINAL_ALLOC(M) reinterpret_cast<StdDlmallocAllocator *>(tmp_mem_storage)

DECLARE_MEM_STORAGE(std_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(tmp_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(ini_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(mid_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(str_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(ui_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(glob_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(lua_mem_storage, StdDlmallocAllocator);

static void real_init_memory()
{
  if (mem_mgr_inited)
    return;

  win_enable_low_fragmentation_heap();

  stdmem = new (std_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("StdMem");
  tmpmem = new (tmp_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("TmpMem");
  inimem = new (ini_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("IniMem");
  midmem = new (mid_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("MidMed");
  strmem = new (str_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("StrMem");
  uimem = new (ui_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("UiMem");
  globmem = new (glob_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("GlobMem");
  scriptmem = new (lua_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("ScriptMem");
  defaultmem = tmpmem;

  memory_tracker.init(MTR_MAX_BLOCKS, MTR_MAX_BT_ELEMS, MTR_DUMP_SIZE_PERCENTS);
  memory_tracker.registerHeap(stdmem, "StdMem");
  memory_tracker.registerHeap(tmpmem, "TmpMem");
  memory_tracker.registerHeap(inimem, "IniMem");
  memory_tracker.registerHeap(midmem, "MidMed");
  memory_tracker.registerHeap(strmem, "StrMem");
  memory_tracker.registerHeap(uimem, "UiMem");
  memory_tracker.registerHeap(globmem, "GlobMem");
  memory_tracker.registerHeap(scriptmem, "ScriptMem");

  mem_mgr_inited = 1;
#if _TARGET_APPLE
  pull_dagor_force_init_memmgr = 1;
#endif

#if _TARGET_STATIC_LIB && DAGOR_DBGLEVEL < 2
  pull_rtlOverride_stubRtl = 0;
#endif
  set_memory_thread_safe(true);
}

void memfree_anywhere(void *p)
{
  memory_tracker.removeBlock(nullptr, p, 0, true);
  // measureFree(p) is not called
  mt_dlfree(p);
}

namespace DagDbgMem
{
const char *get_ptr_allocation_call_stack(void * /*ptr*/) { return "n/a"; }
bool check_ptr(void * /*ptr*/, int * /*out_chk*/) { return true; }

void next_generation() {}
bool check_memory(bool /*only_cur_gen*/) { return true; }
void dump_all_used_ptrs(bool /*only_cur_gen*/) {}
void dump_leaks(bool /*only_cur_gen*/) {}
void enable_thorough_checks(bool /*enable*/) {}
bool enable_stack_fill(bool /*enable*/) { return true; }
void dump_raw_heap(const char *) {}
} // namespace DagDbgMem

#else
// debug memory allocator (can detect leaks and overruns)
#define FINAL_ALLOC(M) reinterpret_cast<DagDebugMemAllocator *>(M ? (void *)M : (void *)std_mem_storage)

#include "dagDbgMem.inc.cpp"

DECLARE_MEM_STORAGE(source_mem_storage, StdDlmallocAllocator);
DECLARE_MEM_STORAGE(std_mem_storage, DagDebugMemAllocator);
#if DBGMEM_SEPARATE_POOLS
DECLARE_MEM_STORAGE(tmp_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(ini_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(mid_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(str_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(ui_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(glob_mem_storage, DagDebugMemAllocator);
DECLARE_MEM_STORAGE(lua_mem_storage, DagDebugMemAllocator);
#endif

#undef DECLARE_MEM_STORAGE

static void real_init_memory()
{
  if (mem_mgr_inited)
    return;

  win_enable_low_fragmentation_heap();

  sourcemem = new (source_mem_storage, _NEW_INPLACE) StdDlmallocAllocator("SourceMem");

  stdmem = new (std_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("StdMem");
#if DBGMEM_SEPARATE_POOLS
  tmpmem = new (tmp_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("TmpMem");
  inimem = new (ini_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("IniMem");
  midmem = new (mid_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("MidMed");
  strmem = new (str_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("StrMem");
  uimem = new (ui_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("UiMem");
  globmem = new (glob_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("GlobMem");
  scriptmem = new (lua_mem_storage, _NEW_INPLACE) DagDebugMemAllocator("ScriptMem");
#else
  tmpmem = stdmem;
  inimem = stdmem;
  midmem = stdmem;
  strmem = stdmem;
  uimem = stdmem;
  globmem = stdmem;
  scriptmem = stdmem;
#endif
  defaultmem = tmpmem;

  memory_tracker.init(MTR_MAX_BLOCKS, MTR_MAX_BT_ELEMS, MTR_DUMP_SIZE_PERCENTS);
  memory_tracker.registerHeap(stdmem, "StdMem");
  memory_tracker.registerHeap(tmpmem, "TmpMem");
  memory_tracker.registerHeap(inimem, "IniMem");
  memory_tracker.registerHeap(midmem, "MidMed");
  memory_tracker.registerHeap(strmem, "StrMem");
  memory_tracker.registerHeap(uimem, "UiMem");
  memory_tracker.registerHeap(globmem, "GlobMem");
  memory_tracker.registerHeap(scriptmem, "ScriptMem");
  memory_tracker.registerHeap(sourcemem, "SourceMem");

  mem_mgr_inited = true;
  set_memory_thread_safe(true);
}

void memfree_anywhere(void *p) { DagDebugMemAllocator::memFreeAnywhere(p); }

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
extern "C" void mi_collect(bool force);
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

#include "stubRtlAlloc.inc.cpp"

#if defined(__clang__)
void *operator new(std::size_t s) DAG_THROW_BAD_ALLOC { return FINAL_ALLOC(defaultmem)->alloc(s); }
void *operator new(std::size_t s, const std::nothrow_t &) DAG_NOEXCEPT { return FINAL_ALLOC(defaultmem)->tryAlloc(s); }
void *operator new[](std::size_t s) DAG_THROW_BAD_ALLOC { return FINAL_ALLOC(defaultmem)->alloc(s); }
void *operator new[](std::size_t s, const std::nothrow_t &) DAG_NOEXCEPT { return FINAL_ALLOC(defaultmem)->tryAlloc(s); }
void operator delete(void *p) DAG_NOEXCEPT { memfree_anywhere(p); }
void operator delete[](void *p) DAG_NOEXCEPT { memfree_anywhere(p); }
#if (__cplusplus >= 201703 || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703)) && !__has_feature(address_sanitizer)
void *operator new(std::size_t sz, std::align_val_t al) { return FINAL_ALLOC(defaultmem)->allocAligned(sz, (size_t)al); }
void *operator new[](std::size_t sz, std::align_val_t al) { return FINAL_ALLOC(defaultmem)->allocAligned(sz, (size_t)al); }
void operator delete(void *ptr, std::align_val_t) noexcept { return FINAL_ALLOC(defaultmem)->freeAligned(ptr); }
void operator delete[](void *ptr, std::align_val_t) noexcept { return FINAL_ALLOC(defaultmem)->freeAligned(ptr); }
// TODO: aligned nothrow?
#endif
#endif

#if defined(__GNUC__) || defined(__clang__)
static class __MemMgrInit
{
public:
  __MemMgrInit() { real_init_memory(); }
} __mem_init __attribute__((init_priority(101)));

void dagor_force_init_memmgr() { real_init_memory(); }

#else
typedef int(__cdecl *_PIFV)(void);
extern "C" _PIFV __c_dagor_memory_real_init;
extern "C" int __cdecl dagor_memory_real_init(void)
{
  real_init_memory();
  return !__c_dagor_memory_real_init;
}

#pragma warning(disable : 4074)
#pragma init_seg(compiler)
static int __cpp_dagor_memory_real_init = dagor_memory_real_init();

#endif

#if _TARGET_APPLE | _TARGET_PC_LINUX | _TARGET_ANDROID | defined(__clang__)
void *memalloc(size_t sz) { return FINAL_ALLOC(defaultmem)->alloc(sz); }
#endif

#define EXPORT_PULL dll_pull_memory_dagmem
#include <supp/exportPull.h>
