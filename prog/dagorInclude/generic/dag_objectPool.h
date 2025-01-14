//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/type_traits.h>
#include <EASTL/algorithm.h>
#include <EASTL/numeric.h>
#include <util/dag_compilerDefs.h>
#include <debug/dag_assert.h>
#include "dag_bitset.h"

// Pool should scan from oldest to newest block for a free slot
struct ObjectPoolBlockSearchOrderFrontToBack
{};
// Pool should scan from newest to oldest block for a free slot
struct ObjectPoolBlockSearchOrderBackToFront
{};

// Default block size is the number of addressing bits
constexpr size_t OBJECT_POOL_DEFAULT_BLOCK_SIZE = sizeof(intptr_t) * 8;

// This policy tracks free object slots with a bitfield and can free individual objects.
// The search policy for free blocks is from oldest to newest.
struct ObjectPoolPolicyRandomAllocationAndFree
{
  using BlockSearchOrder = ObjectPoolBlockSearchOrderFrontToBack;
  template <typename T, size_t BlockSize>
  struct BlockData
  {
    Bitset<BlockSize> freeMask = Bitset<BlockSize>{}.set();

    bool hasSpace() const { return freeMask.any(); }
    size_t findFreeSlot() const { return freeMask.find_first(); }
    void markAsAllocated(size_t index) { freeMask.reset(index); }
    void markAsFree(size_t index) { freeMask.set(index); }
    void markAllAsFree() { freeMask.set(); }
    bool hasAnyAllocated() const
    {
      // faster as !all, as all is implemented as count() == size()
      // NOTE: will be slower when block size is too large
      return (~freeMask).any();
    }
    bool isAllocated(size_t index) const { return !freeMask.test(index); }
    size_t allocatedSlots() const { return BlockSize - freeMask.count(); }
  };
};

// This policy tracks free object slots with the number of allocated slots and can only free objects when a whole block is freed.
// The search policy for free blocks is from newest to oldest.
struct ObjectPoolPolicyLinearAllocationAndNoFree
{
  using BlockSearchOrder = ObjectPoolBlockSearchOrderBackToFront;
  template <typename T, size_t BlockSize>
  struct BlockData
  {
    size_t allocated = 0;

    bool hasSpace() const { return allocated < BlockSize; }
    size_t findFreeSlot() const { return allocated; }
    void markAsAllocated(size_t index)
    {
      EA_UNUSED(index);
      G_ASSERT(index == allocated);
      ++allocated;
    }
    // If this complains, you are using the wrong policy, no individual frees with this policy
    void markAsFree(size_t index);
    void markAllAsFree() { allocated = 0; }
    bool hasAnyAllocated() const { return allocated > 0; }
    bool isAllocated(size_t index) const { return index < allocated; }
    size_t allocatedSlots() const { return allocated; }
  };
};

// Object pool of T that allocates memory to store BlockSize objects in one contiguous block each.
// Behavior and support for some features is dictated by the used policy.
// (sort of) TODO:
// - Version with user supplied memory allocator, right now used eastl::vector default allocator and new for object pools.
// - May remove need for unique_ptr as it has some (minor overhead)
template <typename T, size_t BlockSize = OBJECT_POOL_DEFAULT_BLOCK_SIZE, typename Policy = ObjectPoolPolicyRandomAllocationAndFree>
class ObjectPool
{
  // Shortcut for block search order
  using BlockSearchOrder = typename Policy::BlockSearchOrder;

  // Implements constructor handler for trivially constructible types
  template <bool isTrivial = true, bool = false>
  struct ConstructorHandler
  {
    // basically pod init
    static T *construct(void *ptr) { return reinterpret_cast<T *>(ptr); }
    // have a fallback for parameterized case
    template <typename B, typename... Args>
    static T *construct(void *ptr, B &&b, Args &&...args)
    {
      // "::" forces use of default inplace allocator
      return ::new (ptr) T(eastl::forward<B>(b), eastl::forward<Args>(args)...);
    }
  };
  // Implements constructor handler for non trivially constructible types
  template <bool b>
  struct ConstructorHandler<false, b>
  {
    template <typename... Args>
    static T *construct(void *ptr, Args &&...args)
    {
      // "::" forces use of default inplace allocator
      return ::new (ptr) T(eastl::forward<Args>(args)...);
    }
  };
  // Implements destructor handler for trivially destructible types
  template <bool isTrivial = true, bool = false>
  struct DestructorHandler
  {
    static void destruct(T *) {}
  };
  // Implements destructor handler for non trivially destructible types
  template <bool b>
  struct DestructorHandler<false, b>
  {
    static void destruct(T *ptr) { ptr->~T(); }
  };
  // Composites constructor handler and destructor handler for T
  struct TypeHandler : ConstructorHandler<eastl::is_trivially_default_constructible<T>::value, false>,
                       DestructorHandler<eastl::is_trivially_destructible<T>::value, false>
  {};

  class Block : public Policy::template BlockData<T, BlockSize>
  {
    using StorageUnit = typename eastl::aligned_storage<sizeof(T), alignof(T)>::type;

#if !defined(DAGOR_ADDRESS_SANITIZER)
    // Don't initialize block memory - don't waste any time for that, if possible - its perfectly fine
    StorageUnit objects[BlockSize]; //-V730
#else
    // Under ASAN, we want dedicated allocation to get propper use-after-free reports w/ callstacks
    // Simply poisoning aligned_storage wouldn't work, see https://github.com/google/sanitizers/issues/191
    StorageUnit *objects[BlockSize]{nullptr};
#endif

  public:
#if !defined(DAGOR_ADDRESS_SANITIZER)
    T *get(size_t index) { return reinterpret_cast<T *>(&objects[index]); }
    void *get_raw(size_t index) { return &objects[index]; }
    const T *get(size_t index) const { return reinterpret_cast<const T *>(&objects[index]); }
    const void *get_raw(size_t index) const { return &objects[index]; }
#else
    T *get(size_t index) { return reinterpret_cast<T *>(objects[index]); }
    void *get_raw(size_t index) { return objects[index]; }
    const T *get(size_t index) const { return reinterpret_cast<const T *>(objects[index]); }
    const void *get_raw(size_t index) const { return objects[index]; }
#endif

    T *acquireSlot(size_t i)
    {
      this->markAsAllocated(i);
#if defined(DAGOR_ADDRESS_SANITIZER)
      objects[i] = ::new StorageUnit;
#endif
      return get(i);
    }
    template <typename... Args>
    T *allocSlot(size_t i, Args &&...args)
    {
      return TypeHandler::construct(acquireSlot(i), eastl::forward<Args>(args)...);
    }
#if !defined(DAGOR_ADDRESS_SANITIZER)
    // Calculates offset from &objects[0] to ptr. This value can be anything, negative values and values outside of this block.
    ptrdiff_t calculate_relative_offset(const T *ptr) const { return reinterpret_cast<const StorageUnit *>(ptr) - &objects[0]; }
#else
    ptrdiff_t calculate_relative_offset(const T *ptr) const
    {
      for (size_t i = 0; i < BlockSize; ++i)
        if (reinterpret_cast<uintptr_t>(objects[i]) == reinterpret_cast<uintptr_t>(ptr))
          return i;
      return -1;
    }
#endif
    bool isPartOf(T *ptr) const
    {
      const auto offset = calculate_relative_offset(ptr);
      return offset >= 0 && offset < BlockSize;
    }
    void release(T *ptr)
    {
      const auto offset = calculate_relative_offset(ptr);
#if defined(DAGOR_ADDRESS_SANITIZER)
      ::delete eastl::exchange(objects[offset], nullptr);
#endif
      this->markAsFree(offset);
    }
    void free(T *ptr)
    {
      TypeHandler::destruct(ptr);
      release(ptr);
    }
    void freeAll()
    {
      if (!this->hasAnyAllocated())
        return;
      // for trivial this should end up to be a no-op
      for (size_t i = 0; i < BlockSize; ++i)
        if (this->isAllocated(i))
        {
          TypeHandler::destruct(get(i));
#if defined(DAGOR_ADDRESS_SANITIZER)
          ::delete eastl::exchange(objects[i], nullptr);
#endif
        }
      this->markAllAsFree();
    }
  };

  eastl::vector<eastl::unique_ptr<Block>> blocks;
  // This keeps track of a block that is guaranteed to have at least one free slot.
  // Search for free slot can be massively shortened when this index points to a valid block.
  size_t blockWithSpaceIndex = 0;

  // Finds a block that has a free slot that can be allocated.
  // Parameter overloaded version that searches from front to back.
  Block *findBlockWithSpace(ObjectPoolBlockSearchOrderFrontToBack)
  {
    auto ref = eastl::find_if(begin(blocks), end(blocks), [](auto &block) { return block->hasSpace(); });
    if (ref != end(blocks))
      return ref->get();
    return nullptr;
  }
  // Finds a block that has a free slot that can be allocated.
  // Parameter overloaded version that searches from back to front.
  Block *findBlockWithSpace(ObjectPoolBlockSearchOrderBackToFront)
  {
    auto ref = eastl::find_if(rbegin(blocks), rend(blocks), [](auto &block) { return block->hasSpace(); });
    if (ref != rend(blocks))
      return ref->get();
    return nullptr;
  }
  // Returns an index to a block that has a free slot that can be allocated.
  // Returns blocks.size() when no block could be found.
  // Parameter overloaded version that searches from front to back.
  size_t findBlockIndexWithSpace(ObjectPoolBlockSearchOrderFrontToBack)
  {
    auto ref = eastl::find_if(begin(blocks), end(blocks), [](auto &block) { return block->hasSpace(); });
    return eastl::distance(begin(blocks), ref);
  }
  // Returns an index to a block that has a free slot that can be allocated.
  // Returns blocks.size() when no block could be found.
  // Parameter overloaded version that searches from back to front.
  size_t findBlockIndexWithSpace(ObjectPoolBlockSearchOrderBackToFront)
  {
    auto ref = eastl::find_if(rbegin(blocks), rend(blocks), [](auto &block) { return block->hasSpace(); });
    return eastl::distance(begin(blocks), ref.base());
  }

public:
  ObjectPool() = default;
  // Creates enough blocks to be able to allocate as many objects as are requested without any additional allocations by
  // the object pool it self.
  explicit ObjectPool(size_t reserved)
  {
    G_STATIC_ASSERT(BlockSize > 1); // some logic will break when less than two
    reserve(reserved);
  }
  ~ObjectPool() { freeAll(); }
  // Frees memory of any block that has no objects allocated from.
  // NOTE: invalidates ids, so if ...andId are used, this can not be used to free memory.
  void trim()
  {
    blocks.erase(eastl::remove_if(begin(blocks), end(blocks), [](auto &block) { return !block->hasAnyAllocated(); }), end(blocks));
    // remove_if might reordered stuff in a way we no longer point to the correct block, need to reset
    blockWithSpaceIndex = blocks.size();
  }
  // Tries to free memory in a way that preserves object ids.
  void trimBack()
  {
    while (!blocks.empt())
    {
      auto &back = blocks.back();
      if (back->hasAnyAllocated())
      {
        break;
      }
      blocks.pop_back();
    }
  }
  void trimAndPerserveIds() { trimBack(); }
  // Frees all allocated objects of this object pool.
  // Does not release any memory.
  void freeAll()
  {
    for (auto &&block : blocks)
      block->freeAll();
    blockWithSpaceIndex = 0;
  }
  // Allocates a new object of type T, forwards all arguments to the constructor of T.
  template <typename... Args>
  T *allocate(Args &&...args)
  {
    T *result;
    auto block = blockWithSpaceIndex < blocks.size() ? blocks[blockWithSpaceIndex].get() : findBlockWithSpace(BlockSearchOrder{});
    if (block)
    {
      result = block->allocSlot(block->findFreeSlot(), eastl::forward<Args>(args)...);
      // it might be the last slot we used up of this block, then set the index out of bounds to indicate
      // that we don't have a block and a search is needed.
      if (!block->hasSpace())
        blockWithSpaceIndex = blocks.size();
    }
    else
    {
      // new block has at least one free block after we allocated one (static assert ensures that block size is > 1)
      blockWithSpaceIndex = blocks.size();
      blocks.push_back(eastl::make_unique<Block>());
      block = blocks.back().get();
      result = block->allocSlot(block->findFreeSlot(), eastl::forward<Args>(args)...);
    }
    return result;
  }
  // Acquires memory space of a new type T, but does not construct it.
  T *acquire()
  {
    T *result;
    auto block = blockWithSpaceIndex < blocks.size() ? blocks[blockWithSpaceIndex].get() : findBlockWithSpace(BlockSearchOrder{});
    if (block)
    {
      result = block->acquireSlot(block->findFreeSlot());
      // it might be the last slot we used up of this block, then set the index out of bounds to indicate
      // that we don't have a block and a search is needed.
      if (!block->hasSpace())
        blockWithSpaceIndex = blocks.size();
    }
    else
    {
      // new block has at least one free block after we allocated one (static assert ensures that block size is > 1)
      blockWithSpaceIndex = blocks.size();
      blocks.push_back(eastl::make_unique<Block>());
      block = blocks.back().get();
      result = block->acquireSlot(block->findFreeSlot());
    }
    return result;
  }
  // Allocates a new object of type T and passes its index of this pool as first parameter to the constructor of T and all other
  // arguments are forwarded as follow up parameters.
  template <typename... Args>
  T *allocateAndId(Args &&...args)
  {
    T *result;
    auto blockIndex = blockWithSpaceIndex < blocks.size() ? blockWithSpaceIndex : findBlockIndexWithSpace(BlockSearchOrder{});
    if (blockIndex < blocks.size())
    {
      auto block = blocks[blockIndex].get();
      auto slot = block->findFreeSlot();
      auto id = slot + BlockSize * blockIndex;
      result = block->allocSlot(slot, id, eastl::forward<Args>(args)...);
      // it might be the last slot we used up of this block, then set the index out of bounds to indicate
      // that we don't have a block and a search is needed.
      if (!block->hasSpace())
        blockWithSpaceIndex = blocks.size();
    }
    else
    {
      // new block has at least one free block after we allocated one (static assert ensures that block size is > 1)
      blockWithSpaceIndex = blocks.size();
      auto id = blocks.size() * BlockSize;
      blocks.push_back(eastl::make_unique<Block>());
      auto block = blocks.back().get();
      result = block->allocSlot(0, id, eastl::forward<Args>(args)...);
    }
    return result;
  }
  // Acquires memory space of a new type T, but does not construct it.
  // Returns also object id of this pool.
  T *acquireAndId(size_t &id)
  {
    T *result;
    auto blockIndex = blockWithSpaceIndex < blocks.size() ? blockWithSpaceIndex : findBlockIndexWithSpace(BlockSearchOrder{});
    if (blockIndex < blocks.size())
    {
      auto block = blocks[blockIndex].get();
      auto slot = block->findFreeSlot();
      id = slot + BlockSize * blockIndex;
      result = block->acquireSlot(slot);
      // it might be the last slot we used up of this block, then set the index out of bounds to indicate
      // that we don't have a block and a search is needed.
      if (!block->hasSpace())
        blockWithSpaceIndex = blocks.size();
    }
    else
    {
      // new block has at least one free block after we allocated one (static assert ensures that block size is > 1)
      blockWithSpaceIndex = blocks.size();
      id = blocks.size() * BlockSize;
      blocks.push_back(eastl::make_unique<Block>());
      auto block = blocks.back().get();
      result = block->acquireSlot(0);
    }
    return result;
  }
  // Frees an object.
  // Does not free any memory.
  // NOTE: if object is not part of this pool it will do nothing!
  void free(T *obj)
  {
    for (auto &&block : blocks)
    {
      if (block->isPartOf(obj))
      {
        block->free(obj);
        blockWithSpaceIndex = static_cast<size_t>(&block - blocks.data());
        break;
      }
    }
  }
  // Returns an object to the pool.
  // Does not calls the destructor of frees any memory.
  // NOTE: if object is not part of this pool it will do nothing!
  void release(T *obj)
  {
    for (auto &&block : blocks)
    {
      if (block->isPartOf(obj))
      {
        block->release(obj);
        blockWithSpaceIndex = static_cast<size_t>(&block - blocks.data());
        break;
      }
    }
  }
  // Optimized for freeing multiple objects at once.
  // For best performance input is sorted by the address or grouped by source block.
  // Does not free any memory.
  // NOTE: For each element that is not part of this pool there is nothing done!
  template <typename I>
  void free(I from, I to)
  {
    auto blockPos = begin(blocks);
    for (; from != to; ++from)
    {
      auto blockAt = blockPos;
      if (!(*blockAt)->isPartOf(*from))
      {
        for (++blockAt; blockAt != blockPos; ++blockAt)
        {
          if (blockAt == end(blocks))
            blockAt = begin(blocks);
          if ((*blockAt)->isPartOf(*from))
            break;
        }
        if ((*blockAt)->isPartOf(*from))
        {
          (*blockAt)->free(*from);
        }
      }
      else
      {
        (*blockAt)->free(*from);
      }
    }
  }
  template <typename I>
  void release(I from, I to)
  {
    auto blockPos = begin(blocks);
    for (; from != to; ++from)
    {
      auto blockAt = blockPos;
      if (!(*blockAt)->isPartOf(*from))
      {
        for (++blockAt; blockAt != blockPos; ++blockAt)
        {
          if (blockAt == end(blocks))
            blockAt = begin(blocks);
          if ((*blockAt)->isPartOf(*from))
            break;
        }
        if ((*blockAt)->isPartOf(*from))
        {
          (*blockAt)->release(*from);
        }
      }
      else
      {
        (*blockAt)->release(*from);
      }
    }
  }
  // Iterates over all blocks and invokes clb with a pointer of each allocated object.
  template <typename C>
  void iterateAllocated(C clb)
  {
    for (auto &&block : blocks)
      for (uint32_t i = 0; i < BlockSize; ++i)
        if (block->isAllocated(i))
          clb(block->get(i));
  }

  // Iterates over all blocks and invokes clb with a pointer of each allocated object.
  // stops iteration if clb returns false
  template <typename C>
  void iterateAllocatedBreakable(C clb)
  {
    for (auto &&block : blocks)
      for (uint32_t i = 0; i < BlockSize; ++i)
        if (block->isAllocated(i))
          if (!clb(block->get(i)))
            return;
  }

  // Returns number of objects the pool can hold until it needs to allocate a new block
  size_t capacity() const { return blocks.size() * BlockSize; }

  // Returns number of allocates object slots
  size_t size() const
  {
    return eastl::accumulate(begin(blocks), end(blocks), size_t(0),
      [](auto value, auto &block) //
      { return value + block->allocatedSlots(); });
  }

  // Ensures that at least the number of requested objects can be held by the pool without any additional
  // memory allocations.
  void reserve(size_t size)
  {
    auto cnt = (size + BlockSize - 1) / BlockSize;
    while (cnt > blocks.size())
    {
      blocks.push_back(eastl::make_unique<Block>());
    }
  }

  // Returns true if the object comes from this pool and pool slot the object is placed at is marked as acquired.
  bool isAcquired(T *object) const
  {
    for (auto &&block : blocks)
    {
      if (block->isPartOf(object))
      {
        return true;
      }
    }
    return false;
  }

  // Alias to isAcquired
  bool isAllocated(T *object) const { return isAcquired(object); }
};

// Shorthand for ObjectPool and linear allocate policy
template <typename T, size_t BlockSize = OBJECT_POOL_DEFAULT_BLOCK_SIZE>
using LinearObjectPool = ObjectPool<T, BlockSize, ObjectPoolPolicyLinearAllocationAndNoFree>;
