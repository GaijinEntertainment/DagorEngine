// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>

void bind_dascript::ActionList_create(::resource_slot::detail::ActionList &list, const char *slot_name, const char *resource_name)
{
  list.emplace_back(::resource_slot::Create(slot_name, resource_name));
}

void bind_dascript::ActionList_update(::resource_slot::detail::ActionList &list, const char *slot_name, const char *resource_name,
  int priority)
{
  list.emplace_back(::resource_slot::Update(slot_name, resource_name, priority));
}

void bind_dascript::ActionList_read(::resource_slot::detail::ActionList &list, const char *slot_name, int priority)
{
  list.emplace_back(::resource_slot::Read(slot_name, priority));
}