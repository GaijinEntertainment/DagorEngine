//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <generic/dag_tab.h>
#include <generic/dag_DObject.h>

/// #Tab of #Ptr
template <class T>
class PtrTab : public Tab<Ptr<T>>
{
  typedef Tab<Ptr<T>> BASE;

public:
  PtrTab() = default;
  PtrTab(IMemAlloc *m) : BASE(m) {}
  PtrTab(const PtrTab &a) : BASE(a) {}
  PtrTab(PtrTab &&a) : BASE((BASE &&) a) {}
  PtrTab(const typename BASE::allocator_type &allocator) : BASE(allocator) {}

  PtrTab &operator=(const PtrTab &a)
  {
    BASE::operator=(a);
    return *this;
  }

  PtrTab &operator=(PtrTab &&a)
  {
    BASE::operator=((BASE &&) a);
    return *this;
  }

  /// pushes one element on the stack, see append()
  Ptr<T> &push_back(T *p) { return BASE::push_back(Ptr<T>(p)); }

  using BASE::insert;
  int insert(int at, int n, T *const *p = NULL, int = 0) { return insert_items<BASE>(*this, at, n, (const Ptr<T> *)p); }

  int append(int n, T *const *p = NULL, int = 0) { return append_items<BASE>(*this, n, (const Ptr<T> *)p); }
};

template <typename T>
inline void erase_ptr_items(PtrTab<T> &v, uint32_t at, uint32_t n)
{
  erase_items(v, at, n);
}
template <typename T>
inline void erase_ptr_items(Tab<Ptr<T>> &v, uint32_t at, uint32_t n)
{
  erase_items(v, at, n);
}
