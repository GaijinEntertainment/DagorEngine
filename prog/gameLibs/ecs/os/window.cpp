// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/event.h>

#include <osApiWrappers/dag_wndProcCompMsg.h>
#include <osApiWrappers/dag_wndProcComponent.h>
#include <startup/dag_globalSettings.h>
#include <util/dag_delayedAction.h>
#include <quirrel/bindQuirrelEx/autoBind.h>
#include <ecs/os/window.h>
#include <ecs/scripts/sqBindEvent.h>

ECS_REGISTER_EVENT(EventWindowActivated);
ECS_REGISTER_EVENT(EventWindowDeactivated);

namespace ecs_os
{

static class WndHandler : public IWndProcComponent
{
public:
  RetCode process(void * /*hwnd*/, unsigned msg, uintptr_t wParam, intptr_t /*lParam*/, intptr_t & /*result*/) override
  {
    if (msg == GPCM_Activate)
    {
      if (g_entity_mgr) // Note: could be already destroyed
      {
        if (uint16_t(wParam) != GPCMP1_Inactivate) // Note: in high bits could be minimized state
          g_entity_mgr->broadcastEvent(EventWindowActivated());
        else
          g_entity_mgr->broadcastEvent(EventWindowDeactivated());
      }
    }
    return PROCEED_OTHER_COMPONENTS;
  }
} wnd_handler;

void init_window_handler() { add_wnd_proc_component(&wnd_handler); }

void cleanup_window_handler() { del_wnd_proc_component(&wnd_handler); }


SQ_DEF_AUTO_BINDING_MODULE(bind_events, "os.window")
{
  return ecs::sq::EventsBind<EventWindowActivated, EventWindowDeactivated, ecs::sq::term_event_t>::bindall(vm);
}

} // namespace ecs_os
