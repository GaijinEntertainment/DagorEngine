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

#include <rendInst/constants.h>
#include <rendInst/renderPass.h>


struct RiGenVisibility;

struct RenderRiExtraJob final : public cpujobs::IJob
{
  eastl::unique_ptr<rendinst::render::RiExtraRenderer, rendinst::render::RiExtraRendererDelete> riExRenderer;
  RiGenVisibility *vbase = nullptr;
  GlobalVariableStates gvars;
  TexStreamingContext texContext = TexStreamingContext(0);
  int vbExtraCtxId = 0;
  rendinst::RenderPass renderPass = rendinst::RenderPass::Normal;
  rendinst::RiExtraRenderingSubset renderingSubset = rendinst::RiExtraRenderingSubset::All;

  RenderRiExtraJob(int vb_extra_ctx_id, rendinst::RenderPass pass = rendinst::RenderPass::Normal);

  void prepare(RiGenVisibility &v, int frame_stblk, bool enable, TexStreamingContext texCtx);
  void prepare(RiGenVisibility &v, int frame_stblk, int scene_stblk, bool enable, TexStreamingContext texCtx);
  void start(RiGenVisibility &v, bool wake);
  const char *getJobName(bool &) const override { return "RenderRiExtraJob"; }
  void doJob() override;

  void waitVbFill(const RiGenVisibility *v);
  rendinst::render::RiExtraRenderer *wait(const RiGenVisibility *v);
  void resetRiExtraCtx();
};