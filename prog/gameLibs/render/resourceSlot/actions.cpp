// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/actions.h>
#include <detail/storage.h>

::resource_slot::Create::Create(const char *slot_name, const char *resource_name) :
  slot(::resource_slot::detail::slot_map.id(slot_name)), resource(::resource_slot::detail::resource_map.id(resource_name))
{}

::resource_slot::Update::Update(const char *slot_name, const char *resource_name, int update_priority) :
  slot(::resource_slot::detail::slot_map.id(slot_name)),
  resource(::resource_slot::detail::resource_map.id(resource_name)),
  priority(update_priority)
{}

::resource_slot::Read::Read(const char *slot_name, int read_priority) :
  slot(::resource_slot::detail::slot_map.id(slot_name)), priority(read_priority)
{}