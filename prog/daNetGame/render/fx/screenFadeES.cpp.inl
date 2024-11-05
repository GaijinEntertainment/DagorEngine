// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <math/dag_mathUtils.h>

#define INSIDE_RENDERER
#include "render/world/private_worldRenderer.h"

ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(screen_fade)
static void screen_fade_es_event_handler(const ecs::Event &evt, float screen_fade)
{
  WorldRenderer *renderer = (WorldRenderer *)get_world_renderer();
  if (!renderer)
    return;
  const bool dest = evt.is<ecs::EventEntityDestroyed>() || evt.is<ecs::EventComponentsDisappear>();
  renderer->setFadeMul(dest ? 1 : sqr(sqr((1.f - screen_fade))));
}
