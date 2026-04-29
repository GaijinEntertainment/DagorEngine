// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/string.h>
#include <debug/dag_debug.h>
#include <util/dag_console.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/event.h>
#include <debug/dag_hwExcept.h>
#include "main/hostedServerLauncher.h"


ECS_ON_EVENT(EventHostedInternalServerToStart)
static void event_start_internal_server_es(const EventHostedInternalServerToStart &e)
{
  debug("event_start_internal_server_es");
  G_STATIC_ASSERT(sizeof(SimpleString) == sizeof(const char *));

  const ecs::List<ecs::string> &cmds = e.get<0>();
  eastl::vector<SimpleString> cmdsCopy(cmds.size());
  for (int i = 0; i < cmds.size(); ++i)
    cmdsCopy[i] = cmds[i].c_str();

  int argc = cmdsCopy.size();
  char **argv = (char **)cmdsCopy.data();
  schedule_new_internal_server_with_args(argc, argv);
}

ECS_ON_EVENT(EventHostedInternalServerToStop)
static void event_stop_internal_server_es(const ecs::Event &) { kill_internal_server(false); }
