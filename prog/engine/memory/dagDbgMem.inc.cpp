//-- to be included from dagmem.cpp --
//--  cannot be compiled separately --
#include <debug/dag_log.h>
#include <stdio.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <generic/dag_sort.h>
#include <generic/dag_staticTab.h>
#include <util/dag_stlqsort.h>
#include <osApiWrappers/dag_dbgStr.h>
#include <stdio.h>

#define DBGMEM_SEPARATE_POOLS 0
static thread_local bool allow_fill_stack = true;

static StdDlmallocAllocator *sourcemem = NULL;
struct DagDebugMemCritSecWrapper
{
  DagDebugMemCritSecWrapper() { enter_memory_critical_section(); }
  ~DagDebugMemCritSecWrapper() { leave_memory_critical_section(); }
};

#if !defined(_STD_RTL_MEMORY)
struct DlMemSeg
{
  char *base;
  size_t size;
  DlMemSeg *next;
};
struct DlMemSpan
{
  char *s;
  char *e;
};
extern "C" const DlMemSeg *dlmainseg();
#endif

class DagDebugMemAllocator final : public IMemAlloc
{
#if _TARGET_64BIT
#if MEM_DEBUGALLOC >= 2
  static constexpr int STACK_UNWIND_LEN = 8;
  static constexpr int FRONT_GUARD_DW_SIZE = 4;
  static constexpr int REAR_GUARD_DW_SIZE = 8;
#else
  static constexpr int STACK_UNWIND_LEN = 9;
  static constexpr int FRONT_GUARD_DW_SIZE = 2;
  static constexpr int REAR_GUARD_DW_SIZE = 0;
#endif
#else
#if MEM_DEBUGALLOC >= 2
  static constexpr int STACK_UNWIND_LEN = 15;
  static constexpr int FRONT_GUARD_DW_SIZE = 3;
  static constexpr int REAR_GUARD_DW_SIZE = 8;
#else
  static constexpr int STACK_UNWIND_LEN = 17;
  static constexpr int FRONT_GUARD_DW_SIZE = 1;
  static constexpr int REAR_GUARD_DW_SIZE = 0;
#endif
#endif

#pragma warning(disable : 94)   // "the size of an array must be greater than zero"
#pragma warning(disable : 4200) // "nonstandard extension used : zero-sized array in struct/union"
  struct DebugChunk
  {
    void *allocatedPtr;
    int generation;
#if _TARGET_64BIT
    int _resv;
#endif
    size_t dataSize;
    void *stack[STACK_UNWIND_LEN];
    IMemAlloc *allocator;
    DebugChunk *prev, *next;
    unsigned frontGuard[FRONT_GUARD_DW_SIZE];
    char data[0];

    int checkGuards()
    {
#ifndef __clang__
      G_STATIC_ASSERT((offsetof(DebugChunk, data) & 15) == 0);
#endif
      G_STATIC_ASSERT((sizeof(*this) & 15) == 0);
      // this must be in dynamic memory and QWORD-aligned
#if _TARGET_PC && (_MSC_VER >= 1400 || (_TARGET_PC_MACOSX | _TARGET_PC_LINUX))
      if ((intptr_t(this) < 0x100000 && intptr_t(this) >= 0) || // handle /LARGEADDRESSAWARE switch
          (intptr_t(this) & 0x7) != 0)
        return -1;
#else
      if (intptr_t(this) < 0x100000 || (intptr_t(this) & 0x7) != 0)
        return -1;
#endif
      // check sizes
      if (sourcemem->getSize(allocatedPtr) != totalSize(dataSize) + getAlignOfs())
        return -3;
      // check generation
      if (generation < 0x100)
        return -4;

      // check front guards
      for (int i = 0; i < FRONT_GUARD_DW_SIZE; i++)
        if (frontGuard[i] != 0xAAF0550F)
          return -5;

      if (MEM_DEBUGALLOC >= 2)
      {
        // check rear guards
        unsigned *rearGuard = reinterpret_cast<unsigned *>(data + dataSize);
        for (int i = 0; i < REAR_GUARD_DW_SIZE; i++)
          if (rearGuard[i] != 0xAA55AA55)
            return -6;
      }

      // checks done; seems to be correct
      return 1;
    }

    static size_t totalSize(size_t sz) { return sizeof(DebugChunk) + sz + REAR_GUARD_DW_SIZE * 4; }
    size_t getAlignOfs() const { return uintptr_t(this) - uintptr_t(allocatedPtr); }

    static DebugChunk *alloc(size_t sz, int gen)
    {
      G_STATIC_ASSERT((sizeof(DebugChunk) & 0xf) == 0);
      leave_memory_critical_section();
      DebugChunk *chunk = (DebugChunk *)sourcemem->alloc(totalSize(sz));
      chunk->allocatedPtr = chunk;
      enter_memory_critical_section();
      chunk->init(gen);
      return chunk;
    }

    static DebugChunk *allocAligned(size_t sz, int gen, size_t alignment)
    {
      if (!alignment)
        alignment = 4096;
      G_ASSERTF((alignment & (alignment - 1)) == 0, "alignment=%d", alignment);
      leave_memory_critical_section();
      void *ptr = sourcemem->alloc(totalSize(sz) + alignment - 1);
      DebugChunk *chunk =
        (DebugChunk *)(void *)(((uintptr_t(ptr) + sizeof(DebugChunk) + alignment - 1) & ~(alignment - 1)) - sizeof(DebugChunk));
      chunk->allocatedPtr = ptr;
      enter_memory_critical_section();
      chunk->init(gen);
      return chunk;
    }

    static DebugChunk *tryAlloc(size_t sz, int gen)
    {
      leave_memory_critical_section();
      DebugChunk *chunk = (DebugChunk *)sourcemem->tryAlloc(totalSize(sz));
      enter_memory_critical_section();
      if (chunk)
      {
        chunk->allocatedPtr = chunk;
        chunk->init(gen);
      }
      return chunk;
    }

    static DebugChunk *getChunk(void *dataptr)
    {
      if (!dataptr)
        return NULL;
      return (DebugChunk *)(reinterpret_cast<char *>(dataptr) - sizeof(DebugChunk));
    }

    static void destroyChunk(DebugChunk *dc)
    {
      dc->term();
      leave_memory_critical_section();
      sourcemem->free(dc->allocatedPtr);
      enter_memory_critical_section();
    }
    static bool resizeInplace(DebugChunk *dc, size_t sz)
    {
      leave_memory_critical_section();
      bool success = sourcemem->resizeInplace(dc->allocatedPtr, dc->getAlignOfs() + totalSize(sz));
      enter_memory_critical_section();
      if (success)
        dc->onResize();
      return success;
    }
    static DebugChunk *realloc(DebugChunk *dc, size_t sz)
    {
      leave_memory_critical_section();
      if (dc && dc->getAlignOfs() > 0)
      {
        DebugChunk *ndc = alloc(sz, dc->generation);
        if (ndc)
          memcpy(ndc->data, dc->data, min(dc->dataSize, sz));
        dc->term();
        sourcemem->free(dc->allocatedPtr);
        enter_memory_critical_section();
        return ndc;
      }
      dc = (DebugChunk *)sourcemem->realloc(dc, totalSize(sz));
      dc->allocatedPtr = dc;
      enter_memory_critical_section();
      if (dc)
        dc->onResize();
      return dc;
    }

  protected:
    void init(int gen)
    {
      generation = gen;
      allocator = NULL;
      dataSize = sourcemem->getSize(allocatedPtr) - getAlignOfs() - sizeof(DebugChunk) - REAR_GUARD_DW_SIZE * 4;
      memset_dw(frontGuard, 0xAAF0550F, FRONT_GUARD_DW_SIZE);
      if (MEM_DEBUGALLOC >= 2)
        memset_dw(data + dataSize, 0xAA55AA55, REAR_GUARD_DW_SIZE);
    }
    void term()
    {
      generation = -1;
      dataSize = -1;
      memset_dw(frontGuard, 0x01010101, FRONT_GUARD_DW_SIZE);
      if (MEM_DEBUGALLOC >= 2)
        memset_dw(data + dataSize, 0x10101010, REAR_GUARD_DW_SIZE);
    }
    void onResize()
    {
      size_t new_size = sourcemem->getSize(allocatedPtr) - getAlignOfs() - sizeof(DebugChunk) - REAR_GUARD_DW_SIZE * 4;
      if (new_size == dataSize)
        return;
      dataSize = new_size;
      if (MEM_DEBUGALLOC >= 2)
        memset_dw(data + dataSize, 0xAA55AA55, REAR_GUARD_DW_SIZE);
    }

  private:
    DebugChunk(const DebugChunk &);
    DebugChunk &operator=(DebugChunk &);
  };
#pragma warning(default : 94) // "the size of an array must be greater than zero"

  static int generation;
  DebugChunk *ptrList;
  int ptrNum; // cached statistic
  void *checkStack[STACK_UNWIND_LEN];

public:
  static bool thoroughChecks;

public:
  DagDebugMemAllocator(const char * /*pool_name*/)
  {
    ptrList = NULL;
    ptrNum = 0;
    memset(checkStack, 0xFF, sizeof(void *) * STACK_UNWIND_LEN);
    // out_debug_str_fmt ( "DagDebugMemAllocator ctor" );
  }
  ~DagDebugMemAllocator() {}

  // IMemAlloc interface implementation
  virtual void destroy() { delete this; }
  virtual bool isEmpty() { return false; }
  virtual void *alloc(size_t sz)
  {
    DebugChunk *chunk;
    {
      DagDebugMemCritSecWrapper critSecLock;
      if (MEM_DEBUGALLOC >= 5)
        out_debug_str_fmt("alloc %llu", sz);
      integrityCheck();

      chunk = DebugChunk::alloc(sz, generation);
      registerPtr(chunk);
    }
    fillStack(chunk->stack);
    return chunk->data;
  }
  virtual void *tryAlloc(size_t sz)
  {
    DebugChunk *chunk;
    {
      DagDebugMemCritSecWrapper critSecLock;
      if (MEM_DEBUGALLOC >= 5)
        out_debug_str_fmt("tryAlloc %llu", sz);
      integrityCheck();

      chunk = DebugChunk::tryAlloc(sz, generation);
      if (!chunk)
        return NULL;
      registerPtr(chunk);
    }
    fillStack(chunk->stack);
    return chunk->data;
  }
  virtual void *allocAligned(size_t sz, size_t alignment)
  {
    DebugChunk *chunk;
    {
      DagDebugMemCritSecWrapper critSecLock;
      if (MEM_DEBUGALLOC >= 5)
        out_debug_str_fmt("allocAligned %llu, a=%d", sz, alignment);
      integrityCheck();

      chunk = DebugChunk::allocAligned(sz, generation, alignment);
      registerPtr(chunk);
    }
    fillStack(chunk->stack);
    return chunk->data;
  }
  virtual void free(void *p)
  {
    DagDebugMemCritSecWrapper critSecLock;
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("free %p", p);
    integrityCheck();

    if (!p)
      return;
    if (!assurePtrValid(p, "free"))
      return;
    DebugChunk *chunk = DebugChunk::getChunk(p);
    unregisterPtr(chunk);
    DebugChunk::destroyChunk(chunk);
  }
  virtual void freeAligned(void *p) { free(p); }
  virtual size_t getSize(void *p)
  {
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("getsize %p", p);
    integrityCheck();

    if (!p)
      return 0;
    if (!assurePtrValid(p, "getSize"))
      return 0;
    return DebugChunk::getChunk(p)->dataSize;
  }
  virtual bool resizeInplace(void *p, size_t sz)
  {
    if (!p)
      return (sz == 0);
    DagDebugMemCritSecWrapper critSecLock;
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("resizeInplace %p, %llu", p, sz);
    integrityCheck();

    if (!assurePtrValid(p, "resizeInplace"))
      return false;
    return DebugChunk::resizeInplace(DebugChunk::getChunk(p), sz);
  }
  virtual void *realloc(void *p, size_t sz)
  {
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("realloc %p, %llu", p, sz);
    if (!p)
      return DagDebugMemAllocator::alloc(sz);
    if (p && !sz)
    {
      DagDebugMemAllocator::free(p);
      return NULL;
    }

    DagDebugMemCritSecWrapper critSecLock;
    integrityCheck();

    if (!assurePtrValid(p, "realloc"))
      return NULL;
    DebugChunk *chunk = DebugChunk::getChunk(p);
    unregisterPtr(chunk);
    chunk = DebugChunk::realloc(chunk, sz);
    registerPtr(chunk);
    return chunk->data;
  }

  static void memFreeAnywhere(void *p)
  {
    enter_memory_critical_section();
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("free_any %p", p);

    if (!p)
    {
      leave_memory_critical_section();
      return;
    }

    DebugChunk *dc = DebugChunk::getChunk(p);

    int chk = dc->checkGuards();
    if (chk != 1)
    {
      reportInvalidPointer(p, chk, 1, "free_any");
      leave_memory_critical_section();
      return;
    }
    static_cast<DagDebugMemAllocator *>(dc->allocator)->integrityCheck();
    leave_memory_critical_section();
    dc->allocator->free(p);
  }

  // support for public interface
  static const char *getPtrAllocCallStack(void *p)
  {
    DebugChunk *dc = DebugChunk::getChunk(p);
    int chk = dc->checkGuards();
    if (chk != 1)
      return "invalid ptr";
    return getCallStack(dc->stack);
  }

  static bool checkPtr(void *p, int *out_chk)
  {
    if (!p)
      return false;

    DebugChunk *dc = DebugChunk::getChunk(p);
    int chk = dc->checkGuards();
    if (chk != 1)
    {
      if (out_chk)
        *out_chk = chk;
      return false;
    }
    if (static_cast<DagDebugMemAllocator *>(dc->allocator)->findPtr(dc))
      return true;

    if (out_chk)
      *out_chk = -1000;
    return false;
  }

  static void nextGeneration()
  {
    generation++;
    if (generation < 0x100)
      generation = 0x100;
  }

  bool checkAllPtrs(const char *pool_name, bool /*only_cur_gen*/)
  {
    DebugChunk *dc = ptrList;
    int idx = 0, num = 0;
    size_t size = 0;

    (void)pool_name;
    debug(".. pool %s: checking all pointers (%d)", pool_name, ptrNum);
    while (dc)
    {
      int chk = isPtrValid(dc->data);
      if (chk != 1)
      {
#if MEM_DEBUGALLOC >= 3
        reportInvalidPointer(dc->data, chk, 1, "checkMem");
#endif
        debug("  invalid pointer #%d: %p (chk=%d)", idx, dc->data, chk);
        debug_flush(false);
        return false;
      }
      size += dc->dataSize;
      num++;
      dc = dc->next;
      idx++;
    }
    if (num != ptrNum)
    {
      debug("  inconsistent number of pointers: %d != %d", num, ptrNum);
      debug_flush(false);
      return false;
    }
    debug("all pointers are valid (total size: %llu)", size);
    debug_flush(false);
    return true;
  }

  size_t dumpAllPtrs(const char *pool_name, bool only_cur_gen)
  {
    Tab<DebugChunk *> ptrs(sourcemem);
    DebugChunk *dc = ptrList;
    uint32_t idx = 0;

    (void)pool_name;
    ptrs.reserve(ptrNum);

    debug(".. pool %s: dumping all pointers (%d)", pool_name, ptrNum);
    while (dc)
    {
      int chk = isPtrValid(dc->data);
      if (chk != 1)
      {
#if MEM_DEBUGALLOC >= 3
        reportInvalidPointer(dc->data, chk, 1, "dumpPtrs");
#endif
        debug("  invalid pointer #%d: %p (chk=%d)", idx, dc->data, chk);
        break;
      }
      if (!only_cur_gen || dc->generation == generation)
        ptrs.push_back(dc);
      dc = dc->next;
      idx++;
    }

    debug(".. sorting pointers (%d)", ptrs.size());
    sort(ptrs, &sort_chunks_stack_size);
    struct PtrsAndSize
    {
      uint32_t idx = 0, num = 0;
      size_t sz = 0;
    };
    Tab<PtrsAndSize> ptrsSized(sourcemem);

    size_t used_mem = 0;
    for (idx = 0; idx < ptrs.size();)
    {
      size_t total_size = ptrs[idx]->dataSize;
      uint32_t total_num = 1;
      for (int j = idx + 1; j < ptrs.size(); j++)
        if (memcmp(ptrs[idx]->stack, ptrs[j]->stack, sizeof(ptrs[idx]->stack)) == 0)
        {
          total_size += ptrs[j]->dataSize;
          total_num++;
        }
        else
          break;
      ptrsSized.push_back(PtrsAndSize{uint32_t(idx), total_num, total_size});
      idx += total_num;
      used_mem += total_size;
    }
    stlsort::sort(ptrsSized.begin(), ptrsSized.end(), [](auto &a, auto &b) { return b.sz < a.sz; });

    for (auto &piSz : ptrsSized)
    {
      debug(">>>> allocated %llu bytes in %d pointers", piSz.sz, piSz.num);
      debug("\n  %s", getCallStack(ptrs[piSz.idx]->stack));
    }

    debug("\n===> overall memory used: %llu", used_mem);
    debug_flush(false);
    return used_mem;
  }
  void dumpUsedMemAndGaps(bool summary_only)
  {
    StaticTab<DlMemSpan, 32> seg;
    size_t used_mem = 0, used_mem_mmap = 0, used_ptr_mmap = 0, span_mem = 0;
    for (auto *s = dlmainseg(); s; s = s->next)
    {
      seg.push_back({s->base, s->base + s->size});
      span_mem += s->size;
    }
    sort(seg, &sort_seg_ascend);
    const DlMemSpan *cur_seg = seg.cbegin();

    Tab<DebugChunk *> ptrs(sourcemem);
    ptrs.reserve(ptrNum);

    debug(".. pool: dumping pointers (%d) and gaps", ptrNum);
    debug(".. pool: used %d segments (total %dM)", seg.size(), span_mem >> 20);
    // for (auto &s : seg)
    //   debug("  segment %p..%p  %dK (%dM)", s.s, s.e, (s.e-s.s)>>10, (s.e-s.s)>>20);

    DebugChunk *dc = ptrList;
    for (unsigned idx = 0; dc; ++idx, dc = dc->next)
    {
      int chk = isPtrValid(dc->data);
      if (chk != 1)
      {
        debug("  invalid pointer #%d: %p (chk=%d)", idx, dc->data, chk);
        break;
      }
      ptrs.push_back(dc);
    }

    sort(ptrs, &sort_chunks_addr);
    if (!summary_only)
      debug("%d pointers in range %p..%p (%uM of %uM), ", ptrs.size(), (char *)ptrs.front()->allocatedPtr,
        (char *)ptrs.back()->allocatedPtr + DebugChunk::totalSize(ptrs.back()->dataSize), span_mem >> 20,
        ((char *)ptrs.back()->allocatedPtr - (char *)ptrs.front()->allocatedPtr + DebugChunk::totalSize(ptrs.back()->dataSize)) >> 20);

    struct
    {
      void *s = nullptr, *e = nullptr;
      size_t ptrs = 0, sz = 0, minSz = 0, maxSz = 0;
    } chain;
    size_t gaps = 0, gapTotalSz = 0, maxGapSz = 0, mmapGaps = 0, mmapGapTotalSz = 0;
    auto dump_chain = [&chain, summary_only]() {
      if (!summary_only)
      {
        if (chain.ptrs > 1)
          debug("    === %p..%p (total %6u bytes in %u ptrs, minSz=%u, maxSz=%u, avgSz=%u)", chain.s, chain.e, chain.sz, chain.ptrs,
            chain.minSz, chain.maxSz, chain.sz / chain.ptrs);
        else
          debug("    === %p..%p (total %6u bytes in 1 ptr)", chain.s, chain.e, chain.sz);
      }
      chain.s = nullptr;
    };
    auto dump_gap = [&chain, summary_only](const void *ep) {
      size_t gap_sz = (char *)ep - (char *)chain.e;
      if (!summary_only)
        debug("GAP === %p..%p (%6u bytes)", chain.e, ep, gap_sz);
      return gap_sz;
    };

#if _TARGET_64BIT
    static constexpr int PTR_GAP = 16;
#else
    static constexpr int PTR_GAP = 8;
#endif
    for (auto &p : ptrs)
    {
      bool mmap_ptr = cur_seg == nullptr;
      used_mem += p->dataSize;
      if (!mmap_ptr)
      {
        if ((char *)p->allocatedPtr < cur_seg->s)
          mmap_ptr = true;
        else if ((char *)p->allocatedPtr > cur_seg->e)
        {
          if (chain.s)
          {
            dump_chain();
            gapTotalSz += dump_gap(cur_seg->e) - PTR_GAP;
            gaps++;
          }
          if (!summary_only)
            debug("    --- end of segment (%p..%p)}", cur_seg->s, cur_seg->e);
          if (++cur_seg == seg.end())
            cur_seg = nullptr;
          if (!cur_seg || (char *)p->allocatedPtr < cur_seg->s)
            mmap_ptr = true;
        }
      }

      if (mmap_ptr)
      {
        chain.s = (char *)p->allocatedPtr;
        chain.sz = DebugChunk::totalSize(p->dataSize);
        chain.e = (char *)p->allocatedPtr + DebugChunk::totalSize(p->dataSize);
        void *page_end = (void *)((size_t(chain.e) + 0xFFF) & ~0xFFF);
        size_t gap_sz = (char *)page_end - (char *)chain.e;
        gap_sz += size_t(chain.s) & 0xFFF;
        if (!summary_only)
          debug("    === %p..%p (total %6u bytes in 1 ptr), mmap chunk, GAP=%d bytes", chain.s, chain.e, chain.sz, gap_sz);
        gaps++;
        gapTotalSz += gap_sz;
        mmapGaps++;
        mmapGapTotalSz += gap_sz;
        used_mem_mmap += chain.sz;
        used_ptr_mmap++;
        span_mem += chain.sz + gap_sz;

        chain.s = nullptr;
        continue;
      }

      if (!chain.s && (char *)p->allocatedPtr >= cur_seg->s && (char *)p->allocatedPtr < cur_seg->e)
      {
        chain.e = cur_seg->s;
        if (!summary_only)
          debug("    --- start of segment (%p..%p){", cur_seg->s, cur_seg->e);
        if ((char *)p->allocatedPtr > cur_seg->s + PTR_GAP * 2)
        {
          gapTotalSz += dump_gap(p->allocatedPtr) - PTR_GAP;
          gaps++;
        }
      }

      if (chain.s && (char *)p->allocatedPtr <= (char *)chain.e + PTR_GAP)
      {
        chain.e = (char *)p->allocatedPtr + DebugChunk::totalSize(p->dataSize);
        chain.ptrs++;
        chain.sz += p->dataSize;
        if (chain.minSz > p->dataSize)
          chain.minSz = p->dataSize;
        if (chain.maxSz < p->dataSize)
          chain.maxSz = p->dataSize;
      }
      else if (chain.s)
      {
        dump_chain();
        size_t gap_sz = dump_gap(p->allocatedPtr) - PTR_GAP;
        gaps++;
        gapTotalSz += gap_sz;
        if (maxGapSz < gap_sz)
          maxGapSz = gap_sz;
      }

      if (!chain.s)
      {
        chain.s = p->allocatedPtr;
        chain.e = (char *)p->allocatedPtr + DebugChunk::totalSize(p->dataSize);
        chain.ptrs = 1;
        chain.sz = chain.minSz = chain.maxSz = p->dataSize;
      }
    }
    if (chain.s)
    {
      dump_chain();
      if (cur_seg && (char *)chain.e + PTR_GAP * 2 < cur_seg->e)
      {
        gapTotalSz += dump_gap(cur_seg->e) - PTR_GAP;
        gaps++;
      }
      if (cur_seg && !summary_only)
        debug("    --- end of segment (%p..%p)}", cur_seg->s, cur_seg->e);
    }
    if (span_mem)
      debug("%uK (%uM) in %d GAPs (%.2f%% mem wasted, maxGapSz=%u); %uK (%uM) in %u pointers (effective span %dM)", gapTotalSz >> 10,
        gapTotalSz >> 20, gaps, gapTotalSz * 100.f / span_mem, maxGapSz, used_mem >> 10, used_mem >> 20, ptrs.size(), span_mem >> 20);
    if (gaps && mmapGaps)
      debug("%u bytes in %d mmap GAPs (%.2f%% of total GAPs and %.2f%% of total mem); %uK (%uM) in %u mmap pointers", mmapGapTotalSz,
        mmapGaps, mmapGapTotalSz * 100.f / gapTotalSz, mmapGapTotalSz * 100.f / span_mem, used_mem_mmap >> 10, used_mem_mmap >> 20,
        used_ptr_mmap);
    if (gaps > mmapGaps)
      debug("%uK (%uM) bytes in %d core GAPs (%.2f%% of total GAPs and %.2f%% of total mem); %uK (%uM) in %u core pointers",
        (gapTotalSz - mmapGapTotalSz) >> 10, (gapTotalSz - mmapGapTotalSz) >> 20, gaps - mmapGaps,
        (gapTotalSz - mmapGapTotalSz) * 100.f / gapTotalSz, (gapTotalSz - mmapGapTotalSz) * 100.f / span_mem,
        (used_mem - used_mem_mmap) >> 10, (used_mem - used_mem_mmap) >> 20, ptrs.size() - used_ptr_mmap);
  }

  struct PointerGroup
  {
    int idx;
    int num;
    size_t totalSize;
  };

  size_t dumpLeaks(const char *pool_name, bool only_cur_gen)
  {
    Tab<DebugChunk *> ptrs(sourcemem);
    DebugChunk *dc = ptrList;
    int idx = 0;

    (void)pool_name;
    ptrs.reserve(ptrNum);

    debug(".. pool %s: dumping leaks (%d ptrs)", pool_name, ptrNum);

    while (dc)
    {
      int chk = isPtrValid(dc->data);
      if (chk != 1)
      {
#if MEM_DEBUGALLOC >= 3
        reportInvalidPointer(dc->data, chk, 1, "dumpLeaks");
#endif
        debug("  invalid pointer #%d: %p (chk=%d)", idx, dc->data, chk);
        break;
      }
      if (!only_cur_gen || dc->generation == generation)
        ptrs.push_back(dc);
      dc = dc->next;
      idx++;
    }

    debug(".. sorting pointers (%d)", ptrs.size());
    sort(ptrs, &sort_chunks_stack_size);

    Tab<PointerGroup> pg(sourcemem);
    pg.reserve(ptrs.size());

    for (idx = 0; idx < ptrs.size();)
    {
      size_t total_size = ptrs[idx]->dataSize;
      int total_num = 1;

      for (int j = idx + 1; j < ptrs.size(); j++)
        if (memcmp(ptrs[idx]->stack, ptrs[j]->stack, sizeof(ptrs[idx]->stack)) == 0)
        {
          total_size += ptrs[j]->dataSize;
          total_num++;
        }
        else
          break;

      PointerGroup _pg = {idx, total_num, total_size};
      pg.push_back(_pg);
      idx += total_num;
    }

    debug(".. sorting pointer groups (%d)", pg.size());
    sort(pg, &sort_pg_total_size);
    size_t used_mem = 0;
    for (idx = 0; idx < pg.size(); idx++)
    {
      used_mem += pg[idx].totalSize;
      debug(">>>> allocated %llu bytes in %d pointers; %s", pg[idx].totalSize, pg[idx].num, getCallStack(ptrs[pg[idx].idx]->stack));
    }
    debug("\n===> overall memory used: %llu", used_mem);
    debug_flush(false);
    return used_mem;
  }

  size_t writeRawDump(FILE *fd, uint32_t &numChunks)
  {
    DebugChunk *dc = ptrList;
    size_t ret = 0;
    int numa = 0;
    while (dc)
    {
      // if (isPtrValid(dc->data))
      {
        const size_t s = sourcemem->getSize(dc->allocatedPtr) - dc->getAlignOfs();
        ret += s;
        fwrite(dc, s, 1, fd);
        ++numa;
      }
      dc = dc->next;
    }
    numChunks = numa;
    return ret;
  }

  static const char *getCallStack(void **stack)
  {
    static char stk[8192];
    return stackhlp_get_call_stack(stk, sizeof(stk), stack, STACK_UNWIND_LEN);
  }

protected:
  __forceinline void fillStack(void **stack, int skip = 1)
  {
    if (STACK_UNWIND_LEN)
    {
      if (allow_fill_stack)
        stackhlp_fill_stack(stack, STACK_UNWIND_LEN, skip);
      else
        stack[0] = (void *)(~uintptr_t(0));
    }
  }

  int isPtrValid(void *dataptr)
  {
    DebugChunk *dc = DebugChunk::getChunk(dataptr);
    int chk = dc->checkGuards();
    if (chk != 1)
      return chk;

    if (dc->allocator != this)
      return -1000;

    return findPtr(dc);
  }

  bool assurePtrValid(void *dataptr, const char *label)
  {
    int chk = isPtrValid(dataptr);
    if (chk != 1)
    {
      reportInvalidPointer(dataptr, chk, 1, label);
      return false;
    }
    return true;
  }

  static void reportInvalidPointer(void *dataptr, int chk, int skip_frames, const char *label)
  {
    out_debug_str_fmt("[ERR] ptr not valid p=%p chk=%d  %s", dataptr, chk, label);
    G_UNUSED(skip_frames);
#if MEM_DEBUGALLOC >= 4
    __asm mov eax, chk;
    __asm mov ebx, dataptr;
    __asm ud2;
    G_UNUSED(label);
#else
    if (chk <= -6)
      debug("invalid ptr %p, allocated: %s", dataptr, getCallStack(DebugChunk::getChunk(dataptr)->stack));
    DAG_FATAL("invalid pointer=%p in %s (chk=%d)", dataptr, label, chk);
#endif
  }

  void registerPtr(DebugChunk *dc)
  {
    dc->allocator = this;
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("[regPtr] dc=%p, p=%p ptrNum=%d", dc, dc->data, ptrNum);

    if (!ptrList)
    {
      dc->next = NULL;
      dc->prev = NULL;
      ptrList = dc;
      ptrNum = 1;
      return;
    }
    ptrList->prev = dc;
    dc->prev = NULL;
    dc->next = ptrList;
    ptrList = dc;
    ptrNum++;
  }

#if MEM_DEBUGALLOC < 4
  bool findPtr(DebugChunk *) { return true; }
#else
  bool findPtr(DebugChunk *dc)
  {

    DebugChunk *l = ptrList;
    while (l)
    {
      if (l == dc)
        return true;
      l = l->next;
    }

    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("[findPtr] dc=%p not found! ptrList=%p ptrNum=%d", dc, ptrList, ptrNum);

    return false;
  }
#endif

  void unregisterPtr(DebugChunk *dc)
  {
    if (MEM_DEBUGALLOC >= 5)
      out_debug_str_fmt("[unregPtr] dc=%p, p=%p ptrNum=%d", dc, dc->data, ptrNum);

    if (dc == ptrList)
    {
      ptrList = ptrList->next;
    }
    else
    {
      if (dc->prev)
        dc->prev->next = dc->next;
      if (dc->next)
        dc->next->prev = dc->prev;
    }
    ptrNum--;
  }

  inline void integrityCheck()
  {
    if (!thoroughChecks)
      return;

    DebugChunk *dc = ptrList;
    int idx = 0, num = 0;

    while (dc)
    {
      int chk = isPtrValid(dc->data);
      if (chk != 1)
      {
        debug("last good memmgr call: %s", getCallStack(checkStack));
#if MEM_DEBUGALLOC >= 3
        reportInvalidPointer(dc->data, chk, 1, "integrityCheck");
#endif
        debug("  invalid pointer #%d: %p (chk=%d)", idx, dc->data, chk);
        if (chk <= -6)
          debug("  %s", getCallStack(dc->stack));
        debug_flush(false);
        DAG_FATAL("invalid pointer");
        return;
      }
      num++;
      dc = dc->next;
      idx++;
    }
    if (num != ptrNum)
    {
      debug("  inconsistent number of pointers: %d != %d", num, ptrNum);
      debug_flush(false);
      DAG_FATAL("inconsistent number of pointers: %d != %d", num, ptrNum);
      return;
    }

    fillStack(checkStack);
  }

  static int sort_chunks_stack_size(DebugChunk *const *a, DebugChunk *const *b)
  {
    int ret = memcmp(a[0]->stack, b[0]->stack, sizeof(a[0]->stack));
    if (!ret)
      ret = int(b[0]->dataSize - a[0]->dataSize);
    return ret;
  }
  static int sort_pg_total_size(PointerGroup const *a, PointerGroup const *b) { return int(b->totalSize - a->totalSize); }
  static int sort_chunks_addr(DebugChunk *const *a, DebugChunk *const *b) { return *a > *b ? 1 : *a < *b ? -1 : 0; }
  static int sort_seg_ascend(DlMemSpan const *a, DlMemSpan const *b) { return a->s > b->s ? 1 : a->s < b->s ? -1 : 0; }
};


int DagDebugMemAllocator::generation = 0x100;
bool DagDebugMemAllocator::thoroughChecks = false;

//
// public interface implementation
//
const char *DagDbgMem::get_ptr_allocation_call_stack(void *ptr) { return DagDebugMemAllocator::getPtrAllocCallStack(ptr); }
bool DagDbgMem::check_ptr(void *ptr, int *out_chk) { return DagDebugMemAllocator::checkPtr(ptr, out_chk); }

void DagDbgMem::next_generation() { DagDebugMemAllocator::nextGeneration(); }

bool DagDbgMem::check_memory(bool only_cur_gen)
{
  DagDebugMemCritSecWrapper critSecLock;
  bool ok = true;
#if DBGMEM_SEPARATE_POOLS
  debug("\n== PtrCheck =1==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(midmem)->checkAllPtrs("MidMem", only_cur_gen);
  debug("\n== PtrCheck =2==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(tmpmem)->checkAllPtrs("TmpMem", only_cur_gen);
  debug("\n== PtrCheck =3==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(scriptmem)->checkAllPtrs("ScriptMem", only_cur_gen);
  debug("\n== PtrCheck =4==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(strmem)->checkAllPtrs("StrMem", only_cur_gen);
  debug("\n== PtrCheck =5==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(globmem)->checkAllPtrs("GlobMem", only_cur_gen);
  debug("\n== PtrCheck =6==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(stdmem)->checkAllPtrs("StdMem", only_cur_gen);
  debug("\n== PtrCheck =7==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(uimem)->checkAllPtrs("UiMem", only_cur_gen);
  debug("\n== PtrCheck =8==============================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(inimem)->checkAllPtrs("IniMem", only_cur_gen);
#else
  debug("\n== PtrCheck ================================================================");
  ok = ok && static_cast<DagDebugMemAllocator *>(stdmem)->checkAllPtrs("StdMem", only_cur_gen);
#endif
  debug("\n== EndOfPtrDump ===========================================================");
  return ok;
}

void DagDbgMem::dump_all_used_ptrs(bool only_cur_gen)
{
  DagDebugMemCritSecWrapper critSecLock;
  size_t used_mem = 0;
#if DBGMEM_SEPARATE_POOLS
  debug("\n== PtrDump =1==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(midmem)->dumpAllPtrs("MidMem", only_cur_gen);
  debug("\n== PtrDump =2==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(tmpmem)->dumpAllPtrs("TmpMem", only_cur_gen);
  debug("\n== PtrDump =3==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(scriptmem)->dumpAllPtrs("ScriptMem", only_cur_gen);
  debug("\n== PtrDump =4==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(strmem)->dumpAllPtrs("StrMem", only_cur_gen);
  debug("\n== PtrDump =5==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(globmem)->dumpAllPtrs("GlobMem", only_cur_gen);
  debug("\n== PtrDump =6==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(stdmem)->dumpAllPtrs("StdMem", only_cur_gen);
  debug("\n== PtrDump =7==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(uimem)->dumpAllPtrs("UiMem", only_cur_gen);
  debug("\n== PtrDump =8==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(inimem)->dumpAllPtrs("IniMem", only_cur_gen);
#else
  debug("\n== PtrDump ================================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(stdmem)->dumpAllPtrs("StdMem", only_cur_gen);
#endif
  debug("\n== EndOfPtrDump: overall memory used: %12d=========================", used_mem);
}
void DagDbgMem::dump_used_mem_and_gaps(bool summary_only)
{
  DagDebugMemCritSecWrapper critSecLock;
  debug("\n== Ptrs/Gaps Dump ==========================================================");
  static_cast<DagDebugMemAllocator *>(stdmem)->dumpUsedMemAndGaps(summary_only);
}

void DagDbgMem::dump_leaks(bool only_cur_gen)
{
  DagDebugMemCritSecWrapper critSecLock;
  size_t used_mem = 0;
#if DBGMEM_SEPARATE_POOLS
  debug("\n== LeaksDump =1==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(midmem)->dumpLeaks("MidMem", only_cur_gen);
  debug("\n== LeaksDump =2==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(tmpmem)->dumpLeaks("TmpMem", only_cur_gen);
  debug("\n== LeaksDump =3==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(scriptmem)->dumpLeaks("ScriptMem", only_cur_gen);
  debug("\n== LeaksDump =4==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(strmem)->dumpLeaks("StrMem", only_cur_gen);
  debug("\n== LeaksDump =5==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(globmem)->dumpLeaks("GlobMem", only_cur_gen);
  debug("\n== LeaksDump =6==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(stdmem)->dumpLeaks("StdMem", only_cur_gen);
  debug("\n== LeaksDump =7==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(uimem)->dumpLeaks("UiMem", only_cur_gen);
  debug("\n== LeaksDump =8==============================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(inimem)->dumpLeaks("IniMem", only_cur_gen);
#else
  debug("\n== LeaksDump ================================================================");
  used_mem += static_cast<DagDebugMemAllocator *>(stdmem)->dumpLeaks("StdMem", only_cur_gen);
#endif
  debug("\n== EndOfLeakDump: overall memory used: %12d=========================", used_mem);
}

void DagDbgMem::dump_raw_heap(const char *fn)
{
  FILE *f = fopen(fn, "w");
  if (!f)
    return;
  DagDebugMemAllocator **m = (DagDebugMemAllocator **)&stdmem;
  size_t tm = 0, tc = 0;
  uint32_t c;
  for (int i = 0; i < 8; ++i) // kinda hack
  {
    tm += m[i]->writeRawDump(f, c);
    tc += c;
  }
  fclose(f);
  debug("%s : writed %dK in %d chunks", __FUNCTION__, tm >> 10, tc);
}

void DagDbgMem::enable_thorough_checks(bool enable)
{
  if (DagDebugMemAllocator::thoroughChecks == enable)
    return;
  if (enable)
    if (!check_memory(false))
      DAG_FATAL("memory is corrupted!");

  DagDebugMemAllocator::thoroughChecks = enable;
}

bool DagDbgMem::enable_stack_fill(bool enable)
{
  bool res = allow_fill_stack;
  allow_fill_stack = enable;
  return res;
}
