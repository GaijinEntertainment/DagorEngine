#include "sqpcheader.h"
#include "memtrace.h"
#include <squirrel/sqvm.h>

#define ENABLE_CONTEXT_CHECK 0
#define FIXED_ALLOCATORS_COUNT 42
#define ENABLE_SQ_MEM_STAT (DAGOR_DBGLEVEL > 0)
#ifndef SQ_VAR_TRACE_ENABLED
#define SQ_VAR_TRACE_ENABLED 0
#endif

#if !ENABLE_FIXED_ALLOC && MEM_TRACE_ENABLED
  #error Mem trace will work correctly only when allocctx is initialized which requires ENABLE_FIXED_ALLOC==1
#endif

#if ENABLE_FIXED_ALLOC
#  include <memory/dag_fixedBlockAllocator.h>
#  include <new>
#endif

#if ENABLE_CONTEXT_CHECK
#  include <map>
#endif

typedef struct SQAllocContextT * SQAllocContext;
#if !ENABLE_FIXED_ALLOC
#define ENABLE_RE_USE 1
#else
#undef ENABLE_RE_USE
#endif

struct SQAllocContextT
{
#if ENABLE_FIXED_ALLOC
  FixedBlockAllocator allocators[FIXED_ALLOCATORS_COUNT];
#elif ENABLE_RE_USE
  enum {MAX_BINS = 20, MAX_ALLOCATIONS_PER_BIN = 8};
  uint32_t allocatedBinsCount[MAX_BINS];
  void * allocatedBins[MAX_BINS][MAX_ALLOCATIONS_PER_BIN];
#endif
  HSQUIRRELVM vm;
};

#if ENABLE_CONTEXT_CHECK
static std::map<void *, SQAllocContext> alloc_map;
#endif


static sqmemtrace::HugeAllocHookCB huge_alloc_hook_cb = nullptr;
static SQUnsignedInteger huge_alloc_size_threshold = 8 << 20;

namespace sqmemtrace
{
  int set_huge_alloc_threshold(int size_threshold)
  {
#if SQ_VAR_TRACE_ENABLED == 1
    int prev = huge_alloc_size_threshold / 8;
    huge_alloc_size_threshold = 8 * (SQUnsignedInteger)size_threshold;
#else
    int prev = huge_alloc_size_threshold;
    huge_alloc_size_threshold = (SQUnsignedInteger)size_threshold;
#endif
    return prev;
  }

  void set_huge_alloc_hook(HugeAllocHookCB hook, unsigned int size_threshold)
  {
    huge_alloc_hook_cb = hook;
#if SQ_VAR_TRACE_ENABLED == 1
    huge_alloc_size_threshold = 8 * (SQUnsignedInteger)size_threshold;
#else
    huge_alloc_size_threshold = (SQUnsignedInteger)size_threshold;
#endif
  }

  unsigned mem_used;
#if ENABLE_SQ_MEM_STAT
  unsigned mem_used_max = 0, mem_cur_ptrs = 0, mem_max_ptrs = 0;
#endif
}


#define ALLOCATOR_INDEX(size) (((size) + 7) >> 3)
#define MAX_FIXED_SIZE ((FIXED_ALLOCATORS_COUNT - 1) * 8)

void sq_vm_init_alloc_context(SQAllocContext * ctx)
{
  SQAllocContextT *ctxImpl = (SQAllocContextT *)memalloc(sizeof(SQAllocContextT), midmem);
  *ctx = ctxImpl;
#if ENABLE_FIXED_ALLOC
  for (int i = 0; i < FIXED_ALLOCATORS_COUNT; i++)
  {
    int initSize = (i >= 8 && i <= 11) ? 512 : 16;
    new (&ctxImpl->allocators[i]) FixedBlockAllocator(i * 8, initSize);
  }
#elif ENABLE_RE_USE
  memset(ctxImpl->allocatedBinsCount, 0, sizeof(ctxImpl->allocatedBinsCount));
#endif

  ctxImpl->vm = nullptr;

#if MEM_TRACE_ENABLED == 1
  sqmemtrace::add_ctx(*ctx);
#endif
}

void sq_vm_assign_to_alloc_context(SQAllocContext ctx, HSQUIRRELVM vm)
{
  G_ASSERT_RETURN(ctx, );
  G_ASSERT(!ctx->vm);
  ctx->vm = vm;
}

void sq_vm_destroy_alloc_context(SQAllocContext * ctx)
{
  G_ASSERT(ctx);
#if MEM_TRACE_ENABLED == 1
  sqmemtrace::remove_ctx(*ctx);
#endif

  if (ctx)
  {
#if ENABLE_FIXED_ALLOC
    for (int i = 0; i < FIXED_ALLOCATORS_COUNT; i++)
      (*ctx)->allocators[i].~FixedBlockAllocator();
#elif ENABLE_RE_USE
    for (int i = 0; i < SQAllocContextT::MAX_BINS; i++)
    {
      for (int j = 0, e = (*ctx)->allocatedBinsCount[i]; j < e; j++)
        scriptmem->free((*ctx)->allocatedBins[i][j]);
      (*ctx)->allocatedBinsCount[i] = 0;
    }
#endif

    memfree(*ctx, midmem);
    *ctx = nullptr;
  }
}
enum {ALLOC_ALIGNMENT_SHIFT = 3, ALLOC_ALIGNMENT_MASK = (1<<ALLOC_ALIGNMENT_SHIFT)-1};

void *sq_vm_malloc(SQAllocContext ctx, SQUnsignedInteger size)
{
  size = (size + ALLOC_ALIGNMENT_MASK)&~ALLOC_ALIGNMENT_MASK;
  sqmemtrace::mem_used += size;
#if ENABLE_SQ_MEM_STAT
  sqmemtrace::mem_used_max = sqmemtrace::mem_used > sqmemtrace::mem_used_max ? sqmemtrace::mem_used : sqmemtrace::mem_used_max;
  ++sqmemtrace::mem_cur_ptrs;
  sqmemtrace::mem_max_ptrs = sqmemtrace::mem_cur_ptrs > sqmemtrace::mem_max_ptrs ? sqmemtrace::mem_cur_ptrs : sqmemtrace::mem_max_ptrs;
#endif

  if (ctx && ctx->vm && size >= huge_alloc_size_threshold && huge_alloc_hook_cb)
    huge_alloc_hook_cb(size, huge_alloc_size_threshold / (SQ_VAR_TRACE_ENABLED ? 8 : 1), ctx->vm);

#if ENABLE_FIXED_ALLOC
  if (size <= MAX_FIXED_SIZE && ctx)
  {
    void * n = ctx->allocators[ALLOCATOR_INDEX(size)].allocateOneBlock();
    #if ENABLE_CONTEXT_CHECK
      alloc_map.insert(std::pair<void *, SQAllocContext>(n, ctx));
    #endif

    #if MEM_TRACE_ENABLED == 1
      sqmemtrace::on_alloc(ctx, ctx->vm, n, size);
    #endif

    return n;
  }
#elif ENABLE_RE_USE
  if (ctx)
  {
    const uint32_t bin = (size>>ALLOC_ALIGNMENT_SHIFT) - 1;
    if (bin < ctx->MAX_BINS && ctx->allocatedBinsCount[bin])
    {
      return ctx->allocatedBins[bin][--ctx->allocatedBinsCount[bin]];
    }
  }
#endif
  void * ret = scriptmem->alloc(size);
#if ENABLE_CONTEXT_CHECK
  alloc_map.insert(std::pair<void *, SQAllocContext>(ret, ctx));
#endif

#if MEM_TRACE_ENABLED == 1
  sqmemtrace::on_alloc(ctx, ctx ? ctx->vm : nullptr, ret, size);
#endif

  return ret;
}

void *sq_vm_realloc(SQAllocContext ctx, void *p, SQUnsignedInteger oldsize, SQUnsignedInteger size)
{
  size = (size + ALLOC_ALIGNMENT_MASK)&~ALLOC_ALIGNMENT_MASK;
  oldsize = (oldsize + ALLOC_ALIGNMENT_MASK)&~ALLOC_ALIGNMENT_MASK;
  if (size == oldsize)
    return p;
  sqmemtrace::mem_used += size - oldsize;
#if ENABLE_SQ_MEM_STAT
  if (size)
  {
    sqmemtrace::mem_used_max = sqmemtrace::mem_used > sqmemtrace::mem_used_max ? sqmemtrace::mem_used : sqmemtrace::mem_used_max;
    if (!p)
      sqmemtrace::mem_cur_ptrs++;
  }
  else if (p)
    sqmemtrace::mem_cur_ptrs--;
#endif

  if (ctx && ctx->vm && size >= huge_alloc_size_threshold && huge_alloc_hook_cb)
    huge_alloc_hook_cb(size, huge_alloc_size_threshold / (SQ_VAR_TRACE_ENABLED ? 8 : 1), ctx->vm);

#if ENABLE_CONTEXT_CHECK
  auto found = p ? alloc_map.find(p) : alloc_map.end();
  if (p)
  {
    assert(found != alloc_map.end() && found->second == ctx);
    alloc_map.erase(found);
  }
#endif

#if MEM_TRACE_ENABLED == 1
  if (p)
    sqmemtrace::on_free(ctx, p);
#endif

#if ENABLE_FIXED_ALLOC
  if (ctx && (size <= MAX_FIXED_SIZE || oldsize <= MAX_FIXED_SIZE))
  {
    if (ALLOCATOR_INDEX(oldsize) == ALLOCATOR_INDEX(size))
    {
      #if ENABLE_CONTEXT_CHECK
        alloc_map.insert(std::pair<void *, SQAllocContext>(p, ctx));
      #endif
      #if MEM_TRACE_ENABLED == 1
        sqmemtrace::on_alloc(ctx, ctx->vm, p, size);
      #endif
      return p;
    }

    void * n = (size <= MAX_FIXED_SIZE) ?
      ctx->allocators[ALLOCATOR_INDEX(size)].allocateOneBlock() : scriptmem->alloc(size);

    memcpy(n, p, min(oldsize, size));
    if (oldsize <= MAX_FIXED_SIZE)
      ctx->allocators[ALLOCATOR_INDEX(oldsize)].freeOneBlock(p);
    else
      scriptmem->free(p);

    #if ENABLE_CONTEXT_CHECK
      alloc_map.insert(std::pair<void *, SQAllocContext>(n, ctx));
    #endif
    #if MEM_TRACE_ENABLED == 1
      sqmemtrace::on_alloc(ctx, ctx->vm, n, size);
    #endif
    return n;
  }
#elif ENABLE_RE_USE
  if (ctx)
  {
    const uint32_t binOld = (oldsize>>ALLOC_ALIGNMENT_SHIFT) - 1;
    const uint32_t binNew = (size>>ALLOC_ALIGNMENT_SHIFT) - 1;
    if (binNew < ctx->MAX_BINS && ctx->allocatedBinsCount[binNew])
    {
      void *ret = ctx->allocatedBins[binNew][--ctx->allocatedBinsCount[binNew]];
      memcpy(ret, p, min(oldsize, size));
      if (binOld < ctx->MAX_BINS && ctx->allocatedBinsCount[binOld] < ctx->MAX_ALLOCATIONS_PER_BIN)
        ctx->allocatedBins[binOld][ctx->allocatedBinsCount[binOld]++] = p;
      else
        scriptmem->free(p);
      return ret;
    }
  }
#endif

  void* ret = scriptmem->realloc(p, size);
#if ENABLE_CONTEXT_CHECK
  alloc_map.insert(std::pair<void *, SQAllocContext>(ret, ctx));
#endif
#if MEM_TRACE_ENABLED == 1
  sqmemtrace::on_alloc(ctx, ctx ? ctx->vm : nullptr, ret, size);
#endif
  return ret;
}

void sq_vm_free(SQAllocContext ctx, void *p, SQUnsignedInteger size)
{
  if (!p)
    return;

  size = (size + ALLOC_ALIGNMENT_MASK)&~ALLOC_ALIGNMENT_MASK;
  sqmemtrace::mem_used -= size;
#if ENABLE_SQ_MEM_STAT
  --sqmemtrace::mem_cur_ptrs;
#endif

#if ENABLE_CONTEXT_CHECK
  auto found = alloc_map.find(p);
  assert(found != alloc_map.end() && found->second == ctx);
  alloc_map.erase(found);
#endif

#if MEM_TRACE_ENABLED == 1
  sqmemtrace::on_free(ctx, p);
#endif


#if ENABLE_FIXED_ALLOC
  if (size <= MAX_FIXED_SIZE && ctx)
  {
    FixedBlockAllocator * allocators =  reinterpret_cast<FixedBlockAllocator *>(ctx);
    allocators[ALLOCATOR_INDEX(size)].freeOneBlock(p);
    return;
  }
#elif ENABLE_RE_USE
  if (ctx && size > ALLOC_ALIGNMENT_MASK)
  {
    const uint32_t bin = (size>>ALLOC_ALIGNMENT_SHIFT) - 1;
    if (bin < ctx->MAX_BINS && ctx->allocatedBinsCount[bin] < ctx->MAX_ALLOCATIONS_PER_BIN)
    {
      ctx->allocatedBins[bin][ctx->allocatedBinsCount[bin]++] = p;
      return;
    }
  }
#endif

  scriptmem->free(p);
}
