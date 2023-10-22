#pragma once

#include <debug/dag_debug.h>
#include <dag/dag_vector.h>
#include <EASTL/unique_ptr.h>
#include <math/dag_adjpow2.h>

template <typename T, unsigned initialCapacity, unsigned subArraysCapacity = 8>
class NonRelocatableCont
{
  G_STATIC_ASSERT((initialCapacity & (initialCapacity - 1)) == 0); // pow2 required
  struct SubArray;

public:
  NonRelocatableCont()
  {
    arrays.reserve(subArraysCapacity);
    arrays.push_back(SubArray(capacity(0)));
  }

  template <typename CB>
  uint32_t find_if(CB cb, uint32_t def = ~0u)
  {
    unsigned i = 0;
    for (auto &sa : arrays)
    {
      for (unsigned j = 0, jc = count(i); j < jc; ++j)
        if (cb(sa[j]))
          return build_id(i, j);
      ++i;
    }
    return def;
  }

  template <typename CB>
  void iterate(CB cb)
  {
    unsigned i = 0;
    for (auto &sa : arrays)
    {
      for (unsigned j = 0, jc = count(i); j < jc; ++j)
        cb(sa[j]);
      ++i;
    }
  }

  T *at(unsigned n)
  {
    unsigned i = n >> SA_INDEX_BITS;
    unsigned j = n & ((1 << SA_INDEX_BITS) - 1);
    if (EASTL_LIKELY(i < arrays.size() && j < count(i)))
      return &arrays[i][j];
    else
      return nullptr;
  }
  const T *at(unsigned n) const { return const_cast<NonRelocatableCont *>(this)->at(n); }

  static unsigned build_id(unsigned i, unsigned j)
  {
    G_FAST_ASSERT(i < (1 << (32 - SA_INDEX_BITS)));
    G_FAST_ASSERT(j < (1 << SA_INDEX_BITS));
    return (i << SA_INDEX_BITS) | j;
  }

  T &operator[](unsigned n)
  {
    unsigned i = n >> SA_INDEX_BITS;
    unsigned j = n & ((1 << SA_INDEX_BITS) - 1);
#if 0 // Note: we can't reliably do lockless bounds checking here, since count(i) isn't thread safe
    G_FAST_ASSERT(i < arrays.size());
    G_FAST_ASSERT(j <= count(i)); // <= to allow use &data[cnt] as end()
#endif
    return arrays[i][j];
  }

  void clear()
  {
    arrays.clear();
    backCount = totalCount = 0;
    arrays.push_back(SubArray(capacity(0)));
  }

  // Note: intentionally don't call it 'size', to avoid confustion with vector-like
  uint32_t getTotalCount() const
  {
    return totalCount;
  } // totalCount might be lesser then allocated, so don't use it as index (incl. iteration)

  unsigned push_back(const T &v) { return append(&v, 1); }

  unsigned append(const T *p, uint32_t sz)
  {
    G_FAST_ASSERT(!arrays.empty());
    totalCount += sz;
    do
    {
      uint32_t asize = arrays.size();
      unsigned j = backCount;
      backCount += sz;
      auto &back = arrays.back();
      if (EASTL_LIKELY(backCount <= capacity(asize - 1)))
      {
        if (p)
          for (int i = 0; i < sz; ++i)
            back[j + i] = p[i];
        return build_id(&back - arrays.data(), j);
      }
      backCount = 0;
      arrays.push_back(SubArray(capacity(asize)));
      G_ASSERTF(sz <= capacity(asize), "Attempt to append %d > bucket #%d pow2 capacity %d. Increase container's initialCapacity (%d)",
        sz, asize, capacity(asize), initialCapacity);
    } while (1);
  }

private:
  static constexpr unsigned SA_INDEX_BITS = 24;
  struct SubArray
  {
    eastl::unique_ptr<T[]> data;
    SubArray() = default;
    SubArray(uint32_t cap) : data(eastl::make_unique<T[]>(cap)) {}
    T &operator[](unsigned i) { return data[i]; }
    const T &operator[](unsigned i) const { return data[i]; }
  };
  dag::Vector<SubArray> arrays;
  uint32_t backCount = 0, totalCount = 0;
  uint32_t count(uint32_t i) const
  {
    G_FAST_ASSERT(!arrays.empty());
    return ((i + 1) == arrays.size()) ? backCount : capacity(i);
  }
  uint32_t capacity(uint32_t i) const { return initialCapacity << i; }
};
