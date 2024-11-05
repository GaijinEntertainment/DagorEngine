#include <perfMon/dag_statDrv.h>
#include <util/dag_console.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/event.h>
#include <profileEventsGUI/profileEventsGUI.h>
#include "renderEvent.h"

static ProfileUniqueEventsGUI allEvents;

ECS_NO_ORDER
ECS_TAG(dev, render)
ECS_ON_EVENT(RenderEventDebugGUI)
static inline void unique_event_profile_render_es_event_handler(const ecs::Event &)
{
#if DAGOR_DBGLEVEL > 0
  allEvents.drawProfiled();
#endif
}
#if DAGOR_DBGLEVEL > 0
static bool unique_events_profile_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("profiler", "event", 1, 3)
  {
    allEvents.addProfile(argc > 1 ? argv[1] : "-", argc > 2 ? console::to_int(argv[2]) : 1024);
  }
  CONSOLE_CHECK_NAME("profiler", "flush_event", 1, 2) { allEvents.flush(argc > 1 ? argv[1] : nullptr); }
  return found;
}

REGISTER_CONSOLE_HANDLER(unique_events_profile_console_handler);
#endif