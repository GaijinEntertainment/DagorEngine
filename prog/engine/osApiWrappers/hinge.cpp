// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <osApiWrappers/dag_hinge.h>
#include <debug/dag_assert.h>

static constexpr int MAX_LISTENERS = 8;
static IHingeListener *listeners[MAX_LISTENERS] = {};
static HingeState current_state;

HingeState get_hinge_state() { return current_state; }

void set_hinge_state(const HingeState &state)
{
  if (state.status == current_state.status && state.angle == current_state.angle)
    return;
  current_state = state;
  for (IHingeListener *const &slot : listeners)
    if (IHingeListener *listener = slot)
      listener->onHingeChanged(state);
}

void add_hinge_listener(IHingeListener *listener)
{
  int freeSlot = -1;
  for (int i = 0; i < MAX_LISTENERS; i++)
  {
    if (listeners[i] == listener)
      return;
    if (!listeners[i] && freeSlot < 0)
      freeSlot = i;
  }
  G_ASSERT_RETURN(freeSlot >= 0, );
  listeners[freeSlot] = listener;
}

void del_hinge_listener(IHingeListener *listener)
{
  for (IHingeListener *&slot : listeners)
    if (slot == listener)
      slot = nullptr;
}
