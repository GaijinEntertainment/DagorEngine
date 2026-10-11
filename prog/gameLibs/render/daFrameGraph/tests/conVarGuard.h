// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/type_traits.h>
#include <util/dag_convar.h>


template <class T, bool Ranged>
struct ConVarGuard
{
  ConVarT<T, Ranged> &var;
  const T prevValue;
  ConVarGuard(ConVarT<T, Ranged> &v, eastl::type_identity_t<T> value) : var(v), prevValue(v.get()) { var.set(value); }
  ~ConVarGuard() { var.set(prevValue); }
};
