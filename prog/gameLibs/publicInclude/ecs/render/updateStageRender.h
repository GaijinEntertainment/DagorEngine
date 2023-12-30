//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daECS/core/updateStage.h>
#include <daECS/core/event.h>
#include <math/dag_frustum.h>
#include <vecmath/dag_vecMath_common.h>
#include <3d/dag_drvDecl.h>
#include "renderPasses.h"
#include "transformHolder.h"
#include <3d/dag_texStreamingContext.h>

class Occlusion;


struct UpdateStageInfoBeforeRender : public ecs::Event, public TransformHolder
{
  ECS_INSIDE_EVENT_DECL(UpdateStageInfoBeforeRender, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE);
  float dt = 0.f, actDt = 0.f, realDt = 0.f;
  Frustum mainCullingFrustum;
  Frustum froxelFogCullingFrustum; // TODO: maybe separate it
  Occlusion *mainOcclusion;
  float mainInvHkSq;
  Point3 camPos;
  Point3 dirFromSun;
  // -(roundedCamPos+remainderCamPos) - same as -camPos, but more precise, as calculated in doubles!
  vec4f negRoundedCamPos, negRemainderCamPos;
  UpdateStageInfoBeforeRender(float dt_, float act_dt, float real_dt, const Frustum &main_culling, const Frustum &froxel_fog_culling,
    float main_inv_hk_sq, Occlusion *main_occl, const Point3 &cam_pos, vec4f neg_rounded_cam_pos, vec4f neg_remainder_cam_pos,
    const Point3 &dir_from_sun, const TMatrix &view_tm, const TMatrix4 &proj_tm) :
    ECS_EVENT_CONSTRUCTOR(UpdateStageInfoBeforeRender),
    TransformHolder(view_tm, proj_tm),
    dt(dt_),
    actDt(act_dt),
    realDt(real_dt),
    mainCullingFrustum(main_culling),
    froxelFogCullingFrustum(froxel_fog_culling),
    mainOcclusion(main_occl),
    mainInvHkSq(main_inv_hk_sq),
    camPos(cam_pos),
    dirFromSun(dir_from_sun),
    negRoundedCamPos(neg_rounded_cam_pos),
    negRemainderCamPos(neg_remainder_cam_pos)
  {}
};

namespace dynmodel_renderer
{
struct DynModelRenderingState;
}
struct UpdateStageInfoRender : public ecs::Event, public TransformHolder
{
  Frustum cullingFrustum;
  TMatrix viewItm;
  Point3 mainCamPos;
  // -(roundedCamPos+remainderCamPos) - same as camPos, but more precise, as calculated in doubles!
  vec4f negRoundedCamPos, negRemainderCamPos;
  Occlusion *occlusion;
  TexStreamingContext texCtx;
  const dynmodel_renderer::DynModelRenderingState *pState;
  enum RenderPass : uint8_t
  {
    RENDER_COLOR = 1,
    RENDER_DEPTH = 2,
    RENDER_SHADOW = 4,
    RENDER_MAIN = 8,         // for main camera (i.e. RENDER_SHADOW|RENDER_MAIN - csm shadows, use main camera lods)
    RENDER_MOTION_VECS = 16, // for main camera (i.e. RENDER_SHADOW|RENDER_MAIN - csm shadows, use main camera lods)
  };
  uint8_t hints = 0;
  int renderPass = RENDER_UNKNOWN;
  ECS_INSIDE_EVENT_DECL(UpdateStageInfoRender, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE);
  UpdateStageInfoRender(uint32_t hints_, const Frustum &culling, const TMatrix &itm, const TMatrix &view_tm, const TMatrix4 &proj_tm,
    const Point3 &main_cam, vec4f neg_rounded_cam_pos, vec4f neg_remainder_cam_pos, Occlusion *occl, int render_pass = RENDER_UNKNOWN,
    const dynmodel_renderer::DynModelRenderingState *pState_ = NULL, TexStreamingContext tex_context = TexStreamingContext(0)) :
    ECS_EVENT_CONSTRUCTOR(UpdateStageInfoRender),
    TransformHolder(view_tm, proj_tm),
    cullingFrustum(culling),
    occlusion(occl),
    hints(hints_),
    viewItm(itm),
    renderPass(render_pass),
    mainCamPos(main_cam),
    negRoundedCamPos(neg_rounded_cam_pos),
    negRemainderCamPos(neg_remainder_cam_pos),
    pState(pState_),
    texCtx(tex_context)
  {}
};

struct UpdateStageInfoRenderTrans : public ecs::Event, public TransformHolder
{
  TMatrix viewItm;
  Occlusion *occlusion;
  ECS_INSIDE_EVENT_DECL(UpdateStageInfoRenderTrans, ::ecs::EVCAST_BROADCAST | ::ecs::EVFLG_PROFILE);
  TexStreamingContext texCtx;
  UpdateStageInfoRenderTrans(const TMatrix &view_tm, const TMatrix4 &proj_tm, const TMatrix &itm, Occlusion *occl,
    TexStreamingContext tex_context = TexStreamingContext(0)) :
    ECS_EVENT_CONSTRUCTOR(UpdateStageInfoRenderTrans),
    TransformHolder(view_tm, proj_tm),
    viewItm(itm),
    occlusion(occl),
    texCtx(tex_context)
  {}
};


struct UpdateStageInfoRenderDebug : public ecs::UpdateStageInfoRenderDebug
{
  mat44f globtm;
  TMatrix viewItm;
  UpdateStageInfoRenderDebug(mat44f_cref gtm, const TMatrix &itm) : globtm(gtm), viewItm(itm) {}
};
