// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "typeDb.h"


using namespace dafg;

void TypeDb::registerNativeType(ResourceSubtypeTag native_tag, RTTI &&rtti)
{
  if (rttis.find(native_tag) != rttis.end())
    return;

  rttis[native_tag] = eastl::make_unique<RTTI>(eastl::move(rtti));
}

void TypeDb::registerInteropType(const char *foreign_name, ResourceSubtypeTag native_tag, RTTI &&rtti)
{
  if (const int id = foreignNameToId.getNameId(foreign_name); id >= 0)
  {
    G_ASSERTF(idToTag[id] == native_tag && !isForeignType[id],
      "Tried to register an interop type that already is registered as a foreign type!");
    return;
  }

  foreignNameToId.addNameId(foreign_name);
  idToTag.push_back(native_tag);
  isForeignType.push_back(false);
  G_ASSERT(rttis.find(native_tag) == rttis.end());
  rttis[native_tag] = eastl::make_unique<RTTI>(eastl::move(rtti));
}

ResourceSubtypeTag TypeDb::registerForeignType(const char *foreign_name, RTTI &&rtti)
{
  if (const int id = foreignNameToId.getNameId(foreign_name); id >= 0)
    return idToTag[id];

  foreignNameToId.addNameId(foreign_name);
  // NOTE: address pool must be a deque so that pointers are not invalidated here
  addressPool.emplace_back();
  const auto dynamicTag = static_cast<ResourceSubtypeTag>(eastl::bit_cast<uintptr_t>(&addressPool.back()));
  idToTag.push_back(dynamicTag);
  isForeignType.push_back(true);
  G_ASSERT(rttis.find(dynamicTag) == rttis.end());
  rttis[dynamicTag] = eastl::make_unique<RTTI>(eastl::move(rtti));

  return dynamicTag;
}

ResourceSubtypeTag TypeDb::tagForForeign(const char *foreign_name) const
{
  const int id = foreignNameToId.getNameId(foreign_name);
  G_ASSERTF(id >= 0 && id < idToTag.size(), "foreign_name '%s' was not registered!", foreign_name);
  return idToTag[id];
}

const RTTI *TypeDb::getRTTI(dafg::ResourceSubtypeTag tag) const
{
  const auto it = rttis.find(tag);
  return it != rttis.end() ? it->second.get() : nullptr;
}
