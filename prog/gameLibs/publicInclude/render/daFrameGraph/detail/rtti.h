//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <debug/dag_log.h>
#include <generic/dag_fixedMoveOnlyFunction.h>


namespace dafg::detail
{

struct RTTI
{
  // TODO: we can store names, reflection and etc here and use in visualization
  size_t size;
  size_t align;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *) const> ctor;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *) const> dtor;
  dag::FixedMoveOnlyFunction<4 * sizeof(void *), void(void *, const void *) const> copy;
};

template <class T>
RTTI make_rtti()
{
  return RTTI{sizeof(T), alignof(T),
    // IMPORTANT: () zero-initializes structs
    +[](void *p) { new (p) T(); }, +[](void *p) { static_cast<T *>(p)->~T(); },
    +[](void *ptr, const void *from) {
      if constexpr (eastl::is_copy_constructible<T>::value)
      {
        new (ptr) T(*reinterpret_cast<const T *>(from));
      }
      else
      {
        G_UNUSED(from);
        logerr("daFG: blob type is NOT copy constructible, constructing NEW blob instead");
        new (ptr) T();
      }
    }};
}

} // namespace dafg::detail
