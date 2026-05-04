// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_fastStackCapture.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_stackHlpEx.h>

#if defined(_WIN64)

// Fast stack capture for Win64.
//
// Shared infrastructure:
//    - Caches PE .pdata (RUNTIME_FUNCTION) tables from all loaded modules at init
//    - Binary search replaces RtlLookupFunctionEntry (avoids its internal spinlock)
//    - Incremental module add via GetModuleHandleExW on cache miss (no full reinit)
//    - Negative cache for addresses outside any module (JIT code, trampolines)
//    - CacheMissStrategy flags control miss handling per call site
//
// A) Cached unwind tables (fast_stackhlp_fill_stack_cached)
//    - Uses RtlVirtualUnwind for correct frame unwinding
//    - RtlCaptureContext for register capture
//
// B) Lightweight unwinder (fast_stackhlp_fill_stack_rbp)
//    - Avoids RtlCaptureContext: inline asm (clang/gcc) or _setjmp (MSVC)
//    - Avoids RtlVirtualUnwind: lightweightVirtualUnwind processes UNWIND_INFO
//      codes directly, updating Rip, Rsp, and callee-saved GP registers
//    - Falls back to RtlVirtualUnwind on PUSH_MACHFRAME or unknown opcodes
//    - No OS calls in the hot loop - just memory reads and binary searches


#include <windows.h>
#include <winnt.h>
#include <tlhelp32.h>
#include <intrin.h>
#include <setjmp.h>
#include <stdint.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_ttas_spinlock.h>
#include <util/dag_stlqsort.h>

// ---------------------------------------------------------------------------
// Module cache
// ---------------------------------------------------------------------------

struct CachedModule
{
  ULONG64 base;
  ULONG64 end;
  PRUNTIME_FUNCTION pdata;
  ULONG pdataLast;
};

static constexpr int MAX_CACHED_MODULES = 512;
static CachedModule g_modules[MAX_CACHED_MODULES];
static int g_moduleCount = 0;

static volatile int g_cacheReady = 0;
static volatile int g_cacheLock = 0;

// ---------------------------------------------------------------------------
// Negative cache: direct-mapped table of 64KB-aligned page addresses known
// to not belong to any module (JIT code, trampolines, etc).
// ---------------------------------------------------------------------------

static constexpr int NEG_CACHE_BITS = 6; // 64 entries
static constexpr int NEG_CACHE_SIZE = 1 << NEG_CACHE_BITS;
static volatile uint64_t g_negCache[NEG_CACHE_SIZE]; // page-aligned PCs

static __forceinline bool isNegCached(ULONG64 pc)
{
  uint64_t page = pc >> 16;
  return interlocked_relaxed_load(g_negCache[page & (NEG_CACHE_SIZE - 1)]) == page;
}

static __forceinline void addNegCache(ULONG64 pc)
{
  uint64_t page = pc >> 16;
  interlocked_relaxed_store(g_negCache[page & (NEG_CACHE_SIZE - 1)], page);
}

static void buildModuleCache()
{
  memset((void *)g_negCache, 0, sizeof(g_negCache));

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return;

  MODULEENTRY32W me;
  me.dwSize = sizeof(me);
  g_moduleCount = 0;

  BOOL ok = Module32FirstW(snap, &me);
  while (ok && g_moduleCount < MAX_CACHED_MODULES)
  {
    const BYTE *dllBase = me.modBaseAddr;
    if (dllBase && me.modBaseSize)
    {
      __try
      {
        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)dllBase;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE)
        {
          const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(dllBase + dos->e_lfanew);
          if (nt->Signature == IMAGE_NT_SIGNATURE && nt->OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_EXCEPTION)
          {
            const IMAGE_DATA_DIRECTORY &excDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
            if (excDir.VirtualAddress && excDir.Size >= sizeof(RUNTIME_FUNCTION))
            {
              CachedModule &m = g_modules[g_moduleCount];
              m.base = (ULONG64)dllBase;
              m.end = (ULONG64)dllBase + me.modBaseSize;
              m.pdata = (PRUNTIME_FUNCTION)(dllBase + excDir.VirtualAddress);
              m.pdataLast = (excDir.Size / sizeof(RUNTIME_FUNCTION)) - 1;
              ++g_moduleCount;
            }
          }
        }
      }
      __except (EXCEPTION_EXECUTE_HANDLER)
      {}
    }
    ok = Module32NextW(snap, &me);
  }

  CloseHandle(snap);
  stlsort::sort(g_modules, g_modules + g_moduleCount, [](const CachedModule &a, const CachedModule &b) { return a.base < b.base; });
}

DAGOR_NOINLINE
static void buildCacheInternal()
{
  ttas_spinlock_lock(g_cacheLock);
  if (!interlocked_acquire_load(g_cacheReady))
  {
    buildModuleCache();
    interlocked_release_store(g_cacheReady, 1);
  }
  ttas_spinlock_unlock(g_cacheLock);
}

static __forceinline void ensureCacheReady()
{
  if (DAGOR_UNLIKELY(!interlocked_relaxed_load(g_cacheReady)))
    buildCacheInternal();
}


// For cross-thread capture: trylock to avoid deadlock if the suspended thread
// holds g_cacheLock. Returns true if cache is ready, false if not (caller should bail).
static __forceinline bool ensureCacheReadyForOtherThread()
{
  if (DAGOR_LIKELY(interlocked_acquire_load(g_cacheReady)))
    return true;

  if (!ttas_spinlock_trylock(g_cacheLock))
    return false; // Suspended thread may hold the lock - bail out

  if (!interlocked_relaxed_load(g_cacheReady))
  {
    buildModuleCache();
    interlocked_release_store(g_cacheReady, 1);
  }
  ttas_spinlock_unlock(g_cacheLock);
  return true;
}

void fast_stackhlp_init_cache(bool reinit)
{
  if (!reinit)
    ensureCacheReady();
  else
  {
    ttas_spinlock_lock(g_cacheLock);
    buildModuleCache();
    interlocked_release_store(g_cacheReady, 1);
    ttas_spinlock_unlock(g_cacheLock);
  }
}

static __forceinline bool findModule(ULONG64 pc, const CachedModule *&lastModule, ULONG64 &imageBase)
{
  if (lastModule && pc >= imageBase && pc < lastModule->end)
    return true;

  lastModule = nullptr;
  int lo = 0, hi = g_moduleCount - 1;
  while (lo <= hi)
  {
    int mid = (lo + hi) >> 1;
    auto &mod = g_modules[mid];
    if (pc < mod.base)
      hi = mid - 1;
    else if (pc >= mod.end)
      lo = mid + 1;
    else
    {
      lastModule = &mod;
      imageBase = mod.base;
      return true;
    }
  }
  return false;
}

static __forceinline PRUNTIME_FUNCTION findFunctionEntry(const CachedModule *mod, DWORD rva, int &hint)
{
  PRUNTIME_FUNCTION table = mod->pdata;
  int lo = 0, hi = (int)mod->pdataLast;
  int mid = hint;
  goto first_check;
  do
  {
    mid = (lo + hi) >> 1;
  first_check:
    if (rva < table[mid].BeginAddress)
      hi = mid - 1;
    else if (rva >= table[mid].EndAddress)
      lo = mid + 1;
    else
    {
      hint = mid;
      return &table[mid];
    }
  } while (lo <= hi);
  return nullptr;
}

// Cache-only lookup - NO fallback to RtlLookupFunctionEntry.
// Safe to use when another thread is suspended (avoids RtlLookupFunctionEntry spinlock deadlock).
static __forceinline PRUNTIME_FUNCTION cachedLookupOnly(ULONG64 pc, const CachedModule *&lastModule, ULONG64 &imageBase,
  int &pdataHint)
{
  const CachedModule *prevModule = lastModule;
  if (findModule(pc, lastModule, imageBase))
  {
    if (lastModule != prevModule)
      pdataHint = (int)(lastModule->pdataLast) >> 1;
    return findFunctionEntry(lastModule, (DWORD)(pc - imageBase), pdataHint);
  }
  return nullptr;
}


// ---------------------------------------------------------------------------
// Incremental module add: resolve a single missing module via
// GetModuleHandleExW instead of full CreateToolhelp32Snapshot reinit.
// ---------------------------------------------------------------------------

static bool tryAddModuleForPC(ULONG64 pc)
{
  if (isNegCached(pc))
    return false;

  HMODULE hMod = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)(uintptr_t)pc, &hMod) ||
      !hMod)
  {
    addNegCache(pc);
    return false;
  }

  // Parse PE header for .pdata -- same logic as buildModuleCache, single module
  const BYTE *dllBase = (const BYTE *)hMod;
  CachedModule newMod;
  __try
  {
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)dllBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
    {
      addNegCache(pc);
      return false;
    }
    const IMAGE_NT_HEADERS64 *nt = (const IMAGE_NT_HEADERS64 *)(dllBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->OptionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_EXCEPTION)
    {
      addNegCache(pc);
      return false;
    }
    const IMAGE_DATA_DIRECTORY &excDir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION];
    if (!excDir.VirtualAddress || excDir.Size < sizeof(RUNTIME_FUNCTION))
    {
      addNegCache(pc);
      return false;
    }
    newMod.base = (ULONG64)dllBase;
    newMod.end = (ULONG64)dllBase + nt->OptionalHeader.SizeOfImage;
    newMod.pdata = (PRUNTIME_FUNCTION)(dllBase + excDir.VirtualAddress);
    newMod.pdataLast = (excDir.Size / sizeof(RUNTIME_FUNCTION)) - 1;
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    addNegCache(pc);
    return false;
  }

  // Insert into sorted g_modules under lock
  ttas_spinlock_lock(g_cacheLock);

  // Re-check: another thread may have added it while we were parsing
  int moduleCount = g_moduleCount;
  if (moduleCount >= MAX_CACHED_MODULES)
  {
    ttas_spinlock_unlock(g_cacheLock);
    return false;
  }

  // Binary search for insert position
  int lo = 0, hi = moduleCount - 1;
  int insertPos = moduleCount;
  while (lo <= hi)
  {
    int mid = (lo + hi) >> 1;
    if (newMod.base < g_modules[mid].base)
    {
      insertPos = mid;
      hi = mid - 1;
    }
    else if (newMod.base > g_modules[mid].base)
      lo = mid + 1;
    else
    {
      // Already present (race with another thread)
      ttas_spinlock_unlock(g_cacheLock);
      return true;
    }
  }

  // Shift tail and insert
  memmove(&g_modules[insertPos + 1], &g_modules[insertPos], (moduleCount - insertPos) * sizeof(CachedModule));
  g_modules[insertPos] = newMod;
  interlocked_release_store(g_moduleCount, moduleCount + 1);

  ttas_spinlock_unlock(g_cacheLock);
  return true;
}

// Unhappy path: try incremental module add, then re-lookup.
DAGOR_NOINLINE
static PRUNTIME_FUNCTION tryAddAndLookup(ULONG64 pc, const CachedModule *&lastMod, ULONG64 &imageBase, int &pdataHint)
{
  return tryAddModuleForPC(pc) ? cachedLookupOnly(pc, lastMod, imageBase, pdataHint) : nullptr;
}

// ---------------------------------------------------------------------------
// Shared unwinder from a CONTEXT (used by both self-capture and cross-thread capture)
// ---------------------------------------------------------------------------

// Inner unwinder from a CONTEXT.
// miss_strategy controls what happens on cache miss (see CacheMissStrategy).
static unsigned fast_fill_stack_from_context_inner(void **stack, unsigned max_size, CONTEXT &ctx, int skip_frames,
  CacheMissStrategy miss_strategy)
{
  if (ctx.Rip == 0 && ctx.Rsp != 0)
  {
    ctx.Rip = *(ULONG64 *)ctx.Rsp;
    ctx.Rsp += 8;
  }

  int pdataHint = 0;
  int frame = -skip_frames;
  const CachedModule *lastMod = nullptr;
  ULONG64 imageBase = 0;
  __try
  {
    for (; ctx.Rip && frame < (int)max_size; ++frame)
    {
      if (frame >= 0)
        stack[frame] = (void *)(uintptr_t)ctx.Rip;

      PRUNTIME_FUNCTION pFunc = cachedLookupOnly(ctx.Rip, lastMod, imageBase, pdataHint);
      if (!lastMod && ((uint8_t)miss_strategy & (uint8_t)CacheMissStrategy::UpdateCache))
        pFunc = tryAddAndLookup(ctx.Rip, lastMod, imageBase, pdataHint);

      if (pFunc)
      {
        PVOID handlerData = nullptr;
        ULONG64 establisherFrame = 0;
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, pFunc, &ctx, &handlerData, &establisherFrame, nullptr);
      }
      else
      {
        break;
      }
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {}

  return frame > 0 ? (unsigned)frame : 0;
}


// ---------------------------------------------------------------------------
// Approach A: Cached unwind tables + RtlVirtualUnwind
// ---------------------------------------------------------------------------

DAGOR_NOINLINE
unsigned fast_stackhlp_fill_stack_cached(void **stack, unsigned max_size, int skip_frames, CacheMissStrategy strategy)
{
  ensureCacheReady();

  CONTEXT ctx;
  RtlCaptureContext(&ctx);

  unsigned nframes = fast_fill_stack_from_context_inner(stack, max_size, ctx, skip_frames, strategy);
  if (nframes < max_size)
    stack[nframes] = (void *)(~(uintptr_t)0);
  return nframes;
}


// ---------------------------------------------------------------------------
// Cross-thread stack capture with cached unwind tables
// ---------------------------------------------------------------------------

int fast_get_thread_handle_callstack(intptr_t target_thread, void **stack, uint32_t max_stack_size)
{
  HANDLE thread = (HANDLE)target_thread;
  if (SuspendThread(thread) == ~((DWORD)0)) //-V720
    return -1;

  int result = 0;

  // trylock: if suspended thread holds g_cacheLock, we must not block
  if (ensureCacheReadyForOtherThread())
  {
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (::GetThreadContext(thread, &ctx) != 0)
      result = (int)fast_fill_stack_from_context_inner(stack, max_stack_size, ctx, 0, CacheMissStrategy::None);
  }

  ResumeThread(thread);
  return result;
}


// ---------------------------------------------------------------------------
// Approach B: Lightweight unwinder - no RtlCaptureContext, no RtlVirtualUnwind
// ---------------------------------------------------------------------------

// x64 UNWIND_INFO structures (from Windows SDK / PE spec)
#define UNW_UWOP_PUSH_NONVOL     0
#define UNW_UWOP_ALLOC_LARGE     1
#define UNW_UWOP_ALLOC_SMALL     2
#define UNW_UWOP_SET_FPREG       3
#define UNW_UWOP_SAVE_NONVOL     4
#define UNW_UWOP_SAVE_NONVOL_FAR 5
#define UNW_UWOP_SAVE_XMM128     8
#define UNW_UWOP_SAVE_XMM128_FAR 9
#define UNW_UWOP_PUSH_MACHFRAME  10

struct UNWIND_CODE_ENTRY
{
  BYTE CodeOffset;
  BYTE UnwindOp : 4;
  BYTE OpInfo : 4;
};

struct UNWIND_INFO_HEADER
{
  BYTE VersionAndFlags; // Version:3, Flags:5
  BYTE SizeOfProlog;
  BYTE CountOfCodes;
  BYTE FrameRegAndOffset;          // FrameRegister:4, FrameOffset:4
  UNWIND_CODE_ENTRY UnwindCode[1]; // variable length
};

// GP register reference by index (0=Rax..15=R15). CONTEXT stores Rax..R15 consecutively.
static __forceinline DWORD64 &contextGpReg(CONTEXT &ctx, unsigned regNum) { return (&ctx.Rax)[regNum]; }

// Lightweight virtual unwind: process UNWIND_INFO codes to update CONTEXT.
// Updates Rip, Rsp, and callee-saved GP registers (PUSH_NONVOL, SAVE_NONVOL).
// Returns true on success, false on failure (PUSH_MACHFRAME, unknown opcode).
static __forceinline bool lightweightVirtualUnwind(PRUNTIME_FUNCTION func, ULONG64 imageBase, CONTEXT &ctx)
{
  // Collect chain entries: primary -> chained -> chained ...
  struct ChainEntry
  {
    const UNWIND_INFO_HEADER *info;
    PRUNTIME_FUNCTION func;
  };
  constexpr int MAX_CHAIN = 8;
  ChainEntry chain[MAX_CHAIN];
  int chainLen = 0;

  {
    const UNWIND_INFO_HEADER *cur = (const UNWIND_INFO_HEADER *)(imageBase + func->UnwindData);
    PRUNTIME_FUNCTION curFunc = func;
    while (chainLen < MAX_CHAIN)
    {
      chain[chainLen++] = {cur, curFunc};
      if (!(cur->VersionAndFlags & 0x20)) // UNW_FLAG_CHAININFO
        break;
      unsigned codeSlots = (cur->CountOfCodes + 1) & ~1u;
      curFunc = (PRUNTIME_FUNCTION)&cur->UnwindCode[codeSlots]; //-V1027 x64 ABI: chained RUNTIME_FUNCTION follows unwind codes
      cur = (const UNWIND_INFO_HEADER *)(imageBase + curFunc->UnwindData);
    }
  }

  // SET_FPREG: use frame register as base RSP (handles alloca).
  const UNWIND_INFO_HEADER *primaryInfo = chain[0].info;
  unsigned frameReg = primaryInfo->FrameRegAndOffset & 0x0F;
  if (frameReg != 0)
  {
    unsigned frameOffset = (primaryInfo->FrameRegAndOffset >> 4) & 0x0F;
    ctx.Rsp = contextGpReg(ctx, frameReg) - (DWORD64)frameOffset * 16;
  }
  // Save initialRsp: base for SAVE_NONVOL offsets.
  DWORD64 initialRsp = ctx.Rsp;

  DWORD ripRva = (DWORD)(ctx.Rip - imageBase);

  for (int c = 0; c < chainLen; ++c)
  {
    const UNWIND_INFO_HEADER *info = chain[c].info;
    DWORD funcBegin = chain[c].func->BeginAddress;
    int count = info->CountOfCodes;

    for (int i = 0; i < count;)
    {
      const UNWIND_CODE_ENTRY &code = info->UnwindCode[i];
      bool executed = (c > 0) || (ripRva >= (DWORD)(funcBegin + code.CodeOffset));

      switch (code.UnwindOp)
      {
        case UNW_UWOP_PUSH_NONVOL:
          if (executed)
          {
            contextGpReg(ctx, code.OpInfo) = *(DWORD64 *)ctx.Rsp;
            ctx.Rsp += 8;
          }
          i += 1;
          break;
        case UNW_UWOP_ALLOC_LARGE:
          if (code.OpInfo == 0)
          {
            if (executed && (i + 1) < count)
              ctx.Rsp += *(const USHORT *)&info->UnwindCode[i + 1] * 8;
            i += 2;
          }
          else
          {
            if (executed && (i + 2) < count)
              ctx.Rsp += *(const ULONG *)&info->UnwindCode[i + 1];
            i += 3;
          }
          break;
        case UNW_UWOP_ALLOC_SMALL:
          if (executed)
            ctx.Rsp += code.OpInfo * 8 + 8;
          i += 1;
          break;
        case UNW_UWOP_SET_FPREG: i += 1; break;
        case UNW_UWOP_SAVE_NONVOL:
          if (executed && (i + 1) < count)
          {
            ULONG offset = *(const USHORT *)&info->UnwindCode[i + 1] * 8;
            contextGpReg(ctx, code.OpInfo) = *(DWORD64 *)(initialRsp + offset);
          }
          i += 2;
          break;
        case UNW_UWOP_SAVE_NONVOL_FAR:
          if (executed && (i + 2) < count)
          {
            ULONG offset = *(const ULONG *)&info->UnwindCode[i + 1];
            contextGpReg(ctx, code.OpInfo) = *(DWORD64 *)(initialRsp + offset);
          }
          i += 3;
          break;
        case UNW_UWOP_SAVE_XMM128: i += 2; break;
        case UNW_UWOP_SAVE_XMM128_FAR: i += 3; break;
        case UNW_UWOP_PUSH_MACHFRAME: return false;
        default: return false;
      }
    }
  }

  // Pop return address
  ctx.Rip = *(DWORD64 *)ctx.Rsp;
  ctx.Rsp += 8;
  return true;
}
// The inner walk function. SEH-free so the compiler generates minimal prologue.
// stackLow/stackHigh are the target thread's stack bounds (from TEB for self-capture,
// or 0/~0 for cross-thread where we rely on SEH for safety).
DAGOR_NOINLINE
static unsigned lightweight_unwind_inner_base(void **stack, unsigned max_size, int skip_frames, CONTEXT &ctx, DWORD64 stackLow,
  DWORD64 stackHigh, CacheMissStrategy miss_strategy = CacheMissStrategy::None)
{
  // Hints: consecutive frames often come from the same module and nearby functions.
  const CachedModule *lastMod = nullptr;
  ULONG64 imageBase = 0;
  int pdataHint = -1;

  int frame = -skip_frames;

  for (; ctx.Rip && frame < (int)max_size; ++frame)
  {
    if ((unsigned)frame < max_size)
      stack[frame] = (void *)(uintptr_t)ctx.Rip;

    // Look up function entry - try last module/function first
    PRUNTIME_FUNCTION pFunc = cachedLookupOnly(ctx.Rip, lastMod, imageBase, pdataHint);
    if (!lastMod && ((uint8_t)miss_strategy & (uint8_t)CacheMissStrategy::UpdateCache))
      pFunc = tryAddAndLookup(ctx.Rip, lastMod, imageBase, pdataHint);
    if (!pFunc)
      break;

    if (!lightweightVirtualUnwind(pFunc, imageBase, ctx))
    {
      // PUSH_MACHFRAME or unknown opcode: fall back to OS
      PVOID handlerData = nullptr;
      ULONG64 establisherFrame = 0;
      RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, ctx.Rip, pFunc, &ctx, &handlerData, &establisherFrame, nullptr);
    }

    if (ctx.Rsp < stackLow || ctx.Rsp >= stackHigh)
      break;
  }

  return frame > 0 ? (unsigned)frame : 0;
}

static unsigned __forceinline lightweight_unwind_inner(void **stack, unsigned max_size, int skip_frames, CONTEXT &ctx,
  DWORD64 stackLow, DWORD64 stackHigh, CacheMissStrategy miss_strategy = CacheMissStrategy::None)
{
  if (ctx.Rip == 0 && ctx.Rsp != 0)
  {
    ctx.Rip = *(DWORD64 *)ctx.Rsp;
    ctx.Rsp += 8;
  }
  return lightweight_unwind_inner_base(stack, max_size, skip_frames, ctx, stackLow, stackHigh, miss_strategy);
}

// Entry point: capture GP regs + RIP/RSP, then unwind via lightweightVirtualUnwind.
// Three capture modes:
//   USE_INLINE_ASM_CAPTURE=1: inline asm, 8 GP MOVs + lea/mov for RIP/RSP (~10 insns)
//   USE_SETJMP_CAPTURE=1:    _setjmp intrinsic, also saves XMM6-15 we don't need (~20 insns)
//   neither:                  RtlCaptureContext, full CONTEXT (~1200 bytes, DLL call)
// All start the walk from inside this function so the first unwind iteration
// restores any callee-saved registers the compiler clobbered.
#if defined(__clang__) || defined(__GNUC__)
#define USE_INLINE_ASM_CAPTURE 1
#define USE_SETJMP_CAPTURE     0
#else // MSVC: no x64 inline asm
#define USE_INLINE_ASM_CAPTURE 0
#define USE_SETJMP_CAPTURE     1
#endif
DAGOR_NOINLINE
unsigned fast_stackhlp_fill_stack_rbp(void **stack, unsigned max_size, int skip_frames, CacheMissStrategy strategy)
{
  CONTEXT ctx;
#if USE_INLINE_ASM_CAPTURE
  __asm__ __volatile__("mov %%rbx, %0\n\t"
                       "mov %%rbp, %1\n\t"
                       "mov %%rsi, %2\n\t"
                       "mov %%rdi, %3\n\t"
                       "mov %%r12, %4\n\t"
                       "mov %%r13, %5\n\t"
                       "mov %%r14, %6\n\t"
                       "mov %%r15, %7\n\t"
                       "lea (%%rip), %8\n\t"
                       "mov %%rsp, %9\n\t"
                       : "=m"(ctx.Rbx), "=m"(ctx.Rbp), "=m"(ctx.Rsi), "=m"(ctx.Rdi), "=m"(ctx.R12), "=m"(ctx.R13), "=m"(ctx.R14),
                       "=m"(ctx.R15), "=r"(ctx.Rip), "=r"(ctx.Rsp)::"memory");
#elif USE_SETJMP_CAPTURE
  jmp_buf jb;
#pragma warning(push)
#pragma warning(disable : 4611) // _setjmp + C++ destruction: we never longjmp, just read registers
  _setjmp(jb);
#pragma warning(pop)
  const _JUMP_BUFFER *jbuf = (const _JUMP_BUFFER *)jb;
  ctx.Rbx = jbuf->Rbx;
  ctx.Rbp = jbuf->Rbp;
  ctx.Rsi = jbuf->Rsi;
  ctx.Rdi = jbuf->Rdi;
  ctx.R12 = jbuf->R12;
  ctx.R13 = jbuf->R13;
  ctx.R14 = jbuf->R14;
  ctx.R15 = jbuf->R15;
  ctx.Rip = jbuf->Rip;
  ctx.Rsp = jbuf->Rsp;
#else
  RtlCaptureContext(&ctx);
#endif

  ensureCacheReady();

  NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
  DWORD64 stackLow = (DWORD64)tib->StackLimit;
  DWORD64 stackHigh = (DWORD64)tib->StackBase;

  unsigned nframes = 0;
  __try
  {
    nframes = lightweight_unwind_inner(stack, max_size, skip_frames, ctx, stackLow, stackHigh, strategy);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return uint8_t(strategy) & uint8_t(CacheMissStrategy::ResolveWithOS) ? stackhlp_fill_stack(stack, max_size, skip_frames) : 0;
  }

  if (nframes < max_size)
    stack[nframes] = (void *)(~(uintptr_t)0);
  return nframes;
}


// ---------------------------------------------------------------------------
// Cross-thread stack capture with lightweight unwinder
// ---------------------------------------------------------------------------

int fast_get_thread_handle_callstack_lightweight(intptr_t target_thread, void **stack, uint32_t max_stack_size)
{
  HANDLE thread = (HANDLE)target_thread;
  if (SuspendThread(thread) == ~((DWORD)0)) //-V720
    return -1;

  int result = 0;

  if (ensureCacheReadyForOtherThread())
  {
    CONTEXT ctx;
    ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
    if (::GetThreadContext(thread, &ctx) != 0)
    {
      __try
      {
        result = (int)lightweight_unwind_inner(stack, max_stack_size, 0, ctx, 0, ~(DWORD64)0);
      }
      // there is no point of calling fill_stack_walk_naked if SEH happened, it still does exactly same SEH but with slower unwind
      // function.
      __except (EXCEPTION_EXECUTE_HANDLER)
      {}
    }
  }

  ResumeThread(thread);
  return result;
}

#else // !_WIN64

#error Only include this file on Win64
#include "fastStackCaptureStub.cpp"

#endif
