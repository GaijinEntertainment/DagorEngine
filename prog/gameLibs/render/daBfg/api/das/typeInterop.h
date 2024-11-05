// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daScript/daScript.h>
#include <EASTL/bitvector.h>
#include <EASTL/deque.h>
#include <util/dag_oaHashNameMap.h>

#include <render/daBfg/detail/resourceType.h>


class TypeInterop
{
  inline static constexpr size_t RESERVE_SIZE = 512;

public:
  TypeInterop()
  {
    nameToId.reserve(RESERVE_SIZE);
    idToTag.reserve(RESERVE_SIZE);
    isDasOnlyType.reserve(RESERVE_SIZE);
  }

  void tryRegisterBuiltins(const das::ModuleLibrary &lib);

  dabfg::ResourceSubtypeTag registerInteropType(const char *mangled_name, dabfg::ResourceSubtypeTag tag);

  dabfg::ResourceSubtypeTag tryRegisterDasOnlyType(const char *mangled_name);

  dabfg::ResourceSubtypeTag tagFor(const char *mangled_name) const;

private:
  OAHashNameMap<false> nameToId;
  dag::Vector<dabfg::ResourceSubtypeTag> idToTag;
  eastl::bitvector<> isDasOnlyType;
  eastl::deque<uint8_t> addressPool;
  bool builtinsRegistered = false;
};
