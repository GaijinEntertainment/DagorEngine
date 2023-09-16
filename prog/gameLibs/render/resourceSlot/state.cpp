#include <render/resourceSlot/state.h>

#include <detail/storage.h>

resource_slot::State::State(unsigned storage_id, int node_id) : storageId(storage_id), nodeId(node_id)
{
  // Fits into storage bits
  G_ASSERT(node_id >= 0);
  G_ASSERT(node_id <= (1 << 26));
  G_ASSERT(storage_id <= (1 << 4));
}

// Read current slots
const char *resource_slot::State::resourceToReadFrom(const char *slot_name) const
{
  detail::Storage &storage = detail::storage_list[storageId];
  G_ASSERTF_RETURN(nodeId >= 0 && nodeId < storage.nodeMap.nameCount(), nullptr, "Unexpected nodeId: %d", nodeId);
  detail::NodeId ownerNodeId = detail::NodeId{nodeId};

  const detail::NodeDeclaration &node = storage.registeredNodes[ownerNodeId];
  G_ASSERTF_RETURN(node.id != detail::NodeId::Invalid, nullptr, "Node \"%s\" has already unregistered",
    storage.nodeMap.name(ownerNodeId));

  detail::SlotId slotId = storage.slotMap.id(slot_name);
  const auto *slot = node.resourcesBeforeNode.find(slotId);
  G_ASSERTF_RETURN(slot != node.resourcesBeforeNode.end(), nullptr, "Missed declaration of Read or Update slot \"%s\" in node \"%s\"",
    slot_name, storage.nodeMap.name(ownerNodeId));

  return storage.resourceMap.name(slot->second);
}

const char *resource_slot::State::resourceToCreateFor(const char *slot_name) const
{
  detail::Storage &storage = detail::storage_list[storageId];
  G_ASSERTF_RETURN(nodeId >= 0 && nodeId < storage.nodeMap.nameCount(), nullptr, "Unexpected nodeId: %d", nodeId);
  detail::NodeId ownerNodeId = detail::NodeId{nodeId};

  const detail::NodeDeclaration &node = storage.registeredNodes[ownerNodeId];
  G_ASSERTF_RETURN(node.id != detail::NodeId::Invalid, nullptr, "Node \"%s\" has already unregistered",
    storage.nodeMap.name(ownerNodeId));

  detail::SlotId slotId = storage.slotMap.id(slot_name);
  const char *resourceName = nullptr;

  const detail::AccessDecl *declaration = eastl::find_if(node.action_list.begin(), node.action_list.end(),
    [slotId, &resourceName, &storage](const detail::AccessDecl &declaration) {
      return eastl::visit(
        [slotId, &resourceName, &storage](const auto &decl) {
          typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
          if constexpr (eastl::is_same_v<ValueT, detail::CreateDecl> || eastl::is_same_v<ValueT, detail::UpdateDecl>)
          {
            if (decl.slot == slotId)
            {
              resourceName = storage.resourceMap.name(decl.resource);
              return true;
            }
            return false;
          }

          return false;
        },
        declaration);
    });

  G_ASSERTF_RETURN(declaration != node.action_list.end(), nullptr, "Missed declaration of Create or Update slot \"%s\" in node \"%s\"",
    slot_name, storage.nodeMap.name(ownerNodeId));

  return resourceName;
}