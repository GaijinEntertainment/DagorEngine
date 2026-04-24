// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <math/dag_mathUtils.h>
#include <render/daFrameGraph/registry.h>
#include <render/world/postfxRender.h>
#include <render/world/wrDispatcher.h>

ECS_ON_EVENT(on_appear, on_disappear)
ECS_TRACK(screen_fade)
static void screen_fade_es_event_handler(const ecs::Event &evt, float screen_fade)
{
  if (!WRDispatcher::isReadyToUse())
    return;
  const bool dest = evt.is<ecs::EventEntityDestroyed>() || evt.is<ecs::EventComponentsDisappear>();
  set_fade_mul(dest ? 1 : sqr(sqr((1.f - screen_fade))));
}
