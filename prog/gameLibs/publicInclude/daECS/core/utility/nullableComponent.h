//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <EASTL/type_traits.h>
#include <EASTL/algorithm.h>
#include <util/dag_globDef.h>

template <typename T, bool bConst>
struct NullableComponent
{
  typedef typename eastl::conditional<bConst, const T, T>::type T_const_type;
  typedef const T *cptr_type;
  typedef const T &cref_type;
  typedef T_const_type *ptr_type;
  typedef T_const_type &ref_type;

  NullableComponent() = default;
  explicit NullableComponent(ptr_type a) : attr(a) {}
  template <typename U = T, typename = eastl::enable_if_t<!bConst, void>>
  U &get()
  {
    G_ASSERT(attr);
    return *attr;
  }
  template <typename U = T, typename = eastl::enable_if_t<!bConst, void>>
  ref_type operator*()
  {
    return get();
  }
  template <typename U = T, typename = eastl::enable_if_t<!bConst, void>>
  ptr_type operator->()
  {
    G_ASSERT(attr);
    return attr;
  }
  template <typename U = T, typename = eastl::enable_if_t<!bConst, void>>
  NullableComponent &operator=(const T &a)
  {
    G_ASSERT_RETURN(attr, *this);
    *attr = a;
    return *this;
  }

  cref_type &get() const
  {
    G_ASSERT(attr);
    return *attr;
  }
  cref_type operator*() const { return get(); }
  cptr_type operator->() const
  {
    G_ASSERT(attr);
    return attr;
  }
  bool isValid() const { return attr != nullptr; }

  template <typename U = T, typename = eastl::enable_if_t<eastl::is_rvalue_reference<U &&>::value && !bConst, void>>
  NullableComponent &operator=(U &&a)
  {
    G_ASSERT_RETURN(attr, *this);
    *attr = eastl::move(a);
    return *this;
  }
  NullableComponent(NullableComponent &&a)
  {
    attr = a.attr;
    a.attr = nullptr;
  }

  NullableComponent(const NullableComponent &) = delete;
  NullableComponent &operator=(const NullableComponent &) = delete;
  NullableComponent &operator=(NullableComponent &&) = delete;

protected:
  ptr_type attr = nullptr;
};

template <class T>
using NullableRO = NullableComponent<T, true>;
template <class T>
using NullableRW = NullableComponent<T, false>;
