// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/cameraInCamera.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/cameraViewVisibilityManager.h>
#include <math/dag_occlusionTest.h>
#include <3d/dag_lockTexture.h>

#include "frameGraphNodes.h"

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h>

constexpr static int hzb_mips_count = OcclusionTest<OCCLUSION_W, OCCLUSION_H>::mip_chain_count;

static void upload_hzb(CameraViewVisibilityMgr &view_jobs_mgr, BaseTexture *dst, BaseTexture *staging)
{
  {
    TIME_PROFILE("wait_visibility_reproject")
    view_jobs_mgr.waitVisibilityReproject();
  }

  const Occlusion *occl = view_jobs_mgr.getOcclusion();
  const auto &occlusionTest = occl->getOcclusionTest();

  if (auto data = lock_texture<float>(staging, 0, TEXLOCK_DISCARD | TEXLOCK_WRITE))
  {
    for (int mip = 0; mip < hzb_mips_count; ++mip)
    {
      const size_t mipW = OCCLUSION_W >> mip;
      const size_t mipH = OCCLUSION_H >> mip;
      const size_t mipXOffset = mip > 0 ? OCCLUSION_W : 0;
      const size_t mipYOffset = mip > 0 ? mipH : 0;

      const float *zbuffer = occlusionTest.getZbuffer(mip);
      for (int y = 0; y < mipH; y++)
        memcpy(&data.at(mipXOffset, y + mipYOffset), &zbuffer[y * mipW], mipW * sizeof(float));
    }
  }

  for (int mip = 0; mip < hzb_mips_count; ++mip)
  {
    const size_t mipW = OCCLUSION_W >> mip;
    const size_t mipH = OCCLUSION_H >> mip;
    const size_t mipXOffset = mip > 0 ? OCCLUSION_W : 0;
    const size_t mipYOffset = mip > 0 ? mipH : 0;
    dst->updateSubRegion(staging, 0, mipXOffset, mipYOffset, 0, mipW, mipH, 1, mip, 0, 0, 0);
  }
}

dafg::NodeHandle makeReprojectedHzbImportNode()
{
  const bool needReprojectedHZB = renderer_has_feature(CAMERA_IN_CAMERA);
  if (!needReprojectedHZB)
    return {};

  return dafg::register_node("upload_reprojected_hzb_to_gpu_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.modifyBlob<OrderingToken>("before_world_render_setup_token");

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    // todo: here we need nonmultiplexed versions of the camera params
    // auto cameraHndl = read_camera_in_camera(registry).handle();

    // FIXME: DX12 complain on FG generated barriers for non RT/UAV texture
    const uint32_t texflags = TEXFMT_R32F | (d3d::get_driver_code().is(d3d::dx12) ? TEXCF_RTARGET : TEXCF_UPDATE_DESTINATION);
    auto hzbHndl =
      registry.createTexture2d("reprojected_hzb", dafg::History::No, {texflags, IPoint2{OCCLUSION_W, OCCLUSION_H}, hzb_mips_count})
        .atStage(dafg::Stage::TRANSFER)
        .useAs(dafg::Usage::COPY)
        .handle();

    auto hzbMipsCountHndl = registry.createBlob<int>("reprojected_hzb_mip_count", dafg::History::No).handle();

    return [hzbHndl, hzbMipsCountHndl](dafg::multiplexing::Index multiplexing_index) {
      hzbMipsCountHndl.ref() = hzb_mips_count;

      auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
      CameraViewVisibilityMgr &jobsMgr = multiplexing_index.subCamera == 0 ? wr->mainCameraVisibilityMgr : wr->camcamVisibilityMgr;
      upload_hzb(jobsMgr, hzbHndl.get(), wr->initAndGetHZBUploadStagingTex());
    };
  });
}
