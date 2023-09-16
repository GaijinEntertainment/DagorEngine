#include <memory/dag_framemem.h>
#include <memory/dag_memBase.h>
#include <stddef.h>

#if DAGOR_PREFER_HEAP_ALLOCATION

void reset_framemem() {}
IMemAlloc *framemem_ptr() { return defaultmem; }
FramememScopedRegion::FramememScopedRegion() {}
FramememScopedRegion::~FramememScopedRegion() {}
#if DAGOR_DBGLEVEL > 0
FramememScopedValidator::FramememScopedValidator(const char *, uint32_t) {}
FramememScopedValidator::~FramememScopedValidator() {}
#endif

extern const size_t DEFAULT_FRAMEMEM_SIZE = 0;
IMemAlloc *alloc_thread_framemem(size_t) { return defaultmem; }
void free_thread_framemem() {}

#else

#ifndef PROFILE_FRAMEMEM_USAGE
#if (DAGOR_DBGLEVEL > 0) && _TARGET_PC_WIN
#define PROFILE_FRAMEMEM_USAGE 1
#endif
#endif

#ifndef FRAMEMEM_VALIDATE_LIBERATOR
#if PROFILE_FRAMEMEM_USAGE
#define FRAMEMEM_VALIDATE_LIBERATOR 1
#endif
#endif

#if DAGOR_DBGLEVEL == 0
#undef FRAMEMEM_VALIDATE_LIBERATOR
#endif

#include <util/dag_globDef.h>
#include <debug/dag_debug.h>
#include <math/dag_mathBase.h>
#include <osApiWrappers/dag_miscApi.h>
#include <vecmath/dag_vecMath.h>
#include <string.h>

#define DEF_ALIGNMENT 16u


#define MEM_END(m)           (m->memEnd)
#define IS_OUR_PTR(m, x)     ((uintptr_t)x >= (uintptr_t)m->framemem_space && (uintptr_t)x < (uintptr_t)MEM_END(m))
#define IS_OUR_CUR_PTR(m, x) ((uintptr_t)x >= (uintptr_t)m->framemem_space && (uintptr_t)x <= (uintptr_t)MEM_END(m))

static inline void fill_with_debug_pattern(char *beg, char *end)
{
#if MEM_DEBUGALLOC > 0
  G_FAST_ASSERT(end >= beg);
  G_FAST_ASSERT(!(uintptr_t(beg) & 15));
  G_FAST_ASSERT(!(uintptr_t(end) & 15));
  static const vec4i_const DEBUG_PATTERN = {0x7ffbffff, 0x7ffbffff, 0x7ffbffff, 0x7ffbffff};
  vec4f pattern = (vec4f)DEBUG_PATTERN;
  char *maxEnd = beg + 4096, *end_ = end < maxEnd ? end : maxEnd; // cap of how much memory we filling up for debug (for perfomance)
  for (; beg < end_; beg += 16)
    v_st(beg, pattern);
#else
  G_UNUSED(beg);
  G_UNUSED(end);
#endif
}
#if PROFILE_FRAMEMEM_USAGE
#include <math/dag_adjpow2.h>
#include <util/dag_string.h>
#endif

// Slows everything down about 100500 times, but captures a single stack
// frame of when an allocation happened for printing during validation.
// Useful to debug memory nesting in specific algorithms with unit tests.
#define FRAMEMEM_CAPTURE_STACKS 0

#if FRAMEMEM_CAPTURE_STACKS
#include <osApiWrappers/dag_stackHlp.h>
#endif

struct AllocHeader
{
  uint32_t previousAllocSize;
#if FRAMEMEM_CAPTURE_STACKS
  void *stackframe;
#endif
};

static constexpr size_t ALIGNED_HEADER_SIZE = DEF_ALIGNMENT;
static_assert(sizeof(AllocHeader) <= ALIGNED_HEADER_SIZE && ALIGNED_HEADER_SIZE % DEF_ALIGNMENT == 0);

class FrameMemAlloc final : public IMemAlloc
{
public:
#if FRAMEMEM_VALIDATE_LIBERATOR
  size_t liberatorCount = 0;

  inline void liberatorPush()
  {
    liberatorCount++;
    state.lastLiberatorPtr = state.cur_ptr;
  }
  inline void liberatorInit()
  {
    state.lastLiberatorPtr = state.cur_ptr;
    liberatorCount = 0;
  }
  inline void liberatorPop()
  {
    G_FAST_ASSERT(liberatorCount);
    liberatorCount--;
  }
  inline bool isInsideLastLiberator(void *p) const { return p >= state.lastLiberatorPtr; }

  inline void validateInsideLastLiberator(void *p) const
  {
    if (!isInsideLastLiberator(p))
      logerr("attempt to free block that is not inside liberator block (%p, valid range is %p+, base = %p).", p,
        state.lastLiberatorPtr, framemem_space);
  }
  inline void validateLiberator()
  {
    if (liberatorCount)
      logerr("unpaired liberators! liberatorCount = %d, should be zero", liberatorCount);
  }
#else
  static inline void allocated(intptr_t) {}
  static inline void deallocated(intptr_t) {}
  static inline void liberatorInit() {}
  static inline void liberatorPush() {}
  static inline void liberatorPop() {}
  static inline void validateInsideLastLiberator(void *) {}
  static inline void validateLiberator() {}
#endif
#if PROFILE_FRAMEMEM_USAGE
  size_t logs[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  size_t exhaustedNow = 0, exhaustedTotal = 0;

  void dump()
  {
    validateLiberator();
    size_t totalC = 0;
    for (size_t i = 0; i < 32; ++i)
      totalC += logs[i];
    if (!totalC)
      return;
    String str;
    for (size_t i = 0, cur = 0; i < 32 && cur < totalC; ++i)
    {
      cur += logs[i];
      if (i <= 14 && cur == totalC)
        str.aprintf(128, "| %dkb:%.4g", 1u << (max<int>(0, i - 10)), double(cur) * 100. / totalC);
      if (i > 14 && logs[i])
        str.aprintf(128, "| %dkb:%.4g", 1u << (i - 10), double(cur) * 100. / totalC);
    }
    if (totalC)
      debug("framemem usage percintiles %s, in %.4g%% frames there wasn't enough space", str.c_str(),
        double(exhaustedTotal) * 100. / totalC);
  }
  void profile()
  {
    validateLiberator();
    if (exhaustedNow)
    {
      exhaustedTotal++;
      exhaustedNow = 0;
    }
    else
      logs[get_bigger_log2(state.cur_ptr - framemem_space)]++;
  }
  ~FrameMemAlloc() { dump(); }
  void profileExhausted() { exhaustedNow++; }
#if PROFILE_FRAMEMEM_USAGE == 2
  DAGOR_NOINLINE void isInside(void *p)
  {
    if (p && p != state.lastBlock && !isInsideLastLiberator(p))
      debug_dump_stack();
  }
  DAGOR_NOINLINE void exhausted(size_t sz)
  {
    profileExhausted();
    debug_dump_stack();
    debug("wanted additional %d to current %d", sz, state.cur_ptr - framemem_space);
  }
#else
  inline void exhausted(size_t) { profileExhausted(); }
  static inline void isInside(void *) {}
#endif
#else
  static inline void exhausted(size_t) {}
  static inline void isInside(void *) {}
  static inline void profile() {}
#endif

  FramememState state;

  FrameMemAlloc(size_t space) : memEnd(framemem_space + space), state(FramememState{framemem_space, NULL})
  {
    G_FAST_ASSERT(((uintptr_t)framemem_space & (DEF_ALIGNMENT - 1)) == 0);
    fill_with_debug_pattern(state.cur_ptr, MEM_END(this));
    liberatorInit();
    placeSentinel();
  }

  void placeSentinel()
  {
    AllocHeader sentinel{0};
    memcpy(state.cur_ptr, &sentinel, sizeof(sentinel));
    state.cur_ptr += ALIGNED_HEADER_SIZE;
    state.lastBlock = state.cur_ptr;
  }

  void reset()
  {
    state = FramememState{framemem_space, NULL};
    placeSentinel();
  }

#if FRAMEMEM_CAPTURE_STACKS
  void log_unfreed_allocs(const void *since)
  {
    auto curr = state.lastBlock;
    size_t size = state.cur_ptr - state.lastBlock;
    while (since < curr)
    {
      AllocHeader header;
      memcpy(&header, curr - ALIGNED_HEADER_SIZE, sizeof(header));
      if (header.stackframe)
        logwarn("Unfreed alloc of size %d at: %s", size, ::stackhlp_get_call_stack_str(&header.stackframe, 1));
      curr -= header.previousAllocSize;
      size = header.previousAllocSize;
    }
  }
#else
  void log_unfreed_allocs(const void *) {}
#endif

  static __forceinline size_t aligned_size(size_t sz) { return (sz + (DEF_ALIGNMENT - 1)) & ~(DEF_ALIGNMENT - 1); }

  static __forceinline char *aligned_ptr(char *ptr, size_t align) { return (char *)(((uintptr_t)ptr + (align - 1)) & ~(align - 1)); }

  __forceinline void *ret_ptr(char *ret)
  {
    const size_t prevAllocSize = ret - state.lastBlock;
    state.lastBlock = ret;
    G_FAST_ASSERT(((uintptr_t)state.cur_ptr & (DEF_ALIGNMENT - 1)) == 0);
    G_FAST_ASSERT(((uintptr_t)ret & (DEF_ALIGNMENT - 1)) == 0);
    fill_with_debug_pattern(ret, state.cur_ptr);

    G_FAST_ASSERT(prevAllocSize < ~0u);
    AllocHeader header{static_cast<uint32_t>(prevAllocSize)};
#if FRAMEMEM_CAPTURE_STACKS
    ::stackhlp_fill_stack(&header.stackframe, 1, 3);
#endif
    memcpy(ret - ALIGNED_HEADER_SIZE, &header, sizeof(header));
    return ret;
  }

  virtual void destroy() {}
  virtual bool isEmpty() { return state.cur_ptr >= MEM_END(this); }
  virtual size_t getSize(void *p) // partially supported, only for alloc with asize like functions
  {
    if (DAGOR_UNLIKELY(!p))
      return 0;
    if (DAGOR_UNLIKELY(!IS_OUR_PTR(this, p)))
      return tmpmem->getSize(p);
    if (DAGOR_LIKELY(p == state.lastBlock))
      return state.cur_ptr - state.lastBlock;
    else
      return 0;
  }

  virtual void *alloc(size_t sz)
  {
    char *cp = state.cur_ptr;
    char *ret = cp + ALIGNED_HEADER_SIZE;
    state.cur_ptr = ret + aligned_size(sz);

    if (DAGOR_LIKELY(state.cur_ptr < MEM_END(this)))
      return ret_ptr(ret);

    state.cur_ptr = cp;
    exhausted(sz);
    return tmpmem->alloc(sz);
  }

  virtual void *tryAlloc(size_t sz)
  {
    char *cp = state.cur_ptr;
    char *ret = cp + ALIGNED_HEADER_SIZE;
    state.cur_ptr = ret + aligned_size(sz);

    if (DAGOR_LIKELY(state.cur_ptr < MEM_END(this)))
      return ret_ptr(ret);

    state.cur_ptr = cp;
    exhausted(sz);
    return tmpmem->tryAlloc(sz);
  }

  virtual void *allocAligned(size_t sz, size_t alignment)
  {
    alignment = max(alignment, size_t(DEF_ALIGNMENT));
    char *cp = state.cur_ptr;
    char *ret = aligned_ptr(cp + ALIGNED_HEADER_SIZE, alignment);
    state.cur_ptr = ret + aligned_size(sz);

    if (DAGOR_LIKELY(state.cur_ptr < MEM_END(this)))
      return ret_ptr(ret);

    state.cur_ptr = cp;
    exhausted(sz);
    return tmpmem->allocAligned(sz, alignment);
  }

  inline bool expandLastBlock(size_t sz)
  {
    auto newCurPtr = state.lastBlock + aligned_size(sz);
    if (newCurPtr > MEM_END(this))
      return false;
    if (newCurPtr > state.cur_ptr)
      fill_with_debug_pattern(state.cur_ptr, newCurPtr);
    state.cur_ptr = newCurPtr;
    return true;
  }

  virtual bool resizeInplace(void *p, size_t sz)
  {
    if (!p)
      return false;
    if (!IS_OUR_PTR(this, p))
      return tmpmem->resizeInplace(p, sz);
    validateInsideLastLiberator(p);
    return p == state.lastBlock ? expandLastBlock(sz) : false;
  }

  virtual void *realloc(void *p, size_t sz)
  {
    if (!p)
      return this->alloc(sz);
    else if (!sz)
    {
      this->free(p);
      return nullptr;
    }

    if (!IS_OUR_PTR(this, p))
      return tmpmem->realloc(p, sz);
    validateInsideLastLiberator(p);
    if (p == state.lastBlock && expandLastBlock(sz))
      return p;
    void *ret = this->alloc(sz);
    memcpy(ret, p, min(sz, size_t((uintptr_t)(IS_OUR_PTR(this, ret) ? ret : MEM_END(this)) - (uintptr_t)p)));
    return ret;
  }

  virtual void free(void *p) // only last allocated block could be freed
  {
    if (!p)
      return;
    if (!IS_OUR_PTR(this, p))
      return tmpmem->free(p);
    validateInsideLastLiberator(p);
    if (p == state.lastBlock)
    {
      G_FAST_ASSERT((uintptr_t)state.lastBlock <= (uintptr_t)state.cur_ptr);
      fill_with_debug_pattern((char *)p, state.cur_ptr);
      state.cur_ptr = state.lastBlock - ALIGNED_HEADER_SIZE; // rollback padding for header

      AllocHeader header;
      memcpy(&header, state.cur_ptr, sizeof(header));
      state.lastBlock -= header.previousAllocSize;
#if FRAMEMEM_CAPTURE_STACKS
      header.stackframe = nullptr;
#endif
    }
    else if (state.lastBlock)
    {
      isInside(p);
      G_FAST_ASSERT((uintptr_t)p < (uintptr_t)state.lastBlock);
    }
  }
  void freeAligned(void *p) override { free(p); }
  char *memEnd = nullptr;                                          // points to end of allocation
  alignas(DEF_ALIGNMENT) char framemem_space[DEF_ALIGNMENT] = {0}; // actual size will be bigger
};

#if _TARGET_STATIC_LIB
thread_local IMemAlloc *thread_framemem = tmpmem_ptr();
#else
static thread_local IMemAlloc *thread_framemem = tmpmem_ptr();
IMemAlloc *framemem_ptr() { return thread_framemem; }
#endif

static inline FrameMemAlloc *as_framemem() { return thread_framemem == tmpmem ? nullptr : (FrameMemAlloc *)thread_framemem; }

#if _TARGET_TVOS | _TARGET_IOS | _TARGET_ANDROID | _TARGET_C3
extern const size_t DEFAULT_FRAMEMEM_SIZE = 256 << 10;
#else
extern const size_t DEFAULT_FRAMEMEM_SIZE = 1 << 20;
#endif

IMemAlloc *alloc_thread_framemem(size_t sz)
{
  G_ASSERT_RETURN(!as_framemem(), tmpmem);
  sz = sz == 0 ? DEFAULT_FRAMEMEM_SIZE : sz;
  void *buf = memalloc(sz, tmpmem);
  thread_framemem = new (buf, _NEW_INPLACE) FrameMemAlloc(sz - offsetof(FrameMemAlloc, framemem_space));
  return thread_framemem;
}

void free_thread_framemem()
{
  if (auto m = as_framemem())
  {
    m->~FrameMemAlloc();
    memfree(m, tmpmem);
    thread_framemem = tmpmem;
  }
}

static RAIIThreadFramememAllocator main_thread;

void reset_framemem()
{
  if (auto m = as_framemem())
  {
    m->profile();
    m->reset();
  }
}

static inline FramememState save_cur_framemem()
{
  if (auto m = as_framemem())
  {
    G_ASSERT(IS_OUR_CUR_PTR(m, m->state.cur_ptr));
    // First save current state including the last liberator, then push a new liberator
    auto savedState = m->state;
    m->liberatorPush();
    return savedState;
  }
  return FramememState();
}

static inline void restore_cur_framemem(const FramememState &p)
{
  if (p.cur_ptr)
    if (auto m = as_framemem())
    {
      G_ASSERT(IS_OUR_CUR_PTR(m, p.cur_ptr));
      m->state = p;
      m->liberatorPop();
    }
}

FramememScopedRegion::FramememScopedRegion() : p(save_cur_framemem()) {}
FramememScopedRegion::~FramememScopedRegion() { restore_cur_framemem(p); }

#if DAGOR_DBGLEVEL > 0

static inline const char *get_last_block()
{
  if (auto m = as_framemem())
    return m->state.lastBlock;
  return nullptr;
}
static void validate_cur_framemem(const char *file, uint32_t line, const char *saved_block)
{
  auto m = as_framemem();
  if (!m)
    return;

  if (m->state.lastBlock == saved_block)
    return;

  logerr("Framemem: non-nesting lifetimes detected within validator at %s:%d! Expected last allocation at %p observed at %p.", file,
    line, saved_block, m->state.lastBlock);

  m->log_unfreed_allocs(saved_block);
}

FramememScopedValidator::FramememScopedValidator(const char *in_file, uint32_t in_line) :
  savedLastBlock(get_last_block()), file{in_file}, line{in_line}
{}
FramememScopedValidator::~FramememScopedValidator() { validate_cur_framemem(file, line, savedLastBlock); }

#endif

#endif // DAGOR_PREFER_HEAP_ALLOCATION

#define EXPORT_PULL dll_pull_memory_framemem
#include <supp/exportPull.h>
