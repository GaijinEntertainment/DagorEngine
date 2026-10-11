//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include "riExtraRenderer.h"

#include <3d/dag_texStreamingContext.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <util/dag_threadPool.h>

#include <generic/dag_functionRef.h>

#include <EASTL/array.h>
#include <EASTL/unique_ptr.h>

#include <rendInst/constants.h>
#include <rendInst/renderPass.h>


struct RiGenVisibility;

namespace rendinst::render
{

class RiExtraRendererBuilder
{
public:
  RiExtraRendererBuilder(int vb_extra_ctx_id, RenderPass pass, OptimizeDepthPass optimize_depth_pass = OptimizeDepthPass::No);
  ~RiExtraRendererBuilder();
  RiExtraRendererBuilder(const RiExtraRendererBuilder &) = delete;
  RiExtraRendererBuilder &operator=(const RiExtraRendererBuilder &) = delete;

  RiExtraRenderer *buildNow(RiGenVisibility &v, int frame_stblk, int scene_stblk, TexStreamingContext tex_ctx,
    RiExtraRenderingSubset subset = RiExtraRenderingSubset::All);


  void capture(int frame_stblk, int scene_stblk, TexStreamingContext tex_ctx);
  bool build(RiGenVisibility &v, RiExtraRenderingSubset subset = RiExtraRenderingSubset::All);
  // set_pass_bindings(partitioned) sets the shader var bindings of the node drawing that range (the two nodes bind different vars);
  // the last call stays set, the caller restores. Once per capture: the caller runs once per multiplexing iteration.
  bool captureTransparent(dag::FunctionRef<void(bool partitioned) const> set_pass_bindings);
  void buildTransparent(const RiGenVisibility &v); // needs the filled vb: the sorted entries get their vbOffset there
  void releaseCapture();

  RiExtraRenderer *getRenderer() const { return riExRenderer.get(); }

  RiExtraRenderer *getTransparentRenderer(bool partitioned) const
  {
    return isTransparentBuilt ? transparentRenderers[partitioned ? 0 : 1].get() : nullptr;
  }
  int getVbExtraCtxId() const { return vbExtraCtxId; }

  void resetVbCtx();

private:
  void snapshotIntervals(GlobalVariableStates &dst);

  eastl::unique_ptr<RiExtraRenderer, RiExtraRendererDelete> riExRenderer;
  eastl::array<eastl::unique_ptr<RiExtraRenderer, RiExtraRendererDelete>, 2> transparentRenderers;
  GlobalVariableStates gvars;
  eastl::array<GlobalVariableStates, 2> transparentGvars; // [0] the partitioned range, [1] the rest
  bool isTransparentCaptured = false, isTransparentBuilt = false;
  TexStreamingContext texContext = TexStreamingContext(0);
  int vbExtraCtxId = 0;
  RenderPass renderPass = RenderPass::Normal;
  OptimizeDepthPass optimizeDepthPass = OptimizeDepthPass::No;
};

} // namespace rendinst::render

struct RenderRiExtraJob final : public cpujobs::IJob
{
  rendinst::render::RiExtraRendererBuilder builder;
  RiGenVisibility *vbase = nullptr;
  rendinst::RiExtraRenderingSubset renderingSubset = rendinst::RiExtraRenderingSubset::All;

  struct TransparentJob final : public cpujobs::IJob
  {
    RenderRiExtraJob *owner = nullptr;
    const char *getJobName(bool &) const override { return DAPROFILER_STRING("RenderRiExtraTransparentJob"); }
    void doJob() override;
  } transparentJob;

  RenderRiExtraJob(int vb_extra_ctx_id, rendinst::RenderPass pass = rendinst::RenderPass::Normal,
    rendinst::OptimizeDepthPass optimize_depth_pass = rendinst::OptimizeDepthPass::No);
  ~RenderRiExtraJob() { waitAllJobs(); }

  void prepare(RiGenVisibility &v, int frame_stblk, bool enable, TexStreamingContext texCtx);
  void prepare(RiGenVisibility &v, int frame_stblk, int scene_stblk, bool enable, TexStreamingContext texCtx);
  void start(RiGenVisibility &v, bool wake);
  void startTransparent(dag::FunctionRef<void(bool partitioned) const> set_pass_bindings);
  const char *getJobName(bool &) const override { return DAPROFILER_STRING("RenderRiExtraJob"); }
  void doJob() override;

  void waitVbFill(const RiGenVisibility *v);
  rendinst::render::RiExtraRenderer *wait(const RiGenVisibility *v);
  rendinst::render::RiExtraRenderer *waitTransparent(bool partitioned);
  void waitAllJobs();
  void resetRiExtraCtx();
};