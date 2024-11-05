// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/state.h>

#include <detail/storage.h>

resource_slot::State::State() : nameSpace(dabfg::root()), nodeId(-1), order(0), size(0) {}

resource_slot::State::State(dabfg::NameSpace ns, int node_id, uint16_t order_in_chain, uint16_t size_of_chain) :
  nameSpace(ns), nodeId(node_id), order(order_in_chain), size(size_of_chain)
{
  // Fits into storage bits
  G_ASSERT(node_id >= 0);
  G_ASSERT(node_id <= (1 << 26));
}

[[nodiscard]] static inline bool is_node_registered(resource_slot::detail::NodeId node_id, resource_slot::detail::NodeStatus status)
{
  return node_id != resource_slot::detail::NodeId::Invalid &&
         (status == resource_slot::detail::NodeStatus::Valid || status == resource_slot::detail::NodeStatus::Pruned);
}

// Read current slots
const char *resource_slot::State::resourceToReadFrom(const char *slot_name) const
{
  detail::Storage &storage = detail::storage_list[nameSpace];
  G_ASSERTF_RETURN(nodeId >= 0 && nodeId < storage.nodeMap.nameCount(), nullptr, "Unexpected nodeId: %d", nodeId);
  detail::NodeId ownerNodeId = detail::NodeId{nodeId};

  detail::NodeDeclaration &node = storage.registeredNodes[ownerNodeId];
  G_ASSERTF_RETURN(is_node_registered(node.id, node.status), nullptr, "Node \"%s\" has already unregistered",
    storage.nodeMap.name(ownerNodeId));

  detail::SlotId slotId = resource_slot::detail::slot_map.id(slot_name);
  const auto *slot = node.resourcesBeforeNode.find(slotId);

  if (slot == node.resourcesBeforeNode.end())
  {
    logerr("Missed declaration of Read or Update slot \"%s\" in node \"%s\"; node will be pruned at next frame", slot_name,
      storage.nodeMap.name(ownerNodeId));
    node.status = detail::NodeStatus::Pruned;
    storage.isNodeRegisterRequired = true;
    return resource_slot::detail::resource_map.name(storage.currentSlotsState[slotId]);
  }

  return resource_slot::detail::resource_map.name(slot->second);
}

const char *resource_slot::State::resourceToCreateFor(const char *slot_name) const
{
  detail::Storage &storage = detail::storage_list[nameSpace];
  G_ASSERTF_RETURN(nodeId >= 0 && nodeId < storage.nodeMap.nameCount(), nullptr, "Unexpected nodeId: %d", nodeId);
  detail::NodeId ownerNodeId = detail::NodeId{nodeId};

  detail::NodeDeclaration &node = storage.registeredNodes[ownerNodeId];
  G_ASSERTF_RETURN(is_node_registered(node.id, node.status), nullptr, "Node \"%s\" has already unregistered",
    storage.nodeMap.name(ownerNodeId));

  detail::SlotId slotId = resource_slot::detail::slot_map.id(slot_name);
  const char *resourceName = nullptr;

  const detail::SlotAction *declaration =
    eastl::find_if(node.actionList.begin(), node.actionList.end(), [slotId, &resourceName](const detail::SlotAction &declaration) {
      return eastl::visit(
        [slotId, &resourceName](const auto &decl) {
          typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
          if constexpr (eastl::is_same_v<ValueT, Create> || eastl::is_same_v<ValueT, Update>)
          {
            if (decl.slot == slotId)
            {
              resourceName = resource_slot::detail::resource_map.name(decl.resource);
              return true;
            }
            return false;
          }

          return false;
        },
        declaration);
    });

  if (declaration == node.actionList.end())
  {
    logerr("Missed declaration of Create or Update slot \"%s\" in node \"%s\"; node will be pruned at next frame", slot_name,
      storage.nodeMap.name(ownerNodeId));
    node.status = detail::NodeStatus::Pruned;
    storage.isNodeRegisterRequired = true;
    return nullptr;
  }

  return resourceName;
}