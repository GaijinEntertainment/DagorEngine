#include "stdRtlMemUsage.h"
#ifndef _STD_RTL_MEMORY
#include "dlmalloc-setup.h"

#include <supp/_platform.h>
#include <stdarg.h>
#include <stdio.h>

#include <debug/dag_fatal.h>
#include <memory/dag_mem.h>
#include <debug/dag_debug.h>
#include <osApiWrappers/dag_ttas_spinlock.h>
#include <osApiWrappers/dag_critSec.h>

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
#define assert(a)                                                          \
  {                                                                        \
    if (!(a))                                                              \
    {                                                                      \
      cdebug("assert failed : \"%s\" in %s : %i", #a, __FILE__, __LINE__); \
      DAG_FATAL("assert");                                                 \
      ::terminate_process(13);                                             \
    }                                                                      \
  }
#else
#define assert(a)                                                          \
  {                                                                        \
    if (!(a))                                                              \
    {                                                                      \
      cdebug("assert failed : \"%s\" in %s : %i", #a, __FILE__, __LINE__); \
      DAG_FATAL("assert");                                                 \
    }                                                                      \
  }
#endif
#define MAX_SIZE_T (~(size_t)0)
#define MFAIL      ((void *)(MAX_SIZE_T))

static size_t moreCore_committed = 0, moreCore_committed_max = 0;

#if _TARGET_PC || _TARGET_XBOX
#include "allocStep.h"

#define FIRST_POOL_SZ dagormem_first_pool_sz
#define NEXT_POOL_SZ  dagormem_next_pool_sz
static constexpr int MAX_POOL_NUM = 64;
#endif

static char *reserved[MAX_POOL_NUM] = {NULL};
static char *toCommit = NULL;
static size_t restSize = 0;
static int usedPools = 0;
static bool contiguousMem = true;

// we can't create this critical section deterministically before all compiler objects &
// delete after all compiler objects (init_seg(compiler) do NOT works right!),
// so instead we create it on demand and do not delete it ever (leak it)
static CritSecStorage win32_more_core_crit_sec;
static bool crit_sec_initialized = false;
static void initialize_critsection();

__forceinline WinCritSec *get_win32_more_core_crit_sec()
{
  // no UB
  CritSecStorage *css = &win32_more_core_crit_sec;
  WinCritSec *cs;
  memcpy(&cs, &css, sizeof(void *));
  return cs;
}

class MoreCoreRAIILock
{
public:
  MoreCoreRAIILock()
  {
    if (DAGOR_UNLIKELY(!crit_sec_initialized))
      initialize_critsection();
    enter_critical_section_raw(get_win32_more_core_crit_sec());
  }
  ~MoreCoreRAIILock() { leave_critical_section(get_win32_more_core_crit_sec()); }
};

extern "C" void *win32_more_core(intptr_t size)
{
  MoreCoreRAIILock lock;

  void *result = MFAIL;

  unsigned int flags = 0;

  if (usedPools < 1)
  {
#if _TARGET_PC
#if _TARGET_PC_WIN && !_TARGET_64BIT && defined(FIRST_POOL_SZ)
    MEMORYSTATUSEX statex;
    memset(&statex, 0, sizeof(statex));
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex))
      if ((statex.ullAvailVirtual >> 20) < 2400 && FIRST_POOL_SZ > (384 << 20))
        FIRST_POOL_SZ = (384 << 20);
#endif
    size_t max_sz = FIRST_POOL_SZ;
#if _TARGET_64BIT
    max_sz += NEXT_POOL_SZ * MAX_POOL_NUM;
#else
    max_sz += NEXT_POOL_SZ * 2;
#endif
    if (max_sz < (1600) << 20)
      max_sz = (1600) << 20;
    for (; max_sz >= FIRST_POOL_SZ; max_sz -= 16 << 20)
    {
      reserved[0] = (char *)VirtualAlloc(NULL, max_sz, MEM_RESERVE | MEM_TOP_DOWN | flags, PAGE_READWRITE);

      if (reserved[0])
      {
        VirtualFree(reserved[0], 0, MEM_RELEASE);
        reserved[0] = (char *)VirtualAlloc(reserved[0], FIRST_POOL_SZ, MEM_RESERVE | flags, PAGE_READWRITE);
        break;
      }
    }
#else
    reserved[0] = (char *)VirtualAlloc(NULL, FIRST_POOL_SZ, MEM_RESERVE | flags, PAGE_READWRITE);
#endif
#if _TARGET_PC
    while (!reserved[0] && dagormem_first_pool_sz > (2 << 20))
    {
      dagormem_first_pool_sz >>= 1;
      reserved[0] = (char *)VirtualAlloc(NULL, FIRST_POOL_SZ, MEM_RESERVE | flags, PAGE_READWRITE);
    }
#endif

    assert(reserved[0]);
    // cdebug("MORECORE: reserved %llu\n", uint64_t(FIRST_POOL_SZ));

    toCommit = reserved[0];
    restSize = FIRST_POOL_SZ;
    moreCore_committed = 0;

    usedPools = 1;
  }

  if (size > 0)
  {
    if (!contiguousMem)
      return MFAIL;

    // allocate
    size = (size + 0xFFFF) & ~intptr_t(0xFFFF);

    if (restSize >= size)
    {
      result = VirtualAlloc(toCommit, size, MEM_COMMIT | flags, PAGE_READWRITE);
      if (!result)
      {
        cdebug("MORECORE: cannot(1) commit %llu (commited %llu in %d blocks)", uint64_t(size), uint64_t(moreCore_committed),
          usedPools);
        return MFAIL;
      }
    }
    else if (usedPools < MAX_POOL_NUM)
    {
      size_t committed_rest = restSize;
      if (restSize)
      {
        result = VirtualAlloc(toCommit, restSize, MEM_COMMIT | flags, PAGE_READWRITE);
        if (!result)
        {
          cdebug("MORECORE: cannot(2) commit %llu (commited %llu in %d blocks)", uint64_t(restSize), uint64_t(moreCore_committed),
            usedPools);
          return MFAIL;
        }
        moreCore_committed += restSize;
        size -= restSize;
      }

      void *addr = toCommit + restSize;
      restSize = NEXT_POOL_SZ > size ? NEXT_POOL_SZ : size;

      reserved[usedPools] = (char *)VirtualAlloc(addr, restSize, MEM_RESERVE | flags, PAGE_READWRITE);

      while (!reserved[usedPools] && restSize >= size + (1 << 20))
      {
        restSize -= (1 << 20);
        reserved[usedPools] = (char *)VirtualAlloc(addr, restSize, MEM_RESERVE | flags, PAGE_READWRITE);
      }

      if (!reserved[usedPools])
      {
        if (committed_rest)
        {
          VirtualFree(toCommit, committed_rest, MEM_DECOMMIT);
          moreCore_committed -= committed_rest;
        }
        if (contiguousMem)
        {
          cdebug("MORECORE: cannot reserve contiguous %llu (commited %llu in %d blocks)", uint64_t(restSize),
            uint64_t(moreCore_committed), usedPools);
          contiguousMem = false;
        }
        else
          cdebug("cannot map another %llu bytes\ntotal commited by now: %llu\n"
                 "usedPools=%d first_pool_sz=%llu next_pool_sz=%llu",
            uint64_t(restSize), uint64_t(moreCore_committed), usedPools, uint64_t(FIRST_POOL_SZ), uint64_t(NEXT_POOL_SZ));

        if (committed_rest)
          restSize = committed_rest;
        return MFAIL;
      }

      if (usedPools < 2)
        cdebug("MORECORE: first pool reserved %llu, p[0]=%p\n", uint64_t(FIRST_POOL_SZ), reserved[0]);
      cdebug("MORECORE: +reserved %llu; p[%d]=%p", uint64_t(restSize), usedPools, reserved[usedPools]);

      toCommit = reserved[usedPools];
      usedPools++;

      if (!VirtualAlloc(toCommit, size, MEM_COMMIT | flags, PAGE_READWRITE))
      {
        cdebug("MORECORE: cannot(3) commit %llu (commited %llu in %d blocks)", uint64_t(size), uint64_t(moreCore_committed),
          usedPools);
        return MFAIL;
      }

      char buf[512];
      get_memory_stats(buf, 512);
      cdebug("%s,%d: on morecore\n%s", ". prog\\engine\\memory\\win32MoreCore.cpp", __LINE__, buf);
    }

    toCommit += size;
    restSize -= size;
    moreCore_committed += size;
    if (moreCore_committed > moreCore_committed_max)
      moreCore_committed_max = moreCore_committed;
    // cdebug("MORECORE: commit +%llu; total committed: %llu\n", uint64_t(size), uint64_t(moreCore_committed));
  }
  else if (size == 0)
  {
    // must return one past the end address of memory from previous nonzero call

    result = contiguousMem ? toCommit : MFAIL;
  }
  else if (!contiguousMem)
    return MFAIL;
  else
  {
    char *start_adr = toCommit + size;

    while (usedPools > 0 && toCommit >= start_adr)
    {
      if (reserved[usedPools - 1] >= start_adr)
      {
        VirtualFree(reserved[usedPools - 1], 0, MEM_RELEASE);
        moreCore_committed -= toCommit - reserved[usedPools - 1];
        cdebug("MORECORE: -reserved %llu; p[%d]=%p", uint64_t(toCommit - reserved[usedPools - 1]), usedPools - 1,
          reserved[usedPools - 1]);
        toCommit = reserved[usedPools - 1];
        usedPools--;
        restSize = 0;
      }
      else
      {
#pragma warning(disable : 6250) // warning 6250| Calling 'VirtualFree' without the MEM_RELEASE flag might free memory but not address
                                // descriptors (VADs). This causes address space leaks
        VirtualFree(start_adr, toCommit - start_adr, MEM_DECOMMIT);
        moreCore_committed -= toCommit - start_adr;
        restSize += toCommit - start_adr;
        toCommit = start_adr;
        break;
      }
    }
    result = toCommit;
  }

  return result ? result : MFAIL;
}

DAGOR_NOINLINE
static void initialize_critsection()
{
  ::create_critical_section(win32_more_core_crit_sec);
  crit_sec_initialized = true;
}


extern "C" size_t get_memory_committed() { return moreCore_committed; }
extern "C" size_t get_memory_committed_max() { return moreCore_committed_max; }

#endif
