#pragma once
#include <dag/dag_vector.h>
#include <stdint.h>
#include <EASTL/unique_ptr.h>

namespace ecs
{

template <class Type, int page_size_bits = 10>
struct ChunkedVectorSingleThreaded
{
  struct Chunk
  {
    eastl::unique_ptr<char[]> data;
    uint32_t size = 0, stackAt = 0;
    void init(uint32_t sz)
    {
      data.reset();
      data = eastl::make_unique<char[]>(size = sz);
      stackAt = 0;
    }
  };
  Chunk top;
  dag::Vector<Chunk> chunks;

  Type *__restrict getStack() { return (Type *__restrict)(top.data.get() + top.stackAt); }
  const Type *__restrict getStackEnd() const { return (Type *__restrict)(top.data.get() + top.size); }

  void commit(Type *__restrict cur)
  {
    top.stackAt = (const char *)cur - top.data.get();
    DAECS_EXT_FAST_ASSERT(top.stackAt <= top.size);
  }
  void decommit(Type *__restrict cur)
  {
    const size_t at = (const char *)cur - top.data.get();
    if (at >= top.size)
    {
      // we allocated another chunk on top and can't get to it.
      // Obviously, it is easy to fix (just pop_back from chunks, if chunks-not-empty in else branch)
      // but we don't do it intentionally, we just "leak" chunk, to speed up performance of happy path
      // since we increase size of allocator each time, eventually we stop leaking.
      return;
    }
    top.stackAt = at;
  }
  static constexpr uint32_t page_size = 1 << page_size_bits, page_mask = page_size - 1;

  DAGOR_NOINLINE
  Type *growStack(Type *__restrict &__restrict at, uint32_t step, const Type *__restrict &__restrict end)
  {
    const char *curStack = top.data.get() + top.stackAt;
    const uint32_t currentlyUsed = (const char *)at - curStack;
    G_FAST_ASSERT(at + step > end && end == getStackEnd() && at >= getStack() && at <= end && curStack <= (const char *)at);
    const uint32_t nextSize = (eastl::max(top.size << 1, uint32_t(top.size + sizeof(Type) * step)) + page_mask) & ~page_mask;

    Chunk nextTop;
    nextTop.init(nextSize);
    if (top.size)
    {
      memcpy(nextTop.data.get(), curStack, currentlyUsed);
      chunks.emplace_back(eastl::move(top));
    }
    top = eastl::move(nextTop);
    at = (Type *)(top.data.get() + currentlyUsed);
    end = getStackEnd();
    return (Type *)top.data.get(); // stackAt == 0 no
  }
  uint32_t lastNotEnough = 0, lastEnough = 0;
  // todo: implement probing? (if long time there was enough space, try to reduce lastNotEnough by one page)
  void collapse()
  {
    const bool enough = chunks.empty();
    uint32_t oldSize = enough ? top.size : chunks.front().size;
    uint32_t nextSize = oldSize;
    if (!enough)
    {
      if (lastEnough <= oldSize)
        lastEnough = oldSize * 2;
      lastNotEnough = oldSize;
      nextSize = oldSize + page_size;
      if (nextSize > lastEnough)
        nextSize = lastEnough;
    }
    else
    {
      lastEnough = oldSize;
      nextSize /= 2;
      if (nextSize <= lastNotEnough)
        nextSize = lastNotEnough + page_size;
    }
    const uint32_t alignedSize = (nextSize + page_mask) & ~page_mask;
    chunks.set_capacity(0);
    if (top.size != alignedSize)
      top.init(alignedSize);
    else
      top.stackAt = 0;
  }
};


template <class LinkedListNode>
inline void mt_add_to_list_no_lock(LinkedListNode *node, LinkedListNode *&head)
{
  auto *prevHead = interlocked_acquire_load_ptr(head);
  LinkedListNode *curHead;
  do
  {
    curHead = prevHead;
    interlocked_release_store_ptr(node->next, curHead);
    prevHead = interlocked_compare_exchange_ptr(head, node, curHead);
  } while (prevHead != curHead);
}

template <typename Cb, class LinkedListNode>
inline void iterate_linked_list(LinkedListNode *head, Cb cb)
{
  for (auto s = head; s;)
  {
    auto next = s->next;
    cb(s); // allows delete
    s = next;
  }
}

template <class Data>
struct MTLinkedList
{
  struct Node
  {
    Data data;
    Node **pThreadData = nullptr; // pointer to thread_local data
    Node *next = nullptr;
  };
  Node *next = nullptr;
  static Node *head; // global head
  static thread_local Node *thread_data;

  static Data &getData()
  {
    if (DAGOR_LIKELY(thread_data != nullptr))
      return thread_data->data;
    return newNode().data;
  }
  DAGOR_NOINLINE
  static Node &newNode()
  {
    Node *node = new Node;
    node->pThreadData = &thread_data;
    mt_add_to_list_no_lock(node, head);
    thread_data = node;
    return *node;
  }
  static void collapse_all()
  {
    iterate_linked_list(head, [](auto node) { node->data.collapse(); });
  }
  static void free_all()
  {
    iterate_linked_list(head, [](auto node) {
      *(node->pThreadData) = nullptr;
      delete node;
    });
    head = nullptr;
  }
};

} // namespace ecs