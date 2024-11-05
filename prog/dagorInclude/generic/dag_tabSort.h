//
// Dagor Engine 6.5
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <generic/dag_tab.h>
#include <util/dag_tabHlp.h>

#ifdef __cplusplus

template <class T>
class TabSortedFast : public Tab<T>
{
public:
  TabSortedFast() = default;
  TabSortedFast(IMemAlloc *m) : Tab<T>(m) {}
  TabSortedFast(const TabSortedFast &a) : Tab<T>(a) {}
  TabSortedFast(TabSortedFast &&) = default;
  TabSortedFast(const typename Tab<T>::allocator_type &allocator) : Tab<T>(allocator) {}

  TabSortedFast &operator=(const TabSortedFast &a)
  {
    Tab<T>::operator=(a);
    return *this;
  }
  TabSortedFast &operator=(TabSortedFast &&) = default;

  /**
    add element to array
    @param t initialization data
    @param step number of extra allocated elements when enlarging array
    @return returns inserted element position or -1 on error
  */
  template <class Predicate>
  intptr_t insert(const T &t, const Predicate &pred, int step = 0)
  {
    return tab_sorted_insert(Tab<T>::mBeginAndAllocator.first(), Tab<T>::allocated(), Tab<T>::mCount,
      dag::get_allocator(*static_cast<dag::Vector<T, dag::MemPtrAllocator, false, uint32_t> *>(this)), t, step, pred);
  }

  /**
    find element
    @return returns founded element position or -1 on error
  */
  template <class Predicate>
  int find(const T &t, const Predicate &predicate) const
  {
    const T *p = data_bin_search(t, Tab<T>::data(), Tab<T>::size(), predicate);
    if (!p)
      return -1;
    return p - Tab<T>::data();
  }

  /**
    find element; starting from at; processing n elements
    @return returns founded element position or -1 on error
  */
  template <class Predicate>
  int find(int at, int n, const T &t, const Predicate &predicate) const
  {
    G_ASSERT(at >= 0 && at < Tab<T>::size() && n <= Tab<T>::size());
    const T *p = data_bin_search(t, Tab<T>::data() + at, n, predicate);
    if (!p)
      return -1;
    return p - Tab<T>::data();
  }
};

template <class T, class P>
class TabSortedInline : public TabSortedFast<T>
{
public:
  typedef P Predicate;
  TabSortedInline() = default;
  TabSortedInline(IMemAlloc *m) : TabSortedFast<T>(m) {}
  TabSortedInline(const TabSortedInline &a) : TabSortedFast<T>(a) {}
  TabSortedInline(const typename Tab<T>::allocator_type &allocator) : TabSortedFast<T>(allocator) {}

  TabSortedInline &operator=(const TabSortedInline &a)
  {
    Tab<T>::operator=(a);
    return *this;
  }

  /**
    add element to array
    @param t initialization data
    @param step number of extra allocated elements when enlarging array
    @return returns inserted element position or -1 on error
  */
  intptr_t insert(const T &t, int step = 0) { return TabSortedFast<T>::insert(t, Predicate(), step); }

  /**
    find element
    @return returns founded element position or -1 on error
  */
  int find(const T &t) const { return TabSortedFast<T>::find(t, Predicate()); }

  /**
    find element; starting from at; processing n elements
    @return returns founded element position or -1 on error
  */
  int find(int at, int n, const T &t) const { return TabSortedFast<T>::find(at, n, t, Predicate()); }
};

namespace dag
{
template <class T>
inline void set_allocator(TabSortedFast<T> &v, IMemAlloc *m)
{
  v.get_allocator().m = m;
}
template <class T>
inline IMemAlloc *get_allocator(const TabSortedFast<T> &v)
{
  return v.get_allocator().m;
}
template <class T, class P>
inline void set_allocator(TabSortedInline<T, P> &v, IMemAlloc *m)
{
  v.get_allocator().m = m;
}
template <class T, class P>
inline IMemAlloc *get_allocator(const TabSortedInline<T, P> &v)
{
  return v.get_allocator().m;
}
} // namespace dag
#endif // #ifdef __cplusplus
