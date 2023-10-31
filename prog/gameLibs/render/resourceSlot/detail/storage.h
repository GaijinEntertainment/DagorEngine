#pragma once

#include <render/resourceSlot/state.h>
#include <render/daBfg/nameSpace.h>

#include <detail/nodeDeclaration.h>
#include <detail/resSlotNameMap.h>
#include <detail/autoGrowVector.h>

#include <ska_hash_map/flat_hash_map2.hpp>


namespace resource_slot::detail
{

struct Storage
{
  ResSlotNameMap<NodeId> nodeMap;
  ResSlotNameMap<SlotId> slotMap;
  ResSlotNameMap<ResourceId> resourceMap;

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

using StorageList = ska::flat_hash_map<dabfg::NameSpace, Storage>;
extern StorageList storage_list;

} // namespace resource_slot::detail