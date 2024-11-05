// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/input/debugInputEvents.h>
#include <util/dag_delayedAction.h>
#include <drv/hid/dag_hiGlobals.h>
#include <ecs/input/hidEventRouter.h>


#define DEBUG_INPUT_ECS_EVENT ECS_REGISTER_EVENT
DEBUG_INPUT_ECS_EVENTS
#undef DEBUG_INPUT_ECS_EVENT

namespace debuginputevents
{
class DebugInputEventTranslator : public ecs::IGenHidEventHandler
{
  bool ehIsActive() const override { return DAGOR_DBGLEVEL > 0; }

  bool gmehMove(const Context &, float dx, float dy) override
  {
    g_entity_mgr->broadcastEvent(EventDebugMouseMoved(Point2(dx, dy)));
    return false;
  }

  bool gmehButtonDown(const Context &, int btn_idx) override
  {
    g_entity_mgr->broadcastEvent(EventDebugMousePressed(btn_idx));
    return false;
  }

  bool gmehButtonUp(const Context &, int btn_idx) override
  {
    g_entity_mgr->broadcastEvent(EventDebugMouseReleased(btn_idx));
    return false;
  }

  bool gmehWheel(const Context &, int) override { return false; }

  bool gkehButtonDown(const Context &, int btn_idx, bool repeat, wchar_t) override
  {
    if (!repeat)
      g_entity_mgr->broadcastEvent(EventDebugKeyboardPressed(btn_idx));
    return false;
  }

  bool gkehButtonUp(const Context &, int btn_idx) override
  {
    g_entity_mgr->broadcastEvent(EventDebugKeyboardReleased(btn_idx));
    return false;
  }
};

#if DAGOR_DBGLEVEL > 0
static DebugInputEventTranslator translator;

static void held_event_dispatch(void *)
{
  for (uint32_t i = 0; i < HumanInput::DKEY__MAX_BUTTONS; i++)
  {
    if (HumanInput::raw_state_kbd.isKeyDown(i))
      g_entity_mgr->broadcastEvent(EventDebugKeyboardHeld(i));
  }
  for (uint32_t i = 0; i <= 6; i++)
  {
    if (HumanInput::raw_state_pnt.mouse.isBtnDown(i))
      g_entity_mgr->broadcastEvent(EventDebugMouseHeld(i));
  }
}
#endif

void init()
{
#if DAGOR_DBGLEVEL > 0
  register_hid_event_handler(&translator);
  register_regular_action_to_idle_cycle(held_event_dispatch, nullptr);
#endif
}

void close()
{
#if DAGOR_DBGLEVEL > 0
  unregister_hid_event_handler(&translator);
  unregister_regular_action_to_idle_cycle(held_event_dispatch, nullptr);
#endif
}
} // namespace debuginputevents
