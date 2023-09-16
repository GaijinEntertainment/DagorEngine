#include <osApiWrappers/dag_stackHlpEx.h>

// uses dagor functions for stack unwinding. not safe in win64!
namespace da_profiler
{
struct ThreadStackUnwinder;
struct ThreadStackUnwindProvider;

int unwind_thread_stack(ThreadStackUnwindProvider &, ThreadStackUnwinder &s, uint64_t *addresses, size_t max_size)
{
#if _TARGET_64BIT
  return ::get_thread_handle_callstack(intptr_t(&s), (void **)addresses, max_size);
#else
  static constexpr int MAX_STACK_SIZE = 128;
  void *stack[MAX_STACK_SIZE];
  int ret = ::get_thread_handle_callstack(intptr_t(&s), (void **)stack, max_size < MAX_STACK_SIZE ? max_size : MAX_STACK_SIZE);
  for (int i = 0; i < ret; ++i)
    addresses[i] = uintptr_t(stack[i]);
  return ret;
#endif
}

void stop_unwind_thread_stack(ThreadStackUnwinder *s) { sampling_close_thread((intptr_t)s); }

ThreadStackUnwinder *start_unwind_thread_stack(ThreadStackUnwindProvider &, intptr_t thread_id)
{
  return (ThreadStackUnwinder *)sampling_open_thread(thread_id);
}

ThreadStackUnwindProvider *start_unwind(size_t) { return (ThreadStackUnwindProvider *)(1); }
void stop_unwind(ThreadStackUnwindProvider *) {}

} // namespace da_profiler