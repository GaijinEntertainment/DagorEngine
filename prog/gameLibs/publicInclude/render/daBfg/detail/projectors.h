//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once


namespace dabfg::detail
{

using TypeErasedProjector = void *(*)(void *);

template <class C, typename T>
C owner_type_of_memptr(T C::*memberPtr);

template <class DUMMY, class C, typename T>
T member_type_of_memptr(T C::*memberPtr);

template <auto memberPtr>
TypeErasedProjector projector_for_member()
{
  using PointerClassType = decltype(owner_type_of_memptr(memberPtr));
  return [](void *sv) -> void * { return &(static_cast<PointerClassType *>(sv)->*memberPtr); };
};

} // namespace dabfg::detail
