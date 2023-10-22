#pragma once
// todo: verify invariants: add/delete is happening only from one, owning thread
#include <osApiWrappers/dag_atomic.h>
#include "stl/daProfilerVector.h"
#include "daProfilerDefines.h"

template <size_t p_size = 65536>
struct DefaultPageAllocator
{
  static constexpr size_t page_size = p_size;
  static void *alloc(size_t bytes) { return new char[bytes]; }
  static void free(void *p) { return delete[] (char *)p; }
};

struct NoPageAllocator
{
  static constexpr size_t page_size = 1;
  static void *alloc(size_t bytes) { return new char[bytes]; }
  static void free(void *p) { return delete[] (char *)p; }
};

template <class T, size_t pages, class PageAllocator = DefaultPageAllocator<>>
class ChunkedStorage
{
public:
#if _TARGET_64BIT
  typedef uint64_t size_type;
#else
  typedef uint32_t size_type;
#endif
protected:
  template <size_type Size>
  struct Chunk
  {
    char data[sizeof(T) * Size];
    T *at(size_t i) { return (T *)(data + sizeof(T) * i); }
    const T *at(size_t i) const { return (T *)(data + sizeof(T) * i); }
    Chunk *next = nullptr, *prev = nullptr;
    size_type count = 0;
  };

public:
  typedef T value_t;
  static constexpr size_t page_size = PageAllocator::page_size;
  static constexpr size_t node_overhead = 24;
  static inline constexpr size_t aligned_up_mul(size_t sz, size_t align) { return ((sz + align - 1) / align) * align; }
  static constexpr size_t node_overhead_in_elems = page_size == 1 ? 0 : aligned_up_mul(node_overhead, sizeof(T));
  static constexpr size_t elem_count = (pages * page_size - node_overhead_in_elems) / sizeof(T);

  typedef ChunkedStorage<T, pages, PageAllocator> this_type_t;
  static constexpr size_type Size = elem_count;
  using Node = Chunk<Size>;

  static const Node *const volatile &asConst(Node *const volatile &n) { return (const Node *const volatile &)n; }

  template <typename Cb>
  __forceinline void deleteNonTailChunks(Cb cb) // deleteNonTailChunks and push_back all always happening from same thread as adding
                                                // (only iteration can happen from other)
  {
    // nothing is atomic here, really, interlocked operations to shut down tsan, logically it is NOT thread-safe!
    // that's why all operations are relaxed
    for (Node *h = interlocked_relaxed_load_ptr(head); h && interlocked_relaxed_load_ptr(h->next);
         h = interlocked_relaxed_load_ptr(head))
    {
      T *tb = h->at(0), *te = tb + interlocked_relaxed_load(h->count);
      if (!cb(tb, te))
        return;
      Node *nextHead = interlocked_relaxed_load_ptr(h->next);
      interlocked_release_store_ptr(nextHead->prev, (Node *)nullptr);
      interlocked_release_store_ptr(head, nextHead);
      interlocked_release_store(h->count, 0);
      interlocked_release_store_ptr(h->next, (Node *)nullptr);

      for (--te, --tb; te != tb; --te) // reverse order
        te->~T();
      insertToFreeList(h);
    }
  }

  template <typename Cb>
  __forceinline void forEachChunk(Cb cb) const
  {
    for (const Node *n = interlocked_acquire_load_ptr(asConst(head)); n; n = interlocked_acquire_load_ptr(asConst(n->next)))
      cb(n->at(0), n->at(interlocked_acquire_load(n->count)));
  };

  template <typename Cb>
  __forceinline void forEachCunkReverse(Cb cb) const // this is reversed traversal. Safer for mt access, as if owning thread will
                                                     // delete head chunk, then we will iterate orphaned chunk (stop iteration too
                                                     // early)
  {
    for (const Node *n = interlocked_acquire_load_ptr(asConst(tail)); n; n = interlocked_acquire_load_ptr(asConst(n->prev)))
      cb(n->at(0), n->at(interlocked_acquire_load(n->count)));
  }

  template <typename Cb>
  __forceinline bool forEachCunkReverseStoppable(Cb cb) const // this is reversed traversal. Safer for mt access, as if owning thread
                                                              // will delete head chunk, then we will iterate orphaned chunk (stop
                                                              // iteration too early)
  {
    for (const Node *n = interlocked_acquire_load_ptr(asConst(tail)); n; n = interlocked_acquire_load_ptr(asConst(n->prev)))
    {
      if (cb(n->at(0), n->at(interlocked_acquire_load(n->count))))
        return true;
    }
    return false;
  }

  template <typename Cb>
  __forceinline bool forEachChunkStoppable(Cb cb) const
  {
    for (const Node *n = interlocked_acquire_load_ptr(asConst(head)); n; n = interlocked_acquire_load_ptr(asConst(n->next)))
    {
      if (cb(n->at(0), n->at(interlocked_acquire_load(n->count))))
        return true;
    }
    return false;
  }
  template <typename Cb>
  __forceinline bool forEachChunkStoppable(Cb cb)
  {
    for (Node *n = interlocked_acquire_load_ptr(head); n; n = interlocked_acquire_load_ptr(n->next))
    {
      if (cb(n->at(0), n->at(interlocked_acquire_load(n->count))))
        return true;
    }
    return false;
  }
  template <typename Cb>
  void forEach(Cb cb) const
  {
    forEachChunk([&](const T *ti, const T *te) {
      for (; ti != te; ++ti)
        cb(*ti);
    });
  };
  template <typename Cb>
  bool forEachStop(Cb cb) const
  {
    return forEachChunkStoppable([&](const T *ti, const T *te) {
      for (; ti != te; ++ti)
        if (cb(*ti))
          return true;
      return false;
    });
  };
  template <typename Cb>
  bool forEachReverseStop(Cb cb) const
  {
    return forEachCunkReverseStoppable([&](const T *ti, const T *te) {
      for (ti--, te--; te != ti; --te)
        if (cb(*te))
          return true;
      return false;
    });
  };
  T *safeGet(size_type i)
  {
    for (Node *n = interlocked_relaxed_load_ptr(head); n != nullptr; n = interlocked_relaxed_load_ptr(n->next))
    {
      const size_type nCount = interlocked_relaxed_load(n->count);
      if (i < nCount)
        return n->at(i);
      i -= nCount;
    }
    return nullptr;
  }
  const T *safeGet(size_type i) const { return const_cast<this_type_t *>(this)->safeGet(i); }
  T &operator[](size_type i) { return *safeGet(i); }
  const T &operator[](size_type i) const { return *safeGet(i); }
  T &back()
  {
    auto t = interlocked_relaxed_load_ptr(tail);
    return *t->at(interlocked_relaxed_load(t->count) - 1);
  }
  const T &back() const //-V659
  {
    const auto t = interlocked_relaxed_load_ptr(asConst(tail));
    return *t->at(interlocked_relaxed_load(t->count) - 1);
  }
  T &front() { return *interlocked_relaxed_load_ptr(head)->at(0); }
  const T &front() const { return *interlocked_relaxed_load_ptr(asConst(head))->at(0); } //-V659
  T &push_back(const T &v)
  {
    Node *t = interlocked_relaxed_load_ptr(tail);
    size_type index = t ? t->count : Size;
    if (DAGOR_UNLIKELY(index == Size))
    {
      t = addChunk();
      index = 0;
    }
    return push_back_internal(t, index, v);
  }
  T &push_back(T &&v)
  {
    Node *t = interlocked_relaxed_load_ptr(tail);
    size_type index = t ? t->count : Size;
    if (DAGOR_UNLIKELY(index == Size))
    {
      t = addChunk();
      index = 0;
    }
    return push_back_internal(t, index, (T &&) v);
  }
  T &push_back()
  {
    Node *t = interlocked_relaxed_load_ptr(tail);
    size_type index = t ? t->count : Size;
    if (DAGOR_UNLIKELY(index == Size))
    {
      t = addChunk();
      index = 0;
    }
    return push_back_internal(t, index);
  }

  T *allocContiguous(size_type cnt, bool &newChunk)
  {
    if (cnt > Size)
      return nullptr;
    Node *t = interlocked_relaxed_load_ptr(tail);
    size_type currentCount = t ? interlocked_relaxed_load(t->count) : Size;
    if (DAGOR_UNLIKELY(currentCount + cnt > Size))
    {
      t = addChunk();
      currentCount = 0;
      newChunk = true;
    }
    else
      newChunk = false;
    T *tb = t->at(currentCount);
    for (T *ti = tb, *te = ti + cnt; ti != te; ++ti)
      new (ti, _NEW_INPLACE) T;
#if DAGOR_THREAD_SANITIZER
    interlocked_relaxed_store(t->count, interlocked_relaxed_load(t->count) + cnt);
#else
    t->count += cnt;
#endif
    return tb;
  }

  template <bool is_copy = false>
  T *allocMaxPossible(size_type cnt, size_t &allocated, size_t &left_after_alocation, const T *from = nullptr)
  {
    Node *t = interlocked_relaxed_load_ptr(tail);
    size_type currentCount = t ? interlocked_relaxed_load(t->count) : Size;
    if (DAGOR_UNLIKELY(currentCount == Size))
    {
      t = addChunk();
      currentCount = 0;
    }
    const size_type maxLeft = Size - currentCount;
    cnt = (maxLeft < cnt) ? maxLeft : cnt;
    left_after_alocation = maxLeft - cnt;
    T *tb = t->at(currentCount);
    if (!is_copy || !dag::is_type_relocatable<T>::value)
    {
      for (T *ti = tb, *te = ti + cnt; ti != te; ++ti)
      {
        if (is_copy)
          new (ti, _NEW_INPLACE) T(*(from++));
        else
          new (ti, _NEW_INPLACE) T;
      }
    }
    else
    {
      memcpy(tb, from, sizeof(T) * cnt);
    }
#if DAGOR_THREAD_SANITIZER
    interlocked_relaxed_store(t->count, interlocked_relaxed_load(t->count) + cnt);
#else
    t->count += cnt;
#endif
    allocated = cnt;
    return tb;
  }

  void append(const T *start, const T *end)
  {
    while (start != end)
    {
      size_t allocated;
      size_t leftAfter;
      allocMaxPossible<true>(end - start, allocated, leftAfter, start);
      start += allocated;
    }
  }

  bool empty() const
  {
    const auto t = interlocked_relaxed_load_ptr(asConst(tail));
    return !t || interlocked_relaxed_load(t->count) == 0;
  }
  size_t approximateSize() const // assumes all blocks are fully used, besides tail one
  {
    size_t totalUsefulBlocks = usefulBlocks();
    if (!totalUsefulBlocks)
      return 0;

    const auto t = interlocked_relaxed_load_ptr(asConst(tail));
    return (totalUsefulBlocks - 1) * Size + (t ? interlocked_relaxed_load(t->count) : 0);
  }
  ChunkedStorage() = default;
  ChunkedStorage &operator=(ChunkedStorage &&c) // not thread safe!
  {
    swap(c.head, head);
    swap(c.tail, tail);
    swap(c.freeList, freeList);
    swap(c.allocatedChunks, allocatedChunks);
    swap(c.freeChunks, freeChunks);
    return *this;
  }
  ChunkedStorage(ChunkedStorage &&c) { *this = (ChunkedStorage &&) c; }
  ~ChunkedStorage()
  {
    clear();
    clearFreeList();
  }
  void clear() // this is called only on destructor. assumes thread to be finished
  {
    if (!tail)
      return;
    // interlockeds to shut tsan
    auto t = interlocked_acquire_load_ptr(tail);
    for (T *ti = t->at(interlocked_acquire_load(t->count)) - 1, *tb = t->at(0) - 1; ti != tb; --ti)
      ti->~T();
    for (pop_tail();; pop_tail())
    {
      auto t = interlocked_acquire_load_ptr(tail);
      if (!t)
        break;
      for (T *ti = t->at(t->count) - 1, *tb = tail->at(0) - 1; ti != tb; --ti)
        ti->~T();
    }
    interlocked_release_store_ptr(head, (Node *)NULL);
  }
  size_t allocatedBlocks() const // returns allocated blocks
  {
    return interlocked_relaxed_load(allocatedChunks);
  }
  size_t usefulBlocks() const // returns useful blocks
  {
    return interlocked_relaxed_load(allocatedChunks) - interlocked_relaxed_load(freeChunks);
  }
  size_t usefulMemory() const { return usefulBlocks() * sizeof(Node); }
  size_t memAllocated() const { return allocatedBlocks() * sizeof(Node); }
  void shrink_to_fit() { clearFreeList(); }

protected:
  __forceinline void inc_count(Node *n)
  {
    ++n->count; // can be made relaxed interlocked load + store to avoid tsan report
    // however, both are "safe" in a sense that only ONE thread is writing. So only need when sanitizer is on
  }
  __forceinline void pop_tail()
  {
    auto pt = tail;
    pt->count = 0;
    tail = pt->prev;
    if (tail)
      tail->next = nullptr;

    insertToFreeList(pt);
  }

  __forceinline T &push_back_internal(Node *t, size_type index)
  {
    T &ret = *(new (t->at(index), _NEW_INPLACE) T);
    inc_count(t);
    return ret;
  }
  __forceinline T &push_back_internal(Node *t, size_type index, T &&v)
  {
    T &ret = *(new (t->at(index), _NEW_INPLACE) T((T &&) v));
    inc_count(t);
    return ret;
  }
  __forceinline T &push_back_internal(Node *t, size_type index, const T &v)
  {
    T &ret = *(new (t->at(index), _NEW_INPLACE) T(v));
    inc_count(t);
    return ret;
  }

  void clearFreeList() // thread safe, can be called NOT from owning thread
  {
    // ensure freelist is ours to work with, and get ownershup for it
    Node *prevFreeList = interlocked_acquire_load_ptr(freeList), *curFreeList;
    do
    {
      curFreeList = prevFreeList;
      prevFreeList = interlocked_compare_exchange_ptr(freeList, (Node *)nullptr, curFreeList);
    } while (prevFreeList != curFreeList);

    intptr_t released = 0;
    while (curFreeList)
    {
      Node *n = curFreeList->next;
      interlocked_decrement(allocatedChunks);
      interlocked_decrement(freeChunks);
      curFreeList->~Node();
      PageAllocator::free(curFreeList);
      curFreeList = n;
      ++released;
    }
  }
  void insertToFreeList(Node *n) // can only be called from addChunk, i.e. owning thread. clearFreeList can be called from any thread!
  {
    Node *prevFreeList = interlocked_acquire_load_ptr(freeList), *curFreeList;
    do
    {
      curFreeList = prevFreeList;
      interlocked_release_store_ptr(n->next, curFreeList);
      prevFreeList = interlocked_compare_exchange_ptr(freeList, n, curFreeList);
    } while (prevFreeList != curFreeList);
    interlocked_increment(freeChunks);
  }

  DAGOR_NOINLINE Node *popFreeList() // can only be called from addChunk, i.e. owning thread. clearFreeList can be called from any
                                     // thread!
  {
    // basically, n = freeList; freeList = n->next;
    Node *prevFreeList = interlocked_acquire_load_ptr(freeList), *curFreeList;
    do
    {
      curFreeList = prevFreeList;
      if (prevFreeList)
        prevFreeList = interlocked_compare_exchange_ptr(freeList, curFreeList->next, curFreeList);
    } while (prevFreeList != curFreeList);
    if (prevFreeList)
      interlocked_decrement(freeChunks);
    return prevFreeList;
  }

  DAGOR_NOINLINE Node *addChunk() // addChunk is not thread safe
  {
    Node *n = interlocked_relaxed_load_ptr(freeList) ? popFreeList() : nullptr;

    if (n)
      n = new (n, _NEW_INPLACE) Node;
    else
    {
      n = new (PageAllocator::alloc(sizeof(Node)), _NEW_INPLACE) Node;
      interlocked_increment(allocatedChunks);
    }
    // interlocked is just to shut down tsan for race in shutdown
    auto t = interlocked_relaxed_load_ptr(tail);
    n->prev = t;
    if (t)
      t->next = n;
    interlocked_release_store_ptr(tail, n);
    if (!interlocked_relaxed_load_ptr(head))
      interlocked_release_store_ptr(head, n);
    return n;
  }
  Node *volatile head = NULL, *volatile tail = NULL;
  Node *volatile freeList = NULL;                // reuse chunks list (memory)
  size_type allocatedChunks = 0, freeChunks = 0; // for fast memoryAllocated
  template <class ST>
  void swap(ST &a, ST &b)
  {
    ST c = (ST &&) a;
    a = (ST &&) b;
    b = (ST &&) c;
  }
};
