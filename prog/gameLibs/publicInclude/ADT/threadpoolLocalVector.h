//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/array.h>
#include <EASTL/iterator.h>
#include <dag/dag_vector.h>
#include <debug/dag_assert.h>
#include <util/dag_threadPool.h>
#include <osApiWrappers/dag_miscApi.h> // is_main_thread

//
// Container for fast (no memory sharing!) lockless appends from main or threadpool threads
//
template <typename T, unsigned TLS_COUNT = (threadpool::MAX_WORKER_COUNT + /*main*/ 1), typename VecType = dag::Vector<T>>
class ThreadpoolLocalVector
{
public:
  template <typename, unsigned, typename, bool>
  class Iterator;
  typedef Iterator<T, TLS_COUNT, VecType, true> const_iterator;
  typedef Iterator<T, TLS_COUNT, VecType, false> iterator;

  void push_back(const T &value) { getCurThreadData().push_back(value); }
  void push_back(T &&value) { getCurThreadData().push_back(eastl::move(value)); }
  T &push_back_noinit() { return getCurThreadData().push_back_noinit(); }
  const T &back() const { return getCurThreadData().back(); }
  T &back() { return getCurThreadData().back(); }
  template <typename... Args>
  T &emplace_back(Args &&...args)
  {
    return getCurThreadData().emplace_back(eastl::forward<Args>(args)...);
  }
  T *append(uint32_t sz, const T &v = T{})
  {
    auto &b = getCurThreadData();
    b.resize(b.size() + sz, v);
    return &b[b.size() - sz];
  }
  T *append_noinit(uint32_t sz)
  {
    auto &b = getCurThreadData();
    b.resize_noinit(b.size() + sz);
    return &b[b.size() - sz];
  }
  void clear()
  {
    for (auto &b : tlses)
      b.data.clear();
  }
  void clear_and_shrink()
  {
    for (auto &b : tlses)
      ::clear_and_shrink(b.data);
  }
  bool empty() const
  {
    for (auto &b : tlses)
      if (!b.data.empty())
        return false;
    return true;
  }
  uint32_t size() const
  {
    uint32_t sz = 0;
    for (auto &b : tlses)
      sz += b.data.size();
    return sz;
  }

  template <typename TT, unsigned TC, typename V, bool bConst>
  class Iterator
  {
  public:
    using iterator_category = eastl::forward_iterator_tag; // Note: can be easily extended for `bidirectional_iterator_tag`
    using value_type = TT;
    using difference_type = ptrdiff_t;
    using pointer = eastl::conditional_t<bConst, TT *, const TT *>;
    using reference = eastl::conditional_t<bConst, TT &, const TT &>;

    Iterator(eastl::array<typename ThreadpoolLocalVector<TT, TC, V>::ThreadLocalData, TC> &ts, size_t ti, size_t ei) :
      tlses(ts), tlsI(ti), elemI(ei)
    {
      advance();
    }

    reference operator*() { return tlses[tlsI].data[elemI]; }
    pointer operator->() { return &tlses[tlsI].data[elemI]; }

    Iterator &operator++()
    {
      ++elemI;
      advance();
      return *this;
    }

    Iterator operator++(int)
    {
      Iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const Iterator &other) const { return tlsI == other.tlsI && elemI == other.elemI; }
    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    void advance()
    {
      while (tlsI < TC && elemI >= tlses[tlsI].data.size())
      {
        ++tlsI;
        elemI = 0;
      }
    }

    eastl::array<typename ThreadpoolLocalVector<TT, TC, V>::ThreadLocalData, TC> &tlses;
    uint32_t tlsI, elemI;
  };

  iterator begin() { return iterator(tlses, 0u, 0u); }
  iterator end() { return iterator(tlses, TLS_COUNT, 0u); }
  const_iterator begin() const { return const_iterator(tlses, 0u, 0u); }
  const_iterator end() const { return const_iterator(tlses, TLS_COUNT, 0u); }
  const_iterator cbegin() const { return const_iterator(tlses, 0u, 0u); }
  const_iterator cend() const { return const_iterator(tlses, TLS_COUNT, 0u); }

private:
  struct ThreadLocalData // Note: deliberately not `alignas(64)` as manual aligment is more then enough
  {
    VecType data;
#if defined(__aarch64__) || defined(_M_ARM64)
    char theRestOfCacheLineToAvoidFalseSharing[(sizeof(VecType) < 64) ? (64 - sizeof(VecType)) : 0];
#else
    char theRestOfCacheLineToAvoidFalseSharing[(sizeof(VecType) < 128) ? (128 - sizeof(VecType)) : 0];
#endif
  };
  VecType &getCurThreadData()
  {
    int i = threadpool::get_current_worker_id();
    G_ASSERTF(i >= 0 || is_main_thread(), "Only main/threadpool threads are permitted to access this!");
    return tlses[i + 1].data;
  }
  const VecType &getCurThreadData() const { return const_cast<ThreadpoolLocalVector *>(this)->getCurThreadData(); }
  eastl::array<ThreadLocalData, TLS_COUNT> tlses;
};
