// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include "common.h"
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <dag/dag_vector.h>
#include <util/dag_generationReferencedData.h>
#include <util/dag_fastNameMapTS.h>
#include <osApiWrappers/dag_rwSpinLock.h>

namespace dafx
{
union ValueBind
{
  unsigned raw = 0;
  struct
  {
    unsigned offset : 24;
    unsigned size : 8;
  };
  ValueBind() = default;
  ValueBind(unsigned o, unsigned s) : offset(o), size(s) {}
  bool operator==(const ValueBind &o) const { return raw == o.raw; }
  bool operator!=(const ValueBind &o) const { return raw != o.raw; }
};

extern FastNameMapTS</*ci*/ false, SpinLockReadWriteLock> fxSysNameMap;

using ValueBindMap = dag::Vector<ValueBind>; // Indexed by nameId
using ValueBindId = GenerationRefId<8, ValueBindMap>;
using ValueBindData = GenerationReferencedData<ValueBindId, ValueBindMap>;

struct Binds
{
  ValueBindMap globalValues;
  ValueBindData localValues;
};

inline ValueBind *get_local_value_bind(ValueBindData &dst, ValueBindId bind_id, const eastl::string &name)
{
  ValueBindMap *map = dst.get(bind_id);
  if (!map)
    return nullptr;

  unsigned nameId = fxSysNameMap.getNameId(name.c_str(), name.length());
  if (nameId >= map->size())
    return nullptr;

  auto &v = (*map)[nameId];
  return v != ValueBind{} ? &v : nullptr;
}

bool register_global_value_binds(Binds &dst, const eastl::string &sys_name, const eastl::vector<ValueBindDesc> &binds);
ValueBindId create_local_value_binds(Binds &dst, const eastl::string &sys_name, const eastl::vector<ValueBindDesc> &binds,
  int size_limit);
void release_local_value_binds(Binds &dst, ValueBindId rid);
} // namespace dafx
