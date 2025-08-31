// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/daFrameGraph/daFG.h>

#include "frameGraphNodes.h"
#include <frustumCulling/frustumPlanes.h>
#include <render/world/frameGraphHelpers.h>
#include <render/viewVecs.h>
#include <render/subFrameSample.h>
#include <render/set_reprojection.h>
#include <render/enviCover/enviCover.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_rwResource.h>

#define INSIDE_RENDERER
#include <render/world/private_worldRenderer.h>

namespace var
{
static ShaderVariableInfo zn_zfar("zn_zfar");
static ShaderVariableInfo prev_zn_zfar("prev_zn_zfar");
static ShaderVariableInfo envi_cover_is_temporal_aa("envi_cover_is_temporal_aa");
} // namespace var

static bool isAATemporal(AntiAliasingMode aaMode)
{
  return aaMode == AntiAliasingMode::DLSS || aaMode == AntiAliasingMode::FSR2 || aaMode == AntiAliasingMode::PSSR ||
         aaMode == AntiAliasingMode::TAA || aaMode == AntiAliasingMode::TSR || aaMode == AntiAliasingMode::XESS;
}

static inline void create_envi_cover_rendered_blob(dafg::Registry registry)
{
  registry.createBlob<OrderingToken>("envi_cover_rendered", dafg::History::No);
}

static dafg::NodeHandle makeEnviCoverNode(bool NBS)
{
  auto ns = dafg::root() / "opaque" / "statics";
  return ns.registerNode("envi_cover", DAFG_PP_NODE_SRC, [NBS](dafg::Registry registry) {
    auto gbuf0Handle = registry.modify("gbuf_0_done").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf1Handle = registry.modify("gbuf_1_done").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf2Handle = registry.modify("gbuf_2_done").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf3Handle =
      registry.modify("gbuf_3_done").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    create_envi_cover_rendered_blob(registry);

    registry.readTexture("grav_zone_voxel_grid").optional().atStage(dafg::Stage::CS).bindToShaderVar("grav_zone_grid");
    registry.readBlob<d3d::SamplerHandle>("grav_zone_voxel_grid_sampler").optional().bindToShaderVar("grav_zone_grid_samplerstate");

    registry.read("gbuf_depth_done").texture().atStage(dafg::Stage::CS).bindToShaderVar("depth_gbuf");

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    wr.enviCover->initRender(NBS ? EnviCover::EnviCoverUseType::NBS : EnviCover::EnviCoverUseType::STANDALONE_FGNODE);

    registry.read("current_camera")
      .blob<CameraParams>()
      .bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>("jitteredCamPosToUnjitteredHistoryClip");

    return [gbuf0Handle, gbuf1Handle, gbuf2Handle, gbuf3Handle]() {
      TextureInfo info;
      gbuf0Handle.view()->getinfo(info);

      eastl::array<BaseTexture *, EnviCover::ENVI_COVER_RW_TARGETS> gbufTextures = {
        gbuf0Handle.view().getTex2D(), gbuf1Handle.view().getTex2D(), gbuf2Handle.view().getTex2D(), gbuf3Handle.view().getTex2D()};

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      ShaderGlobal::set_int(var::envi_cover_is_temporal_aa, isAATemporal(wr.getAntiAliasingMode()) ? 1 : 0);
      wr.enviCover->render(info.w, info.h, gbufTextures);
    };
  });
}

static dafg::NodeHandle makeCombinedNode(bool NBS)
{
  return dafg::register_node("resolve_motion_and_envi_cover_node", DAFG_PP_NODE_SRC, [NBS](dafg::Registry registry) {
    auto gbuf0Handle = registry.modify("gbuf_0").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf1Handle = registry.modify("gbuf_1").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf2Handle = registry.modify("gbuf_2").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto gbuf3Handle = registry.modify("gbuf_3").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    create_envi_cover_rendered_blob(registry);

    registry.readTexture("grav_zone_voxel_grid").optional().atStage(dafg::Stage::CS).bindToShaderVar("grav_zone_grid");
    registry.readBlob<d3d::SamplerHandle>("grav_zone_voxel_grid_sampler").optional().bindToShaderVar("grav_zone_grid_samplerstate");
    registry.read("gbuf_depth").texture().atStage(dafg::Stage::CS).bindToShaderVar("depth_gbuf");

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    wr.enviCover->initRender(NBS ? EnviCover::EnviCoverUseType::NBS_COMBINED : EnviCover::EnviCoverUseType::COMBINED_FGNODE);

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camcamHandle =
      read_camera_in_camera(registry)
        .bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>("jitteredCamPosToUnjitteredHistoryClip")
        .handle();

    return [camcamHandle, gbuf0Handle, gbuf1Handle, gbuf2Handle, gbuf3Handle](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, camcamHandle.ref()};

      TextureInfo info;
      gbuf0Handle.view()->getinfo(info);

      eastl::array<BaseTexture *, EnviCover::ENVI_COVER_RW_TARGETS> gbufTextures = {
        gbuf0Handle.view().getTex2D(), gbuf1Handle.view().getTex2D(), gbuf2Handle.view().getTex2D(), gbuf3Handle.view().getTex2D()};

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      ShaderGlobal::set_int(var::envi_cover_is_temporal_aa, isAATemporal(wr.getAntiAliasingMode()) ? 1 : 0);
      wr.enviCover->render(info.w, info.h, gbufTextures);
    };
  });
}

static dafg::NodeHandle makeResolveMotionVectorsNode()
{
  return dafg::register_node("resolve_motion_vectors_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto rt = registry.modifyTexture("motion_vecs");
    read_gbuffer(registry);

    if (!renderer_has_feature(CAMERA_IN_CAMERA))
    {
      registry.requestRenderPass().color({rt});
      read_gbuffer_depth(registry);
    }
    else
      use_and_sample_ro_gbuffer_depth(registry).color({rt});

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camcamHandle =
      read_camera_in_camera(registry)
        .bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>("jitteredCamPosToUnjitteredHistoryClip")
        .handle();

    auto cameraHndl = registry.read("current_camera").blob<CameraParams>().handle();
    auto prevCameraHndl = registry.historyFor("current_camera").blob<CameraParams>().handle();
    auto viewVecsHndl = registry.read("view_vectors").blob<ViewVecs>().handle();
    auto prevViewVecsHndl = registry.historyFor("view_vectors").blob<ViewVecs>().handle();

    auto subFrameSampleHndl = registry.read("sub_frame_sample").blob<SubFrameSample>().handle();
    return [camcamHandle, motionVectorsResolve = PostFxRenderer("motion_vectors_resolve"), subFrameSampleHndl, cameraHndl,
             prevCameraHndl, viewVecsHndl, prevViewVecsHndl](const dafg::multiplexing::Index multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, camcamHandle.ref(), camera_in_camera::USE_STENCIL};

      const auto &camera = cameraHndl.ref();
      const auto &prevCamera = prevCameraHndl.ref();
      const auto &viewVec = viewVecsHndl.ref();
      const auto &prevViewVec = prevViewVecsHndl.ref();
      SubFrameSample subFrameSample = subFrameSampleHndl.ref();

      const bool isMainView = multiplexing_index.subCamera == 0;
      if (isMainView && (subFrameSample == SubFrameSample::Single || subFrameSample == SubFrameSample::Last))
      {
        set_reprojection(camera.jitterProjTm, camera.viewRotJitterProjTm, Point2(prevCamera.znear, prevCamera.zfar),
          prevCamera.viewRotJitterProjTm, viewVec.viewVecLT, viewVec.viewVecRT, viewVec.viewVecLB, viewVec.viewVecRB,
          prevViewVec.viewVecLT, prevViewVec.viewVecRT, prevViewVec.viewVecLB, prevViewVec.viewVecRB, camera.cameraWorldPos,
          prevCamera.cameraWorldPos);
      }

      motionVectorsResolve.render();
    };
  });
}


static dafg::NodeHandle makeSetReprojectionNode()
{
  return dafg::register_node("set_reporjection_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeBefore("resolve_motion_and_envi_cover_node");

    auto cameraHndl = registry.read("current_camera")
                        .blob<CameraParams>()
                        .bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>("jitteredCamPosToUnjitteredHistoryClip")
                        .handle();
    auto prevCameraHndl = registry.historyFor("current_camera").blob<CameraParams>().handle();
    auto viewVecsHndl = registry.read("view_vectors").blob<ViewVecs>().handle();
    auto prevViewVecsHndl = registry.historyFor("view_vectors").blob<ViewVecs>().handle();

    auto subFrameSampleHndl = registry.read("sub_frame_sample").blob<SubFrameSample>().handle();
    return [subFrameSampleHndl, cameraHndl, prevCameraHndl, viewVecsHndl, prevViewVecsHndl]() {
      const auto &camera = cameraHndl.ref();
      const auto &prevCamera = prevCameraHndl.ref();
      const auto &viewVec = viewVecsHndl.ref();
      const auto &prevViewVec = prevViewVecsHndl.ref();
      SubFrameSample subFrameSample = subFrameSampleHndl.ref();
      if (subFrameSample == SubFrameSample::Single || subFrameSample == SubFrameSample::Last)
      {
        set_reprojection(camera.jitterProjTm, camera.viewRotJitterProjTm, Point2(prevCamera.znear, prevCamera.zfar),
          prevCamera.viewRotJitterProjTm, viewVec.viewVecLT, viewVec.viewVecRT, viewVec.viewVecLB, viewVec.viewVecRB,
          prevViewVec.viewVecLT, prevViewVec.viewVecRT, prevViewVec.viewVecLB, prevViewVec.viewVecRB, camera.cameraWorldPos,
          prevCamera.cameraWorldPos);
      }

      const Color4 znZfar = ShaderGlobal::get_color4_fast(var::zn_zfar.get_var_id());
      ShaderGlobal::set_color4(var::prev_zn_zfar,
        Color4(znZfar.r, znZfar.g, 1 / znZfar.g, (znZfar.g - znZfar.r) / (znZfar.r * znZfar.g)));
    };
  });
}


static dafg::NodeHandle makeMotionVecRenameNode()
{
  return dafg::register_node("rename_motion_vector_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.renameTexture("gbuf_3", "motion_vecs", dafg::History::No);
    return []() {};
  });
}

eastl::array<dafg::NodeHandle, 3> makeResolveMotionAndEnviCoverNode(bool has_motion_vecs, bool use_envi_cover_nodes, bool use_NBS)
{
  if (has_motion_vecs)
  {
    if (use_envi_cover_nodes)
      return {makeSetReprojectionNode(), makeCombinedNode(use_NBS), makeMotionVecRenameNode()};
    else
      return {makeMotionVecRenameNode(), makeResolveMotionVectorsNode()};
  }
  else if (use_envi_cover_nodes)
  {
    return {dafg::NodeHandle(), dafg::NodeHandle(), makeEnviCoverNode(use_NBS)};
  }
  else
  {
    return {dafg::NodeHandle(), dafg::NodeHandle(), dafg::NodeHandle()};
  }
}