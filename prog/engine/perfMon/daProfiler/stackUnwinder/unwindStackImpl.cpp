// Copyright (C) Gaijin Games KFT.  All rights reserved.

namespace da_profiler
{


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4505) // unreferenced local function has been removed
#endif


static uintptr_t rewrite_pointer_if_in_stack(const uint8_t *original_stack_bottom, const uintptr_t *original_stack_top,
  const uint8_t *stack_copy_bottom, uintptr_t pointer)
{
  auto original_stack_bottom_uint = reinterpret_cast<uintptr_t>(original_stack_bottom);
  auto original_stack_top_uint = reinterpret_cast<uintptr_t>(original_stack_top);
  auto stack_copy_bottom_uint = reinterpret_cast<uintptr_t>(stack_copy_bottom);

  if (pointer < original_stack_bottom_uint || pointer >= original_stack_top_uint)
    return pointer;

  return stack_copy_bottom_uint + (pointer - original_stack_bottom_uint);
}


template <class T>
static inline T *align_up(T *d, size_t alignment)
{
  alignment--;
  return (T *)((uintptr_t(d) + alignment) & ~alignment);
}


template <class T>
static inline T *align_down(T *d, size_t alignment)
{
  alignment--;
  return (T *)((uintptr_t(d)) & ~alignment); //-V558
}


static const uint8_t *copy_stack_and_rewrite_pointers(const uint8_t *original_stack_bottom, const uintptr_t *original_stack_top,
  size_t platform_stack_alignment, uintptr_t *stack_buffer_bottom)
{
  const uint8_t *byte_src = original_stack_bottom;
  // The first address in the stack with pointer alignment. Pointer-aligned
  // values from this point to the end of the stack are possibly rewritten using
  // RewritePointerIfInOriginalStack(). Bytes before this cannot be a pointer
  // because they occupy less space than a pointer would.
  const uint8_t *first_aligned_address = align_up(byte_src, sizeof(uintptr_t));

  // The stack copy bottom, which is offset from |stack_buffer_bottom| by the
  // same alignment as in the original stack. This guarantees identical
  // alignment between values in the original stack and the copy. This uses the
  // platform stack alignment rather than pointer alignment so that the stack
  // copy is aligned to platform expectations.
  uint8_t *stack_copy_bottom =
    reinterpret_cast<uint8_t *>(stack_buffer_bottom) + (byte_src - align_down(byte_src, platform_stack_alignment));
  uint8_t *byte_dst = stack_copy_bottom;

  // Copy bytes verbatim up to the first aligned address.
  for (; byte_src < first_aligned_address; ++byte_src, ++byte_dst)
    *byte_dst = *byte_src;

  // Copy the remaining stack by pointer-sized values, rewriting anything that
  // looks like a pointer into the stack.
  const uintptr_t *src = reinterpret_cast<const uintptr_t *>(byte_src);
  uintptr_t *dst = reinterpret_cast<uintptr_t *>(byte_dst);
  for (; src < original_stack_top; ++src, ++dst)
    *dst = rewrite_pointer_if_in_stack(original_stack_bottom, original_stack_top, stack_copy_bottom, *src);

  return stack_copy_bottom;
}


static constexpr size_t platform_stack_alignment = 2 * sizeof(uintptr_t);


template <typename T>
static inline uintptr_t &as_uintptr_t(T *value)
{
  static_assert(sizeof(T) == sizeof(uintptr_t), "register state type must be of equivalent size to uintptr_t");
  return *reinterpret_cast<uintptr_t *>(value);
}


#ifdef _MSC_VER
#pragma warning(pop)
#endif


} // namespace da_profiler