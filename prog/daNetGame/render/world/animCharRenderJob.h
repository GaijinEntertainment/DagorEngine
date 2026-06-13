// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <shaders/dag_shaderVarsUtils.h>
#include <util/dag_threadPool.h>
#include <math/dag_TMatrix4.h>

#include <render/renderEvent.h>


struct AnimcharRenderMainJobCtx
{
  int jobsLeft = 0;
  GlobalVariableStates globalVarsState;
  struct AnimcharRenderMainJob *jobs = nullptr;
  uint32_t lastJobTPQPos = 0;
  TMatrix4 viewTm = TMatrix4::IDENT;
  TMatrix4 projTm = TMatrix4::IDENT;
  TMatrix4 prevViewTm = TMatrix4::IDENT;
  TMatrix4 prevProjTm = TMatrix4::IDENT;
};

struct AnimcharRenderMainJob final : public cpujobs::IJob
{
  AnimcharRenderMainJobCtx *mainCtx = nullptr;
  AnimcharRenderAsyncFilter flt;
  uint32_t hints;
  const Occlusion *occlusion;
  const Frustum *frustum; // Note: points to `current_camera` blob within dafg
  dynrend::ContextId dynCtx = dynrend::ContextId::INVALID;
  TexStreamingContext texCtx = TexStreamingContext(0);

  void start(AnimcharRenderMainJobCtx *ctx,
    const AnimcharRenderAsyncFilter flt_,
    const char *state_name,
    const uint32_t hints_,
    const Occlusion *occlusion_,
    const Frustum *fg_cam_blob_frustum,
    const TexStreamingContext tex_ctx);
  const char *getJobName(bool &) const override { return "AnimcharRenderMainJob"; }
  void doJob() override;
};