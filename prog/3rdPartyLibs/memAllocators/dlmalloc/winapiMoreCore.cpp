// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "dlmalloc-setup.h"

#include <supp/_platform.h>
#include <stdarg.h>
#include <stdio.h>

#include <debug/dag_fatal.h>
#include <memory/dag_mem.h>
#include <debug/dag_debug.h>
#include <util/dag_finally.h>

#if _TARGET_PC && DAGOR_DBGLEVEL > 0
#include <osApiWrappers/dag_miscApi.h>
#define TERM_PROC(e) ::terminate_process(e)
#else
#define TERM_PROC(e) ((void)0)
#endif

#define assert(a)                                                          \
  do                                                                       \
  {                                                                        \
    if (!(a))                                                              \
    {                                                                      \
      cdebug("assert failed : \"%s\" in %s : %i", #a, __FILE__, __LINE__); \
      DAG_FATAL("assert");                                                 \
      TERM_PROC(13);                                                       \
    }                                                                      \
  } while (0)

#define MFAIL ((void *)(~(size_t)0))

static size_t moreCore_committed = 0, moreCore_committed_max = 0;

#if _TARGET_PC || _TARGET_XBOX
#include "allocStep.h"

#define FIRST_POOL_SZ dagormem_first_pool_sz
#define NEXT_POOL_SZ  dagormem_next_pool_sz
static constexpr int MAX_POOL_NUM = 64;
#endif

static char *reserved[MAX_POOL_NUM] = {};
static char *toCommit = NULL;
static size_t restSize = 0;
static unsigned usedPools = 0;
static bool contiguousMem = true;

extern "C" void *winapi_more_core(intptr_t size)
{
  void *result = MFAIL;

  if (!usedPools) [[unlikely]]
  {
#if _TARGET_64BIT
    size_t max_sz = FIRST_POOL_SZ + NEXT_POOL_SZ * MAX_POOL_NUM;
    size_t sz_dec = NEXT_POOL_SZ;
#else
    MEMORYSTATUSEX statex;
    memset(&statex, 0, sizeof(statex));
    statex.dwLength = sizeof(statex);
    if (GlobalMemoryStatusEx(&statex))
      if ((statex.ullAvailVirtual >> 20) < 2400 && FIRST_POOL_SZ > (384 << 20))
        FIRST_POOL_SZ = (384 << 20);
    size_t max_sz = FIRST_POOL_SZ + NEXT_POOL_SZ * 2;
    if (max_sz < (1600) << 20)
      max_sz = (1600) << 20;
    size_t sz_dec = 16 << 20;
#endif
    do
    {
      reserved[0] = (char *)VirtualAlloc(NULL, max_sz, MEM_RESERVE | MEM_TOP_DOWN, PAGE_READWRITE);
      if (reserved[0])
      {
        VirtualFree(reserved[0], 0, MEM_RELEASE);
        size_t toResv = max_sz > FIRST_POOL_SZ ? FIRST_POOL_SZ : max_sz;
        if ((reserved[0] = (char *)VirtualAlloc(reserved[0], toResv, MEM_RESERVE, PAGE_READWRITE)))
        {
          FIRST_POOL_SZ = toResv;
          break;
        }
      }
      max_sz = max_sz > FIRST_POOL_SZ ? (max_sz - sz_dec) : (max_sz / 2);
    } while (max_sz > (2 << 20));

    assert(reserved[0]);
    // cdebug("MORECORE: reserved %d K", int(FIRST_POOL_SZ >> 10));

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
      result = VirtualAlloc(toCommit, size, MEM_COMMIT, PAGE_READWRITE);
      if (!result)
      {
        cdebug("MORECORE: cannot(%d) commit %d K (commited %d K in %d blocks)", 1, int(size >> 10), int(moreCore_committed >> 10),
          usedPools);
        return MFAIL;
      }
    }
    else if (usedPools < MAX_POOL_NUM)
    {
      size_t committed_rest = restSize;
      if (restSize)
      {
        result = VirtualAlloc(toCommit, restSize, MEM_COMMIT, PAGE_READWRITE);
        if (!result)
        {
          cdebug("MORECORE: cannot(%d) commit %d K (commited %d K in %d blocks)", 2, int(size >> 10), int(moreCore_committed >> 10),
            usedPools);
          return MFAIL;
        }
        moreCore_committed += restSize;
        size -= restSize;
      }

      void *addr = toCommit + restSize;
      restSize = NEXT_POOL_SZ > size ? NEXT_POOL_SZ : size;

      reserved[usedPools] = (char *)VirtualAlloc(addr, restSize, MEM_RESERVE, PAGE_READWRITE);

      while (!reserved[usedPools] && restSize >= size + (1 << 20))
      {
        restSize -= (1 << 20);
        reserved[usedPools] = (char *)VirtualAlloc(addr, restSize, MEM_RESERVE, PAGE_READWRITE);
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
          cdebug("MORECORE: cannot reserve contiguous %d K (commited %d K in %d blocks)", int(restSize >> 10),
            int(moreCore_committed >> 10), usedPools);
          contiguousMem = false;
        }
        else
          cdebug("cannot map another %d K\ntotal commited by now: %d K\n"
                 "usedPools=%d first_pool_sz=%d K next_pool_sz=%d K",
            int(restSize >> 10), int(moreCore_committed >> 10), usedPools, int(FIRST_POOL_SZ >> 10), int(NEXT_POOL_SZ >> 10));

        if (committed_rest)
          restSize = committed_rest;
        return MFAIL;
      }

      if (usedPools == 1)
        cdebug("MORECORE: first pool reserved %d K; p[0]=%p", int(FIRST_POOL_SZ >> 10), reserved[0]);
      cdebug("MORECORE: +reserved %d K; p[%d]=%p", int(restSize >> 10), usedPools, reserved[usedPools]);

      toCommit = reserved[usedPools++];

      if (!VirtualAlloc(toCommit, size, MEM_COMMIT, PAGE_READWRITE))
      {
        cdebug("MORECORE: cannot(%d) commit %d K (commited %d K in %d blocks)", 3, int(size >> 10), int(moreCore_committed >> 10),
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
    // cdebug("MORECORE: commit +%d K; total committed: %d K", int(size >> 10), int(moreCore_committed >> 10));
  }
  else if (size == 0) // must return one past the end address of memory from previous nonzero call
    result = contiguousMem ? toCommit : MFAIL;
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
        cdebug("MORECORE: -reserved %d K; p[%d]=%p", int((toCommit - reserved[usedPools - 1]) >> 10), usedPools - 1,
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

extern "C" size_t get_memory_committed() { return moreCore_committed; }
extern "C" size_t get_memory_committed_max() { return moreCore_committed_max; }
