// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_fastStackCapture.h>
#include <osApiWrappers/dag_stackHlp.h>
#include <osApiWrappers/dag_stackHlpEx.h>

void fast_stackhlp_init_cache(bool) {}

unsigned fast_stackhlp_fill_stack_cached(void **stack, unsigned max_size, int skip_frames, CacheMissStrategy)
{
  return stackhlp_fill_stack(stack, max_size, skip_frames);
}

unsigned fast_stackhlp_fill_stack_rbp(void **stack, unsigned max_size, int skip_frames, CacheMissStrategy)
{
  return stackhlp_fill_stack(stack, max_size, skip_frames);
}

int fast_get_thread_handle_callstack(intptr_t thread_handle, void **stack, uint32_t max_stack_size)
{
  return get_thread_handle_callstack(thread_handle, stack, max_stack_size);
}

int fast_get_thread_handle_callstack_lightweight(intptr_t thread_handle, void **stack, uint32_t max_stack_size)
{
  return get_thread_handle_callstack(thread_handle, stack, max_stack_size);
}
