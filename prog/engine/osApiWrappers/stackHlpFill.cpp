// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_atomic.h>
#include <osApiWrappers/dag_spinlock.h>

inline int clamp0(int a) { return a > 0 ? a : 0; }

#if _TARGET_PC_WIN || _TARGET_XBOX
#include <windows.h>
#include <winnt.h>

#if !_TARGET_64BIT

unsigned fill_stack_walk_naked(void **stack, unsigned size, CONTEXT &ctx, int frames_to_skip = 0)
{
  // Note: omit frame pointer optimization is assumed to be disabled (/Oy-)
  struct X86StackFrameList
  {
    const X86StackFrameList *next; // aka parent EBP
    const void *pCaller;
  } const *cur_frame = (const X86StackFrameList *)(ctx.Ebp);

  int iter = -frames_to_skip;
  if (iter >= 0)
    stack[iter++] = (void *)(ctx.Eip);
  for (; iter < (int)size; ++iter)
  {
    if (cur_frame == NULL || ::IsBadReadPtr(cur_frame, sizeof(X86StackFrameList)) || ::IsBadCodePtr((FARPROC)cur_frame->pCaller))
      break;

    if (iter >= 0)
      stack[iter] = (void **)cur_frame->pCaller;

    cur_frame = cur_frame->next;
  }
  return iter > 0 ? iter : 0;
}

#if _TARGET_STATIC_LIB
inline unsigned fill_stack_walk(void **stack, unsigned size, CONTEXT &ctx, int frames_to_skip)
{
  return fill_stack_walk_naked(stack, size, ctx, frames_to_skip);
}
inline unsigned fill_stack_walk(void **stack, unsigned size, const void *context, int frames_to_skip = 0)
{
  return fill_stack_walk_naked(stack, size, *(CONTEXT *)context, frames_to_skip);
}
#else

#include <dbghelp.h>
extern OSSpinlock dbghelp_spinlock;

unsigned fill_stack_walk(void **stack, unsigned size, CONTEXT &ctx, int frames_to_skip = 0)
{
  STACKFRAME frm;
  memset(&frm, 0, sizeof(frm));
  frm.AddrPC.Offset = ctx.Eip;
  frm.AddrPC.Mode = AddrModeFlat;
  frm.AddrStack.Offset = ctx.Esp;
  frm.AddrStack.Mode = AddrModeFlat;
  frm.AddrFrame.Offset = ctx.Ebp;
  frm.AddrFrame.Mode = AddrModeFlat;

  HANDLE ph = GetCurrentProcess();
  HANDLE th = GetCurrentThread();
  int iter = -frames_to_skip;
  // All DbgHelp functions are single threaded.
  // https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-undecoratesymbolname
  OSSpinlockScopedLock lock(dbghelp_spinlock);
  for (; iter < size; ++iter)
    if (!StackWalk(IMAGE_FILE_MACHINE_I386, ph, th, &frm, &ctx, NULL, ::SymFunctionTableAccess, ::SymGetModuleBase, NULL))
      break;
    else if (iter >= 0)
      stack[iter] = (void *)frm.AddrPC.Offset;
  return iter > 0 ? iter : 0;
}
unsigned fill_stack_walk(void **stack, unsigned size, const void *context, int frames_to_skip = 0)
{
  CONTEXT ctx; // We don't want to overwrite whatever is passed as context
  memcpy(&ctx, context, sizeof(ctx));
  return fill_stack_walk(stack, size, ctx, frames_to_skip);
}

#endif

unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int skip_frames)
{
  CONTEXT ctx;
  RtlCaptureContext(&ctx);
#if _TARGET_STATIC_LIB
  // Note: omit frame pointer optimization is assumed to be disabled (/Oy-)
  const unsigned nframes = fill_stack_walk_naked(stack, max_size, ctx, skip_frames);
#else
  const unsigned nframes = fill_stack_walk(stack, max_size, &ctx, skip_frames);
#endif
  if (nframes < max_size)
    stack[nframes] = (void *)(~uintptr_t(0));
  return nframes;
}

bool get_thread_handle_is_walking_stack(intptr_t) { return false; }

#else // _TARGET_64BIT

#define USE_RTL_CAPTURE_STACK              1 // on xbox it is safer. performance wise difference is 4%
#define USE_NTQUERY_TO_ACCESS_THREAD_STATE 1

#if USE_NTQUERY_TO_ACCESS_THREAD_STATE

// Issue:
// Win64 stack walk uses spinlock to acquire ModuleList FunctionTable.
// Not even RWlock, just stupid spinlock.
// So, if we Suspend target thread in a locked state, we can not retrieve it's callstack.
// that is - suspend during 1) stack unwinding 2) loading modules 3) RtlPcToFileHeader (which we never call)
// There is absolutely now way to get this Lock state or tryLock same lock, it is not exposed.
// Same lock  is acquired in loading modules also (which is obvious). So we want to avoid this deadlock.
//
// Solutions:
// we provide two ways: portable and not-so-portable and an idea for ideal solution in an ideal way.
// But let's talk about all possible solutions
//
// Naive (but few lines, and portable) solution would be:
//
// Naive1) To have our own spinlock above all, and use trylock of it when accessing stack of suspended thread. Contention in dev builds
// will be VERY high.
//
// Naive2) To have interlocked counter of number of threads trying to get backtrace, and ignore backtrace if counter was > 0 when
// accessing stack of suspended thread. Naive2 - is exactly what Mozilla solution does. And they even suggested (in comment) our
// Porable solution None of those solves (un)loading dlls (which happen more often than we would prefer,
//   and both solutions makes much less stacks possible  when accessing stack of suspended thread.
//
// Two "good" ways.
// Portable and not-so-portable. Both use tls.
//
// Portable A)
// Provide external sampling thread with threadlocal variable of "backtracing".
// Requires explicit hook, fine for profiler not so fine for watchdog. Doesn't save in case of dll load.
//
// Steps
// 1) use thread local (dynamic tls) variable to set "is_walking_stack" variable in each thread where we are unwinding stack.
// then, when we unwind OTHER threads, we do the following:
// 2) after SuspendThread, we check this suspended thread local variable
// 3) In this variant, the hook is called on thread registration, providing address of this local variable. So we can easily check it.
//
// Loading DLL hazard can still be happening, watchdog is not protected.
//
// Not-So-Portable B) (but simpler to use)
//
// Steps are same, but in unportable to other Platforms (current) version, we acces threadlocal variable of the suspended thread! (just
// as Mozilla suggested)
//
// While it seems dangerous, this is quiet portable thing to do _on windows_, although require A LOT of internal winnt knowledge.
// see! https://github.com/multikill/TLS_Reader
//
// Because, to keep compatibility with current executables winnt has to provide same TEB mapping (regarding TLS) forever.
// The only unportable thins is QueryThreadInformation, but debuggers, profilers and drivers also rely on it.
// So, I would say it is rather portable code (almost guaranteed).
//
// Then if variable is 'on', then thread was stopped during stack unwinding, and we can't safely unwind stack there so we just return.
// This is same check as in portableA solution (which we still use in samler/profiler), so we check it twice (in external and internal
// code).
//
// With that solution LoaderLock is also checked with comparing LockCount in pLoaderLock in PEB (process table).
//
// That seem to be less portable, as we only rely on just partially documented RtlCriticalSection content and undocumented pLoaderLock
// in PEB. However, in a worst case scenario we will skip some callstacks from threads (if we would rather not do it).
//
// Even more portable solution would be to get from PEB table from current thread and call tryLock on loaderLock.
// It would be 30x times slower than current code, but will make RtlCriticalSection "opaque" to us.
//
// All in all, this allows fast and more safe unwinding in win64 and win32.
// However, portable A call (hook) can be completely removed, as it seems (if non-portable is working)
//
// No, how to make ideal solition.
// However, there is a better way for stackwalk!
//
// OK, THE ONLY issue we want to avoid is to dead-locking of Spinlick 'PsLoadedModuleSpinLock' inside RtlLookupFunctionEntry
// this is the only deadlock we are facing. see https://github.com/bigzz/WRK
//
// Nor loaders lock, neither backtracing themselves are dangerous.
//
// however, this is NO way to obtain that spin lock or it state!
//  (Rtl doesn't expose try lock, or lock query, or pointer to it is noowhere in open)
//
// So, alternatively we could generate our own function table RtlLookupFunctionEntry
// And that is surprisingly easy to do!
// We would need just two dbghelp functions: EnumerateLoadedModules64, ImageDirectoryEntryToData
//
// 1)  after each dll load (or whenever needed) use EnumerateLoadedModules64 or request PEB_LDR_DATA from NtCurrentTeb() (also these
// are documented sections) 2)  fill sorted base on ImageBase functiontables ImageBase/ImageSize of each module
// (RtlInsertInvertedFunctionTable code)
//   using ImageDirectoryEntryToData with IMAGE_DIRECTORY_ENTRY_EXCEPTION
// 3)  than on backtrace implement RtlLookupFunctionTable ( IN PVOID ControlPc, OUT PVOID *ImageBase, OUT PULONG SizeOfTable)
//    which is find function tables which control is inside ImageBase/ImageSize (binary search)
// 4) and than binary search function tables until we hit required function
//
// Obviously, we would need our own SRWlock for that table load/update either, but unlike freacking windows,
//  we can check state of this spinlock (or just simply use trylock always when in external thread). It wouldn't be updated often.
//
// That solution would require some memory, but implementation is as easy as copy-paste some functions:
//  RtlLookupFunctionEntry, RtlLookupFunctionTable, RtlpSearchInvertedFunctionTable and RtlpConvertFunctionEntry
//  (altogether would be may be 300 LoCs) from https://github.com/bigzz/WRK.git lookup.c, exdsptch.c:
//  using dbgHelp.h functions EnumerateLoadedModules64, ImageDirectoryEntryToData
// The _only_ questions is 1) how to check if some dlls were loaded 2) even more importantly, when they were unloaded

// 1) loading dlls can be done just relying on misses (may be maintaing "unknown addresses" list)
// also there is some hook for that in windows for deferred load (and unload)
// https://docs.microsoft.com/en-us/cpp/build/reference/understanding-the-helper-function?view=msvc-170
// Dealing with LoadLibrary and especially UnloadLibrary is much harder. Loading can be updated based on "miss", but unloading is much
// harder simple solution would be to just add reference to all loaded libraries added to list, and then just sometimes decrease
// reference https://blogs.msmvps.com/vandooren/2006/10/09/preventing-a-dll-from-being-unloaded-by-the-app-that-uses-it/ so, basically.
// 1) use 'miss' criteria to trigger update. 2) add references to all dlls that have been enumerated. 3) from time to time check if
// handles are still needed (simply unload all held dlls, than re-fill all again)

#include "masqueradePEB.h"
#include <VersionHelpers.h>

static pNtQueryInformationThread NtQueryInformationThread = NULL;
static uint32_t is_walking_tls_index = TLS_OUT_OF_INDEXES;
#if !_TARGET_XBOX
static constexpr bool checkLoaderLockCount = true;
#else
static constexpr bool checkLoaderLockCount = false; // dlls are not loading under xbox?
#endif
static constexpr int TLS_MAX_COUNT = TLS_EXPANSION_SLOTS + TLS_MINIMUM_AVAILABLE;
// static OSSpinlock initing_stack_walk_spinlock;
static struct InitIsWalking
{
  InitIsWalking()
  {
// Due to changes in LockCount interpretation in Visa
// https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/displaying-a-critical-section
#if !_TARGET_XBOX
    if (IsWindowsVistaOrGreater())
#endif
    {
      NtQueryInformationThread =
        (pNtQueryInformationThread)(void *)GetProcAddress(GetModuleHandle("ntdll.dll"), "NtQueryInformationThread");
      is_walking_tls_index = NtQueryInformationThread ? TlsAlloc() : TLS_OUT_OF_INDEXES;
    }
  }
  ~InitIsWalking() { TlsFree(is_walking_tls_index); }

} it_is_walking_stack;


// this is rather portable code, as it relies on runtime compatibility !
static inline int *get_thread_is_walking_stack_handle(const PTEB teb)
{
  // If the variable is in the main array, return it.
  if (is_walking_tls_index < TLS_MINIMUM_AVAILABLE)
    return (int *)&teb->TlsSlots[is_walking_tls_index]; // TlsExpansionSlots & TlsSlots ARE portable to use!
  // Otherwise it's in the expansion array.
  if (!teb->TlsExpansionSlots)
    return nullptr;
  // Fetch the value from the expansion array.
  return (int *)&teb->TlsExpansionSlots[is_walking_tls_index - TLS_MINIMUM_AVAILABLE];
}
static inline int *get_thread_is_walking_stack_handle()
{
  return is_walking_tls_index >= TLS_MAX_COUNT ? nullptr : get_thread_is_walking_stack_handle(NtCurrentTeb());
}

static inline int &get_thread_is_walking_stack_handle_rw(int &dummy)
{
  int *tls = get_thread_is_walking_stack_handle();
  return tls ? *tls : dummy;
}

struct ScopeWalkingStack
{
  int dummy;
  int &is;
  ScopeWalkingStack() : is(get_thread_is_walking_stack_handle_rw(dummy)) { interlocked_release_store(is, 1); }
  ~ScopeWalkingStack() { interlocked_release_store(is, 0); }
};

bool get_thread_handle_is_walking_stack(intptr_t thread_handle)
{
  if (is_walking_tls_index >= TLS_MAX_COUNT)
    return false;

  // this is ALMOST portable code. NtQueryInformationThread is not guaranteed to return PTEB, but that's fine, it will just fail then
  // (can result in deadlock, as it used to be before that code).
  const PTEB teb = get_thread_pteb(NtQueryInformationThread, (HANDLE)thread_handle);
  if (!teb)
    return false;
  // portable again
  const int *is_walking = get_thread_is_walking_stack_handle(teb);
  if (is_walking && *is_walking)
    return true;

  if (checkLoaderLockCount)
  {
    // this is a bit more risky/non-portable code, as we rely on RtlCriticalSection meaning. It is known to be changed after WindowsXP
    // but it is somehow documented, so we are probably fine.
    // worst case scenario - we would randomly skip some _external_ call stacks

    // also, we can trylock loaderLock crit section. That is a bit more portable, and yet slower code (as it locks crit section!)
    // if (!RtlTryEnterCriticalSection(pLoaderLock))
    //  return true;
    // RtlLeaveCriticalSection(pLoaderLock);
    // RtlTryEnterCriticalSection/RtlLeaveCriticalSection has to be resolved from ntdll, same as NtQueryInformationThread, but thats
    // portable

    auto pLock = (teb->ProcessEnvironmentBlock)->LoaderLock;
    // this is reasonable check (DWORD)pLock->OwningThread == GetThreadId((HANDLE)thread_handle).
    //  but we can't use it, until we know only one thread is waiting. OwningThread is set later then lock is obtained, and so it is
    //  race
    if ((pLock->LockCount & 1) == 0) // we are waiting for loaderLock in this thread. It is dangerous, as can result in deadlock
      return true;
  }
  return false;
}
#else
// portable version. just have one global counter
static int stack_walker_counter = 0;
struct ScopeWalkingStack
{
  ScopeWalkingStack() { interlocked_increment(stack_walker_counter); }
  ~ScopeWalkingStack() { interlocked_decrement(stack_walker_counter); }
};
bool get_thread_handle_is_walking_stack(intptr_t) { return interlocked_acquire_load(stack_walker_counter) != 0; }
#endif

#if UNWIND_BROKEN_STACK
static bool is_address_readable(const void *address, bool def)
{
  MEMORY_BASIC_INFORMATION mbi;

  if (VirtualQuery(address, &mbi, sizeof(mbi)))
  {
    const DWORD flags = (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READONLY | PAGE_READWRITE);
    return (mbi.State == MEM_COMMIT) && ((mbi.Protect & flags) != 0);
  }
  return false;
}
#else
static inline bool is_address_readable(const void *, bool d) { return d; }
#endif

static unsigned fill_stack_walk_naked_unsafe(void **stack, unsigned size, CONTEXT &context, int frames_to_skip = 0) // requires copy of
                                                                                                                    // Context
{
#if defined(_M_ARM64)
#define EIP_REG Pc
#define ESP_REG Sp
#else
#define EIP_REG Rip
#define ESP_REG Rsp
#endif

  if (context.EIP_REG == 0 && context.ESP_REG != 0 && is_address_readable((const void *)context.ESP_REG, true))
  {
    context.EIP_REG = (ULONG64)(*(PULONG64)context.ESP_REG);
    // reset the stack pointer (+8 since we know there has been no prologue run requiring a larger number since RIP == 0)
    context.ESP_REG += 8;
  }
  // inside RtlLookupFunctionEntry there is an (optional) spinlock.
  // so, if we are calling those functions on suspended thread, we can face a deadlock
  // prevent it with tls and checking that tls
  int frameIter = -frames_to_skip;
  for (; context.EIP_REG && frameIter < (int)size; ++frameIter)
  {
    if (uint32_t(frameIter) < size)
      stack[frameIter] = (void *)(uintptr_t)context.EIP_REG;

    PRUNTIME_FUNCTION pRuntimeFunction = nullptr;
    ULONG64 imageBase = 0;
#define TRY       __try
#define EXCEPT(a) __except (a)
    TRY { pRuntimeFunction = (PRUNTIME_FUNCTION)RtlLookupFunctionEntry(context.EIP_REG, &imageBase, NULL); }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER) { break; }

    if (pRuntimeFunction)
    {
      VOID *handlerData = NULL;
      ULONG64 framePointers[2] = {0, 0};
      // Under at least the XBox One platform, RtlVirtualUnwind can crash here.
      TRY
      {
        RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.EIP_REG, pRuntimeFunction, &context, &handlerData, framePointers, NULL);
      }
      EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
        context.EIP_REG = 0;
        context.ContextFlags = 0;
      }
    }
    else
    {
      // If we don't have a RUNTIME_FUNCTION, then we've encountered an error of some sort (mostly likely only for cases of corruption)
      // or leaf function (which doesn't make sense, given that we are moving up in the call sequence). Adjust the stack appropriately.
      // we can do
      // if (context.ESP_REG && is_address_readable((const void*)context.ESP_REG, false))
      //{
      //  context.EIP_REG  = (ULONG64)(*(PULONG64)context.ESP_REG);
      //  context.ESP_REG += 8;
      //} else
      break; // but since we use this function only in rare cases of crashes and in sampling, just skip reminding callstack
    }
#undef TRY
#undef EXCEPT
  }
  return frameIter > 0 ? frameIter : 0;
}

unsigned fill_stack_walk_naked(void **stack, unsigned size, CONTEXT &context, int frames_to_skip = 0) // requires copy of Context
{
  ScopeWalkingStack guard; // so we can use seh
  return fill_stack_walk_naked_unsafe(stack, size, context, frames_to_skip);
}

static inline unsigned fill_stack_walk(void **stack, unsigned size, CONTEXT &ctx, int frames_to_skip = 0)
{
  return fill_stack_walk_naked(stack, size, ctx, frames_to_skip);
}

static inline unsigned fill_stack_walk(void **stack, unsigned size, const void *context, int frames_to_skip = 0)
{
  CONTEXT ctx; // We don't want to overwrite whatever is passed as context
  memcpy(&ctx, context, sizeof(ctx));
  return fill_stack_walk_naked(stack, size, ctx, frames_to_skip);
}

unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int skip_frames)
{
#if USE_RTL_CAPTURE_STACK
  // inside RtlCaptureStackBackTrace there is RtlLookupFunctionEntry
  // inside RtlLookupFunctionEntry there is an (optional) spinlock.
  // so, if we are calling those functions on suspended thread, we can face a deadlock
  // prevent it with tls and checking that tls
  unsigned nframes;
  {
    ScopeWalkingStack guard;
    nframes = RtlCaptureStackBackTrace(skip_frames, max_size, stack, NULL);
  }
#else
  CONTEXT ctx;
  RtlCaptureContext(&ctx);
  const unsigned nframes = fill_stack_walk(stack, max_size, ctx, skip_frames);
#endif
  if (nframes < max_size)
    stack[nframes] = (void *)(~uintptr_t(0));
  // inside RtlCaptureStackBackTrace there is RtlLookupFunctionEntry
  // inside RtlLookupFunctionEntry there is an (optional) spinlock.
  // so, if we are calling those functions on suspended thread, we can face a deadlock
  // that's why we fail in get_thread_handle_callstack if there is global_stackwalk_counter
  return nframes;
}
#endif // _TARGET_64BIT

unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size, const void *ctx_ptr)
{
  // called only on exception, so safe
  return fill_stack_walk(stack, max_size, ctx_ptr, 0);
}

#elif _TARGET_APPLE | _TARGET_PC_LINUX
#include <execinfo.h>
#include <signal.h>
#include <string.h>

int g_in_backtrace = 0; // for reading in signal handlers
unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int /*skip_frames*/)
{
  __atomic_fetch_add(&g_in_backtrace, 1, __ATOMIC_RELAXED);
  const unsigned nframes = clamp0(backtrace((void **)stack, max_size));
  __atomic_fetch_sub(&g_in_backtrace, 1, __ATOMIC_RELAXED);
  if (nframes < max_size)
    stack[nframes] = (void *)(~uintptr_t(0));
  return nframes;
}

unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size, const void * /*ctx_ptr*/)
{
  if (max_size)
    stack[0] = (void *)(~uintptr_t(0));
  return 0;
}
#elif _TARGET_C3

#elif _TARGET_ANDROID

#include <util/dag_globDef.h>
#include <string.h>

#define UNW_LOCAL_ONLY
#include <libunwind.h>


unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int skip_frames)
{
  unw_context_t context;
  unw_getcontext(&context);

  unw_cursor_t cursor;
  unw_init_local(&cursor, &context);

  bool isStepOkay = true;
  void **it = stack, **end = stack + max_size;
  for (; isStepOkay && it != end; ++it, isStepOkay = unw_step(&cursor) > 0)
  {
    unw_word_t pc = 0;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);

    *it = reinterpret_cast<void *>(pc);
  }

  const unsigned nframes = max(int(it - stack) - skip_frames, 0);
  if (nframes != 0 && skip_frames > 0)
    ::memmove(stack, stack + skip_frames, nframes);

  if (nframes < max_size)
  {
    stack[nframes] = (void *)(~uintptr_t(0));
    return nframes;
  }

  return max_size;
}

unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size, const void * /*ctx_ptr*/)
{
  if (max_size)
    stack[0] = (void *)(~uintptr_t(0));
  return 0;
}

#else
#include <string.h>

unsigned stackhlp_fill_stack(void **stack, unsigned max_size, int /*skip_frames*/)
{
  if (max_size)
    stack[0] = (void *)(~uintptr_t(0));
  return 0;
}

unsigned stackhlp_fill_stack_exact(void **stack, unsigned max_size, const void * /*ctx_ptr*/)
{
  if (max_size)
    stack[0] = (void *)(~uintptr_t(0));
  return 0;
}
#endif

namespace
{
thread_local stackhelp::ext::CallStackContext extended_call_stack_capture_context;
}

stackhelp::ext::CallStackContext stackhelp::ext::set_extended_call_stack_capture_context(CallStackContext new_context)
{
  CallStackContext r = extended_call_stack_capture_context;
  extended_call_stack_capture_context = new_context;
  return r;
}

stackhelp::ext::CallStackResolverCallbackAndSizePair stackhelp::ext::capture_extended_call_stack(stackhelp::CallStackInfo stack)
{
  return extended_call_stack_capture_context(stack);
}
