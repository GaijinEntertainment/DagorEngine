// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daBfg/bfg.h>
#include <render/motionBlurECS.h>
#include <render/world/cameraParams.h>
#include "frameGraphNodes.h"
#include <render/world/frameGraphNodes/motionBlurNode.h>

static MotionBlur::DepthType motion_blur_mode_to_depth_type(MotionBlurMode mode)
{
  switch (mode)
  {
    case MotionBlurMode::RAW_DEPTH: return MotionBlur::DepthType::RAW;
    case MotionBlurMode::LINEAR_DEPTH: return MotionBlur::DepthType::LINEAR;
    default: return MotionBlur::DepthType::RAW;
  }
}

void enable_motion_blur_node(MotionBlurNodePointers pointers)
{
  auto [accumulatePtr, combinePtr, statusPtr] = pointers;
  G_ASSERT_RETURN(accumulatePtr && combinePtr && statusPtr, );
  if (*statusPtr == MotionBlurNodeStatus::ENABLED)
    return;
  *statusPtr = MotionBlurNodeStatus::ENABLED;

  auto motionBlurNs = dabfg::root() / "motion_blur" / "cinematic_motion_blur";
  *accumulatePtr = motionBlurNs.registerNode("motion_blur_accumulate_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    auto sourceHndl =
      registry.readTexture("color_target").atStage(dabfg::Stage::PRE_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHistoryHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");
    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dabfg::Stage::PRE_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    auto motionVectorHndl =
      registry.readTexture("motion_vecs").atStage(dabfg::Stage::PRE_RASTER).useAs(dabfg::Usage::SHADER_RESOURCE).optional().handle();
    auto accumulationHndl = registry
                              .createTexture2d("motion_blur_accumulation_tex", dabfg::History::No,
                                {MotionBlur::getAccumulationFormat() | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                              .atStage(dabfg::Stage::POST_RASTER)
                              .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                              .handle();

    return [sourceHndl, cameraHndl, cameraHistoryHndl, depthHndl, motionVectorHndl, accumulationHndl]() {
      auto [motionBlur, mode] = query_motion_blur();

      if (!motionBlur)
        return;

      if (mode == MotionBlurMode::MOTION_VECTOR)
      {
        if (auto motionGbuf = motionVectorHndl.view())
        {
          motionBlur->accumulateMotionVectorVersion(sourceHndl.view(), motionGbuf, accumulationHndl.view());
          return;
        }
      }

      motionBlur->accumulateDepthVersion(sourceHndl.view(), depthHndl.view(), motion_blur_mode_to_depth_type(mode),
        cameraHndl.ref().jitterGlobtm, cameraHistoryHndl.ref().jitterGlobtm, cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm,
        accumulationHndl.view());
    };
  });
  *combinePtr = motionBlurNs.registerNode("motion_blur_combine_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    auto finalTargetHndl =
      registry.modifyTexture("color_target_done").atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
    auto accumulationHndl =
      registry.readTexture("motion_blur_accumulation_tex").atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
    return [finalTargetHndl, accumulationHndl]() {
      auto [motionBlur, mode] = query_motion_blur();
      motionBlur->combine(finalTargetHndl.view(), accumulationHndl.view());
    };
  });
}

void disable_motion_blur_node(MotionBlurNodePointers pointers)
{
  auto [accumulatePtr, combinePtr, statusPtr] = pointers;
  G_ASSERT_RETURN(accumulatePtr && combinePtr && statusPtr, );
  if (*statusPtr == MotionBlurNodeStatus::DISABLED)
    return;
  *statusPtr = MotionBlurNodeStatus::DISABLED;
  *accumulatePtr = {};
  *combinePtr = {};
}