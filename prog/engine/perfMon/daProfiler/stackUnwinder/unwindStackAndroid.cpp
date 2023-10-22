#include "daProfilePlatform.h"

#include <libunwind.h>
#include <unistd.h>
#include <pthread.h>

#include <string>
#include <atomic>

#include "unwindStackImpl.cpp"


namespace da_profiler
{


struct ThreadStackUnwindProvider
{
  size_t max_stack_size = 1 << 19; // 4mb
  uintptr_t *buffer = nullptr;

  ThreadStackUnwindProvider(size_t sz) : max_stack_size(sz), buffer(new uintptr_t[max_stack_size]) {}
  ~ThreadStackUnwindProvider() { delete[] buffer; }
};


struct ThreadStackUnwinder
{
  unw_context_t context;
  unw_cursor_t cursor;

  pid_t tid = -1;
  uintptr_t top = 0;

  ThreadStackUnwinder(pid_t _tid, uintptr_t _top) : tid(_tid), top(_top) {}
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
  pthread_attr_t attr;
  pthread_getattr_np(thread_id, &attr);

  void *address;
  size_t size;
  pthread_attr_getstack(&attr, &address, &size);
  pthread_attr_destroy(&attr);

  return new ThreadStackUnwinder(pthread_gettid_np(thread_id), reinterpret_cast<uintptr_t>(address) + size);
}


void stop_unwind_thread_stack(ThreadStackUnwinder *s) { delete s; }


struct UnwindSignalCtx
{
  ThreadStackUnwindProvider &provider;
  ThreadStackUnwinder &unwinder;
};


static std::atomic<UnwindSignalCtx *> g_unwind_signal_ctx = nullptr;
static std::atomic<bool> g_unwind_signal_done = false;
static bool g_allow_external_threads_unwind = false;


static void unwind_signal_handler(int, siginfo_t *, void *)
{
  if (UnwindSignalCtx *ctx = g_unwind_signal_ctx.load(std::memory_order_acquire))
  {
    unw_getcontext(&ctx->unwinder.context);
    unw_init_local(&ctx->unwinder.cursor, &ctx->unwinder.context);

    unw_word_t sp;
    unw_get_reg(&ctx->unwinder.cursor, UNW_AARCH64_SP, &sp);

    const uintptr_t bottom = sp;
    const uintptr_t top = ctx->unwinder.top;
    if ((top - bottom) <= ctx->provider.max_stack_size)
    {
      const uint8_t *stack_copy_bottom = copy_stack_and_rewrite_pointers(reinterpret_cast<uint8_t *>(bottom),
        reinterpret_cast<uintptr_t *>(top), platform_stack_alignment, ctx->provider.buffer);

      unw_set_stack_boundaries(&ctx->unwinder.cursor, (unw_word_t)stack_copy_bottom,
        (unw_word_t)stack_copy_bottom + unw_word_t(top - bottom));

#define REWRITE_REG(reg)                                                                                                      \
  {                                                                                                                           \
    unw_word_t regValue;                                                                                                      \
    unw_get_reg(&ctx->unwinder.cursor, reg, &regValue);                                                                       \
    unw_set_reg(&ctx->unwinder.cursor, reg,                                                                                   \
      rewrite_pointer_if_in_stack(reinterpret_cast<uint8_t *>(bottom), reinterpret_cast<uintptr_t *>(top), stack_copy_bottom, \
        regValue));                                                                                                           \
  }

      REWRITE_REG(UNW_AARCH64_SP);
      REWRITE_REG(UNW_AARCH64_X19);
      REWRITE_REG(UNW_AARCH64_X20);
      REWRITE_REG(UNW_AARCH64_X21);
      REWRITE_REG(UNW_AARCH64_X22);
      REWRITE_REG(UNW_AARCH64_X23);
      REWRITE_REG(UNW_AARCH64_X24);
      REWRITE_REG(UNW_AARCH64_X25);
      REWRITE_REG(UNW_AARCH64_X26);
      REWRITE_REG(UNW_AARCH64_X27);
      REWRITE_REG(UNW_AARCH64_X28);
      REWRITE_REG(UNW_AARCH64_X29);

#undef REWRITE_REG
    }
  }

  g_unwind_signal_done.store(true, std::memory_order_release);
}


static int try_unwind(unw_cursor_t &cursor, uint64_t *addresses, int max_size, int skip_frames_top = 0, int skip_frames_bottom = 0)
{
  bool isStepOkay = true;
  uint64_t *it = addresses, *end = addresses + max_size;
  for (; isStepOkay && it != end; ++it, isStepOkay = unw_step(&cursor) > 0)
  {
    unw_word_t pc = 0;
    unw_get_reg(&cursor, UNW_REG_IP, &pc);

    *it = pc;
  }

  const int nframes = int(it - addresses) - skip_frames_top - skip_frames_bottom;
  if (nframes <= 0)
  {
    addresses[0] = ~uint64_t(0);
    return 0;
  }

  if (skip_frames_top > 0)
    ::memmove(addresses, addresses + skip_frames_top, nframes);

  if (nframes < max_size)
  {
    addresses[nframes] = ~uint64_t(0);
    return nframes;
  }

  return max_size;
}


int unwind_thread_stack(ThreadStackUnwindProvider &p, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size)
{
  if (gettid() != s.tid)
  {
    if (!g_allow_external_threads_unwind)
      return 0;

    if (max_size != 0)
      addresses[0] = ~uint64_t(0);

    g_unwind_signal_done.store(false, std::memory_order_release);

    UnwindSignalCtx ctx{p, s};
    g_unwind_signal_ctx.store(&ctx, std::memory_order_release);

    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_sigaction = unwind_signal_handler;
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    struct sigaction origAct;
    sigaction(SIGURG, &act, &origAct);

    if (tgkill(getpid(), s.tid, SIGURG) == 0)
      spin_wait_no_profile([]() { return !g_unwind_signal_done.load(std::memory_order_acquire); });
    else
      g_unwind_signal_done.store(true, std::memory_order_release);

    sigaction(SIGURG, &origAct, nullptr);
    g_unwind_signal_ctx.store(nullptr, std::memory_order_release);

    // Skip some frames:
    // - Empty frames before android_main()
    // - Signal handler frames: syscall() and __rt_sigreturn()
    return try_unwind(s.cursor, addresses, max_size, 2 /* skip_frames_top */, 1 /* skip_frames_bottom */);
  }

  unw_context_t context;
  unw_getcontext(&context);

  unw_cursor_t cursor;
  unw_init_local(&cursor, &context);

  return try_unwind(cursor, addresses, max_size);
}


} // namespace da_profiler