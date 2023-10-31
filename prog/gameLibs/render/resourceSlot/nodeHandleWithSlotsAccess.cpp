#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>

#include <detail/storage.h>
#include <detail/unregisterAccess.h>
#include <render/daBfg/bfg.h>


ECS_REGISTER_RELOCATABLE_TYPE(resource_slot::NodeHandleWithSlotsAccess, nullptr);

void resource_slot::NodeHandleWithSlotsAccess::reset()
{
  if (!valid)
    return;
  valid = false;

  detail::NodeId nodeId = detail::NodeId{id};
  detail::Storage &storage = detail::storage_list[nameSpace];

  detail::unregister_access(storage, nodeId, generation);
}
resource_slot::NodeHandleWithSlotsAccess::~NodeHandleWithSlotsAccess() { reset(); }

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(dabfg::NameSpace ns, int handle_id, unsigned generation_number) :
  nameSpace(ns), id(handle_id), generation(generation_number), valid(true)
{
  G_ASSERT(handle_id >= 0);
  G_ASSERT(handle_id <= (1 << 27));
  G_ASSERT(generation_number <= (1U << 31U));
};

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess() : nameSpace(dabfg::root()), id(0), generation(0), valid(false) {}

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(NodeHandleWithSlotsAccess &&h) :
  nameSpace(h.nameSpace), id(h.id), generation(h.generation), valid(h.valid)
{
  h.valid = false;
}

resource_slot::NodeHandleWithSlotsAccess &resource_slot::NodeHandleWithSlotsAccess::operator=(NodeHandleWithSlotsAccess &&h)
{
  reset();
  nameSpace = h.nameSpace;
  id = h.id;
  generation = h.generation;
  valid = h.valid;
  h.valid = false;
  return *this;
}