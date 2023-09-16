#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>

#include <detail/storage.h>
#include <detail/unregisterAccess.h>

ECS_REGISTER_RELOCATABLE_TYPE(resource_slot::NodeHandleWithSlotsAccess, nullptr);

void resource_slot::NodeHandleWithSlotsAccess::reset()
{
  if (!valid)
    return;
  valid = false;

  detail::NodeId nodeId = detail::NodeId{id};
  detail::Storage &storage = detail::storage_list[storageId];

  detail::unregister_access(storage, nodeId, generation);
}
resource_slot::NodeHandleWithSlotsAccess::~NodeHandleWithSlotsAccess() { reset(); }

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(unsigned storage_id, int handle_id, unsigned generation_number) :
  storageId(storage_id), id(handle_id), generation(generation_number), valid(true)
{
  G_ASSERT(handle_id >= 0);
  G_ASSERT(handle_id <= (1 << 27));
  G_ASSERT(storage_id <= (1 << 4));
  G_ASSERT(generation_number <= (1U << 31U));
};

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess() : storageId(0), id(0), generation(0), valid(false) {}

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(NodeHandleWithSlotsAccess &&h) :
  storageId(h.storageId), id(h.id), generation(h.generation), valid(h.valid)
{
  h.valid = false;
}

resource_slot::NodeHandleWithSlotsAccess &resource_slot::NodeHandleWithSlotsAccess::operator=(NodeHandleWithSlotsAccess &&h)
{
  reset();
  storageId = h.storageId;
  id = h.id;
  generation = h.generation;
  valid = h.valid;
  h.valid = false;
  return *this;
}