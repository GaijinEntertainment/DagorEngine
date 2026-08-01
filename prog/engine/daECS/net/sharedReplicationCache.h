// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <EASTL/bitvector.h>
#include <daNet/bitStream.h>
#include <dag/dag_vector.h>
#include <memory/dag_framemem.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <daECS/core/entityId.h>
#include <daECS/net/object.h>


namespace net
{

struct SharedReplicationCache
{
  struct SerializerState
  {
    uint16_t componentsInTemplate = 0, prevComponent = 0, writtenComponents = 0;
  };
  struct ConstructionEntry
  {
    bool valid = false;
    SerializerState state;
    danet::BitStream bs{framemem_ptr()};
    eastl::bitvector<framemem_allocator, uint64_t> objectKeysUsed;
    dag::Vector<eastl::pair<ecs::component_index_t, net::compver_t>, framemem_allocator> forceReplicaVersionComps;
  };
  ska::flat_hash_map<ecs::EntityId, ConstructionEntry, ecs::EidHashFNV1a, eastl::equal_to<ecs::EntityId>, framemem_allocator>
    construction;
  danet::BitStream stagingBs{framemem_ptr()};
};

} // namespace net
