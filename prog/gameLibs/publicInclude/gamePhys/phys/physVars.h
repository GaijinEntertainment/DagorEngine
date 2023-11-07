//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <ADT/fastHashNameMap.h>
#include <EASTL/bitvector.h>
#include <generic/dag_smallTab.h>
#include <math/dag_check_nan.h>

class PhysVars
{
  SmallTab<float> vars;
  FastHashNameMap names;
  eastl::bitvector<> varsPullable;
  int registerVarImpl(const char *name, float val, bool pull);

public:
  typename FastHashNameMap::NameIdsType::const_iterator begin() const { return names.begin(); }
  typename FastHashNameMap::NameIdsType::const_iterator end() const { return names.end(); }

  int registerVar(const char *name, float val);
  int registerPullVar(const char *name, float val);
  int getVarId(const char *name) const;
  bool isVarPullable(int var_id) const;
  float getVar(int var_id) const;
  float getVarUnsafe(int var_id) const;

  void setVar(int var_id, float val);
  void setupVar(const char *name, float val);

  const int getVarsCount() const { return vars.size(); }
  const char *getVarName(const int var_id) const { return names.getNameSlow(var_id); }
};

inline float PhysVars::getVarUnsafe(int var_id) const { return vars[var_id]; }
inline int PhysVars::getVarId(const char *name) const { return names.getNameId(name); }
inline bool PhysVars::isVarPullable(int var_id) const { return varsPullable.test(var_id, false); }

inline void PhysVars::setVar(int var_id, float val)
{
  G_ASSERTF_RETURN(var_id < vars.size() && check_finite(val), , "Invalid var_id %d/%d or val=%.9f", var_id, vars.size(), val);
  vars[var_id] = val;
}
