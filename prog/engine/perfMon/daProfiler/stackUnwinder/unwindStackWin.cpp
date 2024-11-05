// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <stdint.h>
#include <windows.h>

#include "unwindStackImpl.cpp"

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM_64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCH_X86_32 1
#else
#error unknown architecture
#endif

namespace da_profiler
{

// todo: we can remove priority boost on wakeup if it hadn't had one. However, probably doesn't matter (and a bit faster in profiling)
struct ScopedResumeThread
{
  HANDLE thread;
  ~ScopedResumeThread() { ResumeThread(thread); }
};

#if ARCH_X86_64 || ARCH_ARM_64
// deadlock free version. Instead of unwind callstack when thread is suspended, we copy whole stack, re-write pointers and unwind later
// can be a bit slower in total (my measurement shows ~10% slower in common cases), due to stack size, but:
//  * slowdown of profiled thread is a bit less than (~7%), since unwinding happen after thread is resumed
//  * deadlock-free. No TLS magic needed, it just works as it should

typedef LONG NTSTATUS;

static inline uintptr_t &context_stack_pointer(CONTEXT &context)
{
#if ARCH_X86_64
  return context.Rsp;
#elif ARCH_ARM_64
  return context.Sp;
#elif ARCH_X86_32
  return as_uintptr_t(&context.Esp);
#else
#error unknown architecture
#endif
}

inline uintptr_t &context_frame_pointer(CONTEXT &context)
{
#if ARCH_X86_64
  return context.Rbp;
#elif ARCH_ARM_64
  return context.Fp;
#else
  return as_uintptr_t(&context.Ebp);
#endif
}

inline uintptr_t &context_instruction_pointer(CONTEXT &context)
{
#if ARCH_X86_64
  return context.Rip;
#elif ARCH_ARM_64
  return context.Pc;
#else
  return as_uintptr_t(&context.Eip);
#endif
}

static bool PointsToGuardPage(uintptr_t stack_pointer)
{
  MEMORY_BASIC_INFORMATION memory_info;
  SIZE_T result = ::VirtualQuery(reinterpret_cast<LPCVOID>(stack_pointer), &memory_info, sizeof(memory_info));
  return result != 0 && (memory_info.Protect & PAGE_GUARD);
}

// Returns the thread environment block pointer for |thread_handle|.
static uintptr_t get_thread_stack_help(HANDLE thread_handle)
{
  // Define the internal types we need to invoke NtQueryInformationThread.
  enum EnumThreadInfoClass
  {
    ThreadBasicInformation
  };

  struct CLIENT_ID
  {
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
  };

  struct TEB
  {
    NT_TIB Tib;
    // Rest of struct is ignored.
  };
  struct ThreadBasicInformationInfo
  {
    NTSTATUS ExitStatus;
    TEB *Teb;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    LONG Priority;
    LONG BasePriority;
  };

  using NtQueryInformationThreadFunction = NTSTATUS(WINAPI *)(HANDLE, LONG, PVOID, ULONG, PULONG);

  static bool inited = false;
  static NtQueryInformationThreadFunction nt_query_information_thread = nullptr;
  if (!inited)
  {
    inited = true;
    nt_query_information_thread = reinterpret_cast<NtQueryInformationThreadFunction>(
      (void *)::GetProcAddress(::GetModuleHandle("ntdll.dll"), "NtQueryInformationThread"));
  }
  if (!nt_query_information_thread)
    return 0;

  ThreadBasicInformationInfo basic_info = {0};
  NTSTATUS status =
    nt_query_information_thread(thread_handle, ThreadBasicInformation, &basic_info, sizeof(ThreadBasicInformationInfo), nullptr);
  if (status != 0)
    return 0;

  return (uintptr_t)basic_info.Teb->Tib.StackBase;
}

static bool suspend_and_copy_stack(uintptr_t *aligned_stack_buffer, size_t stack_buffer_size, uintptr_t &stack_top, HANDLE thread,
  uintptr_t thread_stack_top,
  CONTEXT &ctx) // can be get once for thread
{
  if (SuspendThread(thread) == ~((DWORD)0)) //-V720
    return false;
  ScopedResumeThread resume = {thread};
  ctx.ContextFlags = CONTEXT_FULL;
  const bool ctxCaptured = ::GetThreadContext(thread, &ctx) != 0;
  if (!ctxCaptured)
    return false;

  uintptr_t bottom = 0;
  const uint8_t *stack_copy_bottom = nullptr;
  {
    bottom = context_stack_pointer(ctx);

    // The StackBuffer allocation is expected to be at least as large as the
    // largest stack region allocation on the platform, but check just in case
    // it isn't *and* the actual stack itself exceeds the buffer allocation
    // size.
    if ((thread_stack_top - bottom) > stack_buffer_size)
      return false;

    if (PointsToGuardPage(bottom))
      return false;

    stack_copy_bottom = copy_stack_and_rewrite_pointers(reinterpret_cast<uint8_t *>(bottom),
      reinterpret_cast<uintptr_t *>(thread_stack_top), platform_stack_alignment, aligned_stack_buffer);
  }

  stack_top = reinterpret_cast<uintptr_t>(stack_copy_bottom) + (thread_stack_top - bottom);
#define REWRITE_REG(REG)                                                                                                  \
  REG = rewrite_pointer_if_in_stack(reinterpret_cast<uint8_t *>(bottom), reinterpret_cast<uintptr_t *>(thread_stack_top), \
    stack_copy_bottom, REG)
#if ARCH_X86_64
  REWRITE_REG(ctx.R12);
  REWRITE_REG(ctx.R13);
  REWRITE_REG(ctx.R14);
  REWRITE_REG(ctx.R15);
  REWRITE_REG(ctx.Rdi);
  REWRITE_REG(ctx.Rsi);
  REWRITE_REG(ctx.Rbx);
  REWRITE_REG(ctx.Rbp);
  REWRITE_REG(ctx.Rsp);
#elif ARCH_ARM_64
  REWRITE_REG(ctx.X19);
  REWRITE_REG(ctx.X20);
  REWRITE_REG(ctx.X21);
  REWRITE_REG(ctx.X22);
  REWRITE_REG(ctx.X23);
  REWRITE_REG(ctx.X24);
  REWRITE_REG(ctx.X25);
  REWRITE_REG(ctx.X26);
  REWRITE_REG(ctx.X27);
  REWRITE_REG(ctx.X28);
  REWRITE_REG(ctx.Fp);
  REWRITE_REG(ctx.Lr);
  REWRITE_REG(ctx.Sp);
#endif

  return true;
}


struct ModuleCache
{
  bool GetModuleForAddress(uintptr_t) { return true; } // fixme: cache module, so we increase ref counter
};

struct ThreadStackUnwindProvider
{
  ModuleCache moduleCache;
  size_t max_stack_size = 1 << 19; // 4mb
  uintptr_t *buffer = nullptr;
  ThreadStackUnwindProvider(size_t sz) : max_stack_size(sz), buffer(new uintptr_t[max_stack_size]) {}
  ~ThreadStackUnwindProvider() { delete[] buffer; }
};

struct ThreadStackUnwinder
{
  HANDLE thread = nullptr;
  uintptr_t top = 0; // stack top
  ~ThreadStackUnwinder() { CloseHandle(thread); }
};

ThreadStackUnwindProvider *start_unwind(size_t max_stack_size)
{
  max_stack_size = max_stack_size >> (sizeof(uintptr_t) == 8 ? 3 : 2);
  static constexpr size_t min_stack_size = 64 << 10; // 64 kb at least! most of threads are bigger than that, we only care about
                                                     // maximum call stack
  return new ThreadStackUnwindProvider(max_stack_size < min_stack_size ? min_stack_size : max_stack_size);
}
void stop_unwind(ThreadStackUnwindProvider *p) { delete p; }

ThreadStackUnwinder *start_unwind_thread_stack(ThreadStackUnwindProvider &, intptr_t thread_id)
{
  HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
    FALSE, thread_id);
  if (!thread)
    return nullptr;
  uintptr_t top = get_thread_stack_help(thread);
  if (!top)
  {
    CloseHandle(thread);
    return nullptr;
  }
  return new ThreadStackUnwinder{thread, top};
}

void stop_unwind_thread_stack(ThreadStackUnwinder *s) { delete s; }

#if !ARCH_X86_64 && !ARCH_ARM_64
struct RUNTIME_FUNCTION
{
  DWORD BeginAddress;
  DWORD EndAddress;
};
using PRUNTIME_FUNCTION = RUNTIME_FUNCTION *;
#endif // !defined(ARCH_CPU_64_BITS)

static inline ULONG64 ContextPC(CONTEXT &context) { return ULONG64(context_instruction_pointer(context)); }

#if defined(ARCH_X86_64) || ARCH_ARM_64
static __forceinline PRUNTIME_FUNCTION LookupFunctionEntry(DWORD64 program_counter, PDWORD64 image_base)
{
  return ::RtlLookupFunctionEntry(program_counter, image_base, nullptr);
}

static __forceinline void VirtualUnwind(DWORD64 image_base, DWORD64 program_counter, PRUNTIME_FUNCTION runtime_function,
  CONTEXT &context)
{
  void *handler_data = nullptr;
  ULONG64 establisher_frame;
  KNONVOLATILE_CONTEXT_POINTERS nvcontext = {};
  ::RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, program_counter, runtime_function, &context, &handler_data, &establisher_frame,
    &nvcontext);
}
#endif

static bool TryUnwindModule(bool at_top_frame, CONTEXT &context)
{
#if ARCH_X86_64 || ARCH_ARM_64
  // Ensure we found a valid module for the program counter.
  ULONG64 image_base = 0;
  // Try to look up unwind metadata for the current function.
  PRUNTIME_FUNCTION runtime_function = LookupFunctionEntry(ContextPC(context), &image_base);

  if (runtime_function)
  {
    // DCHECK_EQ(module->GetBaseAddress(), image_base);
    VirtualUnwind(image_base, ContextPC(context), runtime_function, context);
    return true;
  }

  if (at_top_frame)
  {
    // This is a leaf function (i.e. a function that neither calls a function,
    // nor allocates any stack space itself).
#if ARCH_X86_64
    // For X64, return address is at RSP.
    if (reinterpret_cast<DWORD64 *>(context.Rsp) == nullptr) // proven to be happening. pure virtual function call?
      return false;
    context.Rip = *reinterpret_cast<DWORD64 *>(context.Rsp);
    context.Rsp += 8;
#elif ARCH_ARM_64
    // For leaf function on Windows ARM64, return address is at LR(X30).  Add
    // CONTEXT_UNWOUND_TO_CALL flag to avoid unwind ambiguity for tailcall on
    // ARM64, because padding after tailcall is not guaranteed.
    context.Pc = context.Lr;
    context.ContextFlags |= CONTEXT_UNWOUND_TO_CALL;
#else
#error Unsupported Windows 64-bit Arch
#endif
    return true;
  }

  // In theory we shouldn't get here, as it means we've encountered a function
  // without unwind information below the top of the stack, which is forbidden
  // by the Microsoft x64 calling convention.
  return false;
#else
  // Note: omit frame pointer optimization is assumed to be disabled (/Oy-)
  return false;
#endif
}

static int try_unwind(CONTEXT &thread_context, uintptr_t stack_top, uint64_t *stack, int max_stack_size, ModuleCache &module_cache)
{
  // Record the first stack frame from the context values.
  int i = 0;
  uintptr_t ip = context_instruction_pointer(thread_context);
  stack[i] = ip;
  if (!module_cache.GetModuleForAddress(ip)) // module was already unloaded when we start unwind
    return i;
  ++i;

  __try // this is happening on Xbox at least. both Rtl* functions can fail with AV
  {
    for (; i < max_stack_size;)
    {
      uintptr_t prev_stack_pointer = context_stack_pointer(thread_context);
      if (!TryUnwindModule(i == 1, thread_context))
        return i;

      ULONG64 cpc = ContextPC(thread_context);
      if (cpc == 0)
        return i;

      // Exclusive range of expected stack pointer values after the unwind.
      struct
      {
        uintptr_t start;
        uintptr_t end;
      } expected_stack_pointer_range = {prev_stack_pointer, stack_top};

#if ARCH_ARM_64
      // Leaf frames on Arm can re-use the stack pointer, so they can validly have
      // the same stack pointer as the previous frame.
      if (i == 1)
        expected_stack_pointer_range.start--;
#endif
      // Abort if the unwind produced an invalid stack pointer.
      uintptr_t sp = context_stack_pointer(thread_context);
      if (sp <= expected_stack_pointer_range.start || sp >= expected_stack_pointer_range.end)
        return i;
      if (!module_cache.GetModuleForAddress(cpc)) // module was already unloaded when we start unwind
        return i;
      stack[i++] = cpc;
    }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return 0;
  }
  return i;
}

int unwind_thread_stack(ThreadStackUnwindProvider &p, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size)
{
  uintptr_t stack_top;
  CONTEXT ctx = {0};
  if (!suspend_and_copy_stack(p.buffer, p.max_stack_size,
        stack_top, // can be get once for thread
        s.thread, s.top, ctx))
    return 0;
  return try_unwind(ctx, stack_top, addresses, max_size, p.moduleCache);
}

#elif ARCH_X86_32

struct ThreadStackUnwinder;
struct ThreadStackUnwindProvider;

// todo: module cache
ThreadStackUnwindProvider *start_unwind(size_t) { return (ThreadStackUnwindProvider *)(1); }
void stop_unwind(ThreadStackUnwindProvider *) {}

void stop_unwind_thread_stack(ThreadStackUnwinder *s) { CloseHandle((HANDLE)s); }

ThreadStackUnwinder *start_unwind_thread_stack(ThreadStackUnwindProvider &, intptr_t thread_id)
{
  HANDLE thread = OpenThread(THREAD_QUERY_INFORMATION | THREAD_QUERY_LIMITED_INFORMATION | THREAD_GET_CONTEXT | THREAD_SUSPEND_RESUME,
    FALSE, thread_id);
  return (ThreadStackUnwinder *)(thread);
}

int unwind_thread_stack(ThreadStackUnwindProvider &, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size)
{
  return 0;
  // directly and immediately unroll whole stack. there is no risk of having deadlock
  HANDLE thread = (HANDLE)(&s);
  if (SuspendThread(thread) == ~((DWORD)0)) //-V720
    return -1;
  ScopedResumeThread resume = {thread};
  CONTEXT ctx = {0};
  ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
  const bool ctxCaptured = ::GetThreadContext(thread, &ctx) != 0;
  if (!ctxCaptured)
    return 0;
  struct X86StackFrameList
  {
    const X86StackFrameList *next; // aka parent EBP
    const void *pCaller;
  } const *cur_frame = (const X86StackFrameList *)(ctx.Ebp);

  int iter = 0;
  // todo: module cache
  addresses[iter++] = uint64_t(ctx.Eip);
  for (; iter < (int)max_size; ++iter)
  {
    if (cur_frame == NULL || ::IsBadReadPtr(cur_frame, sizeof(X86StackFrameList)) || ::IsBadCodePtr((FARPROC)cur_frame->pCaller))
      break;

    addresses[iter] = uint64_t(cur_frame->pCaller);
    cur_frame = cur_frame->next;
  }
  return iter;
}

#endif

} // namespace da_profiler