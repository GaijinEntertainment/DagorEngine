// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <render/daFrameGraph/daFG.h>
#include <util/dag_console.h>

#include "frameGraphNodes.h"
#include <frustumCulling/frustumPlanes.h>
#include <render/world/frameGraphHelpers.h>
#include <render/viewVecs.h>
#include <render/renderEvent.h>
#include <render/subFrameSample.h>
#include <render/set_reprojection.h>
#include <render/enviCover/enviCover.h>
#include <drv/3d/dag_decl.h>
#include <drv/3d/dag_rwResource.h>
#include <render/world/wrDispatcher.h>

#define INSIDE_RENDERER
#include <render/world/private_worldRenderer.h>


extern ConVarT<bool, false> envi_cover_use_NBS;
extern ConVarT<bool, false> envi_cover_disable_fg_nodes;

namespace var
{
static ShaderVariableInfo zn_zfar("zn_zfar", true);
static ShaderVariableInfo prev_zn_zfar("prev_zn_zfar", true);
static ShaderVariableInfo envi_cover_is_temporal_aa("envi_cover_is_temporal_aa", true);
static ShaderVariableInfo envi_cover_use_in_resolve("envi_cover_use_in_resolve", true);
} // namespace var

static bool isAATemporal(AntiAliasingMode aaMode)
{
  return aaMode == AntiAliasingMode::DLSS || aaMode == AntiAliasingMode::FSR || aaMode == AntiAliasingMode::TSR ||
         aaMode == AntiAliasingMode::XESS
#if _TARGET_C2

#endif
    ;
}

static inline dafg::NameSpace getNameSpaceStaticsOrMixing()
{
  return renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS) ? dafg::root() / "opaque" / "mixing"
                                                                          : dafg::root() / "opaque" / "statics";
}

static inline dafg::NameSpace getNameSpaceRootOrMixing()
{
  return renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS) ? dafg::root() / "opaque" / "mixing" : dafg::root();
}

static inline EnviCover::EnviCoverUseType getEnviCoverRenderType(bool has_combined_motion_vecs, bool is_NBS, bool is_normals_packed)
{
  if (is_NBS)
  {
    if (has_combined_motion_vecs && is_normals_packed)
      return EnviCover::EnviCoverUseType::NBS_COMBINED_PACKED_NORMS;
    else if (has_combined_motion_vecs && !is_normals_packed)
      return EnviCover::EnviCoverUseType::NBS_COMBINED;
    else if (!has_combined_motion_vecs && is_normals_packed)
      return EnviCover::EnviCoverUseType::NBS_PACKED_NORMS;
    else
      return EnviCover::EnviCoverUseType::NBS;
  }
  else
  {
    return has_combined_motion_vecs ? EnviCover::EnviCoverUseType::COMBINED_FGNODE : EnviCover::EnviCoverUseType::STANDALONE_FGNODE;
  }
}

static dafg::NodeHandle makeEnviCoverNode(bool NBS)
{
  return getNameSpaceStaticsOrMixing().registerNode("envi_cover", DAFG_PP_NODE_SRC, [NBS](dafg::Registry registry) {
    bool isNormalsPacked = renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS);
    auto gbuf0Handle = registry.modify(isNormalsPacked ? "gbuf_0" : "gbuf_0_done")
                         .texture()
                         .atStage(dafg::Stage::CS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .handle();
    auto gbuf1Handle = registry.modify(isNormalsPacked ? "unpacked_normals" : "gbuf_1_done")
                         .texture()
                         .atStage(dafg::Stage::CS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .handle();
    auto gbuf2Handle = registry.modify(isNormalsPacked ? "gbuf_2" : "gbuf_2_done")
                         .texture()
                         .atStage(dafg::Stage::CS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .handle();
    auto gbuf3Handle = registry.modify(isNormalsPacked ? "gbuf_3" : "gbuf_3_done")
                         .texture()
                         .atStage(dafg::Stage::CS)
                         .useAs(dafg::Usage::SHADER_RESOURCE)
                         .optional()
                         .handle();
    auto gbufExtraHandle =
      registry.modify("gbuf_1_ro").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    registry.modifyBlob<OrderingToken>("envi_cover_rendered");

    if (isNormalsPacked)
    {
      registry.readBlob<OrderingToken>("dynamic_decals_rendered");
      registry.readBlob<OrderingToken>("dagdp_decals_rendered").optional();
    }

    registry.readTexture("grav_zone_voxel_grid").optional().atStage(dafg::Stage::CS).bindToShaderVar("grav_zone_grid");
    registry.readBlob<d3d::SamplerHandle>("grav_zone_voxel_grid_sampler").optional().bindToShaderVar("grav_zone_grid_samplerstate");

    registry.readTexture("clouds_rain_map_tex").optional().atStage(dafg::Stage::CS).bindToShaderVar("clouds_rain_map_tex");

    registry.read(isNormalsPacked ? "gbuf_depth" : "gbuf_depth_done").texture().atStage(dafg::Stage::CS).bindToShaderVar("depth_gbuf");

    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    wr.enviCover->initRender(getEnviCoverRenderType(false, NBS, isNormalsPacked));

    registry.read("current_camera")
      .blob<CameraParams>()
      .bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>("jitteredCamPosToUnjitteredHistoryClip");

    return [gbuf0Handle, gbuf1Handle, gbuf2Handle, gbuf3Handle, gbufExtraHandle]() {
      TextureInfo info;
      gbuf0Handle.view()->getinfo(info);

      eastl::array<BaseTexture *, EnviCover::ENVI_COVER_MAX_RW_TARGETS> gbufTextures = {gbuf0Handle.view().getTex2D(),
        gbuf1Handle.view().getTex2D(), gbuf2Handle.view().getTex2D(), gbuf3Handle.view().getTex2D(),
        gbufExtraHandle.view().getTex2D()};

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

      ShaderGlobal::set_int(var::envi_cover_is_temporal_aa, isAATemporal(wr.getAntiAliasingMode()) ? 1 : 0);
      wr.enviCover->render(info.w, info.h, eastl::move(gbufTextures));
    };
  });
}

static dafg::NodeHandle makeCombinedNode(bool NBS)
{
  return getNameSpaceRootOrMixing().registerNode("resolve_motion_and_envi_cover_node", DAFG_PP_NODE_SRC,
    [NBS](dafg::Registry registry) {
      bool isNormalsPacked = renderer_has_feature(FeatureRenderFlags::GBUFFER_PACKED_NORMALS);
      auto gbuf1Handle =
        isNormalsPacked
          ? registry.modify("unpacked_normals").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle()
          : registry.modify("gbuf_1").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

      auto gbuf0Handle = registry.modify("gbuf_0").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto gbuf2Handle = registry.modify("gbuf_2").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto gbuf3Handle = registry.modify("gbuf_3").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      auto gbufExtraHandle =
        registry.modify("gbuf_1_ro").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

      registry.modifyBlob<OrderingToken>("envi_cover_rendered");

      if (isNormalsPacked)
      {
        registry.readBlob<OrderingToken>("dynamic_decals_rendered");
        registry.readBlob<OrderingToken>("dagdp_decals_rendered").optional();
      }

      registry.readTexture("grav_zone_voxel_grid").optional().atStage(dafg::Stage::CS).bindToShaderVar("grav_zone_grid");
      registry.readBlob<d3d::SamplerHandle>("grav_zone_voxel_grid_sampler").optional().bindToShaderVar("grav_zone_grid_samplerstate");
      registry.read("gbuf_depth").texture().atStage(dafg::Stage::CS).bindToShaderVar("depth_gbuf");

      registry.readTexture("clouds_rain_map_tex").optional().atStage(dafg::Stage::CS).bindToShaderVar("clouds_rain_map_tex");

      auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
      wr.enviCover->initRender(getEnviCoverRenderType(true, NBS, isNormalsPacked));

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry).bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>(
        "jitteredCamPosToUnjitteredHistoryClip");
      auto camcamHandle = CameraViewShvars{camera}.bindViewVecs().toHandle();

      return [camcamHandle, gbuf0Handle, gbuf1Handle, gbuf2Handle, gbuf3Handle, gbufExtraHandle](
               const dafg::multiplexing::Index &multiplexing_index) {
        camera_in_camera::ApplyPostfxState camcam{multiplexing_index, camcamHandle.ref()};

        TextureInfo info;
        gbuf0Handle.view()->getinfo(info);

        eastl::array<BaseTexture *, EnviCover::ENVI_COVER_MAX_RW_TARGETS> gbufTextures = {gbuf0Handle.view().getTex2D(),
          gbuf1Handle.view().getTex2D(), gbuf2Handle.view().getTex2D(), gbuf3Handle.view().getTex2D(),
          gbufExtraHandle.view().getTex2D()};

        auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());

        ShaderGlobal::set_int(var::envi_cover_is_temporal_aa, isAATemporal(wr.getAntiAliasingMode()) ? 1 : 0);
        wr.enviCover->render(info.w, info.h, eastl::move(gbufTextures));
      };
    });
}

static dafg::NodeHandle makeResolveMotionVectorsNode()
{
  return getNameSpaceRootOrMixing().registerNode("resolve_motion_vectors_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto rt = registry.modifyTexture("motion_vecs");
    read_gbuffer(registry, dafg::Stage::PS, readgbuffer::MATERIAL);

    if (!renderer_has_feature(CAMERA_IN_CAMERA))
    {
      registry.requestRenderPass().color({rt});
      read_gbuffer_depth(registry);
    }
    else
      use_and_sample_ro_gbuffer_depth(registry).color({rt});

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camcamBlob = read_camera_in_camera(registry).bindToShaderVar<&CameraParams::jitteredCamPosToUnjitteredHistoryClip>(
      "jitteredCamPosToUnjitteredHistoryClip");
    auto camcamHandle = CameraViewShvars{camcamBlob}.bindViewVecs().toHandle();

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
  return getNameSpaceRootOrMixing().registerNode("set_reporjection_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
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

      const Color4 znZfar = ShaderGlobal::get_float4(var::zn_zfar.get_var_id());
      ShaderGlobal::set_float4(var::prev_zn_zfar,
        Color4(znZfar.r, znZfar.g, 1 / znZfar.g, (znZfar.g - znZfar.r) / (znZfar.r * znZfar.g)));
    };
  });
}


static dafg::NodeHandle makeMotionVecRenameNode()
{
  return getNameSpaceRootOrMixing().registerNode("rename_motion_vector_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.renameTexture("gbuf_3", "motion_vecs");
    return []() {};
  });
}

static dafg::NodeHandle makeEnviCoverTokenProvider(bool has_motion_vecs)
{
  auto ns = has_motion_vecs ? getNameSpaceRootOrMixing() : getNameSpaceStaticsOrMixing();
  return ns.registerNode("envi_cover_token_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createBlob<OrderingToken>("envi_cover_rendered");
    return []() {};
  });
}

eastl::array<dafg::NodeHandle, 4> makeResolveMotionAndEnviCoverNode(bool has_motion_vecs, bool use_envi_cover_nodes, bool use_NBS)
{
  if (has_motion_vecs)
  {
    if (use_envi_cover_nodes)
      return {
        makeSetReprojectionNode(), makeEnviCoverTokenProvider(has_motion_vecs), makeCombinedNode(use_NBS), makeMotionVecRenameNode()};
    else
      return {makeMotionVecRenameNode(), makeResolveMotionVectorsNode(), dafg::NodeHandle(), dafg::NodeHandle()};
  }
  else if (use_envi_cover_nodes)
  {
    return {dafg::NodeHandle(), dafg::NodeHandle(), makeEnviCoverTokenProvider(has_motion_vecs), makeEnviCoverNode(use_NBS)};
  }
  else
  {
    return {dafg::NodeHandle(), dafg::NodeHandle(), dafg::NodeHandle(), dafg::NodeHandle()};
  }
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraNodeConstruction)
static void create_envi_cover_and_mvec_nodes_es(const OnCameraNodeConstruction &evt)
{
  EnviCover *enviCover = WRDispatcher::getEnviCover();
  const bool isEnviCover = WRDispatcher::isEnviCover();
  const bool isEnviCoverCompatible = WRDispatcher::isEnviCoverCompatible();

  bool isValidNBS = enviCover->getValidNBS() && envi_cover_use_NBS.get();

  if (!isValidNBS && envi_cover_use_NBS.get())
  {
    console::print_d("Tried to turn on envi_cover NBS, but there is no root_graph defined in the .blk!");
    envi_cover_use_NBS.set(false);
  }

  isValidNBS = isValidNBS && isEnviCover && isEnviCoverCompatible;
  bool isValidNode = !envi_cover_disable_fg_nodes.get() && isEnviCover && isEnviCoverCompatible;

  // Fallback resolve version:
  ShaderGlobal::set_int(var::envi_cover_use_in_resolve, (!isValidNBS && !isValidNode && isEnviCover) ? 1 : 0);
  for (auto &n : makeResolveMotionAndEnviCoverNode(evt.hasMotionVectors, isValidNode, isValidNBS))
    evt.nodes->push_back(eastl::move(n));
}