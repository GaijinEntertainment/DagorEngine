#pragma once

inline unsigned get_stack_size(const void *const *stack, unsigned max_size)
{
  unsigned stack_size = 0;

  for (stack_size = 0; stack_size < max_size; stack_size++)
    if (stack[stack_size] == (const void *)(~uintptr_t(0)))
      break;
  return stack_size;
}
