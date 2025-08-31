// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <dag/dag_vector.h>
#include <util/dag_compilerDefs.h>

// #define CAPTURE_STACK_FRAMES 20

#if CAPTURE_STACK_FRAMES
#include <osApiWrappers/dag_stackHlp.h>
#include <EASTL/array.h>
#include <dag/dag_vectorMap.h>
#endif


#if DAGOR_ADDRESS_SANITIZER
extern "C" void __asan_poison_memory_region(void const volatile *addr, size_t size);
extern "C" void __asan_unpoison_memory_region(void const volatile *addr, size_t size);

#define ASAN_POISON_MEMORY_REGION(addr, size)   __asan_poison_memory_region((addr), (size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#elif 0
#define ASAN_POISON_MEMORY_REGION(addr, size)   memset(addr, 0x7C, size)
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#else
#define ASAN_POISON_MEMORY_REGION(addr, size)   ((void)(addr), (void)(size))
#define ASAN_UNPOISON_MEMORY_REGION(addr, size) ((void)(addr), (void)(size))
#endif


/**
 * \brief An allocator that works great for "cyclic" usage scenarios, such as
 * daFG's recompilations.
 * \details This allocator keeps memory pools: one for "current" cyclic iteration,
 * and one for the "previous" iteration. User is supposed to flip the blocks between
 * iterations, but as the blocks are separate, the current iteration can copy stuff
 * from the previous one. When we run out of memory for an iteration, we allocate
 * additional blocks for the iteration, which then get merged into a single block
 * after a few iterations. Deallocations work like framemem.
 * \note The allocator stores the blocks statically. Use the \p Tag to have separate
 * instances of the allocator for different usages.
 * \warning Default alignment is 8 bytes, not 16!
 * \warning Not thread-safe!!!
 * \tparam Tag A tag type to distinguish between different instances.
 */
template <class Tag>
class CycAlloc
{
  static constexpr size_t BLOCK_SIZE_ALIGN = 4096;
  static constexpr size_t DEFAULT_ALIGNMENT = 8;

  template <class T>
  static constexpr T align_up(T value, T align)
  {
    return (value + align - 1) / align * align;
  }

public:
  CycAlloc() = default;
  CycAlloc(const char *) {}
  CycAlloc(const CycAlloc &) = default;
  CycAlloc &operator=(const CycAlloc &) = default;

  static void set_name(const char *) {}

  template <class U>
  struct rebind
  {
    using other = CycAlloc<U>;
  };

  static void flip()
  {
    if (oldState.head != oldState.tail)
    {
      size_t totalSize = 0;
      for (Block *it = oldState.head; it; it = it->next)
        totalSize += it->size;

      freeState(oldState);
      if (totalSize)
        oldState.tail = oldState.head = allocBlock(totalSize);
    }
    else if (oldState.allocCount != 0)
    {
      G_ASSERT_FAIL("CycAlloc: memory leaked in oldState! Please free all old allocations before flipping!");
      oldState.allocCount = 0;
      oldState.sizeStack.clear();
#if CAPTURE_STACK_FRAMES
      oldState.stackStack.clear();
#endif
    }

    eastl::swap(state, oldState);
  }

  static void wipe()
  {
    freeState(state);
    freeState(oldState);
  }

  static void *allocate(size_t size) { return allocate(size, DEFAULT_ALIGNMENT, 0); }
  static void *allocate(size_t size, size_t align, size_t)
  {
    if (size == 0)
      return nullptr;

    uint32_t padding = !state.tail ? 0 : align_up(state.tail->used, (uint32_t)align) - state.tail->used;

    if (!state.tail || state.tail->used + padding + size > state.tail->size)
    {
      Block *block = allocBlock(size);
      G_ASSERT_RETURN(block, nullptr);

      if (state.tail)
        state.tail->next = block;
      else
        state.head = block;

      state.tail = block;
    }
    else if (padding)
    {
      // Expand last alloc by padding size.
      state.tail->used += padding;
      state.sizeStack.back() += padding;
    }

    void *result = state.tail->data + state.tail->used;
    state.tail->used += size;
    state.sizeStack.push_back(size);
    state.allocCount += 1;
#if CAPTURE_STACK_FRAMES
    ::stackhlp_fill_stack(state.stackStack[result].data(), CAPTURE_STACK_FRAMES, 0);
#endif
    ASAN_UNPOISON_MEMORY_REGION(result, size);
    return result;
  }

  static bool resizeInplace(void *data, size_t new_size)
  {
    if (!data)
      return false;

    G_ASSERT(findOwnerBlock(data) != nullptr);

    const size_t oldSize = state.sizeStack.back();

    if (!isLastInBlock(state.tail, data, oldSize))
      return false;

    if (state.tail->used - state.sizeStack.back() + new_size > state.tail->size)
      return false;

    if (new_size == oldSize)
      return true;

    state.tail->used -= oldSize;
    state.tail->used += new_size;
    state.sizeStack.back() = new_size;
#if CAPTURE_STACK_FRAMES
    ::stackhlp_fill_stack(state.stackStack[data].data(), CAPTURE_STACK_FRAMES, 0);
#endif
    if (oldSize < new_size)
      ASAN_UNPOISON_MEMORY_REGION((char *)data + oldSize, new_size - oldSize);
    else
      ASAN_POISON_MEMORY_REGION((char *)data + new_size, oldSize - new_size);

    return true;
  }

  static void deallocate(void *data, size_t size)
  {
    if (!data)
      return;

    if (Block *block = findOwnerBlock(data))
    {
      if (isLastInBlock(block, data, state.sizeStack.back()))
      {
        state.tail->used -= state.sizeStack.back();
        state.sizeStack.pop_back();
      }
      state.allocCount -= 1;
#if CAPTURE_STACK_FRAMES
      state.stackStack.erase(data);
#endif
    }
    else
    {
      // Deallocation of stuff allocated before the last flip.
      oldState.allocCount -= 1;
#if CAPTURE_STACK_FRAMES
      oldState.stackStack.erase(data);
#endif
    }

    ASAN_POISON_MEMORY_REGION(data, size);
  }

private:
  struct Block
  {
    Block *next;
    uint32_t size;
    uint32_t used;
    alignas(16) char data[];
  };
  // For alignment
  static_assert(offsetof(Block, data) == 16);

  static Block *allocBlock(size_t size)
  {
    G_ASSERT_RETURN(size, nullptr);
    constexpr size_t MALLOC_METADATA_SIZE_GUESS = 32;
    Block *block = static_cast<Block *>(
      malloc(align_up(MALLOC_METADATA_SIZE_GUESS + sizeof(Block) + size, BLOCK_SIZE_ALIGN) - MALLOC_METADATA_SIZE_GUESS));
    G_ASSERT_RETURN(block, nullptr);
    block->next = nullptr;
    block->size = size;
    block->used = 0;
    ASAN_POISON_MEMORY_REGION(block->data, block->size);
    return block;
  }

  static Block *findOwnerBlock(void *data)
  {
    Block *block = state.head;
    while (block && !isInBlock(block, data))
      block = block->next;
    return block;
  }

  static bool isInBlock(Block *block, void *data) { return data >= block->data && data < block->data + block->size; }

  static bool isLastInBlock(Block *block, void *data, size_t size) { return (char *)data + size == block->data + block->used; }

  static void freeBlock(Block *block)
  {
    ASAN_UNPOISON_MEMORY_REGION(block->data, block->size);
    free(block);
  }

  static void freeBlockList(Block *head)
  {
    while (head)
      freeBlock(eastl::exchange(head, head->next));
  }

  struct State
  {
    Block *head = nullptr;
    Block *tail = nullptr;
    dag::Vector<uint32_t> sizeStack;
#if CAPTURE_STACK_FRAMES
    dag::VectorMap<void *, eastl::array<void *, CAPTURE_STACK_FRAMES>> stackStack;
#endif
    uint32_t allocCount = 0;
  };

  static void freeState(State &st)
  {
#if CAPTURE_STACK_FRAMES
    if (st.allocCount)
    {
      logerr("CycAlloc: freeState called with %d allocations still present, this is a memory leak!", st.allocCount);
      for (auto &[ptr, stack] : st.stackStack)
        logerr("CycAlloc: Alloc at %p allocated at %s", ptr, ::stackhlp_get_call_stack_str(stack.data(), CAPTURE_STACK_FRAMES));
    }
#endif

    G_ASSERT(st.allocCount == 0);

    freeBlockList(st.head);
    st.head = st.tail = nullptr;
    st.sizeStack.clear();
    st.allocCount = 0;
#if CAPTURE_STACK_FRAMES
    st.stackStack.clear();
#endif
  }

  inline static State state;
  inline static State oldState;
};
