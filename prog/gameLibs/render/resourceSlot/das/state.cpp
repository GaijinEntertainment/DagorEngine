// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/resourceSlot/das/resourceSlotCoreModule.h>

const char *bind_dascript::resourceToReadFrom(const resource_slot::State &state, const char *slot_name)
{
  return state.resourceToReadFrom(slot_name);
}

const char *bind_dascript::resourceToCreateFor(const resource_slot::State &state, const char *slot_name)
{
  return state.resourceToCreateFor(slot_name);
}