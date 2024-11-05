//
// DaEditorX
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daECS/core/event.h>
#include <math/dag_TMatrix.h>
#include <math/dag_Point3.h>
#include <math/dag_frustum.h>
#include <scene/dag_occlusion.h>

struct BeforeRender : public ecs::Event
{
  TMatrix viewItm;
  Frustum cullingFrustum;
  Occlusion *occlusion;

  ECS_BROADCAST_EVENT_DECL(BeforeRender)
  BeforeRender(const mat44f &globtm, const TMatrix &view_itm, Occlusion *occlusion) :
    ECS_EVENT_CONSTRUCTOR(BeforeRender), viewItm(view_itm), cullingFrustum(globtm), occlusion(occlusion)
  {}
};

struct RenderStage : public ecs::Event
{
  enum Stages
  {
    Shadow,
    Color,
    Transparent
  } stage;

  TMatrix viewItm;
  Frustum cullingFrustum;
  Occlusion *occlusion;

  ECS_BROADCAST_EVENT_DECL(RenderStage)
  RenderStage(Stages stage, const mat44f &globtm, const TMatrix &view_itm, Occlusion *occlusion) :
    ECS_EVENT_CONSTRUCTOR(RenderStage), stage(stage), viewItm(view_itm), cullingFrustum(globtm), occlusion(occlusion)
  {}
};
