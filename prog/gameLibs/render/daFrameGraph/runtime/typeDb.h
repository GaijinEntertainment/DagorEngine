// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>
#include <EASTL/deque.h>
#include <util/dag_oaHashNameMap.h>
#include <ska_hash_map/flat_hash_map2.hpp>

#include <render/daFrameGraph/detail/resourceType.h>
#include <render/daFrameGraph/detail/rtti.h>


namespace dafg
{

using dafg::detail::RTTI;

// Stores information about various types known to the dafg runtime,
// including mappings between C++ and scripting languages.
class TypeDb
{
  inline static constexpr size_t RESERVE_SIZE = 512;

public:
  TypeDb()
  {
    foreignNameToId.reserve(RESERVE_SIZE);
    idToTag.reserve(RESERVE_SIZE);
    isForeignType.reserve(RESERVE_SIZE);
  }

  void registerNativeType(ResourceSubtypeTag native_tag, RTTI &&rtti);
  void registerInteropType(const char *foreign_name, ResourceSubtypeTag native_tag, RTTI &&rtti);
  ResourceSubtypeTag registerForeignType(const char *foreign_name, RTTI &&rtti);
  ResourceSubtypeTag tagForForeign(const char *foreign_name) const;

  const RTTI *getRTTI(dafg::ResourceSubtypeTag tag) const;

private:
  OAHashNameMap<false> foreignNameToId;
  dag::Vector<dafg::ResourceSubtypeTag> idToTag;
  eastl::bitvector<> isForeignType;
  eastl::deque<uint8_t> addressPool;

  // RTTI objects need to be stable in memory to avoid pointer invalidation
  ska::flat_hash_map<dafg::ResourceSubtypeTag, eastl::unique_ptr<RTTI>> rttis;
};

} // namespace dafg
