#include <stdint.h>

#include "unwindStackImpl.cpp"
#include <mach/mach.h>
#include <mach/thread_act.h>
#include <pthread.h>
#include <pthread/stack_np.h>

#if defined(_M_X64) || defined(__x86_64__)
#define ARCH_X86_64 1
#elif defined(__aarch64__) || defined(_M_ARM64)
#define ARCH_ARM_64 1
#else
#error unknown architecture
#endif

namespace da_profiler
{

struct ScopedResumeThread
{
  mach_port_t thread;
  ~ScopedResumeThread() { thread_resume(thread); }
};

#if ARCH_X86_64
static constexpr mach_msg_type_number_t kThreadStateCount = x86_THREAD_STATE64_COUNT;
static constexpr thread_state_flavor_t kThreadStateFlavor = x86_THREAD_STATE64;
using ThreadContext = x86_thread_state64_t;

inline uintptr_t &context_stack_pointer(x86_thread_state64_t &context) { return as_uintptr_t(&context.__rsp); }

inline uintptr_t &context_frame_pointer(x86_thread_state64_t &context) { return as_uintptr_t(&context.__rbp); }

inline uintptr_t &context_instruction_pointer(x86_thread_state64_t &context) { return as_uintptr_t(&context.__rip); }

#elif ARCH_ARM_64
static constexpr mach_msg_type_number_t kThreadStateCount = ARM_THREAD_STATE64_COUNT;
static constexpr thread_state_flavor_t kThreadStateFlavor = ARM_THREAD_STATE64;
using ThreadContext = arm_thread_state64_t;

inline uintptr_t &context_stack_pointer(arm_thread_state64_t &context) { return as_uintptr_t(&context.__sp); }

inline uintptr_t &context_frame_pointer(arm_thread_state64_t &context) { return as_uintptr_t(&context.__fp); }

inline uintptr_t &context_instruction_pointer(arm_thread_state64_t &context) { return as_uintptr_t(&context.__pc); }
#endif

static inline bool GetThreadContext(thread_act_t target_thread, ThreadContext &state)
{
  auto count = kThreadStateCount;
  return thread_get_state(target_thread, kThreadStateFlavor, reinterpret_cast<thread_state_t>(&state), &count) == KERN_SUCCESS;
}

static bool suspend_and_copy_stack(uintptr_t *aligned_stack_buffer, size_t stack_buffer_size, uintptr_t &stack_top, mach_port_t thread,
  uintptr_t thread_stack_top,
  ThreadContext &ctx) // can be get once for thread
{
  if (thread_suspend(thread) != KERN_SUCCESS)
    return false;

  ScopedResumeThread resume = {thread};
  if (!GetThreadContext(thread, ctx))
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

    // if (PointsToGuardPage(bottom))
    //   return false;

    stack_copy_bottom = copy_stack_and_rewrite_pointers(reinterpret_cast<uint8_t *>(bottom),
      reinterpret_cast<uintptr_t *>(thread_stack_top), platform_stack_alignment, aligned_stack_buffer);
  }

  stack_top = reinterpret_cast<uintptr_t>(stack_copy_bottom) + (thread_stack_top - bottom);
#define REWRITE_REG(REG)                                                                                                  \
  REG = rewrite_pointer_if_in_stack(reinterpret_cast<uint8_t *>(bottom), reinterpret_cast<uintptr_t *>(thread_stack_top), \
    stack_copy_bottom, REG)
#if ARCH_X86_64
  REWRITE_REG(ctx.__r12);
  REWRITE_REG(ctx.__r13);
  REWRITE_REG(ctx.__r14);
  REWRITE_REG(ctx.__r15);
  REWRITE_REG(ctx.__rbx);
  REWRITE_REG(ctx.__rbp);
  REWRITE_REG(ctx.__rsp);
#elif ARCH_ARM_64
  REWRITE_REG(ctx.__x[19]);
  REWRITE_REG(ctx.__x[20]);
  REWRITE_REG(ctx.__x[21]);
  REWRITE_REG(ctx.__x[22]);
  REWRITE_REG(ctx.__x[23]);
  REWRITE_REG(ctx.__x[24]);
  REWRITE_REG(ctx.__x[25]);
  REWRITE_REG(ctx.__x[26]);
  REWRITE_REG(ctx.__x[27]);
  REWRITE_REG(ctx.__x[28]);
  REWRITE_REG(ctx.__fp);
  REWRITE_REG(ctx.__sp);
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
  mach_port_t thread = 0;
  uintptr_t top = 0;        // stack top
  ~ThreadStackUnwinder() {} // todo: add refererence
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
  uintptr_t top = reinterpret_cast<uintptr_t>(pthread_get_stackaddr_np((pthread_t)thread_id));
  if (!top)
    return nullptr;
  auto thread = pthread_mach_thread_np((pthread_t)thread_id);

  // This class suspends threads, and those threads might be suspended in dyld.
  // Therefore, for all the system functions that might be linked in dynamically
  // that are used while threads are suspended, make calls to them to make sure
  // that they are linked up.
  ThreadContext ctx;
  GetThreadContext(thread, ctx);

  return new ThreadStackUnwinder{thread, top};
}

void stop_unwind_thread_stack(ThreadStackUnwinder *s) { delete s; }

static inline uintptr_t decode_frame(uintptr_t frame_pointer, uintptr_t *return_address)
{
  if (__builtin_available(macOS 10.14, iOS 12, *))
    return pthread_stack_frame_decode_np(frame_pointer, return_address);
  const uintptr_t *fp = reinterpret_cast<uintptr_t *>(frame_pointer);
  uintptr_t next_frame = *fp;
  *return_address = *(fp + 1);
  return next_frame;
}

static int try_unwind(ThreadContext &thread_context, uintptr_t stack_top, uint64_t *stack, int max_stack_size,
  ModuleCache &module_cache)
{
#if ARCH_ARM_64
  constexpr uintptr_t align_mask = 0x1;
#elif ARCH_X86_64
  constexpr uintptr_t align_mask = 0xf;
#endif
  // Record the first stack frame from the context values.
  uintptr_t next_frame = context_frame_pointer(thread_context);
  uintptr_t frame_lower_bound = context_stack_pointer(thread_context);
  const auto is_fp_valid = [&](uintptr_t fp) {
    return next_frame >= frame_lower_bound && (next_frame + sizeof(uintptr_t) * 2) <= stack_top && (next_frame & align_mask) == 0;
  };

  int i = 0;
  uintptr_t retaddr = context_instruction_pointer(thread_context);
  if (!module_cache.GetModuleForAddress(retaddr))
    return -1;
  stack[i] = retaddr;
  if (!is_fp_valid(next_frame))
    return 0;
  for (; i < max_stack_size;)
  {
    uintptr_t frame = next_frame;
    next_frame = decode_frame(frame, &retaddr);
    frame_lower_bound = frame + 1;
    // If `next_frame` is 0, we've hit the root and `retaddr` isn't useful.
    // Bail without recording the frame.
    if (next_frame == 0)
      return i;
    if (!module_cache.GetModuleForAddress(retaddr))
      return i;
    if (!is_fp_valid(next_frame))
      return i;

    context_frame_pointer(thread_context) = next_frame;
    context_instruction_pointer(thread_context) = retaddr;
    context_stack_pointer(thread_context) = frame + sizeof(uintptr_t) * 2;
    stack[i++] = retaddr;
  }
  return i;
}

int unwind_thread_stack(ThreadStackUnwindProvider &p, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size)
{
  uintptr_t stack_top;
  ThreadContext ctx = {0};
  if (!suspend_and_copy_stack(p.buffer, p.max_stack_size,
        stack_top, // can be get once for thread
        s.thread, s.top, ctx))
    return 0;
  return try_unwind(ctx, stack_top, addresses, max_size, p.moduleCache);
}

} // namespace da_profiler