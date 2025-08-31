// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "priorityManagedShadervar.h"
#include <math/dag_color.h>
#include <limits.h>
#include <render/world/wrDispatcher.h>

PriorityManagedShadervar::PriorityManagedShadervar(int id) : varId(id), type(ShaderGlobal::get_var_type(varId))
{
  defaultValue = get();
}

void PriorityManagedShadervar::clear(int priority)
{
  values.erase(priority);
  if (values.empty())
    set(defaultValue);
  else
    setHighestPriority();
}

void PriorityManagedShadervar::setHighestPriority()
{
  G_ASSERT(!values.empty());
  int maxPrio = INT_MIN;
  ShadervarUnion maxVal = {};
  for (auto [prio, val] : values)
  {
    if (prio >= maxPrio)
    {
      maxPrio = prio;
      maxVal = val;
    }
  }
  set(maxVal);
}

PriorityManagedShadervar::ShadervarUnion PriorityManagedShadervar::get()
{
  ShadervarUnion result = {};
  switch (type)
  {
    case SHVT_REAL: result = ShaderGlobal::get_real(varId); break;
    case SHVT_INT: result = ShaderGlobal::get_int(varId); break;
    case SHVT_COLOR4: result = Point4::rgba(ShaderGlobal::get_color4(varId)); break;
    case SHVT_INT4: result = ShaderGlobal::get_int4(varId); break;
    default: G_ASSERT_FAIL("Unsupported type in PriorityManagedShadervar!!! Type = %d", type); break;
  }
  return result;
}

void PriorityManagedShadervar::set(ShadervarUnion v)
{
  switch (type)
  {
    case SHVT_REAL: ShaderGlobal::set_real(varId, eastl::get<float>(v)); break;
    case SHVT_INT: ShaderGlobal::set_int(varId, eastl::get<int>(v)); break;
    case SHVT_COLOR4: ShaderGlobal::set_color4(varId, eastl::get<Point4>(v)); break;
    case SHVT_INT4: ShaderGlobal::set_int4(varId, eastl::get<IPoint4>(v)); break;
    default: G_ASSERT_FAIL("Unsupported type in PriorityManagedShadervar!!! Type = %d", type); break;
  }
}

int PriorityManagedShadervar::getVarId() { return varId; }

PriorityManagedShadervar &PriorityManagedShadervarMap::getEntry(int id)
{
  auto ptr = map.find(id);
  if (ptr != map.end())
  {
    G_ASSERT(ptr->second.getVarId() == id);
    return ptr->second;
  }
  else
  {
    return map[id] = PriorityManagedShadervar(id);
  }
}

namespace PriorityShadervar
{
static PriorityManagedShadervarMap map;

void set_real(int id, int prio, float value) { map.getEntry(id).update(prio, value); }
void set_int(int id, int prio, int value) { map.getEntry(id).update(prio, value); }
void set_color4(int id, int prio, Point4 value) { map.getEntry(id).update(prio, value); }
void set_int4(int id, int prio, IPoint4 value) { map.getEntry(id).update(prio, value); }
void clear(int id, int prio) { map.getEntry(id).clear(prio); }
} // namespace PriorityShadervar
