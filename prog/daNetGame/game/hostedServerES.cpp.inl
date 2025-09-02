// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "main/gameLoad.h"
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "daECS/scene/scene.h"

void (*on_server_loaded_callback)() = NULL;

ECS_ON_EVENT(ecs::EventOnLocalSceneEntitiesCreated)
static void hosted_server_tracking_loaded_local_scene_entities_es(const ecs::Event &)
{
  debug("hosted_server_tracking_loaded_local_scene_entities");
  if (on_server_loaded_callback)
    on_server_loaded_callback();
  // I track this on server and if I'm hosted -> I need to invoke provided callback
}

#if _TARGET_PC_LINUX || _TARGET_APPLE
extern "C" __attribute__((visibility("default"))) void hosted_server_on_loaded(void *callback)
{
  debug("hosted_server_on_loaded");
  on_server_loaded_callback = (void (*)())callback;
}
#endif

#if _TARGET_PC_WIN
extern "C" __declspec(dllexport) void hosted_server_on_loaded(void *callback)
{
  debug("hosted_server_on_loaded");
  on_server_loaded_callback = (void (*)())callback;
}
#endif
