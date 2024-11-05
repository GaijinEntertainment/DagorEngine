// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <startup/dag_globalSettings.h>
#include <visualConsole/dag_visualConsole.h>
#include <ioSys/dag_dataBlock.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>

ECS_REQUIRE(ecs::Tag visualConsoleObject)
ECS_ON_EVENT(on_appear)
static void visual_cnsl_es_event_handler(const ecs::Event &)
{
#if _TARGET_PC_WIN
  if (dgs_get_settings()->getBlockByNameEx("debug")->getBool("canCreateConsole", false))
  {
    debug("setup_visual_cnsl_driver");
    setup_visual_console_driver();
  }
#endif
}
