// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "animCharRenderJob.h"

#include <daECS/core/entityManager.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/render/updateStageRender.h>

#include <render/world/dynModelRenderer.h>


void AnimcharRenderMainJob::start(AnimcharRenderMainJobCtx *ctx,
  const AnimcharRenderAsyncFilter flt_,
  const char *state_name,
  const uint32_t hints_,
  const Occlusion *occlusion_,
  const Frustum *fg_cam_blob_frustum,
  const TexStreamingContext tex_ctx)
{
  mainCtx = ctx;
  flt = flt_;
  hints = hints_;
  occlusion = occlusion_;
  frustum = fg_cam_blob_frustum;
  dstate = dynmodel_renderer::create_state(state_name);
  texCtx = tex_ctx;

  threadpool::add(this, threadpool::PRIO_HIGH, ctx->lastJobTPQPos, threadpool::AddFlags::None);
}

void AnimcharRenderMainJob::doJob()
{
  if (eastl::to_underlying(flt) == 0) // First one wake others up
    threadpool::wake_up_all();

  G_FAST_ASSERT(dstate && frustum);
  const animchar_visbits_t add_vis_bits = VISFLG_MAIN_VISIBLE | VISFLG_MAIN_CAMERA_RENDERED;
  const animchar_visbits_t check_bits = VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_MAIN_VISIBLE;
  const uint8_t filterMask = UpdateStageInfoRender::RENDER_MAIN;
  g_entity_mgr->broadcastEventImmediate(
    AnimcharRenderAsyncEvent(*dstate, &mainCtx->globalVarsState, occlusion, *frustum, add_vis_bits, check_bits, filterMask,
      /*needPreviousMatrices*/ (hints & UpdateStageInfoRender::RENDER_MOTION_VECS) != 0, flt, texCtx));
  // To consider: pre-sort partially here to make final sort faster?
  if (interlocked_decrement(mainCtx->jobsLeft) == 0) // Last one finalizes to 0th
  {
    for (int n = 1; n < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; n++)
      mainCtx->jobs[0].dstate->addStateFrom(eastl::move(*mainCtx->jobs[n].dstate));
    mainCtx->jobs[0].dstate->prepareForRender();
  }
}
