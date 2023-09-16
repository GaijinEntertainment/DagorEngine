//
// Dagor Engine 6.5
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <shaders/dag_linkedListOfShadervars.h>
#include <util/dag_convar.h>
#include <EASTL/optional.h>

template <typename IntervalConVarBase, typename IntervalType>
class IntervalConVar : public IntervalConVarBase
{
public:
  using IntervalConVarBase::IntervalConVarBase;

  IntervalConVar &operator=(IntervalType val)
  {
    set(static_cast<int>(val));
    return *this;
  }
  IntervalType get() const
  {
    IntervalType val = static_cast<IntervalType>(ConVarT::get());
    return val;
  }
  bool operator==(IntervalType val) const { return get() == val; }
  bool operator!=(IntervalType val) const { return !(*this == val); }
  operator IntervalType() const { return get(); }
  void setMinMax(IntervalType min, IntervalType max) { ConVarT::setMinMax(static_cast<int>(min), static_cast<int>(max)); }
};

#ifndef CONSOLE_INTERVAL
#define CONSOLE_INTERVAL(domain, name, Class, defVal) Class##ConVar name(domain "." #name, Class::defVal)
#endif
