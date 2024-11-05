// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"

ECS_TAG(render)
ECS_REQUIRE(ecs::Tag request_depth_above_render_transparent)
ECS_ON_EVENT(on_appear)
static void request_depth_above_render_transparent_on_appear_es(const ecs::Event)
{
  if (auto wr = (WorldRenderer *)get_world_renderer())
    wr->requestDepthAboveRenderTransparent();
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
ECS_REQUIRE(ecs::Tag request_depth_above_render_transparent)
static void request_depth_above_render_transparent_on_disappear_es(const ecs::Event)
{
  if (auto wr = (WorldRenderer *)get_world_renderer())
    wr->revokeDepthAboveRenderTransparent();
}