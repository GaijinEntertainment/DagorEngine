// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_renderPass.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/unique_ptr.h>
#include <shaders/dag_postFxRenderer.h>

struct MobileDeferredResources
{
  d3d::RenderPass *opaque = nullptr;
  eastl::unique_ptr<PostFxRenderer> deferredResolve;
  bool vertexDensityInitStatus = false;

  MobileDeferredResources() = default;
  MobileDeferredResources(int rt_fmt, int depth_fmt, bool simplified);
  ~MobileDeferredResources();
  const MobileDeferredResources &operator=(const MobileDeferredResources &&rvl) = delete;
  MobileDeferredResources &operator=(MobileDeferredResources &&rvl);
  void destroy();

  void initRenderPasses(int rt_fmt, int depth_fmt, bool simplified);

  constexpr static int iEX = RenderPassExtraIndexes::RP_SUBPASS_EXTERNAL_END;
  constexpr static int iDS = RenderPassExtraIndexes::RP_SLOT_DEPTH_STENCIL;

  constexpr static ResourceBarrier rwMrt = RB_STAGE_PIXEL | RB_RW_RENDER_TARGET;
  constexpr static ResourceBarrier inputAtt = RB_STAGE_PIXEL | RB_RO_SRV;
  constexpr static ResourceBarrier rwDepth = RB_STAGE_PIXEL | RB_RW_DEPTH_STENCIL_TARGET;
  constexpr static ResourceBarrier roDepth = RB_STAGE_PIXEL | RB_RO_CONSTANT_DEPTH_STENCIL_TARGET;

  enum
  {
    OPAQUE_SP,
    DECALS_SP,
    RESOLVE_SP,
    PANORAMA_APPLY_SP
  };

  enum
  {
    FULL_MAT_IGBUF0,
    FULL_MAT_IGBUF1,
    FULL_MAT_IGBUF2,
    FULL_MAT_IDEPTH,
    FULL_MAT_IRESOLVE,
    FULL_MAT_MAIN_RT_COUNT
  };

  enum
  {
    SIMPLE_MAT_IGBUF0,
    SIMPLE_MAT_IGBUF1,
    SIMPLE_MAT_IDEPTH,
    SIMPLE_MAT_IRESOLVE,
    SIMPLE_MAT_MAIN_RT_COUNT
  };

  static eastl::vector<RenderPassBind> constructOpaqueBinds(
    bool simplified, int32_t opaqueSp, int32_t decalsSp, int32_t resolveSp, int32_t panoramaApplySp);
  static eastl::fixed_vector<RenderPassTargetDesc, FULL_MAT_MAIN_RT_COUNT + 1> getTargetDescriptions(
    uint32_t rtFmt, uint32_t depthFmt, bool simplified);

private:
  void initShaders();
};