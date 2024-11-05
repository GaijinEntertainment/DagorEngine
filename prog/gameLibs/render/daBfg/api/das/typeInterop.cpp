// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "typeInterop.h"


#define BUILTIN_TYPES \
  X(bool)             \
  X(int8_t)           \
  X(uint8_t)          \
  X(int16_t)          \
  X(uint16_t)         \
  X(int32_t)          \
  X(uint32_t)         \
  X(int64_t)          \
  X(uint64_t)         \
  X(float)            \
  X(double)

void TypeInterop::tryRegisterBuiltins(const das::ModuleLibrary &lib)
{
  if (eastl::exchange(builtinsRegistered, true))
    return;

#define X(T) registerInteropType(das::typeFactory<T>::make(lib)->getMangledName(true).c_str(), dabfg::tag_for<T>());
  BUILTIN_TYPES
#undef X
}

dabfg::ResourceSubtypeTag TypeInterop::registerInteropType(const char *mangled_name, dabfg::ResourceSubtypeTag tag)
{
  if (const int id = nameToId.getNameId(mangled_name); id >= 0)
  {
    G_ASSERTF(idToTag[id] == tag && !isDasOnlyType[id],
      "Tried to register a das-c++ interop type that already is registered as a das-only type!");
    return dabfg::ResourceSubtypeTag::Invalid;
  }

  nameToId.addNameId(mangled_name);
  idToTag.push_back(tag);
  isDasOnlyType.push_back(false);

  return tag;
}

dabfg::ResourceSubtypeTag TypeInterop::tryRegisterDasOnlyType(const char *mangled_name)
{
  if (const int id = nameToId.getNameId(mangled_name); id >= 0)
    return idToTag[id];

  nameToId.addNameId(mangled_name);
  // NOTE: address pool must be a deque so that pointers are not invalidated here
  addressPool.emplace_back();
  const auto dynamicTag = static_cast<dabfg::ResourceSubtypeTag>(eastl::bit_cast<uintptr_t>(&addressPool.back()));
  idToTag.push_back(dynamicTag);
  isDasOnlyType.push_back(true);

  return dynamicTag;
}

dabfg::ResourceSubtypeTag TypeInterop::tagFor(const char *mangled_name) const
{
  const int id = nameToId.getNameId(mangled_name);
  G_ASSERT(id >= 0 && id < idToTag.size());
  return idToTag[id];
}
