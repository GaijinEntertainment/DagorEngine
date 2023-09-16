#include <render/resourceSlot/detail/registerAccess.h>
#include <render/resourceSlot/actions.h>

#include <detail/storage.h>

resource_slot::NodeHandleWithSlotsAccess resource_slot::detail::register_access(const char *name, const char *source_location,
  ActionList &&action_list, AccessCallback &&declaration_callback, unsigned storage_id)
{
  Storage &storage = storage_list[storage_id];
  storage.isNodeRegisterRequired = true;
  NodeId nodeId = storage.nodeMap.id(name);

  NodeDeclaration &node = storage.registeredNodes[nodeId];
  unsigned generation = node.generation + 1;
  node = NodeDeclaration(nodeId, generation, source_location, eastl::move(declaration_callback));

  for (const SlotAction &declaration : action_list)
  {
    eastl::visit(
      [&node, &storage](const auto &decl) {
        typedef eastl::remove_cvref_t<decltype(decl)> ValueT;
        if constexpr (eastl::is_same_v<ValueT, Create>)
        {
          node.action_list.push_back(CreateDecl{storage.slotMap.id(decl.slotName), storage.resourceMap.id(decl.resourceName)});
        }
        else if constexpr (eastl::is_same_v<ValueT, Update>)
        {
          node.action_list.push_back(
            UpdateDecl{storage.slotMap.id(decl.slotName), storage.resourceMap.id(decl.resourceName), decl.priority});
        }
        else if constexpr (eastl::is_same_v<ValueT, Read>)
        {
          node.action_list.push_back(ReadDecl{storage.slotMap.id(decl.slotName), decl.priority});
        }
      },
      declaration);
  }

  return NodeHandleWithSlotsAccess{storage_id, static_cast<int>(nodeId), generation};
}
