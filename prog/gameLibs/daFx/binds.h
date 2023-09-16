#pragma once

#include "common.h"
#include <EASTL/string.h>
#include <EASTL/vector_map.h>
#include <util/dag_generationReferencedData.h>
#include <util/dag_fastNameMapTS.h>

namespace dafx
{
struct ValueBind
{
  unsigned offset : 24;
  unsigned size : 8;
};

extern FastNameMapTS<false> fxSysNameMap;

using ValueBindMap = eastl::vector_map<uint32_t, ValueBind, eastl::less<uint32_t>, EASTLAllocatorType,
  dag::Vector<eastl::pair<uint32_t, ValueBind>, EASTLAllocatorType>>;
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

  int nameId = fxSysNameMap.getNameId(name.c_str(), name.length());
  if (nameId < 0)
    return nullptr;

  ValueBindMap::iterator bindIt = map->find(nameId);
  if (bindIt == map->end())
    return nullptr;

  return &bindIt->second;
}

bool register_global_value_binds(Binds &dst, const eastl::string &sys_name, const eastl::vector<ValueBindDesc> &binds);
ValueBindId create_local_value_binds(Binds &dst, const eastl::string &sys_name, const eastl::vector<ValueBindDesc> &binds,
  int size_limit);
void release_local_value_binds(Binds &dst, ValueBindId rid);
} // namespace dafx