// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_TMatrix4D.h>

#include <ecs/camera/getActiveCameraSetup.h>
#include <render/daFrameGraph/daFG.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>

#include <render/world/cameraInCamera.h>
#include <render/world/cameraParams.h>
#include <render/world/reprojectionTm.h>
#include <render/world/frameGraphNodes/frameGraphNodes.h>

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>

eastl::fixed_vector<dafg::NodeHandle, 2, false> makeCameraInCameraSetupNodes()
{
  eastl::fixed_vector<dafg::NodeHandle, 2, false> nodes;
  nodes.emplace_back(dafg::register_node("lens_camera_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto lensCameraHndl = registry.createBlob<CameraParams>("lens_area_camera", dafg::History::ClearZeroOnFirstFrame).handle();
    auto prevLensCameraHndl = registry.readBlobHistory<CameraParams>("lens_area_camera").handle();
    auto mainCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevMainCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();

    return [lensCameraHndl, prevLensCameraHndl, mainCameraHndl, prevMainCameraHndl]() {
      if (!camera_in_camera::is_lens_render_active())
        return;

      const auto &camera = mainCameraHndl.ref();
      auto *wr = static_cast<WorldRenderer *>(get_world_renderer());

      auto &lensCamera = lensCameraHndl.ref();
      lensCamera = *wr->camcamParams;
      lensCamera.jobsMgr = &wr->camcamVisibilityMgr;
      lensCamera.jitterPersp.ox = camera.jitterPersp.ox;
      lensCamera.jitterPersp.oy = camera.jitterPersp.oy;
      matrix_perspective_add_jitter(lensCamera.jitterProjTm, lensCamera.jitterPersp.ox, lensCamera.jitterPersp.oy);
      lensCamera.jitterGlobtm = TMatrix4(lensCamera.viewTm) * lensCamera.jitterProjTm;
      lensCamera.jitterOffsetUv = camera.jitterOffsetUv;

      ReprojectionTransforms reprojectionTms = calc_reprojection_transforms(prevLensCameraHndl.ref(), lensCameraHndl.ref());
      lensCamera.jitteredCamPosToUnjitteredHistoryClip = reprojectionTms.jitteredCamPosToUnjitteredHistoryClip;

      camera_in_camera::update_transforms(mainCameraHndl.ref(), prevMainCameraHndl.ref(), lensCamera);
    };
  }));

  nodes.emplace_back(dafg::register_node("lens_camera_multiplex_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").handle();
    auto mainCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto outputCameraHndl = registry.createBlob<CameraParams>("camera_in_camera", dafg::History::ClearZeroOnFirstFrame).handle();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);

    return [lensCameraHndl, mainCameraHndl, outputCameraHndl](const dafg::multiplexing::Index &multiplexing_index) {
      CameraParams &outCameraParams = outputCameraHndl.ref();

      if (camera_in_camera::is_lens_render_active())
        outCameraParams = multiplexing_index.subCamera == 0 ? mainCameraHndl.ref() : lensCameraHndl.ref();
      else
        outCameraParams = mainCameraHndl.ref();
    };
  }));

  return nodes;
}