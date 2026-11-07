// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "shadowsManager.h"

#include <util/dag_convar.h>
#include <util/dag_console.h>
#include <util/dag_threadPool.h>

#include <osApiWrappers/dag_clipboard.h>
#include <render/scopeRenderTarget.h>
#include <landMesh/lmeshManager.h>

#include <memory/dag_framemem.h>

#include <math/dag_frustum.h>
#include <math/dag_mathUtils.h>
#include <math/dag_vecMathCompatibility.h>

#include <ioSys/dag_dataBlock.h>

#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_commands.h>
#include <3d/dag_render.h>

#include <shaders/dag_shaderBlock.h>

#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>

#include <fx/dag_leavesWind.h>

#include <render/cascadeShadows.h>
#include <render/raytracedToroidalStaticShadow/raytracedToroidalStaticShadow_api.h>
#include <render/staticShadowsCB.h>
#include <render/lruCollision.h>
#include <daGI2/lruCollisionVoxelization.h>
#include <ecs/render/renderPasses.h>
#include <render/skies.h>
#include <render/lights/clusteredLights.h>
#include <render/viewTransformData.h>
#include <render/debugMesh.h>

#include <heightmap/heightmapHandler.h>

#include <startup/dag_globalSettings.h>
#include <shaders/dag_overrideStates.h>
#include <shaders/dag_computeShaders.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstRenderJob.h>
#include <rendInst/ccExtra.h>
#include <rendInst/visibility.h>
#include <rendInst/gpuObjects.h>
#include <util/dag_stlqsort.h>

#include "wrDispatcher.h"
#include "global_vars.h"
#include "overridden_params.h"
#include "ssss.h"
#include "depthAOAbove.h"
#include <daRg/dag_panelRenderer.h>
#include <ui/uiRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/frameGraphHelpers.h>
#include <render/cameraInCamera/cameraInCamera.h>
#include "depthBounds.h"
#include <render/world/bvh.h>
#include <render/denoiser/denoiser_api.h>
#include "main/level.h"


CONSOLE_FLOAT_VAL_MINMAX("csm", resolution_mul, 1.0f, 0.5f, 4.0f);
CONSOLE_BOOL_VAL("csm", cascade_depth_high_precision, false);
CONSOLE_BOOL_VAL("shadows", static_shadows_occlusion, true);

// 0: the classic match plus rtss_additional_levels
CONSOLE_INT_VAL("shadows", rtss_levels, 0, 0, RTSS_MAX_LEVELS);
CONSOLE_FLOAT_VAL_MINMAX("shadows", rtss_texel0, 0.f, 0.f, 1000.f);
CONSOLE_FLOAT_VAL("shadows", rtss_tolerance_deg, 0.f); // of level 0, doubled per level
CONSOLE_FLOAT_VAL("shadows", rtss_tolerance_max_deg, 2.5f);
CONSOLE_FLOAT_VAL("shadows", rtss_raster_cost, 1.f);
CONSOLE_INT_VAL("shadows", rtss_budget, 15, 1, 256);
CONSOLE_INT_VAL("shadows", rtss_batch_frames, 1, 1, 16);
CONSOLE_INT_VAL("shadows", rtss_burst, 15, 1, 256);
CONSOLE_INT_VAL("shadows", rtss_raster_batch, 64, 1, 256);
CONSOLE_INT_VAL("shadows", rtss_raster_box, 2, 1, 64);
CONSOLE_BOOL_VAL("shadows", rtss_raster_transpose, true);
CONSOLE_FLOAT_VAL("shadows", rtss_call_cost, 7.f); // a raster call costs its rendinst draws, not its area
CONSOLE_FLOAT_VAL_MINMAX("shadows", rtss_riex_min_texels, 1.f, 0.f, 8.f);
CONSOLE_INT_VAL("shadows", rtss_additional_levels, 1, 0, 2);        // no RIGen, cut past maxCasterReach
CONSOLE_BOOL_VAL("shadows", static_shadows_raster_collision, true); // riExtra collision, not meshes, on RIGen levels

CONSOLE_FLOAT_VAL_MINMAX("shadow", depth_bias, 0.2f, -0.5, 0.5);
CONSOLE_FLOAT_VAL_MINMAX("shadow", const_depth_bias, 0.0005, -0.5, 0.5);
CONSOLE_FLOAT_VAL_MINMAX("shadow", slope_depth_bias, 0.4f, -1, 1);

CONSOLE_FLOAT_VAL("shadow", pow_weight_override, -1.f);
CONSOLE_FLOAT_VAL("shadow", cascade0_dist_override, -1.f);
CONSOLE_BOOL_VAL("csm", always_render_dynamic_rendinsts, false);

extern ConVarT<bool, false> volfog_enabled;
extern ConVarT<float, true> sun_direction_update_threshold_deg;

#define SHADOW_MANAGER_VARS         \
  VAR(projtm_psf_0)                 \
  VAR(projtm_psf_1)                 \
  VAR(projtm_psf_2)                 \
  VAR(projtm_psf_3)                 \
  VAR(shadow_frame)                 \
  VAR(temporal_shadow_frame)        \
  VAR_OPT(dafx_use_voxel_shadows)   \
  VAR_OPT(offset_foliage_shadows)   \
  VAR_OPT(parallax_shadow_enabled)  \
  VAR_OPT(rasterize_collision_type) \
  VAR_OPT(force_rtss_shadows)       \
  VAR_OPT(static_shadows_cascades)

#define VAR(a)     static ShaderVariableInfo a##VarId(#a);
#define VAR_OPT(a) static ShaderVariableInfo a##VarId(#a, true);
SHADOW_MANAGER_VARS
#undef VAR_OPT
#undef VAR

enum class ShadowOcclType : uint32_t
{
  SHADOW_OCCL_ANIMCHAR,
  SHADOW_OCCL_RIEXTRA
};

struct ShadowOcclUsrData
{
  uint32_t occlType : 1;
  uint32_t frameNo : 31;
};

static void safe_znzf(Point2 &znzf)
{
  if (abs(znzf.x) < VERY_SMALL_NUMBER)
    znzf.x = znzf.x < 0 ? -VERY_SMALL_NUMBER : VERY_SMALL_NUMBER;
  if (abs(znzf.y) < VERY_SMALL_NUMBER)
    znzf.y = znzf.y < 0 ? -VERY_SMALL_NUMBER : VERY_SMALL_NUMBER;
}

struct ShadowsManager::StaticShadows
{
  RaytracedToroidalStaticShadow rtss;
  RaytracedToroidalStaticShadowSettings match; // the classic layout: the RIGen levels
  int64_t lastTicks = 0;
  Point3 viewPos = Point3(0, 0, 0);
  uint32_t convergedFrames = 0;
  // MAIN on purpose: another context costs vb memory
  rendinst::render::RiExtraRendererBuilder riexBuilder{
    RI_EXTRA_VB_CTX_MAIN, rendinst::RenderPass::ToShadow, rendinst::OptimizeDepthPass::Yes};
  shaders::UniqueOverrideStateId flipCullOverride;
  eastl::shared_ptr<LRURendinstCollision> lruCollision; // null where the dump has no render_lru_collision
  LRUCollisionVoxelization collisionDraw;
  eastl::unique_ptr<ComputeShaderElement> boxOcclusionCs;

  StaticShadows(const RaytracedToroidalStaticShadowSettings &settings, const RaytracedToroidalStaticShadowSettings &classic_match) :
    rtss(settings), match(classic_match)
  {}
  int riGenLevels() const { return min(match.levels, rtss.getSettings().levels); }
  float texel(int level) const { return rtss.getSettings().texel0 * float(1 << level); }
};

ShadowsManager::ShadowsManager(IShadowInfoProvider &provider, ClusteredLights &lights) :
  shadowInfoProvider{provider}, clusteredLights{lights}, shadowsGpuOcclusionMgr({}), riShadowCullBboxesLoader(*this)
{}

ShadowsManager::~ShadowsManager() = default;

void ShadowsManager::initVisibilityNode()
{
  for (auto &node : staticShadowsVisibilityNodes)
    node = {};

  staticShadowsVisibilityNodes[0] =
    dafg::register_node("static_shadows_visibility_cpu", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      registry.create("shadow_visibility_cpu_token").blob<OrderingToken>();

      return [this, cameraHndl]() {
        if (!staticShadows)
          return;
        // Find next animchar objects
        Point3 cam_pos = cameraHndl.ref().viewItm.getcol(3);
        testAnimcharTestBboxes(cam_pos, dagor_frame_no());

        // Retrive results from previous frames
        dag::Vector<bbox3f, framemem_allocator, false> cullBboxes;
        {
          uint32_t maxFrameNo = -1;
          WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
          shadowsGpuOcclusionMgr.getVisibilityTestResults([&](bool visible, const GpuVisibilityTestManager::TestObjectData &obj_data) {
            ShadowOcclUsrData usrData = bitwise_cast<ShadowOcclUsrData>(v_extract_wi(v_cast_vec4i(obj_data.bmax)));
            if (usrData.occlType == eastl::to_underlying(ShadowOcclType::SHADOW_OCCL_ANIMCHAR))
            {
              // We can receive data from multiple frames. But only the latest is needed.
              if (maxFrameNo < usrData.frameNo)
              {
                cullBboxes.clear();
                maxFrameNo = usrData.frameNo;
              }
              if (!visible)
                cullBboxes.push_back(obj_data);
            }
            else if (usrData.occlType == eastl::to_underlying(ShadowOcclType::SHADOW_OCCL_RIEXTRA))
            {
              uint32_t nodeId = v_extract_wi(v_cast_vec4i(obj_data.bmin));
              rendinst::setRIGenExtraNodeShadowsVisibility(nodeId, visible);
            }
          });
        }
        if (!cullBboxes.empty())
          setAnimcharShadowCullBboxes(make_span(cullBboxes), cam_pos);

        // Find next ri objects
        riShadowCullBboxesLoader.start();
      };
    });

  staticShadowsVisibilityNodes[1] =
    dafg::register_node("static_shadows_visibility_gpu", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.create("shadow_visibility_token").blob<OrderingToken>();
      registry.read("shadow_visibility_cpu_token");
      registry.requestState().setFrameBlock("global_frame");

      return [this]() { testStaticShadowOcclusion(); };
    });
}

void ShadowsManager::initShadowsDownsampleNode()
{
  prepareDownsampledShadowsNode = {};

  // TODO: Are all of these checks really necessary?
  // downsampled_shadows used only for volume lights
  if (!shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::DOWNSAMPLED_SHADOWS) ||
      !shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::VOLUME_LIGHTS) || !volfog_enabled.get() || csm == nullptr)
    return;

  prepareDownsampledShadowsNode = dafg::register_node("downsample_shadows_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    registry.orderMeBefore("prepare_lights_node");
    int size = csm->getSettings().cascadeWidth / 2;
    auto downsampledShadowsHndl =
      registry.createTexture2d("downsampled_shadows", {TEXCF_RTARGET | TEXFMT_DEPTH16, IPoint2{size, size}})
        .atStage(dafg::Stage::POST_RASTER)
        .useAs(dafg::Usage::DEPTH_ATTACHMENT)
        .handle();
    registry.requestState().setFrameBlock("global_frame");

    auto csmTextureHndl = registry.readTexHndlPs("csm_texture");

    return [this, downsampledShadowsHndl, csmTextureHndl]() {
      csm->setCascadesToShader();

      const BaseTexture *csm_shadows = csmTextureHndl.get();

      // TODO: shouldn't this be an assertion?
      if (!csm_shadows)
        return;

      G_ASSERT_RETURN(shadowsDownsample.getElem(), );

      TIME_D3D_PROFILE(downsampleShadows)
      SCOPE_RENDER_TARGET;

      d3d::set_render_target({downsampledShadowsHndl.get(), 0, 0}, DepthAccess::RW, {});

      shadowsDownsample.render();
      shaders::overrides::reset();
    };
  });
}

void ShadowsManager::initCombineShadowsNode()
{
  combineShadowsNode = {};
  combineShadowsResProviderNode = {};

  if (!shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::COMBINED_SHADOWS))
  {
    combineShadowsNode = dafg::register_node("combine_shadows_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.readBlob("after_prepare_lights_node_token");
      read_all_camera_view_tokens(registry.root(), {"after_gtao_node_token", "after_ssao_node_token", "after_ssr_node_token"});

      auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
      return [this, cameraHndl]() { updateCsmData(cameraHndl.ref()); };
    });
    return;
  }

  combine_shadows.init("combine_shadows");

  if (isSsssReflectanceBlurEnabled())
  {
    combineShadowsResProviderNode =
      dafg::register_node("combine_shadows_camera_res_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        registry.createTexture2d("ssss_depth_mask", {TEXFMT_DEPTH16 | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

        uint32_t fmt =
          combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : (isSsssTransmittanceEnabled() ? TEXFMT_R8G8 : TEXFMT_R8);

        registry.createTexture2d("combined_shadows", {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
        registry.create("combined_shadows_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));
      });

    combineShadowsNode = dafg::register_node("combine_shadows_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.readBlob("after_prepare_lights_node_token");
      read_all_camera_view_tokens(registry.root(), {"after_gtao_node_token", "after_ssao_node_token", "after_ssr_node_token"});
      denoiser::read_shadow_output(registry); // the shadow comes through bindless, the optional read orders after the denoiser
      read_gbuffer(registry);
      read_gbuffer_depth(registry);
      registry.bindTexPs("far_downsampled_depth", "downsampled_far_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("far_downsampled_depth_sampler", "downsampled_far_depth_tex_samplerstate");

      shaders::OverrideState pipelineStateOverride;
      pipelineStateOverride.set(shaders::OverrideState::Z_FUNC);
      pipelineStateOverride.zFunc = CMPF_ALWAYS;
      registry.requestState().setFrameBlock("global_frame").enableOverride(pipelineStateOverride);
      combined_shadows_bind_additional_textures(registry);
      registry.readTexture("csm_texture").atStage(dafg::Stage::POST_RASTER);

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
      registry.requestRenderPass().depth("ssss_depth_mask").color({"combined_shadows"});

      return
        [this, cameraHndl](const dafg::multiplexing::Index multiplex_index) { combineShadows(multiplex_index, cameraHndl.ref()); };
    });

    return;
  }

  if (::depth_bounds_enabled())
  {
    combineShadowsResProviderNode =
      dafg::register_node("combine_shadows_camera_res_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
        uint32_t fmt = combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : TEXFMT_R8;
        registry.createTexture2d("combined_shadows", {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
        registry.create("combined_shadows_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));
      });

    combineShadowsNode = dafg::register_node("combine_shadows_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
      registry.readBlob("after_prepare_lights_node_token");
      read_all_camera_view_tokens(registry.root(), {"after_gtao_node_token", "after_ssao_node_token", "after_ssr_node_token"});
      denoiser::read_shadow_output(registry); // the shadow comes through bindless, the optional read orders after the denoiser
      read_gbuffer(registry);
      registry.bindTexPs("far_downsampled_depth", "downsampled_far_depth_tex");
      registry.bindBlob<d3d::SamplerHandle>("far_downsampled_depth_sampler", "downsampled_far_depth_tex_samplerstate");

      shaders::OverrideState pipelineStateOverride;
      pipelineStateOverride.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
      registry.requestState().setFrameBlock("global_frame").enableOverride(pipelineStateOverride);
      combined_shadows_bind_additional_textures(registry);

      registry.requestRenderPass().color({"combined_shadows"}).depthReadTestAndSample("gbuf_depth", {"depth_gbuf"});
      registry.bindBlob<d3d::SamplerHandle>("gbuf_sampler", "depth_gbuf_samplerstate");
      registry.readTexture("csm_texture").atStage(dafg::Stage::POST_RASTER);

      registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
      auto camera = read_camera_in_camera(registry);
      auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

      return [this, cameraHndl](const dafg::multiplexing::Index multiplex_index) {
        ::api_set_depth_bounds(::far_plane_depth(get_gbuffer_depth_format()), 1);
        combineShadows(multiplex_index, cameraHndl.ref());
      };
    });

    return;
  }

  combineShadowsResProviderNode =
    dafg::register_node("combine_shadows_camera_res_provider_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
      uint32_t fmt = combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : TEXFMT_R8;
      registry.createTexture2d("combined_shadows", {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
      registry.create("combined_shadows_sampler").blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));
    });

  combineShadowsNode = dafg::register_node("combine_shadows_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    registry.readBlob("after_prepare_lights_node_token");
    read_all_camera_view_tokens(registry.root(), {"after_gtao_node_token", "after_ssao_node_token", "after_ssr_node_token"});
    denoiser::read_shadow_output(registry); // the shadow comes through bindless, the optional read orders after the denoiser
    read_gbuffer(registry);
    read_gbuffer_depth(registry);
    registry.bindTexPs("far_downsampled_depth", "downsampled_far_depth_tex");
    registry.bindBlob<d3d::SamplerHandle>("far_downsampled_depth_sampler", "downsampled_far_depth_tex_samplerstate");
    registry.requestState().setFrameBlock("global_frame");
    combined_shadows_bind_additional_textures(registry);
    registry.requestRenderPass().color({"combined_shadows"});
    registry.readTexture("csm_texture").atStage(dafg::Stage::POST_RASTER);

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    return [this, cameraHndl](const dafg::multiplexing::Index multiplex_index) { combineShadows(multiplex_index, cameraHndl.ref()); };
  });
}

Point4 ShadowsManager::getCascadeShadowAnchor(int cascade_no)
{
  if (shadowsQuality == ShadowsQuality::SHADOWS_MIN && cascade_no == 0)
    return Point4(0, 0, 0, 0); // world origin == no anchoring for the first cascade (snap to camera)
  else
    return Point4::xyz0(-getThisFrameCameraPos()); // negated to compensate camera movement (snap to world)
}

void ShadowsManager::renderCascadeShadowDepth(int cascade_no, const Point2 &znzf)
{
  Color4 zn_zfar_old = ShaderGlobal::get_float4(zn_zfarVarId);
  Point2 safeZnzf = znzf;
  safe_znzf(safeZnzf);
  ShaderGlobal::set_float4(zn_zfarVarId, safeZnzf.x, safeZnzf.y, 0.f, 0.f);

  const TMatrix4 &viewTm = csm->getRenderViewMatrix(cascade_no);

  d3d::settm(TM_VIEW, &viewTm);
  d3d::settm(TM_PROJ, &csm->getRenderProjMatrix(cascade_no));
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, reinterpret_cast<void *>(1));

  const TMatrix &shadowViewItm = csm->getShadowViewItm(cascade_no);
  const Point3 camPos = getThisFrameCameraPos();
  LeavesWindEffect::setNoAnimShaderVars(shadowViewItm.getcol(0), shadowViewItm.getcol(1), shadowViewItm.getcol(2));
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));

  renderShadowsOpaque(cascade_no, !shouldRenderCsmRendinst(cascade_no), shadowViewItm, camPos);

  auto uiScenes = uirender::get_all_scenes();
  for (darg::IGuiScene *scn : uiScenes)
    darg_panel_renderer::render_panels_in_world(*scn, darg_panel_renderer::RenderPass::Shadow, camPos, tmatrix(viewTm));

  if (shouldRenderCsmStatic(cascade_no))
    // we don't render displaced heightmap to shadows, for performance reasons.
    // Instead we displace heightmap always "down" to avoid false occlusion
    renderGroundShadows(csm->getRenderCameraWorldViewPos(cascade_no), 0 /*cascade_no == 3 ? 0 : displacementSubDiv*/,
      Frustum((mat44f_cref)csm->getWorldCullingMatrix(cascade_no)), shadowInfoProvider.getCurrentFrameCameraParams().noJitterProjTm);

  d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL);
  ShaderGlobal::set_float4(zn_zfarVarId, zn_zfar_old);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
}

void ShadowsManager::getCascadeShadowSparseUpdateParams(
  int /*cascade_no*/, const Frustum &, float &out_min_sparse_dist, int &out_min_sparse_frame)
{
  out_min_sparse_dist = 100000;
  out_min_sparse_frame = -1000;
}

void ShadowsManager::setShadowFrameIndex(const CameraParams &cur_frame_camera)
{
  uint32_t shadow_frames_count, tempCount, frameNo;
  if (cur_frame_camera.subSamples > 1)
  {
    tempCount = shadow_frames_count = cur_frame_camera.subSamples;
    // offset a bit with super index as well
    frameNo = (cur_frame_camera.subSampleIndex + cur_frame_camera.superSampleIndex * 13) % cur_frame_camera.subSamples;
  }
  else
  {
    shadow_frames_count = shadowInfoProvider.getShadowFramesCount();
    tempCount = shadowInfoProvider.getTemporalShadowFramesCount();
    frameNo = ::dagor_frame_no() - shadowFrameBase;
  }
  ShaderGlobal::set_int(shadow_frameVarId, frameNo % shadow_frames_count);
  ShaderGlobal::set_int4(temporal_shadow_frameVarId, frameNo % tempCount, frameNo % shadow_frames_count, frameNo & ((1 << 23) - 1),
    tempCount);
}

void ShadowsManager::resetShadowFrame() { shadowFrameBase = ::dagor_frame_no(); }

void ShadowsManager::updateCsmData(const CameraParams &cur_frame_camera)
{
  setShadowFrameIndex(cur_frame_camera);

  TMatrix4 projTm = cur_frame_camera.jitterProjTm.transpose();
  ShaderGlobal::set_float4(projtm_psf_0VarId, Color4(projTm[0]));
  ShaderGlobal::set_float4(projtm_psf_1VarId, Color4(projTm[1]));
  ShaderGlobal::set_float4(projtm_psf_2VarId, Color4(projTm[2]));
  ShaderGlobal::set_float4(projtm_psf_3VarId, Color4(projTm[3]));
  if (csm)
    csm->setCascadesToShader();
}

void ShadowsManager::combineShadows(const dafg::multiplexing::Index multiplex_index, const CameraParams &cur_frame_camera)
{
  TIME_D3D_PROFILE(combineShadows);
  ShaderGlobal::set_int(shadows_qualityVarId, shadowsQuality > ShadowsQuality::SHADOWS_LOW ? 1 : 0);
  updateCsmData(cur_frame_camera);
  if (csm && shadowsQuality > ShadowsQuality::SHADOWS_LOW)
    csm->setNearCullingNearPlaneToShader();

  camera_in_camera::ApplyPostfxState camcamState{multiplex_index, cur_frame_camera};
  combine_shadows.render();
}

static bool are_ri_visibility_lods_loaded(const RiGenVisibility *visibility, bool any_lod_quality_fine)
{
  return any_lod_quality_fine ? rendinst::isRiGenVisibilityResLoadingFinished(visibility)
                              : rendinst::isRiGenVisibilityForcedLodLoaded(visibility);
}

// cull_tm clips the region to x, y in [-1, 1], z in [0, 1]
static void gather_collision_casters(Tab<rendinst::riex_handle_t> &handles, mat44f_cref cull_tm, float min_size)
{
  mat44f clipToWorld, frame;
  v_mat44_inverse43(clipToWorld, cull_tm);
  frame.col2 = v_norm3(clipToWorld.col2);
  frame.col0 = v_norm3(v_sub(clipToWorld.col0, v_mul(frame.col2, v_dot3(clipToWorld.col0, frame.col2))));
  frame.col1 = v_cross3(frame.col2, frame.col0);
  frame.col3 = clipToWorld.col3;
  TMatrix tm;
  v_mat_43cu_from_mat44(&tm.m[0][0], frame);
  auto along = [&](vec3f axis) {
    return fabsf(v_extract_x(v_dot3_x(clipToWorld.col0, axis))) + fabsf(v_extract_x(v_dot3_x(clipToWorld.col1, axis)));
  };
  const float ex = along(frame.col0), ey = along(frame.col1), dz = along(frame.col2);
  const float depth = v_extract_x(v_length3_x(clipToWorld.col2)) + 2.f * dz;
  // the grid walks aabbs: segments about as deep as wide
  const int segments = clamp(int(ceilf(depth / (2.f * max(ex, ey)))), 1, 8);
  handles.clear();
  for (int s = 0; s < segments; ++s)
    rendinst::gatherRIGenExtraCollidableMin(handles, tm,
      BBox3(Point3(-ex, -ey, depth * s / segments - dz), Point3(ex, ey, depth * (s + 1) / segments - dz)), min_size * 0.5f);
  stlsort::sort(handles.begin(), handles.end()); // by pool; border spheres came twice
  handles.resize(eastl::unique(handles.begin(), handles.end()) - handles.begin());
}

struct StaticShadowCullJob final : public cpujobs::IJob
{
  mat44f cullTm = {};
  Point3 viewPos;
  float riexMinSize = 0;
  bool riGen = false, riexCollision = false, ready = false;
  RiGenVisibility *visibility = nullptr;
  Tab<rendinst::riex_handle_t> collisionHandles;
  LandMeshManager *lmeshMgr = nullptr;
  LandMeshCullDesc lmeshCullDesc;
  LandMeshCullingData lmeshCull;

  StaticShadowCullJob()
  {
    visibility = rendinst::createRIGenVisibility(midmem);
    rendinst::setRIGenVisibilityMinLod(visibility, rendinst::MAX_LOD_COUNT, 1);
    rendinst::setRIGenVisibilityRendering(visibility, rendinst::VisibilityRenderingFlag::Static);
  }
  StaticShadowCullJob(const StaticShadowCullJob &) = delete;
  ~StaticShadowCullJob() { rendinst::destroyRIGenVisibility(visibility); }

  const char *getJobName(bool &) const override { return DAPROFILER_STRING("staticShadowVisibility"); }

  void doJob() override
  {
    if (lmeshMgr)
      landmesh::frustum_cull(*lmeshMgr, lmeshCullDesc, lmeshCull);
    if (riexCollision)
      gather_collision_casters(collisionHandles, cullTm, riexMinSize);
    else
    {
      const float minSizeSq = sqr(riexMinSize);
      const auto bigEnough = [minSizeSq](vec4f bbmin, vec4f bbmax) {
        vec4f diagonal = v_sub(bbmax, bbmin);
        return v_extract_x(v_dot3_x(diagonal, diagonal)) >= minSizeSq;
      };
      rendinst::prepareRIGenExtraVisibility(*visibility, cullTm, {.viewPos = viewPos, .externalFilter = bigEnough, .forShadow = true});
    }
    if (riGen)
      rendinst::prepareRIGenVisibility(Frustum(cullTm), viewPos, visibility, true, nullptr);
  }
};
static dag::Vector<eastl::unique_ptr<StaticShadowCullJob>> static_shadow_jobs; // by raster region of the frame

void wait_static_shadows_cull_jobs()
{
  TIME_PROFILE(wait_static_shadows_cull_jobs);
  for (auto &job : static_shadow_jobs)
    threadpool::wait(job.get());
}

struct ShadowsManager::StaticShadowCallback final : public IStaticShadowsCB
{
  ShadowsManager &shadowsManager;
  Color4 zn_zfar_old;
  TMatrix invShadowViewMatrix;

  StaticShadowCallback(ShadowsManager &s) : shadowsManager(s) {}

  void startRenderStaticShadow(const TMatrix &lightView, const Point2 &zn_zf, float, float) override
  {
    zn_zfar_old = ShaderGlobal::get_float4(zn_zfarVarId);
    Point2 safeZnzf = zn_zf;
    safe_znzf(safeZnzf);
    ShaderGlobal::set_float4(zn_zfarVarId, safeZnzf.x, safeZnzf.y, 0.f, 0.f);

    d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, reinterpret_cast<void *>(1));

    invShadowViewMatrix = orthonormalized_inverse(lightView);
    LeavesWindEffect::setNoAnimShaderVars(invShadowViewMatrix.getcol(0), invShadowViewMatrix.getcol(1), invShadowViewMatrix.getcol(2));
  }

  bool readyToRenderRegionWithFinalQuality(const ViewTransformData &, int region) override
  {
    G_ASSERT_RETURN(region < (int)static_shadow_jobs.size(), false);
    StaticShadowCullJob &job = *static_shadow_jobs[region];
    threadpool::wait(&job);
    job.ready = !job.riGen || !rendinst::rendinstGlobalShadows || rendinst::render::isRIGenGlobalShadowTexturesReady();
    if (!job.riexCollision)
    {
      const bool anyLodQualityFine = is_bvh_enabled();
      const bool riLodsLoaded = are_ri_visibility_lods_loaded(job.visibility, anyLodQualityFine);
      if (!riLodsLoaded && !anyLodQualityFine)
        rendinst::riGenVisibilityScheduleForcedLodLoading(job.visibility);
      job.ready &= riLodsLoaded;
    }
    else if (!job.collisionHandles.empty()) // a failed upload keeps the page dirty
      job.ready &= shadowsManager.staticShadows->lruCollision->updateLRU(job.collisionHandles);
    return job.ready;
  }

  RasterizedResult renderStaticShadowDepth(const mat44f &cull_matrix, const ViewTransformData &transform, int region) override
  {
    G_ASSERT_RETURN(region < (int)static_shadow_jobs.size(), RasterizedResult::NothingRendered);
    const StaticShadowCullJob &job = *static_shadow_jobs[region];
    StaticShadows &ss = *shadowsManager.staticShadows;
    TIME_D3D_PROFILE(static_shadow_region);
    FRAME_LAYER_GUARD(globalFrameBlockId);

    shadowsManager.shadowInfoProvider.renderStaticSceneForShadowPass(RENDER_STATIC_SHADOW, job.viewPos, invShadowViewMatrix,
      Frustum(cull_matrix));

    if (job.riGen || !job.riexCollision)
    {
      rendinst::render::RiExtraRenderer *riexRenderer = nullptr;
      if (!job.riexCollision)
        riexRenderer = ss.riexBuilder.buildNow(*job.visibility, globalFrameBlockId, rendinstDepthSceneBlockId, TexStreamingContext(0));
      rendinst::LayerFlags layers = job.riexCollision ? rendinst::LayerFlags() : rendinst::LayerFlag::Opaque;
      if (job.riGen)
        layers |= rendinst::LayerFlag::NotExtra;
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, job.visibility, invShadowViewMatrix, layers,
        rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, riexRenderer);
      if (!job.riexCollision) // both faces of riExtra cast, for the thin single sided meshes
      {
        const shaders::OverrideStateId rasterState = shaders::overrides::get_current();
        shaders::overrides::reset();
        shaders::overrides::set(ss.flipCullOverride.get());
        rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, job.visibility, invShadowViewMatrix, rendinst::LayerFlag::Opaque,
          rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, riexRenderer);
        shaders::overrides::reset();
        shaders::overrides::set(rasterState);
      }
    }

    if (job.riexCollision && !job.collisionHandles.empty())
    {
      TIME_D3D_PROFILE(static_shadow_ri_collision);
      rasterize_collision_typeVarId.set_int(2); // the depth variant, a null pixel shader
      rendinst::ScopedRIExtraReadLock riexLock; // the LRU reads riTm by handle
      ss.lruCollision->draw(job.collisionHandles, nullptr, nullptr, 1, *ss.collisionDraw.renderCollisionElem);
    }

    if (LandMeshRenderer *lmeshRenderer = WRDispatcher::getLandMeshRenderer(); lmeshRenderer && job.lmeshMgr)
    {
      TIME_D3D_PROFILE(render_ground);
      LandMeshRenderDesc desc;
      desc.mode = LMeshRenderingMode::RENDERING_DEPTH;
      desc.invGeomLodDist = 1. / 30000;
      TMatrix4 globtm;
      v_mat_44cu_from_mat44(&globtm._11, transform.globtm);
      const TMatrix4 globtmTr = globtm.transpose();
      ShaderGlobal::set_float4(globtm_psf_0VarId, Color4(globtmTr[0]));
      ShaderGlobal::set_float4(globtm_psf_1VarId, Color4(globtmTr[1]));
      ShaderGlobal::set_float4(globtm_psf_2VarId, Color4(globtmTr[2]));
      ShaderGlobal::set_float4(globtm_psf_3VarId, Color4(globtmTr[3]));
      lmeshRenderer->renderCulled(*job.lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER, desc, job.lmeshCull, job.viewPos);
    }

    if (job.ready && transform.cascade == ss.riGenLevels() - 1 && shadowsManager.hasRIShadowOcclusion())
      shadowsManager.riShadowCullBboxesLoader.addTest(cull_matrix, ss.texel(transform.cascade));
    return RasterizedResult::SomethingRendered;
  }

  void endRenderStaticShadow() override
  {
    ShaderGlobal::set_float4(zn_zfarVarId, zn_zfar_old);
    d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL);
  }
};

// of the coarsest RIGen level, which gathers the riExtra tests
float ShadowsManager::getStaticShadowsBiggestTexelSize() const
{
  return staticShadows ? staticShadows->texel(staticShadows->riGenLevels() - 1) : 0;
}

bool ShadowsManager::hasRIShadowOcclusion() const
{
  return anyCsmCascadeRendersStatic() && static_shadows_occlusion.get() && staticShadows;
}

int ShadowsManager::gatherAndTryAddRiForShadowsVisibilityTest(mat44f_cref globtm_cull, float texel_size)
{
  FRAMEMEM_REGION;
  Tab<GpuVisibilityTestManager::TestObjectData> objData(framemem_ptr());
  ShadowOcclUsrData usrData;
  usrData.occlType = eastl::to_underlying(ShadowOcclType::SHADOW_OCCL_RIEXTRA);
  int availableSpace = 0;
  {
    WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
    availableSpace = shadowsGpuOcclusionMgr.getAvailableSpace();
    shadowsGpuOcclusionMgr.reserveSpaceForObjectsGroup(availableSpace);
  }
  uint32_t riLeftCount = 0;
  rendinst::gatherRIGenExtraToTestForShadows(objData, globtm_cull, texel_size, bitwise_cast<uint32_t>(usrData), availableSpace,
    riLeftCount);

  {
    WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
    shadowsGpuOcclusionMgr.clearReservedSpace();
    shadowsGpuOcclusionMgr.addObjectGroupToTest(make_span_const(objData), ViewTransformData{});
  }

  return riLeftCount;
}

void ShadowsManager::testAnimcharTestBboxes(const Point3 &cam_pos, int frame_no)
{
  // TODO: reserve to something meaningfull (tuned according to real usage data)
  dag::Vector<GpuVisibilityTestManager::TestObjectData, framemem_allocator> objData;
  ShadowOcclUsrData usrData;
  usrData.occlType = eastl::to_underlying(ShadowOcclType::SHADOW_OCCL_ANIMCHAR);
  usrData.frameNo = frame_no;
  gatherBboxesForAnimcharShadowCull(cam_pos, objData, bitwise_cast<uint32_t>(usrData));
  if (objData.empty())
    return;

  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  if (shadowsGpuOcclusionMgr.getAvailableSpace() < objData.size())
    return;
  shadowsGpuOcclusionMgr.addObjectGroupToTest(make_span_const(objData), ViewTransformData{});
}

void ShadowsManager::testStaticShadowOcclusion()
{
  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  if (staticShadows)
    shadowsGpuOcclusionMgr.doTestsOnGpu(*staticShadows->boxOcclusionCs);
  else
    shadowsGpuOcclusionMgr.clearObjectsToTest();
}

void ShadowsManager::resetShadowsVisibilityTesting()
{
  riShadowCullBboxesLoader.reset(); // Must be before locking shadowsGpuOcclusionMgrMutex.
  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  shadowsGpuOcclusionMgr.clearObjectsToTest();
}

bool ShadowsManager::validStaticShadows(const Point3 &pos, float cube_size) const
{
  return staticShadows && staticShadows->rtss.isReady(BBox3(pos, cube_size));
}

bool ShadowsManager::fullyUpdatedStaticShadows(const Point3 &pos, float cube_size) const
{
  return staticShadows && staticShadows->convergedFrames > 0 && validStaticShadows(pos, cube_size);
}

uint32_t ShadowsManager::getStaticShadowsConvergedFrames() const
{
  return staticShadows ? staticShadows->convergedFrames : UINT32_MAX;
}

static const float CLASSIC_SUN_STEP_DEG = RadToDeg(acosf(0.999f));

// the classic reach, at most 7.2% over its texel
static RaytracedToroidalStaticShadowSettings rtss_match_classic(float distance, int resolution, int cascades)
{
  RaytracedToroidalStaticShadowSettings s;
  const float reachTexels = float((s.windowPages / 2 - 1) * s.pageTexels());
  const float classicTexel0 = 2 * (cascades < 2 ? distance : distance / (cascades + 1)) / max(resolution, 1);
  s.levels = clamp(1 + (int)ceilf(log2f(distance / (reachTexels * classicTexel0)) - 0.1f), 1, (int)RTSS_MAX_LEVELS);
  s.texel0 = distance / (reachTexels * float(1 << (s.levels - 1)));
  return s;
}

static RaytracedToroidalStaticShadowSettings rtss_settings(const RaytracedToroidalStaticShadowSettings &match)
{
  RaytracedToroidalStaticShadowSettings s = match;
  s.levels = min(match.levels + rtss_additional_levels.get(), (int)RTSS_MAX_LEVELS);
  s.texel0 = rtss_texel0.get() > 0 ? rtss_texel0.get() : s.texel0;
  const float reachTexels = float((s.windowPages / 2 - 1) * s.pageTexels());
  while (s.levels > match.levels && reachTexels * s.texel0 * float(1 << (s.levels - 1)) > s.maxCasterReach)
    --s.levels;
  s.levels = rtss_levels.get() > 0 ? rtss_levels.get() : s.levels;
  s.toleranceDeg0 = rtss_tolerance_deg.get() > 0 ? rtss_tolerance_deg.get() : CLASSIC_SUN_STEP_DEG;
  s.toleranceDegMax = rtss_tolerance_max_deg.get() > 0 ? rtss_tolerance_max_deg.get() : s.toleranceDeg0;
  s.updatePagesPerFrame = rtss_budget.get();
  s.batchFrames = rtss_batch_frames.get();
  s.validityBurstPagesPerFrame = rtss_burst.get();
  s.rasterBatchPages = rtss_raster_batch.get();
  s.rasterBoxPagesPerJob = rtss_raster_box.get();
  s.rasterCallCost = rtss_call_cost.get();
  s.rasterTranspose = rtss_raster_transpose.get();
  s.sunJumpDeg = 2.f * sun_direction_update_threshold_deg.get(); // a sunDirectionUpdateStage step is a move, not a jump
  for (int k = 0; k < s.levels; ++k)
    s.recipes[k] = {RtssRaster::All, rtss_raster_cost.get()};
  return s;
}

bool ShadowsManager::updateStaticShadowAround(const Point3 &pos)
{
  if (!staticShadows)
    return false;
  TIME_PROFILE(updateStaticShadowAround);
  StaticShadows &ss = *staticShadows;
  wait_static_shadows_cull_jobs();

  if (worldBBoxDirty)
  {
    staticShadowsSetWorldSize();
    worldBBoxDirty = false;
  }

  bool invalidate = rtss_riex_min_texels.pullValueChange(); // the pages hold the casters of the old filter
  invalidate |= static_shadows_raster_collision.pullValueChange();
  if (invalidate)
    ss.rtss.invalidate();
  ss.rtss.setSettings(rtss_settings(ss.match));

  const float dt = ss.lastTicks ? get_time_usec(ss.lastTicks) * 1e-6f : 0.f;
  ss.lastTicks = ref_time_ticks();
  const Point3 velocity = dt > 0 ? (pos - ss.viewPos) / dt : Point3(0, 0, 0);
  ss.viewPos = pos;
  ss.rtss.setSun(shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::STATIC));
  ss.rtss.update(dpoint3(pos), velocity, dt);

  RaytracedToroidalStaticShadow::Stats stats;
  ss.rtss.getStats(stats);
  const bool converged =
    stats.rasterJobs == 0 && eastl::none_of(stats.levels.begin(), stats.levels.end(),
                               [](const RtssSchedulerLevelStats &l) { return l.dirtySlots || l.requiredPending || l.sweeping; });
  ss.convergedFrames = converged ? ss.convergedFrames + 1 : 0;

  const int regionCount = ss.rtss.getRasterRegionCount();
  while ((int)static_shadow_jobs.size() < regionCount)
    static_shadow_jobs.push_back(eastl::make_unique<StaticShadowCullJob>());
  LandMeshManager *lmeshMgr = WRDispatcher::getLandMeshManager();
  for (int r = 0; r < regionCount; ++r)
  {
    const ViewTransformData &region = ss.rtss.getRasterRegion(r);
    StaticShadowCullJob &job = *static_shadow_jobs[r];
    job.cullTm = region.globtm;
    job.viewPos = pos;
    job.riexMinSize = rtss_riex_min_texels.get() * ss.texel(region.cascade);
    job.riGen = region.cascade < ss.riGenLevels();
    job.riexCollision = ss.lruCollision && (!job.riGen || static_shadows_raster_collision.get());
    job.lmeshMgr = lmeshMgr;
    if (lmeshMgr)
    {
      job.lmeshCullDesc = LandMeshCullDesc::forView(pos, Frustum(region.globtm));
      job.lmeshCullDesc.useExclBox = lmeshMgr->cullingState.useExclBox;
      job.lmeshCullDesc.hmapMetrics = {0, 0, HeightmapMetricsQuality::FASTEST};
    }
    threadpool::add(&job, threadpool::PRIO_LOW, /*wake*/ false);
  }
  return regionCount > 0;
}

void ShadowsManager::renderStaticShadows()
{
  if (!staticShadows)
    return;
  StaticShadowCallback cb(*this);
  staticShadows->rtss.render(&cb, nullptr);
}

void ShadowsManager::debugRenderStaticShadows(int screen_mode, int atlas_level)
{
  if (staticShadows && screen_mode > 0)
    staticShadows->rtss.debugRenderScreen(screen_mode, false); // the FG node holds a native pass
  if (staticShadows && atlas_level >= 0)
    staticShadows->rtss.debugDrawAtlas(atlas_level);
}

void ShadowsManager::prepareShadowsMatrices(
  const TMatrix &itm, const Driver3dPerspective &p, const TMatrix4 &proj_tm, const Frustum &frustum, float cascade0_dist)
{
  if (!csm)
    return;

  TIME_PROFILE(prepareShadowsMatrices);

  // called from visibility job
  extern float shadow_render_expand_mul;
  shadow_render_expand_mul = 0.0f; // we support different occlusion matrix and depth clamp
  CascadeShadows::Settings csmSettings;
  csmSettings.shadowDepthBias = depth_bias.get();
  csmSettings.shadowConstDepthBias = const_depth_bias.get();
  csmSettings.shadowDepthSlopeBias = slope_depth_bias.get();
  csm->setDepthBiasSettings(csmSettings);
  const float staticDist = staticShadows ? staticShadows->rtss.getDistance(staticShadows->riGenLevels() - 1) : 1e6f;
  // Adjust shadow distance based on FOV (roughly keep top-down frustum area constant)
  const float fovMultiplier = csmShadowsDistanceFovScaling ? p.wk : 1.0f;
  const float adjustedMaxDist = fovMultiplier * csmShadowsMaxDist;
  csm_mode.maxDist = clamp(adjustedMaxDist, (csm_mode.numCascades > 1) ? 16.f : 0.f, staticDist * 0.5f);
  csm_mode.powWeight = pow_weight_override.get() >= 0 ? pow_weight_override.get() : cvt(csm_mode.maxDist, 30.f, 50.f, 0.7f, 0.9f);
  csm_mode.cascade0Dist = cascade0_dist_override.get() >= 0 ? cascade0_dist_override.get() : cascade0_dist;
  csm_mode.shadowStart = p.zn + csmStartOffsetDistance;
  csm_mode.cameraFov = min(p.hk, p.wk);
  G_ASSERT(shadowInfoProvider.getRendinstShadowVisibilities().size() >= csm_mode.numCascades);
  csm->prepareShadowCascades(csm_mode, shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::CSM),
    orthonormalized_inverse(itm), itm.getcol(3), proj_tm, frustum, Point2(p.zn, p.zf), p.zn);
}

void ShadowsManager::initStaticShadow()
{
  if (!ShaderGlobal::is_var_assumed(force_rtss_shadowsVarId) ||
      ShaderGlobal::get_interval_assumed_value(force_rtss_shadowsVarId) != 1 || !shader_exists("rtss_resolve_cs") ||
      !shader_exists("rtss_box_occlusion_cs"))
  {
    LOGERR_ONCE("static shadows: the shader dump has no rtss (force_rtss_shadows and its shaders), no static shadow");
    return;
  }
  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  const int cascades = shadowsQuality < ShadowsQuality::SHADOWS_LOW ? 1 : 2;
  const int resolution = lvl_override::getInt(lvlSettings, "staticShadowsResolution", 4096);
  const float distance = lvl_override::getReal(lvlSettings, "staticShadowsDistance", 700.f);
  if (resolution <= 0 || distance <= 0)
    logerr("static shadows: staticShadowsResolution %d and staticShadowsDistance %g must be positive", resolution, distance);
  RaytracedToroidalStaticShadowSettings match = rtss_match_classic(max(distance, 1.f), resolution, cascades);
  match.minSunY = lvl_override::getReal(lvlSettings, "staticShadowsMinSunY", match.minSunY);
  static_shadows_occlusion.set(
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("staticShadowsOcclusion", static_shadows_occlusion.get()));
  debug("static shadows: rtss, %d RIGen levels from %g m for %g m, %d texels, %d cascades", match.levels, match.texel0, distance,
    resolution, cascades);
  if (staticShadows)
  {
    if (staticShadows->riGenLevels() != match.levels) // the pages hold the RIGen of the old levels
      staticShadows->rtss.invalidate();
    staticShadows->match = match;
    return;
  }

  staticShadows = eastl::make_unique<StaticShadows>(rtss_settings(match), match);
  shaders::OverrideState flipCull; // as the raster state of the rtss
  flipCull.set(shaders::OverrideState::Z_CLAMP_ENABLED | shaders::OverrideState::Z_FUNC | shaders::OverrideState::FLIP_CULL);
  flipCull.zFunc = CMPF_LESSEQUAL;
  flipCull.colorWr = 0;
  staticShadows->flipCullOverride = shaders::overrides::create(flipCull);
  staticShadows->collisionDraw.init(false);
  if (staticShadows->collisionDraw.renderCollisionElem)
    staticShadows->lruCollision = LRURendinstCollision::acquire();
  staticShadows->boxOcclusionCs.reset(new_compute_shader("rtss_box_occlusion_cs"));
  static_shadows_cascadesVarId.set_int(2); // consumers gate the static shadow on it
  worldBBoxDirty = true;

  staticShadowRenderNode = dafg::register_node("static_shadow_render_node", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    registry.orderMeAfter("after_world_render_node");
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.executionHas(dafg::SideEffects::External);
    return [this]() { renderStaticShadows(); };
  });
}

void ShadowsManager::staticShadowsSetWorldSize()
{
  BBox3 shadowBox = shadowInfoProvider.getStaticShadowsBBox();
  if (!staticShadows || shadowBox.isempty())
    return;
  shadowBox[0].y = max(shadowBox[0].y, shadowInfoProvider.getWaterLevel() - 10.f);
  shadowBox[1].y = shadowBox[1].y + staticShadowsAdditionalHeight;
  staticShadows->rtss.setWorldBox(shadowBox);
}

void ShadowsManager::markWorldBBoxDirty() { worldBBoxDirty = true; }

void ShadowsManager::shadowsInvalidate(bool force)
{
  TIME_PROFILE_DEV(shadows_invalidate);
  if (staticShadows && force)
    staticShadows->rtss.afterDeviceReset();
  else if (staticShadows)
    staticShadows->rtss.invalidate();
  if (staticShadows)
    staticShadows->convergedFrames = 0;
  {
    OSSpinlockScopedLock scopedLock{clusteredLights.lightLock};
    clusteredLights.invalidateAllShadows();
  }
  if (auto *ao = WRDispatcher::getDepthAOAboveCtx())
    ao->invalidateAO(force);
  resetShadowsVisibilityTesting();
  invalidateAnimCharShadowOcclusion();
  if (hasRIShadowOcclusion())
    rendinst::render::invalidateRIGenExtraShadowsVisibility();
  if (voxelShadows)
    voxelShadows->invalidate();
}

void ShadowsManager::shadowsInvalidate(const BBox3 *boxes, uint32_t cnt)
{
  TIME_PROFILE_DEV(shadows_invalidate_boxes);
  if (staticShadows && cnt) // eden calls each frame, mostly empty
  {
    staticShadows->rtss.invalidateBoxes(boxes, cnt);
    staticShadows->convergedFrames = 0;
  }
  for (auto be = boxes + cnt; boxes != be; ++boxes)
  {
    auto &box = *boxes;
    bbox3f vbox;
    vbox.bmin = v_ldu(&box[0].x);
    vbox.bmax = v_ldu(&box[1].x); //  Note: calling code is expected to make sure that box[1].w is accessible (readed, but not used)
    clusteredLights.invalidateStaticObjects(vbox);
    if (auto *ao = WRDispatcher::getDepthAOAboveCtx())
      ao->invalidateAO(box);
    invalidateAnimCharShadowOcclusionBox(box);
    if (hasRIShadowOcclusion())
    {
      // this can be also optimized to be called with many boxes, as there is mutex locked inside RI
      rendinst::render::invalidateRIGenExtraShadowsVisibilityBox(vbox);
    }
  }
  if (voxelShadows)
    voxelShadows->invalidate();
}

void ShadowsManager::closeStaticShadow()
{
  closeAllStaticShadowsVisibility();
  staticShadows.reset();
  static_shadows_cascadesVarId.set_int(0);
  staticShadowRenderNode = {};
}

void ShadowsManager::conditionalEnableGPUObjsCSM()
{
  disableGPUObjsCSM();

  if (!is_level_loaded_not_empty())
    return;

  if (shadowsQuality >= ShadowsQuality::SHADOWS_HIGH && shadowsQuality <= ShadowsQuality::SHADOWS_ULTRA_HIGH)
  {
    eastl::span<RiGenVisibility *> riShadowVises = shadowInfoProvider.getRendinstShadowVisibilities();
    G_ASSERT(riShadowVises.size() >= CSM_STATIC_CASCADE_COUNT);
    for (int cascade = 0; cascade < CSM_STATIC_CASCADE_COUNT; cascade++)
    {
      if (riShadowVises[cascade]) // Workaround for shadows being initialized before the level is loaded
        rendinst::gpuobjects::enable_for_visibility(riShadowVises[cascade]);
    }
  }
}

void ShadowsManager::disableGPUObjsCSM()
{
  eastl::span<RiGenVisibility *> riShadowVises = shadowInfoProvider.getRendinstShadowVisibilities();
  G_ASSERT(riShadowVises.size() >= CSM_STATIC_CASCADE_COUNT);
  for (int cascade = 0; cascade < CSM_STATIC_CASCADE_COUNT; cascade++)
  {
    if (riShadowVises[cascade]) // Workaround for shadows being initialized before the level is loaded
      rendinst::gpuobjects::disable_for_visibility(riShadowVises[cascade]);
  }
}

void ShadowsManager::initDynamicShadows()
{
  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  changeSpotShadowBiasBasedOnQuality();

  CascadeShadows::Settings csmSettings;

  csmStartOffsetDistance = lvl_override::getReal(lvlSettings, "dynamicShadowsStartOffset", 0.0f);

  csmShadowsMaxDist = lvl_override::getReal(lvlSettings, "dynamicShadowsDistance", 50.f);
  csmShadowsDistanceFovScaling = lvl_override::getBool(lvlSettings, "dynamicShadowsDistanceFovScaling", true);

  csm_mode.numCascades = lvl_override::getInt(lvlSettings, "dynamicShadowsNumCascades", 4);
  G_ASSERT(csm_mode.numCascades > 0 && csm_mode.numCascades <= 4);
  csm_mode.numCascades = clamp(csm_mode.numCascades, 1, 4);

  csmSettings.cascadeWidth = lvl_override::getInt(lvlSettings, "dynamicShadowsResolution", 1024);

  ShadowsQuality activeShadowQuality = shadowsQuality;

  // avoid using better shadowsQuality if dynamic shadows are disabled
  if (settingsShadowQuality == ShadowsQuality::SHADOWS_ULTRA_HIGH) //-V1051
    activeShadowQuality = ShadowsQuality::SHADOWS_ULTRA_HIGH;
  else if (settingsShadowQuality == ShadowsQuality::SHADOWS_STATIC_ONLY)
    activeShadowQuality = ShadowsQuality::SHADOWS_STATIC_ONLY;

  ShaderGlobal::set_int(offset_foliage_shadowsVarId, settingsShadowQuality == ShadowsQuality::SHADOWS_ULTRA_HIGH ? 1 : 0);
  conditionalEnableGPUObjsCSM();

  switch (activeShadowQuality)
  {
    case ShadowsQuality::SHADOWS_MIN:
    case ShadowsQuality::SHADOWS_LOW:
      csm_mode.shadowCascadeZExpansion = lvl_override::getReal(lvlSettings, "shadowCascadeZExpansionLow", 40.f);
      csmShadowsMaxDist *= lvl_override::getReal(lvlSettings, "dynamicShadowsDistanceMultiplier_Low", 0.8f);
      csmSettings.cascadeWidth *= lvl_override::getReal(lvlSettings, "dynamicShadowsResolutionMultiplier_Low", 0.5f);
      break;
    case ShadowsQuality::SHADOWS_MEDIUM:
      csm_mode.shadowCascadeZExpansion = lvl_override::getReal(lvlSettings, "shadowCascadeZExpansion", 80.f);
      csmShadowsMaxDist *= lvl_override::getReal(lvlSettings, "dynamicShadowsDistanceMultiplier_Medium", 0.8f);
      csmSettings.cascadeWidth *= lvl_override::getReal(lvlSettings, "dynamicShadowsResolutionMultiplier_Medium", 0.5f);
      break;
    case ShadowsQuality::SHADOWS_STATIC_ONLY: del_it(csm); return;
    default: csm_mode.shadowCascadeZExpansion = lvl_override::getReal(lvlSettings, "shadowCascadeZExpansion", 80.f); break;
  }

  csm_mode.shadowCascadeZExpansion_znearOffset = lvl_override::getReal(lvlSettings, "shadowCascadeZExpansion_znearOffset", 0.f);

  csmSettings.cascadeWidth *= resolution_mul.get();

  csmSettings.cascadeDepthHighPrecision = cascade_depth_high_precision.get();
  if (csm_mode.numCascades == 4)
    csmSettings.splitsW = csmSettings.splitsH = 2; //-V1048
  else
  {
    csmSettings.splitsW = csm_mode.numCascades;
    csmSettings.splitsH = 1;
  }

  const Point3 defaultCsmBias(depth_bias.getDefault(), const_depth_bias.getDefault(), slope_depth_bias.getDefault());
  Point3 csmBias = lvl_override::getPoint3(lvlSettings, "dynamicShadowsBias", defaultCsmBias);
  depth_bias.set(csmBias.x);
  const_depth_bias.set(csmBias.y);
  slope_depth_bias.set(csmBias.z);

  pow_weight_override.set(lvl_override::getReal(lvlSettings, "dynamicShadowsCascadeDistributionPowerWeightOverride", -1));
  cascade0_dist_override.set(lvl_override::getReal(lvlSettings, "dynamicShadowsCascade0DistanceOverride", -1));

  if (csm)
  {
    const CascadeShadows::Settings &old = csm->getSettings();
    if (csmSettings.cascadeWidth == old.cascadeWidth && csmSettings.cascadeDepthHighPrecision == old.cascadeDepthHighPrecision &&
        csmSettings.splitsW == old.splitsW && csmSettings.splitsH == old.splitsH)
      return;
  }
  csmSettings.shadowDepthBias = depth_bias.get();
  csmSettings.shadowConstDepthBias = const_depth_bias.get();
  csmSettings.shadowDepthSlopeBias = slope_depth_bias.get();

  del_it(csm);
  csm = CascadeShadows::make(this, csmSettings);
  setNeedSsss(isSsssTransmittanceEnabled());

  csmExportResourcesNode = dafg::register_node("export_csm_resources", DAFG_PP_NODE_SRC, [this](dafg::Registry registry) {
    registry.multiplex(dafg::multiplexing::Mode::None);
    registry.registerTexture("csm_texture", [this](auto) { return csm->getShadowsCascadeView(); });
    auto csmHandle = registry.create("cascade_shadows").blob<CascadeShadows *>().handle();
    return [csmHandle, this](const dafg::multiplexing::Index) { csmHandle.ref() = csm; };
  });
}

static void set_voxel_shadow_cascades(VoxelShadows::Settings &voxel_shadow_settings,
  const IPoint4 &sparse_div,
  const Point4 &temoral_speed,
  const IPoint4 &compute_group_order,
  const IPoint4 &allow_dynamic)
{
  voxel_shadow_settings.cascadeCount = 4;
  for (int i = 0; i < 4; i++)
  {
    VoxelShadows::CascadeSettings &cs = voxel_shadow_settings.cascades.push_back();
    cs.sparseDiv = sparse_div[i];
    cs.allowDynamic = allow_dynamic[i] != 0;
    cs.temporalSpeed = temoral_speed[i];
    cs.computeGroupOrder = compute_group_order[i];
  }
}

void ShadowsManager::initVoxelShadows()
{
  voxelShadows.reset();
  dafx_use_voxel_shadowsVarId.set_int(0);

  if (WRDispatcher::isBareMinimum())
    return;

  // copy padding and the temporal update load and store voxel_shadows_tex through one UAV
  if (!(d3d::get_texformat_usage(TEXFMT_R8 | TEXCF_UNORDERED, D3DResourceType::VOLTEX) & d3d::USAGE_UNORDERED_LOAD))
    return;

  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  VoxelShadows::Settings voxelShadowSettings;
  voxelShadowSettings.xzDim = lvl_override::getInt(lvlSettings, "voxelShadowsDimXZ", 64);
  voxelShadowSettings.yDim = lvl_override::getInt(lvlSettings, "voxelShadowsDimY", 32);
  voxelShadowSettings.voxelSize = lvl_override::getReal(lvlSettings, "voxelShadowsVoxelSize", 0.8f);
  voxelShadowSettings.heightOffsetRatio = lvl_override::getReal(lvlSettings, "voxelShadowsHeightOffsetRatio", 0.4f);
  voxelShadowSettings.moveThreshold = lvl_override::getInt(lvlSettings, "voxelShadowsMoveThreshold", 4);

  switch (settingsShadowQuality)
  {
    case ShadowsQuality::SHADOWS_ULTRA_HIGH:
    case ShadowsQuality::SHADOWS_HIGH:
      set_voxel_shadow_cascades(voxelShadowSettings, IPoint4(2, 2, 4, 4), Point4(0.1f, 0.1f, 0.5f, 0.5f), IPoint4(0, 1, 2, 2),
        IPoint4(1, 1, 1, 1));
      break;
    case ShadowsQuality::SHADOWS_MEDIUM:
      set_voxel_shadow_cascades(voxelShadowSettings, IPoint4(4, 4, 4, 4), Point4(0.5f, 0.5f, 0.5f, 0.5f), IPoint4(0, 0, 1, 1),
        IPoint4(1, 1, 0, 0));
      break;
    case ShadowsQuality::SHADOWS_LOW:
      voxelShadowSettings.xzDim /= 2;
      voxelShadowSettings.yDim /= 2;
      voxelShadowSettings.voxelSize *= 2;
      set_voxel_shadow_cascades(voxelShadowSettings, IPoint4(4, 4, 4, 4), Point4(0.5f, 0.5f, 0.5f, 0.5f), IPoint4(0, 1, 2, 3),
        IPoint4(0, 0, 0, 0));
      break;
    default: // SHADOWS_MIN, SHADOWS_STATIC_ONLY:
      return;
  }

  int fxVoxelShadowUse = settingsShadowQuality == ShadowsQuality::SHADOWS_ULTRA_HIGH ? 2 : 1;
  dafx_use_voxel_shadowsVarId.set_int(fxVoxelShadowUse);

  voxelShadows.reset(new VoxelShadows(voxelShadowSettings));
}

rendinst::VisibilityRenderingFlags ShadowsManager::getCsmVisibilityRendering(int cascade) const
{
  G_ASSERT(shouldRenderCsmRendinst(cascade));
  return shouldRenderCsmStatic(cascade) ? rendinst::VisibilityRenderingFlag::All : rendinst::VisibilityRenderingFlag::Dynamic;
}

bool ShadowsManager::shouldRenderCsmRendinst(int cascade) const
{
  if (!csm)
    return false;
  if (always_render_dynamic_rendinsts || shouldRenderCsmStatic(cascade))
    return true;
  return shadowsQuality >= ShadowsQuality::SHADOWS_MEDIUM;
}

bool ShadowsManager::shouldRenderCsmStatic(int cascade) const
{
  if (!csm)
    return false;
  bool onlyDynamics = shadowsQuality <= ShadowsQuality::SHADOWS_LOW;
  return !onlyDynamics && cascade < CSM_DYNAMIC_ONLY_CASCADE;
}

bool ShadowsManager::anyCsmCascadeRendersStatic() const
{
  if (!csm)
    return false;
  for (int i = 0; i < csm->getNumCascadesToRender(); i++)
  {
    if (shouldRenderCsmStatic(i))
      return true;
  }
  return false;
}

void ShadowsManager::initParallaxShadows()
{
  ShaderGlobal::set_int(parallax_shadow_enabledVarId, settingsShadowQuality == ShadowsQuality::SHADOWS_ULTRA_HIGH ? 1 : 0);
}

void ShadowsManager::initShadows()
{
  shadowsDownsample.init("downsample_shadows_depth_4x");

  initShadowsSettings();
  initDynamicShadows();
  initVoxelShadows();
  if (shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::STATIC_SHADOWS))
    initStaticShadow();
  initParallaxShadows();

  // As CSM state might have changed
  initShadowsDownsampleNode();
}

void ShadowsManager::initShadowsSettings()
{
  const DataBlock *graphicsBlock = dgs_get_settings()->getBlockByNameEx("graphics");
  const char *quality = graphicsBlock->getStr("shadowsQuality", "low");

  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();

  const char *qualityOverride = lvl_override::getStr(lvlSettings, "shadowsQuality", quality);
  always_render_dynamic_rendinsts.set(graphicsBlock->getBool("csmAlwaysRenderDynamicRendinsts", false));

  shadowQualityLoweringThreshold = lvl_override::getReal(lvlSettings, "shadowQualityLoweringThreshold", 0.25f);
  debug("shadowQualityLoweringThreshold = %f", shadowQualityLoweringThreshold);

  if (shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::LEVEL_SHADOWS))
    quality = qualityOverride;
  else if (strcmp(quality, qualityOverride) != 0)
  {
    // compatibility does not allow shadow quality, other that setted up by default
    debug("shadow quality override %s -> %s ignored on compatibility render", quality, qualityOverride);
  }

  if (strcmp(quality, "minimum") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_MIN;
  else if (strcmp(quality, "low") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_LOW;
  else if (strcmp(quality, "medium") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_MEDIUM;
  else if (strcmp(quality, "high") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_HIGH;
  else if (strcmp(quality, "ultra") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_ULTRA_HIGH;
  else if (strcmp(quality, "static_only") == 0)
    settingsShadowQuality = ShadowsQuality::SHADOWS_STATIC_ONLY;
  else
  {
    settingsShadowQuality = ShadowsQuality::SHADOWS_MEDIUM;
    logerr("invalid shadows quality <%s>", quality);
  }
  updateShadowsQFromSun();
}

void ShadowsManager::closeShadows()
{
  closeStaticShadow();
  csmExportResourcesNode = {};
  del_it(csm);
  closeVSM();
  voxelShadows.reset();
}

void ShadowsManager::setNeedSsss(bool need_ssss)
{
  if (csm)
    csm->setNeedSsss(need_ssss);
  clusteredLights.setNeedSsss(need_ssss);
}

bbox3f ShadowsManager::getActiveShadowVolume() const { return clusteredLights.getActiveShadowVolume(); }

void ShadowsManager::renderShadowsOpaque(int cascade, bool only_dynamic, const TMatrix &itm, const Point3 &cam_pos)
{
  TIME_D3D_PROFILE(opaque);

  G_STATIC_ASSERT(RENDER_SHADOWS_CSM == 0);

  if (!only_dynamic)
    shadowInfoProvider.renderStaticSceneForShadowPass(cascade, cam_pos, csm->getShadowViewItm(cascade), csm->getFrustum(cascade));
  shadowInfoProvider.renderDynamicsForShadowPass(cascade, itm, cam_pos);
}

// todo: replace with vsm?
void ShadowsManager::renderGroundShadows(
  const Point3 &origin, int displacement_subdiv, const Frustum &culling_frustum, const TMatrix4 &viewProjTm)
{
  G_UNUSED(displacement_subdiv); // displaced heightmap is not rendered to shadows (see call site); kept for API
  auto *lmeshMgr = WRDispatcher::getLandMeshManager();

  if (!lmeshMgr)
    return;

  auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  G_ASSERT(lmeshRenderer != nullptr);

  TIME_D3D_PROFILE(render_ground);
  LandMeshCullingData cullingData(framemem_ptr());

  LandMeshCullDesc cullDesc = LandMeshCullDesc::forView(origin, culling_frustum);
  cullDesc.useExclBox = lmeshMgr->cullingState.useExclBox;
  cullDesc.hmapMetrics = {proj_to_distance_scale(viewProjTm)};
  cullDesc.hmapMetrics.maxRelativeTexelTess = 1;

  // fixme: culling should be done in thread!!!
  landmesh::frustum_cull(*lmeshMgr, cullDesc, cullingData);

  LandMeshRenderDesc desc;
  desc.mode = LMeshRenderingMode::RENDERING_DEPTH;
  desc.invGeomLodDist = lmeshRenderer->getInvGeomLodDist();
  lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER, desc, cullingData, origin);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
}

void ShadowsManager::closeAllStaticShadowsVisibility()
{
  wait_static_shadows_cull_jobs();
  clear_and_shrink(static_shadow_jobs);
  if (staticShadows && staticShadows->lruCollision)
    staticShadows->lruCollision->clearRiInfo();
}

void ShadowsManager::updateShadowsQFromSun()
{
  shadowsQuality = settingsShadowQuality;
  if (is_rtsm_enabled())
  {
    // TODO: This is a hack that uses CSM to have dynamic object shadows in RTSM, when they are not in BVH.
    // It relies on static objects not getting rendered into CSM on low quality.
    // It also can introduce light leaks in reflections due to static shadow accuracy issues.
    shadowsQuality = is_bvh_dyn_models_enabled() ? ShadowsQuality::SHADOWS_MEDIUM : ShadowsQuality::SHADOWS_LOW;
    return;
  }
  if (!get_daskies() || shadowsQuality <= ShadowsQuality::SHADOWS_LOW)
    return;

  // Reduce shadows quality for morning and evening presets, because it needs to render a lot of objects to cascades.
  float sunHeight = get_daskies()->getPrimarySunDir().y;
  if (sunHeight < shadowQualityLoweringThreshold && settingsShadowQuality != ShadowsQuality::SHADOWS_ULTRA_HIGH)
    shadowsQuality = ShadowsQuality::SHADOWS_LOW;
}

void ShadowsManager::updateShadowsQFromSunAndReinit()
{
  ShadowsQuality oldQuality = shadowsQuality;
  updateShadowsQFromSun();

  if (shadowsQuality != oldQuality)
    initShadows();
}

void ShadowsManager::afterDeviceReset()
{
  if (staticShadows)
  {
    staticShadows->rtss.afterDeviceReset();
    staticShadows->convergedFrames = 0;
    if (staticShadows->lruCollision)
      staticShadows->lruCollision->reset(); // its heaps are created again empty
  }
  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  shadowsGpuOcclusionMgr.afterDeviceReset();
  if (vsm.isInited())
    vsm.forceUpdate();
  if (voxelShadows)
    voxelShadows->invalidate();
}

void ShadowsManager::changeSpotShadowBiasBasedOnQuality()
{
  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  Point4 defaultSpotShadowBias = lvl_override::getPoint4(lvlSettings, "spotShadowBias", Point4(-0.00006f, -0.1f, 0.01f, 0.015f));
  Point4 highSpotShadowBias = lvl_override::getPoint4(lvlSettings, "highSpotShadowBias", Point4(-0.00006f, -0.1f, 0.001f, 0.005f));
  Point4 spotBias = shadowsQuality <= ShadowsQuality::SHADOWS_MEDIUM ? defaultSpotShadowBias : highSpotShadowBias;
  clusteredLights.setShadowBias(spotBias.x, spotBias.y, spotBias.z, spotBias.w);
}

void ShadowsManager::initVSM()
{
  // Don't reallocate on level change, but rerender everything
  if (!vsm.isInited())
  {
    vsm.init(VSM_TEXTURE_SIZE, VSM_TEXTURE_SIZE, vsm.VSM_HW);
    vsm.box_update_threshold = VSM_UPDATE_THRESHOLD_SQ;
    vsm.box_full_update_threshold = VSM_UPDATE_THRESHOLD_SQ * 4;
    vsm.enable_vsm_treshold = 1; // To reduce shader variants it's assumed on
  }
  vsm.forceUpdate();
}

void ShadowsManager::prepareVSM(const Point3 &camera_pos)
{
  if (!vsm.isInited())
    return;

  LandMeshManager *lmeshMgr = WRDispatcher::getLandMeshManager();
  LandMeshRenderer *lmeshRenderer = WRDispatcher::getLandMeshRenderer();

  if (!lmeshMgr || !lmeshRenderer)
  {
    vsm.setOff();
    return;
  }

  BBox3 landBox = lmeshMgr->getBBox();
  auto fromSunDir = -shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::STATIC);

  TIME_D3D_PROFILE(prepareVsm);
  Variance::CullingInfo vsmCullingInfo;
  if (vsm.startShadowMap(landBox, fromSunDir, VSM_SHADOW_DISTANCE, &vsmCullingInfo))
  {
    TIME_D3D_PROFILE(render_ground_vsm);

    TMatrix4 shadow_glob_tm = vsmCullingInfo.viewProj;
    Frustum frustum = shadow_glob_tm;

    LandMeshCullingData cullingData;
    {
      TIME_PROFILE(vsm_lmesh_cull)
      LandMeshCullDesc cullDesc = LandMeshCullDesc::forView(camera_pos, frustum);
      cullDesc.useExclBox = lmeshMgr->cullingState.useExclBox;
      cullDesc.hmapMetrics = {0, -1, HeightmapMetricsQuality::FASTEST};
      landmesh::frustum_cull(*lmeshMgr, cullDesc, cullingData);
    }

    TMatrix4 globtmTr = shadow_glob_tm.transpose();
    ShaderGlobal::set_float4(globtm_psf_0VarId, Color4(globtmTr[0]));
    ShaderGlobal::set_float4(globtm_psf_1VarId, Color4(globtmTr[1]));
    ShaderGlobal::set_float4(globtm_psf_2VarId, Color4(globtmTr[2]));
    ShaderGlobal::set_float4(globtm_psf_3VarId, Color4(globtmTr[3]));

    {
      FRAME_LAYER_GUARD(globalFrameBlockId);

      LandMeshRenderDesc desc;
      desc.mode = LMeshRenderingMode::RENDERING_VSM;
      desc.invGeomLodDist = 1. / 30000;
      lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER, desc, cullingData, camera_pos);
    }

    vsm.endShadowMap();
  }
}

void ShadowsManager::closeVSM() { vsm.close(); }

void ShadowsManager::prepareVoxelShadows(const Point3 &camera_pos)
{
  if (!voxelShadows)
    return;

  TIME_D3D_PROFILE(voxel_shadows);
  voxelShadows->updateOrigin(camera_pos);
  voxelShadows->calculateVolumes();
}

void ShadowsManager::debugRenderVoxelShadows()
{
  if (!voxelShadows)
    return;

  voxelShadows->renderDebugOpt();
}

static bool shadows_console_handler(const char *argv[], int argc)
{
  int found = 0;
  CONSOLE_CHECK_NAME("shadow", "spot_bias", 1, 5)
  {
    auto &lights = WRDispatcher::getClusteredLights();

    float zBias, slopeZBias, shaderZBias, shaderSlopeZBias;
    lights.getShadowBias(zBias, slopeZBias, shaderZBias, shaderSlopeZBias);
    if (argc < 5)
    {
      console::print_d("Current shadow bias values for spot lights: "
                       "zBias: %f, slopeZBias: %f, shaderZBias: %f, shaderSlopeZBias: %f (saved to clipboard)",
        zBias, slopeZBias, shaderZBias, shaderSlopeZBias);
      console::print("Usage: shadow.spot_bias zBias, slopeZBias, shaderZBias, shaderSlopeZBias");
      String str(128, "%f, %f, %f, %f", zBias, slopeZBias, shaderZBias, shaderSlopeZBias);
      clipboard::set_clipboard_ansi_text(str.str());
    }
    else
    {
      zBias = atof(argv[1]);
      slopeZBias = atof(argv[2]);
      shaderZBias = atof(argv[3]);
      shaderSlopeZBias = atof(argv[4]);
      lights.setShadowBias(zBias, slopeZBias, shaderZBias, shaderSlopeZBias);
      console::print_d("Spot bias set. Current shadow bias values for spot lights: "
                       "zBias: %f, slopeZBias: %f, shaderZBias: %f, shaderSlopeZBias: %f",
        zBias, slopeZBias, shaderZBias, shaderSlopeZBias);
    }
  }
  CONSOLE_CHECK_NAME("shadow", "csm_start_offset", 1, 2)
  {
    auto &shadowsManager = WRDispatcher::getShadowsManager();
    if (argc < 2)
    {
      console::print_d("Current distance offset for CSM: %f", shadowsManager.csmStartOffsetDistance);
      console::print("Usage: shadow.csm_start_offset offset");
    }
    else
    {
      float offset = atof(argv[1]);
      shadowsManager.csmStartOffsetDistance = offset;
      console::print_d("CSM start offset distance set to %f", offset);
    }
  }
  CONSOLE_CHECK_NAME("shadow", "reinitCsm", 1, 1)
  {
    auto &shadowsManager = WRDispatcher::getShadowsManager();
    shadowsManager.initShadows();
  }
  CONSOLE_CHECK_NAME("shadow", "invalidate", 1, 2)
  {
    auto &shadowsManager = WRDispatcher::getShadowsManager();
    bool force = true;
    if (argc > 1)
      force = atoi(argv[1]) != 0;
    shadowsManager.shadowsInvalidate(force);
  }
  CONSOLE_CHECK_NAME("shadow", "reset_frame", 1, 1) { WRDispatcher::getShadowsManager().resetShadowFrame(); }
  return found;
}

REGISTER_CONSOLE_HANDLER(shadows_console_handler);
