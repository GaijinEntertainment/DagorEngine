//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <math/dag_Point4.h>
#include <math/dag_frustum.h>
#include <daECS/core/event.h>
#include <ecs/render/transformHolder.h>

namespace animchar_disintegration
{

struct DisintegrationParameters
{
  float duration = 0;
  float scale = 1;
};

inline Point4 get_additional_data(float animation_time, const DisintegrationParameters &params)
{
  float progress = params.duration > 0 ? clamp(0.f, 1.f, animation_time / params.duration) : 0.f;
  return Point4(progress, params.scale, 0, 0);
}

struct RenderDisintegrationDepthPrepassEvent : public ecs::Event, public TransformHolder
{
  Frustum cullingFrustum;
  TMatrix viewItm;
  Occlusion *occlusion;
  ECS_INSIDE_EVENT_DECL(RenderDisintegrationDepthPrepassEvent, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE);
  TexStreamingContext texCtx;
  RenderDisintegrationDepthPrepassEvent(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &itm,
    const Frustum &culling_frustum, Occlusion *occl, TexStreamingContext tex_context = TEX_STREAMING_CTX_NULL) :
    ECS_EVENT_CONSTRUCTOR(RenderDisintegrationDepthPrepassEvent),
    TransformHolder(view_tm, proj_tm),
    cullingFrustum(culling_frustum),
    viewItm(itm),
    occlusion(occl),
    texCtx(tex_context)
  {}
};

} // namespace animchar_disintegration