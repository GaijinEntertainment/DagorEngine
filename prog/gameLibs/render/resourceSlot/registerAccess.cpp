// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/detail/registerAccess.h>
#include <render/resourceSlot/actions.h>

#include <detail/storage.h>

resource_slot::NodeHandleWithSlotsAccess resource_slot::detail::register_access(dafg::NameSpace ns, const char *name,
  ActionList &&action_list, AccessCallback &&declaration_callback)
{
  Storage &storage = storage_list[ns];
  storage.isNodeRegisterRequired = true;
  NodeId nodeId = storage.nodeMap.id(name);

  NodeDeclaration &node = storage.registeredNodes[nodeId];
  unsigned generation = node.generation + 1;
  node = NodeDeclaration(nodeId, generation, eastl::move(action_list), eastl::move(declaration_callback));

  return NodeHandleWithSlotsAccess{ns, static_cast<int>(nodeId), generation};
}
