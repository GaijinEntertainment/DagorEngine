//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "riExtraRenderer.h"

#include <3d/dag_texStreamingContext.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <util/dag_threadPool.h>

#include <EASTL/unique_ptr.h>


struct RiGenVisibility;

struct RenderRiExtraJob final : public cpujobs::IJob
{
  eastl::unique_ptr<rendinst::render::RiExtraRenderer, rendinst::render::RiExtraRendererDelete> riExRenderer;
  RiGenVisibility *vbase = nullptr;
  GlobalVariableStates gvars;
  TexStreamingContext texContext = TexStreamingContext(0);
  int vbExtraCtxId = 0;

  RenderRiExtraJob(int vb_extra_ctx_id);

  void prepare(RiGenVisibility &v, int frame_stblk, bool enable, TexStreamingContext texCtx);
  void start(RiGenVisibility &v, bool wake);
  const char *getJobName(bool &) const override { return "prepare_render_riex_opaque"; }
  void doJob() override;

  void waitVbFill(const RiGenVisibility *v);
  rendinst::render::RiExtraRenderer *wait(const RiGenVisibility *v);
};