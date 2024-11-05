// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccess.h>
#include <render/resourceSlot/ecs/nodeHandleWithSlotsAccessVector.h>

#include <detail/storage.h>
#include <detail/unregisterAccess.h>
#include <render/daBfg/bfg.h>


ECS_DEF_PULL_VAR(nodeHandleWithSlotsAccess);
ECS_REGISTER_RELOCATABLE_TYPE(resource_slot::NodeHandleWithSlotsAccess, nullptr);
ECS_REGISTER_RELOCATABLE_TYPE(resource_slot::NodeHandleWithSlotsAccessVector, nullptr);

void resource_slot::NodeHandleWithSlotsAccess::reset()
{
  if (!isValid)
    return;
  isValid = false;

  detail::NodeId nodeId = detail::NodeId{id};
  detail::Storage &storage = detail::storage_list[nameSpace];

  detail::unregister_access(storage, nodeId, generation);
}
resource_slot::NodeHandleWithSlotsAccess::~NodeHandleWithSlotsAccess() { reset(); }

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(dabfg::NameSpace ns, int handle_id, unsigned generation_number) :
  nameSpace(ns), id(handle_id), generation(generation_number), isValid(true)
{
  G_ASSERT(handle_id >= 0);
  G_ASSERT(handle_id <= (1 << 27));
  G_ASSERT(generation_number <= (1U << 31U));
};

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess() : nameSpace(dabfg::root()), id(0), generation(0), isValid(false)
{}

resource_slot::NodeHandleWithSlotsAccess::NodeHandleWithSlotsAccess(NodeHandleWithSlotsAccess &&h) :
  nameSpace(h.nameSpace), id(h.id), generation(h.generation), isValid(h.isValid)
{
  h.isValid = false;
}

resource_slot::NodeHandleWithSlotsAccess &resource_slot::NodeHandleWithSlotsAccess::operator=(NodeHandleWithSlotsAccess &&h)
{
  reset();
  nameSpace = h.nameSpace;
  id = h.id;
  generation = h.generation;
  isValid = h.isValid;
  h.isValid = false;
  return *this;
}

bool resource_slot::NodeHandleWithSlotsAccess::valid() const { return isValid; }

void resource_slot::NodeHandleWithSlotsAccess::_noteContext(const das::Context *context) const
{
  G_ASSERT_RETURN(isValid, );

  detail::NodeId nodeId = detail::NodeId{id};
  detail::Storage &storage = detail::storage_list[nameSpace];

  storage.registeredNodes[nodeId].context = context;
}
