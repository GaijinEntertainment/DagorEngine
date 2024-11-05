// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <render/resourceSlot/state.h>
#include <render/daBfg/nameSpace.h>

#include <detail/nodeDeclaration.h>
#include <detail/resSlotNameMap.h>
#include <detail/autoGrowVector.h>

#include <EASTL/vector_map.h>


namespace resource_slot::detail
{

struct Storage
{
  ResSlotNameMap<NodeId> nodeMap;

  AutoGrowVector<NodeId, NodeDeclaration, EXPECTED_MAX_NODE_COUNT> registeredNodes;
  AutoGrowVector<SlotId, ResourceId, EXPECTED_MAX_SLOT_COUNT> currentSlotsState;
  bool isNodeRegisterRequired = false;
  int validNodeCount;

  Storage() = default;
  ~Storage() = default;
  Storage(Storage &&) = default;
  Storage &operator=(Storage &&) = default;
  Storage(const Storage &) = delete;
  Storage &operator=(const Storage &) = delete;
};

using StorageList = eastl::vector_map<dabfg::NameSpace, Storage>;
extern StorageList storage_list;
extern ResSlotNameMap<SlotId> slot_map;
extern ResSlotNameMap<ResourceId> resource_map;

} // namespace resource_slot::detail