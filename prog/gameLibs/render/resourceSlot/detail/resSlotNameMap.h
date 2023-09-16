#pragma once

#include <util/dag_oaHashNameMap.h>

namespace resource_slot::detail
{

template <typename SomeId>
struct ResSlotNameMap
{
  SomeId id(const char *name) { return SomeId{nameMap.addNameId(name)}; }
  const char *name(SomeId some_id) const { return nameMap.getName(static_cast<int>(some_id)); }
  const int nameCount() const { return nameMap.nameCount(); }

private:
  NameMap nameMap;
};

} // namespace resource_slot::detail