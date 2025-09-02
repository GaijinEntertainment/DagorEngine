//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <ADT/fastHashNameMap.h>
#include <EASTL/bitvector.h>
#include <generic/dag_smallTab.h>
#include <math/dag_check_nan.h>
#include <osApiWrappers/dag_rwSpinLock.h>

struct PhysVarsGlobalNameMap
{
  FastHashNameMap names;
  SpinLockReadWriteLock mutex;
};

class PhysVars
{
  struct GlobalRemap
  {
    eastl::bitvector<> usedVars;
  };
  PhysVarsGlobalNameMap *const globalNameMapPtr;
  GlobalRemap globalRemap;
  FastHashNameMap localNamesMap;
  SmallTab<float> vars;
  eastl::bitvector<> varsPullable;

  int registerVarImpl(const char *name, float val, bool pull);

public:
  explicit PhysVars(PhysVarsGlobalNameMap *global = nullptr) : globalNameMapPtr(global) {}
  PhysVars(const PhysVars &) = delete;
  void operator=(const PhysVars &) = delete;

  template <typename F>
  void iter(F &&f) const
  {
    if (globalNameMapPtr)
    {
      globalNameMapPtr->mutex.lockRead();
      for (auto &[name, id] : globalNameMapPtr->names)
        if (globalRemap.usedVars.test(id, false))
          f(name.c_str(), id);
      globalNameMapPtr->mutex.unlockRead();
    }
    else
      for (auto &[name, id] : localNamesMap)
        f(name.c_str(), id);
  }

  int registerVar(const char *name, float val);
  int registerPullVar(const char *name, float val);
  int getVarId(const char *name) const;
  bool isVarPullable(int var_id) const;
  float getVar(int var_id) const;
  float getVarUnsafe(int var_id) const;

  void setVar(int var_id, float val);
  void setupVar(const char *name, float val);

  int getVarsCount() const { return vars.size(); }
  const char *getVarName(const int var_id) const
  {
    if (globalNameMapPtr)
    {
      globalNameMapPtr->mutex.lockRead();
      const char *name = globalNameMapPtr->names.getNameSlow(var_id);
      globalNameMapPtr->mutex.unlockRead();
      return name;
    }
    return localNamesMap.getNameSlow(var_id);
  }
};

inline float PhysVars::getVarUnsafe(int var_id) const { return vars[var_id]; }
inline int PhysVars::getVarId(const char *name) const
{
  if (globalNameMapPtr)
  {
    globalNameMapPtr->mutex.lockRead();
    int id = globalNameMapPtr->names.getNameId(name);
    if (!globalRemap.usedVars.test(id, false))
      id = -1;
    globalNameMapPtr->mutex.unlockRead();
    return id;
  }
  return localNamesMap.getNameId(name);
}
inline bool PhysVars::isVarPullable(int var_id) const { return varsPullable.test(var_id, false); }

inline void PhysVars::setVar(int var_id, float val)
{
  G_ASSERTF_RETURN(var_id < vars.size(), , "Invalid var_id: %d. Vars registered: %d. (value set = %.9f)", var_id, vars.size(), val);
  G_ASSERTF_RETURN(check_finite(val), , "Invalid value for var (%d) '%s' = %.9f", var_id, getVarName(var_id), val);
  vars[var_id] = val;
}
