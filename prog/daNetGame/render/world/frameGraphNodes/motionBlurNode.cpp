// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>
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

  auto motionBlurNs = dafg::root() / "motion_blur" / "cinematic_motion_blur";
  *accumulatePtr = motionBlurNs.registerNode("motion_blur_accumulate_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto sourceHndl =
      registry.readTexture("color_target").atStage(dafg::Stage::PRE_RASTER).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto cameraHistoryHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");
    auto depthHndl =
      registry.readTexture("depth_for_transparency").atStage(dafg::Stage::PRE_RASTER).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto motionVectorHndl =
      registry.readTexture("motion_vecs").atStage(dafg::Stage::PRE_RASTER).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    auto accumulationHndl = registry
                              .createTexture2d("motion_blur_accumulation_tex", dafg::History::No,
                                {MotionBlur::getAccumulationFormat() | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                              .atStage(dafg::Stage::POST_RASTER)
                              .useAs(dafg::Usage::COLOR_ATTACHMENT)
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
  *combinePtr = motionBlurNs.registerNode("motion_blur_combine_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.requestState().setFrameBlock("global_frame");
    auto finalTargetHndl =
      registry.modifyTexture("color_target_done").atStage(dafg::Stage::POST_RASTER).useAs(dafg::Usage::COLOR_ATTACHMENT).handle();
    auto accumulationHndl =
      registry.readTexture("motion_blur_accumulation_tex").atStage(dafg::Stage::PS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
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