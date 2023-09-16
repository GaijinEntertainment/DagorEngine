#pragma once

#include <render/resourceSlot/state.h>

#include <detail/nodeDeclaration.h>
#include <detail/resSlotNameMap.h>
#include <detail/autoGrowVector.h>


namespace resource_slot::detail
{

struct Storage
{
  ResSlotNameMap<NodeId> nodeMap;
  ResSlotNameMap<SlotId> slotMap;
  ResSlotNameMap<ResourceId> resourceMap;

  AutoGrowVector<NodeId, NodeDeclaration, EXPECTED_MAX_NODE_COUNT> registeredNodes;
  bool isNodeRegisterRequired = false;

  Storage() = default;
  ~Storage() = default;
  Storage(Storage &&) = default;
  Storage &operator=(Storage &&) = default;
  Storage(const Storage &) = delete;
  Storage &operator=(const Storage &) = delete;
};

// NOTE: in the future, we could have several storages for several frame graphs
extern dag::RelocatableFixedVector<Storage, EXPECTED_MAX_STORAGE_COUNT> storage_list;

} // namespace resource_slot::detail