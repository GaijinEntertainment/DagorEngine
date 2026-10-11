// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <EASTL/shared_ptr.h>

#include <render/daFrameGraph/daFG.h>

#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>

#include <ambientOcclusion_api.h>
#include <render/renderEvent.h>

#include <render/cameraParams.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/global_vars.h>
#include <render/world/dafgCameraRegistrator.h>

ECS_TAG(render)
ECS_ON_EVENT(ResetAoNodes)
static void reset_ao_camera_nodes_es(const ResetAoNodes &evt,
  int &dng_ao_camera_nodes__w,
  int &dng_ao_camera_nodes__h,
  uint32_t &dng_ao_camera_nodes__creation_flags,
  bool &dng_ao_camera_nodes__useGTAO,
  const ecs::string &dafg_camera_registrator__name)
{
  dng_ao_camera_nodes__w = evt.aoW;
  dng_ao_camera_nodes__h = evt.aoH;
  dng_ao_camera_nodes__creation_flags = evt.creationFlags;
  dng_ao_camera_nodes__useGTAO = evt.useGtao;

  ShaderGlobal::set_int(use_contact_shadowsVarId,
    dng_ao_camera_nodes__creation_flags & (SSAO_USE_CONTACT_SHADOWS | SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED) ? 1 : 0);

  recreate_camera_registrator_nodes(dafg_camera_registrator__name);
}

static const ambient_occlusion::ResourceNames ao_resource_names{
  .closeDepthTex = "close_depth",
  .closeDepthSampler = "close_depth_sampler",
  .motionVectorsTex = "downsampled_motion_vectors_tex",
  .motionVectorsSampler = "downsampled_motion_vectors_tex_sampler",
  .motionVectorsHistoryShaderVar = "prev_downsampled_motion_vectors_tex",
  .normalsTex = "downsampled_normals",
  .normalsSampler = "downsampled_normals_sampler",
  .fomShadowsCosTex = "fom_shadows_cos",
  .fomShadowsSinTex = "fom_shadows_sin",
  .fomShadowsSampler = "fom_shadows_sampler",
  .materialGbufferTex = "gbuf_2",
  .materialGbufferSampler = "gbuf_sampler",
  .subFrameSampleBlob = "sub_frame_sample",
};

struct AoRenderContext
{
  camera_in_camera::ApplyPostfxState camcam;
  AoRenderContext(const CameraParams &camera, const CameraParams &prev_camera, bool is_main_view) :
    camcam{is_main_view, camera, prev_camera}
  {}
};

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
static void create_gtao_camera_main_view_nodes_es(const OnCameraMainViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (!dng_ao_camera_nodes__useGTAO)
    return;

  const auto target = ambient_occlusion::make_gtao_target_desc(dng_ao_camera_nodes__creation_flags);

  evt.nodes->push_back(
    ambient_occlusion::make_gtao_prepare_node(dafg::root(), {.target = target, .creationFlags = dng_ao_camera_nodes__creation_flags}));

  evt.nodes->push_back(ambient_occlusion::make_gtao_filter_node(dafg::root(), {.target = target, .names = ao_resource_names}));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
static void create_gtao_camera_view_nodes_es(const OnCameraPerViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (!dng_ao_camera_nodes__useGTAO)
    return;

  const auto target = ambient_occlusion::make_gtao_target_desc(dng_ao_camera_nodes__creation_flags);
  const dafg::NameSpace viewNs = dafg::root() / evt.viewNsName;

  evt.nodes->push_back(ambient_occlusion::make_gtao_render_node<OrderingToken, AoRenderContext>(viewNs,
    {
      .target = target,
      .names = ao_resource_names,
      .isMainView = evt.isMainView,
      // volfog shadow is integrated into the SSAO/GTAO pass as an additional shadow
      .waitTokenNames = {"after_prepare_lights_node_token", "after_esm_ao_node_token", "volfog_shadow_token"},
      .doneTokenName = "after_gtao_node_token",
    }));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraMainViewNodeConstruction)
static void create_ssao_camera_main_view_nodes_es(const OnCameraMainViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (dng_ao_camera_nodes__useGTAO)
    return;

  const auto target = ambient_occlusion::make_ssao_target_desc(dng_ao_camera_nodes__creation_flags);

  evt.nodes->push_back(
    ambient_occlusion::make_ssao_prepare_node(dafg::root(), {.target = target, .creationFlags = dng_ao_camera_nodes__creation_flags}));

  evt.nodes->push_back(ambient_occlusion::make_ssao_filter_node(dafg::root(), {.target = target, .names = ao_resource_names}));
}

ECS_TAG(render)
ECS_ON_EVENT(OnCameraPerViewNodeConstruction)
static void create_ssao_camera_view_nodes_es(const OnCameraPerViewNodeConstruction &evt,
  const int dng_ao_camera_nodes__w,
  const int dng_ao_camera_nodes__h,
  const uint32_t dng_ao_camera_nodes__creation_flags,
  const bool dng_ao_camera_nodes__useGTAO)
{
  if (dng_ao_camera_nodes__w <= 0 || dng_ao_camera_nodes__h <= 0)
    return;

  if (dng_ao_camera_nodes__useGTAO)
    return;

  const dafg::NameSpace viewNs = dafg::root() / evt.viewNsName;

  evt.nodes->push_back(ambient_occlusion::make_ssao_render_node<OrderingToken, AoRenderContext>(viewNs,
    {
      .target = ambient_occlusion::make_ssao_target_desc(dng_ao_camera_nodes__creation_flags),
      .names = ao_resource_names,
      .isMainView = evt.isMainView,
      // volfog shadow is integrated into the SSAO/GTAO pass as an additional shadow
      .waitTokenNames = {"after_prepare_lights_node_token", "after_esm_ao_node_token", "volfog_shadow_token"},
      .doneTokenName = "after_ssao_node_token",
      .cameraViewOrdering =
        evt.isMainView ? ambient_occlusion::CameraViewOrdering::MainView : ambient_occlusion::CameraViewOrdering::SecondaryView,
    }));
}
