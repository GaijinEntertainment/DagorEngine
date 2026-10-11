// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/cameraInCamera/cameraInCameraNodes.h>

#include <math/dag_TMatrix4.h>
#include <render/cameraInCamera/cameraInCamera.h>
#include <render/cameraParams.h>
#include <render/daFrameGraph/daFG.h>
#include <render/reprojectionTm.h>
#include <render/viewVecs.h>

namespace camera_in_camera
{

dafg::NodeHandle make_view_camera_provider_node(const char *view_ns, const char *src_camera_blob)
{
  return (dafg::root() / view_ns).registerNode("view_camera_provider", DAFG_PP_NODE_SRC, [src_camera_blob](dafg::Registry registry) {
    auto srcHndl = registry.root().readBlob<CameraParams>(src_camera_blob).handle();
    auto dstHndl = registry.createBlob<CameraParams>("current_camera").withHistory().handle();
    return [srcHndl, dstHndl]() { dstHndl.ref() = srcHndl.ref(); };
  });
}

eastl::fixed_vector<dafg::NodeHandle, 2, false> make_camera_nodes()
{
  eastl::fixed_vector<dafg::NodeHandle, 2, false> nodes;
  nodes.emplace_back(dafg::register_node("lens_camera_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto mainCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto srcCameraHndl = registry.readBlob<CameraParams>(LENS_AREA_CAMERA_SOURCE_BLOB).handle();
    auto lensCameraHndl = registry.createBlob<CameraParams>("lens_area_camera").withHistory().handle();
    auto prevLensCameraHndl = registry.readBlobHistory<CameraParams>("lens_area_camera").handle();
    auto cockpitCameraHndl = registry.readBlob<CameraParams>("current_cockpit_camera").handle();
    auto prevCockpitCameraHndl = registry.readBlobHistory<CameraParams>("current_cockpit_camera").handle();

    return [mainCameraHndl, srcCameraHndl, lensCameraHndl, prevLensCameraHndl, cockpitCameraHndl, prevCockpitCameraHndl]() {
      if (!camera_in_camera::is_lens_render_active())
        return;

      const auto &camera = cockpitCameraHndl.ref();

      auto &lensCamera = lensCameraHndl.ref();
      lensCamera = srcCameraHndl.ref();
      lensCamera.jitterPersp.ox = camera.jitterPersp.ox;
      lensCamera.jitterPersp.oy = camera.jitterPersp.oy;
      const Point2 jitterNdc(camera.jitterOffsetUv.x * 2.0f, -camera.jitterOffsetUv.y * 2.0f);
      matrix_perspective_add_jitter(lensCamera.jitterProjTm, jitterNdc.x, jitterNdc.y);
      lensCamera.jitterGlobtm = TMatrix4(lensCamera.viewTm) * lensCamera.jitterProjTm;
      lensCamera.viewRotJitterProjTm = lensCamera.viewRotTm * lensCamera.jitterProjTm;
      lensCamera.jitterOffsetUv = camera.jitterOffsetUv;
      lensCamera.jitterOffset = camera.jitterOffset;

      ReprojectionTransforms reprojectionTms = calc_reprojection_transforms(prevLensCameraHndl.ref(), lensCameraHndl.ref());
      lensCamera.jitteredCamPosToUnjitteredHistoryClip = reprojectionTms.jitteredCamPosToUnjitteredHistoryClip;

      lensCamera.viewVecs = calc_view_vecs(lensCamera.viewTm, lensCamera.jitterProjTm);

      camera_in_camera::update_transforms(mainCameraHndl.ref(), cockpitCameraHndl.ref(), prevCockpitCameraHndl.ref(), lensCamera);
    };
  }));

  nodes.emplace_back(dafg::register_node("lens_camera_multiplex_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").handle();
    auto mainCameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    auto outputCameraHndl = registry.createBlob<CameraParams>("camera_in_camera").withHistory().handle();

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

} // namespace camera_in_camera
