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
#include <render/dag_cur_view.h>

#include <shaders/dag_shaderBlock.h>

#include <perfMon/dag_statDrv.h>
#include <perfMon/dag_cpuFreq.h>

#include <fx/dag_leavesWind.h>

#include <render/cascadeShadows.h>
#include <render/toroidal_update.h>
#include <render/toroidalStaticShadows.h>
#include <ecs/render/renderPasses.h>
#include <render/skies.h>
#include <render/clusteredLights.h>
#include <render/viewTransformData.h>
#include <render/debugMesh.h>

#include <heightmap/heightmapHandler.h>

#include <startup/dag_globalSettings.h>
#include <shaders/dag_overrideStates.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/visibility.h>
#include <rendInst/gpuObjects.h>

#include "wrDispatcher.h"
#include "global_vars.h"
#include "overridden_params.h"
#include "lmesh_modes.h"
#include "ssss.h"
#include "depthAOAbove.h"
#include <daRg/dag_panelRenderer.h>
#include <ui/uiRender.h>
#include <render/world/dynModelRenderPass.h>
#include <render/world/frameGraphHelpers.h>
#include "depthBounds.h"

#include "main/level.h"


CONSOLE_FLOAT_VAL_MINMAX("csm", resolution_mul, 1.0f, 0.5f, 4.0f);
CONSOLE_FLOAT_VAL_MINMAX("shadows", static_shadows_depth_bias, 0.0f, -10.0f, 2000.0f);
CONSOLE_BOOL_VAL("csm", cascade_depth_high_precision, false);
CONSOLE_BOOL_VAL("shadows", static_shadows_occlusion, true);
CONSOLE_INT_VAL("shadows", static_shadows_invalidation_grouping_us, 100 * 1000, 0, 10000 * 1000);
CONSOLE_INT_VAL("shadows", static_shadows_invalidation_merge_boxes_threshold, 250, 0, 10000);

CONSOLE_BOOL_VAL("shadows", render_static_shadows_every_frame, false);
CONSOLE_BOOL_VAL("shadows", always_render_all_cascades, false); // for debugging

CONSOLE_FLOAT_VAL_MINMAX("shadow", depth_bias, 0.2f, -0.5, 0.5);
CONSOLE_FLOAT_VAL_MINMAX("shadow", const_depth_bias, 0.0005, -0.5, 0.5);
CONSOLE_FLOAT_VAL_MINMAX("shadow", slope_depth_bias, 0.4f, -1, 1);

CONSOLE_FLOAT_VAL("shadow", pow_weight_override, -1.f);
CONSOLE_FLOAT_VAL("shadow", cascade0_dist_override, -1.f);
CONSOLE_BOOL_VAL("csm", always_render_dynamic_rendinsts, false);

extern ConVarT<bool, false> volfog_enabled;

#define SHADOW_MANAGER_VARS \
  VAR(projtm_psf_0)         \
  VAR(projtm_psf_1)         \
  VAR(projtm_psf_2)         \
  VAR(projtm_psf_3)         \
  VAR(shadow_frame)         \
  VAR(temporal_shadow_frame)

#define VAR(a) static ShaderVariableInfo a##VarId(#a);
SHADOW_MANAGER_VARS
#undef VAR

enum class ShadowOcclType : uint32_t
{
  SHADOW_OCCL_ANIMCHAR,
  SHADOW_OCCL_RIEXTRA
};

enum
{
  TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX = 4
};

struct ShadowOcclUsrData
{
  uint32_t occlType : 1;
  uint32_t frameNo : 31;
};

static struct StaticShadowsLandmeshCullingJob final : public cpujobs::IJob
{
  LandMeshManager *lmeshMgr;
  Frustum frustum;
  Point3 hmapOrigin;
  float hmapCameraHeight;
  float hmapWaterLevel;
  LandMeshCullingState lmeshState;
  LandMeshCullingData cullingData;
  void start(
    const Frustum &frustum_, LandMeshManager *lmesh_mgr, LandMeshRenderer *lmesh_renderer, const HeightmapHandler *hmap_handler)
  {
    lmeshMgr = lmesh_mgr;
    frustum = frustum_;
    hmapOrigin = hmap_handler ? hmap_handler->getPreparedOriginPos() : Point3::ZERO;
    hmapCameraHeight = hmap_handler ? hmap_handler->getPreparedCameraHeight() : 0.f;
    hmapWaterLevel = hmap_handler ? hmap_handler->getPreparedWaterLevel() : 0.f;
    lmeshState = {};
    lmeshState.copyLandmeshState(*lmeshMgr, *lmesh_renderer);
    threadpool::add(this, threadpool::PRIO_HIGH);
  }

  void clear() { clear_and_shrink(cullingData.heightmapData.patches); }

  virtual void doJob() override
  {
    TIME_PROFILE(static_shadow_lmesh_cull)
    lmeshState.frustumCulling(*lmeshMgr, frustum, nullptr, cullingData, nullptr, 0, hmapOrigin, hmapCameraHeight, hmapWaterLevel, 0);
  }
} static_shadows_lmesh_cull_job;

static void safe_znzf(Point2 &znzf)
{
  if (abs(znzf.x) < VERY_SMALL_NUMBER)
    znzf.x = znzf.x < 0 ? -VERY_SMALL_NUMBER : VERY_SMALL_NUMBER;
  if (abs(znzf.y) < VERY_SMALL_NUMBER)
    znzf.y = znzf.y < 0 ? -VERY_SMALL_NUMBER : VERY_SMALL_NUMBER;
}

static void shadow_invalidate(const BBox3 &box) { WRDispatcher::getShadowsManager().shadowsAddInvalidBBox(box); }

static BBox3 get_shadows_bbox()
{
  ShadowsManager &mgr = WRDispatcher::getShadowsManager();
  int cascadeCnt = mgr.getStaticShadowsCascadesCount();
  if (cascadeCnt <= 0)
    return BBox3();

  vec4f minCrd = V_C_MAX_VAL, maxCrd = V_C_MIN_VAL;

  ViewTransformData views[TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX];
  int viewCnt;
  mgr.getStaticShadowsCascadeWorldBoxViews(cascadeCnt - 1, views, viewCnt);

  for (int viewId = 0; viewId < viewCnt; ++viewId)
  {
    mat44f invGlobtm;
    v_mat44_inverse(invGlobtm, views[viewId].globtm);
    for (int i = 0; i < 8; ++i)
    {
      vec4f cornerPos =
        v_mat44_mul_vec3p(invGlobtm, v_make_vec4f(i / 4 ? 1.0f : -1.0f, i / 2 % 2 ? 1.0f : -1.0f, i % 2 ? 1.0f : -1.0f, 1.0f));
      minCrd = v_min(minCrd, cornerPos);
      maxCrd = v_max(maxCrd, cornerPos);
    }
  }

  BBox3 ret;
  v_stu_bbox3(ret, bbox3f{minCrd, maxCrd});
  return BBox3(as_point3(&minCrd), as_point3(&maxCrd));
}

shaders::OverrideState get_static_shadows_override_state()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  state.set(shaders::OverrideState::Z_BIAS);
  state.set(shaders::OverrideState::Z_FUNC);
  state.zBias = 0.0000025 * 130; // average zfar-znear is 130m.
  state.slopeZBias = 0.7;
  state.zFunc = CMPF_LESSEQUAL;
  return state;
}

ShadowsManager::ShadowsManager(IShadowInfoProvider &provider, ClusteredLights &lights) :
  shadowInfoProvider{provider},
  clusteredLights{lights},
  isTimeDynamic{provider.isTimeDynamic()},
  shadowsGpuOcclusionMgr(get_static_shadows_override_state()),
  riShadowCullBboxesLoader(*this)
{}

void ShadowsManager::initVisibilityNode()
{
  processVisibilityNode = {};

  if (shadowInfoProvider.isForward())
    return;

  // Forward doesn't draw dynamic shadows, so no occlusion is required
  processVisibilityNode = dabfg::register_node("process_shadows_visibility_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.create("shadow_visibility_token", dabfg::History::No).blob<OrderingToken>();

    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.requestState().setFrameBlock("global_frame");
    return [this, cameraHndl]() { processShadowsVisibility(cameraHndl.ref().viewItm.getcol(3)); };
  });
}

void ShadowsManager::initShadowsDownsampleNode()
{
  prepareDownsampledShadowsNode = {};

  if (shadowInfoProvider.isForward())
    return;

  // TODO: Are all of these checks really necessary?
  // downsampled_shadows used only for volume lights
  if (!shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::DOWNSAMPLED_SHADOWS) ||
      !shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::VOLUME_LIGHTS) || !volfog_enabled.get() || csm == nullptr)
    return;

  prepareDownsampledShadowsNode = dabfg::register_node("downsample_shadows_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.orderMeAfter("downsample_depth_node");
    registry.orderMeBefore("prepare_lights_node");
    int size = csm->getSettings().cascadeWidth / 2;
    auto downsampledShadowsHndl =
      registry.createTexture2d("downsampled_shadows", dabfg::History::No, {TEXCF_RTARGET | TEXFMT_DEPTH16, IPoint2{size, size}})
        .atStage(dabfg::Stage::POST_RASTER)
        .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
        .handle();
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Compare;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      registry.create("downsampled_shadows_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
    }
    registry.requestState().setFrameBlock("global_frame");
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    d3d::SamplerHandle csmSampler = d3d::request_sampler(smpInfo);

    return
      [this, downsampledShadowsHndl, csmSamplerVarId = get_shader_variable_id("shadow_cascade_depth_tex_samplerstate"), csmSampler]() {
        csm->setCascadesToShader();

        BaseTexture *csm_shadows = csm->getShadowsCascade();

        // TODO: shouldn't this be an assertion?
        if (!csm_shadows)
          return;
        d3d::SamplerHandle previousCsmSampler = ShaderGlobal::get_sampler(csmSamplerVarId);
        ShaderGlobal::set_sampler(csmSamplerVarId, csmSampler);

        G_ASSERT_RETURN(shadowsDownsample.getElem(), );

        TIME_D3D_PROFILE(downsampleShadows)
        SCOPE_RENDER_TARGET;

        d3d::set_render_target(nullptr, 0);
        d3d::set_depth(downsampledShadowsHndl.get(), DepthAccess::RW);

        shadowsDownsample.render();
        ShaderGlobal::set_sampler(csmSamplerVarId, previousCsmSampler);
        shaders::overrides::reset();
      };
  });
}

void ShadowsManager::initCombineShadowsNode()
{
  combineShadowsNode = {};

  if (shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::FORWARD_RENDERING))
    return;

  if (!shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::COMBINED_SHADOWS))
  {
    combineShadowsNode = dabfg::register_node("combine_shadows_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");
      return [this]() {
        setShadowFrameIndex();
        if (csm)
          updateCsmData();
      };
    });
    return;
  }

  combine_shadows.init("combine_shadows");

  if (isSsssReflectanceBlurEnabled())
  {
    combineShadowsNode = dabfg::register_node("combine_shadows_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");
      registry.readBlob<OrderingToken>("rtsm_token").optional();
      read_gbuffer(registry);
      read_gbuffer_depth(registry);
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
      auto ssssDepthMaskHndl = registry
                                 .createTexture2d("ssss_depth_mask", dabfg::History::No,
                                   {TEXFMT_DEPTH16 | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
                                 .atStage(dabfg::Stage::POST_RASTER)
                                 .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                                 .handle();

      shaders::OverrideState pipelineStateOverride;
      pipelineStateOverride.set(shaders::OverrideState::Z_FUNC);
      pipelineStateOverride.zFunc = CMPF_ALWAYS;
      registry.requestState().setFrameBlock("global_frame").enableOverride(pipelineStateOverride);
      combined_shadows_bind_additional_textures(registry);

      uint32_t fmt =
        combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : (isSsssTransmittanceEnabled() ? TEXFMT_R8G8 : TEXFMT_R8);
      auto combineShadowsHndl =
        registry.createTexture2d("combined_shadows", dabfg::History::No, {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")})
          .atStage(dabfg::Stage::POST_RASTER)
          .useAs(dabfg::Usage::COLOR_ATTACHMENT)
          .handle();
      registry.create("combined_shadows_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));

      return [this, ssssDepthMaskHndl, combineShadowsHndl]() {
        d3d::set_render_target({ssssDepthMaskHndl.view().getTex2D(), 0}, DepthAccess::RW, {{combineShadowsHndl.get(), 0}});
        combineShadows();
      };
    });

    return;
  }

  if (::depth_bounds_enabled())
  {
    combineShadowsNode = dabfg::register_node("combine_shadows_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.orderMeAfter("prepare_lights_node");
      registry.readBlob<OrderingToken>("rtsm_token").optional();
      read_gbuffer(registry);
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
      registry.read("far_downsampled_depth_sampler")
        .blob<d3d::SamplerHandle>()
        .bindToShaderVar("downsampled_far_depth_tex_samplerstate");

      shaders::OverrideState pipelineStateOverride;
      pipelineStateOverride.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
      registry.requestState().setFrameBlock("global_frame").enableOverride(pipelineStateOverride);
      combined_shadows_bind_additional_textures(registry);

      uint32_t fmt = combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : TEXFMT_R8;
      registry.createTexture2d("combined_shadows", dabfg::History::No, {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
      registry.create("combined_shadows_sampler", dabfg::History::No)
        .blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));
      registry.requestRenderPass().color({"combined_shadows"}).depthRoAndBindToShaderVars("gbuf_depth", {"depth_gbuf"});
      registry.read("gbuf_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("depth_gbuf_samplerstate");
      return [this]() {
        ::api_set_depth_bounds(::far_plane_depth(get_gbuffer_depth_format()), 1);
        combineShadows();
      };
    });

    return;
  }

  combineShadowsNode = dabfg::register_node("combine_shadows_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.orderMeAfter("prepare_lights_node");
    registry.readBlob<OrderingToken>("rtsm_token").optional();
    read_gbuffer(registry);
    read_gbuffer_depth(registry);
    registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::POST_RASTER).bindToShaderVar("downsampled_far_depth_tex");
    registry.read("far_downsampled_depth_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("downsampled_far_depth_tex_samplerstate");
    registry.requestState().setFrameBlock("global_frame");
    combined_shadows_bind_additional_textures(registry);
    uint32_t fmt = combined_shadows_use_additional_textures() ? TEXFMT_R8G8B8A8 : TEXFMT_R8;
    registry.createTexture2d("combined_shadows", dabfg::History::No, {fmt | TEXCF_RTARGET, registry.getResolution<2>("main_view")});
    registry.create("combined_shadows_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(d3d::SamplerInfo()));
    registry.requestRenderPass().color({"combined_shadows"});
    return [this]() { combineShadows(); };
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
  Color4 zn_zfar_old = ShaderGlobal::get_color4(zn_zfarVarId);
  Point2 safeZnzf = znzf;
  safe_znzf(safeZnzf);
  ShaderGlobal::set_color4(zn_zfarVarId, safeZnzf.x, safeZnzf.y, 0.f, 0.f);

  d3d::settm(TM_VIEW, &csm->getRenderViewMatrix(cascade_no));
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
    darg_panel_renderer::render_panels_in_world(*scn, camPos, darg_panel_renderer::RenderPass::Shadow);

  if (shouldRenderCsmStatic(cascade_no))
    // we don't render displaced heightmap to shadows, for performance reasons.
    // Instead we displace heightmap always "down" to avoid false occlusion
    renderGroundShadows(csm->getRenderCameraWorldViewPos(cascade_no), 0 /*cascade_no == 3 ? 0 : displacementSubDiv*/,
      Frustum((mat44f_cref)csm->getWorldCullingMatrix(cascade_no)));

  d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL);
  ShaderGlobal::set_color4(zn_zfarVarId, zn_zfar_old);
  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
}

void ShadowsManager::getCascadeShadowSparseUpdateParams(
  int /*cascade_no*/, const Frustum &, float &out_min_sparse_dist, int &out_min_sparse_frame)
{
  out_min_sparse_dist = 100000;
  out_min_sparse_frame = -1000;
}

void ShadowsManager::setShadowFrameIndex()
{
  const CameraParams &currentCamera = shadowInfoProvider.getCurrentFrameCameraParams();
  uint32_t shadow_frames_count, tempCount, frameNo;
  if (currentCamera.subSamples > 1)
  {
    tempCount = shadow_frames_count = currentCamera.subSamples;
    // offset a bit with super index as well
    frameNo = (currentCamera.subSampleIndex + currentCamera.superSampleIndex * 13) % currentCamera.subSamples;
  }
  else
  {
    shadow_frames_count = shadowInfoProvider.getShadowFramesCount();
    tempCount = shadowInfoProvider.getTemporalShadowFramesCount();
    frameNo = ::dagor_frame_no();
  }
  ShaderGlobal::set_int(shadow_frameVarId, frameNo % shadow_frames_count);
  ShaderGlobal::set_int4(temporal_shadow_frameVarId, frameNo % tempCount, frameNo % shadow_frames_count, frameNo & ((1 << 23) - 1),
    tempCount);
}

void ShadowsManager::updateCsmData()
{
  TMatrix4 projTm = shadowInfoProvider.getCurrentFrameCameraParams().jitterProjTm.transpose();
  ShaderGlobal::set_color4(projtm_psf_0VarId, Color4(projTm[0]));
  ShaderGlobal::set_color4(projtm_psf_1VarId, Color4(projTm[1]));
  ShaderGlobal::set_color4(projtm_psf_2VarId, Color4(projTm[2]));
  ShaderGlobal::set_color4(projtm_psf_3VarId, Color4(projTm[3]));
  if (csm)
    csm->setCascadesToShader();
}

void ShadowsManager::combineShadows()
{
  TIME_D3D_PROFILE(combineShadows);
  ShaderGlobal::set_int(shadows_qualityVarId, shadowsQuality > ShadowsQuality::SHADOWS_LOW ? 1 : 0);
  setShadowFrameIndex();
  updateCsmData();
  if (csm && shadowsQuality > ShadowsQuality::SHADOWS_LOW)
    csm->setNearCullingNearPlaneToShader();
  combine_shadows.render();
}

struct StaticShadowCullJob final : public cpujobs::IJob
{
  mat44f cullTm;
  Point3 viewPos;
  RiGenVisibility *visibility = nullptr;

  StaticShadowCullJob() = default;
  StaticShadowCullJob(const StaticShadowCullJob &) = delete;
  ~StaticShadowCullJob() { rendinst::destroyRIGenVisibility(visibility); }

  void doJob() override
  {
    TIME_PROFILE(staticShadowVisibility);
    rendinst::prepareRIGenExtraVisibility(cullTm, viewPos, *visibility, true, nullptr);
    rendinst::prepareRIGenVisibility(Frustum(cullTm), viewPos, visibility, true, nullptr);
  }
};
DAG_DECLARE_RELOCATABLE(StaticShadowCullJob);
static dag::RelocatableFixedVector<StaticShadowCullJob, 8, true, framemem_allocator>
  static_shadow_jobs[ShadowsManager::MAX_NUM_STATIC_SHADOWS_CASCADES];
static void free_overflowed_static_shadow_cull_jobs()
{
  for (auto &jobs : static_shadow_jobs)
  {
    for ([[maybe_unused]] auto &j : jobs)
      G_ASSERT(interlocked_relaxed_load(j.done));       // Expected to be waited on by `renderStaticShadowsRegion`
    if (DAGOR_UNLIKELY(jobs.size() > jobs.static_size)) // Overflowed?
      jobs.clear();
  }
}

struct ShadowsManager::StaticShadowCallback final : public IStaticShadowsCB
{
  ShadowsManager &shadowsManager;
  Color4 zn_zfar_old;
  TMatrix invShadowViewMatrix;

  StaticShadowCallback(ShadowsManager &s) : shadowsManager(s) {}

  void startRenderStaticShadow(const TMatrix &lightView, const Point2 &zn_zf, float, float) override
  {
    zn_zfar_old = ShaderGlobal::get_color4(zn_zfarVarId);
    Point2 safeZnzf = zn_zf;
    safe_znzf(safeZnzf);
    ShaderGlobal::set_color4(zn_zfarVarId, safeZnzf.x, safeZnzf.y, 0.f, 0.f);

    d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, reinterpret_cast<void *>(1));

    invShadowViewMatrix = orthonormalized_inverse(lightView);
    LeavesWindEffect::setNoAnimShaderVars(invShadowViewMatrix.getcol(0), invShadowViewMatrix.getcol(1), invShadowViewMatrix.getcol(2));
  }

  void renderStaticShadowDepth(const mat44f &cull_matrix, const ViewTransformData &curTransform, int region) override
  {
    TMatrix4 lightGlobTm;
    v_mat_44cu_from_mat44(&lightGlobTm._11, curTransform.globtm);

    shadowsManager.renderStaticShadowsRegion(cull_matrix, lightGlobTm, invShadowViewMatrix, curTransform.cascade, region);

    if (shadowsManager.hasRIShadowOcclusion() && curTransform.cascade == shadowsManager.staticShadows->cascadesCount() - 1)
    {
      shadowsManager.riShadowCullBboxesLoader.addTest(cull_matrix, curTransform);
    }
  }

  void endRenderStaticShadow() override
  {
    ShaderGlobal::set_color4(zn_zfarVarId, zn_zfar_old);
    d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL);
  }
};

float ShadowsManager::getStaticShadowsSmallestTexelSize() const
{
  return staticShadows ? staticShadows->getSmallestTexelSize() : 10000;
}

float ShadowsManager::getStaticShadowsBiggestTexelSize() const { return staticShadows ? staticShadows->getBiggestTexelSize() : 0; }

int ShadowsManager::getStaticShadowsCascadesCount() const { return staticShadows ? staticShadows->cascadesCount() : 0; }

void ShadowsManager::getStaticShadowsBoxViews(const BBox3 &box, ViewTransformData *out_views, int &out_cnt) const
{
  out_cnt = 0;
  if (staticShadows)
  {
    int cascade = staticShadows->getBoxViews(box, out_views, out_cnt);
    for (int vi = 0; vi < out_cnt; ++vi)
      out_views[vi].layer = cascade;
  }
}

bool ShadowsManager::getStaticShadowsCascadeWorldBoxViews(int cascade_id, ViewTransformData *out_views, int &out_cnt) const
{
  out_cnt = 0;
  if (staticShadows)
    return staticShadows->getCascadeWorldBoxViews(cascade_id, out_views, out_cnt);
  return true;
}

bool ShadowsManager::hasRIShadowOcclusion() const
{
  return anyCsmCascadeRendersStatic() && static_shadows_occlusion.get() && staticShadows;
}

int ShadowsManager::gatherAndTryAddRiForShadowsVisibilityTest(mat44f_cref globtm_cull, const ViewTransformData &transform)
{
  Tab<GpuVisibilityTestManager::TestObjectData> objData(framemem_ptr());
  float texelSize = getStaticShadowsBiggestTexelSize();
  ShadowOcclUsrData usrData;
  usrData.occlType = eastl::to_underlying(ShadowOcclType::SHADOW_OCCL_RIEXTRA);
  rendinst::gatherRIGenExtraToTestForShadows(objData, globtm_cull, texelSize, bitwise_cast<uint32_t>(usrData));

  int riLeftCount = objData.size();
  if (objData.size())
  {
    WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
    int availableSpace = shadowsGpuOcclusionMgr.getAvailableSpace();
    if (objData.size() <= availableSpace)
    {
      shadowsGpuOcclusionMgr.addObjectGroupToTest(make_span_const(objData), transform);
      riLeftCount = 0;
    }
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
  bbox3f overallBbox = gatherBboxesForAnimcharShadowCull(cam_pos, objData, bitwise_cast<uint32_t>(usrData));
  if (objData.empty())
    return;

  ViewTransformData views[TOROIDAL_TEXTURE_MAX_VIEWS_PER_BOX];
  int viewsCnt;
  BBox3 box;
  v_stu_bbox3(box, overallBbox);
  getStaticShadowsBoxViews(box, views, viewsCnt);

  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  for (int i = 0; i < viewsCnt; ++i)
    shadowsGpuOcclusionMgr.addObjectGroupToTest(make_span_const(objData), views[i]);
}

void ShadowsManager::processShadowsVisibility(const Point3 &cam_pos)
{
  static int frameNo = -1;
  ++frameNo;
  testAnimcharTestBboxes(cam_pos, frameNo);
  dag::Vector<bbox3f, framemem_allocator, false> cullBboxes;
  int maxFrameNo = -1;

  {
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

    shadowsGpuOcclusionMgr.doTests(getStaticShadowsTex());
  }

  if (!cullBboxes.empty())
    setAnimcharShadowCullBboxes(make_span(cullBboxes), cam_pos);

  riShadowCullBboxesLoader.start();
}

void ShadowsManager::resetShadowsVisibilityTesting()
{
  riShadowCullBboxesLoader.reset(); // Must be before locking shadowsGpuOcclusionMgrMutex.
  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  shadowsGpuOcclusionMgr.clearObjectsToTest();
}

BaseTexture *ShadowsManager::getStaticShadowsTex() { return staticShadows ? staticShadows->getTex().getBaseTex() : nullptr; }

void ShadowsManager::restoreShadowSampler()
{
  if (staticShadows)
    staticShadows->restoreShadowSampler();
}

bool ShadowsManager::insideStaticShadows(const Point3 &pos, float radius) const
{
  return staticShadows ? staticShadows->isInside(BBox3(pos, radius)) : false;
}

bool ShadowsManager::validStaticShadows(const Point3 &pos, float radius) const
{
  // sphere is inside valid static shadows. Valid means something was ever rendered to it
  return staticShadows ? staticShadows->isValid(BBox3(pos, radius)) : false;
}

bool ShadowsManager::fullyUpdatedStaticShadows(const Point3 &pos, float radius) const
{
  // sphere is inside valid static shadows. Valid means something was ever rendered to it and it wasnt invalidated after that
  return staticShadows ? staticShadows->isCompletelyValid(BBox3(pos, radius)) : false;
}

bool ShadowsManager::updateStaticShadowAround(const Point3 &pos, bool update_only_last_cascade)
{
  G_ASSERT_RETURN(staticShadows, false);
  TIME_D3D_PROFILE(updateStaticShadowAround);

  G_ASSERT(staticShadows->cascadesCount() <= MAX_NUM_STATIC_SHADOWS_CASCADES);

  if (worldBBoxDirty)
  {
    staticShadowsSetWorldSize();
    worldBBoxDirty = false;
  }

  if (always_render_all_cascades)
    update_only_last_cascade = false;

  bool timeDynamicityChanged = isTimeDynamic != shadowInfoProvider.isTimeDynamic();
  if (timeDynamicityChanged)
  {
    // Force reinit to not have issues with changing settings mid-transition.
    closeStaticShadow();
    isTimeDynamic = shadowInfoProvider.isTimeDynamic();
    initStaticShadow();
  }
  constexpr float staticShadowThreshold = 0.999;
  auto dirToSun = shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::STATIC);
  bool sunDirChanged = staticShadows->setSunDir(dirToSun, staticShadowThreshold, staticShadowThreshold);

  bool depthBiasChanged = false;
  int cascadesCount = staticShadows->cascadesCount();
  if (cascadesCount > 1)
  {
    for (int i = 0; i < cascadesCount - 1; i++)
      depthBiasChanged |= staticShadows->setDepthBiasForCascade(i, static_shadows_depth_bias.get());

    // This bias applies only to the last cascade.
    // This is done in order to get rid of the wierd self-shadowing in the far terrain,
    // because it does into the gbuffer from the nonzero lod, but into the shadowmap from lod0
    // https://youtrack.gaijin.team/issue/38-16644
    depthBiasChanged |=
      staticShadows->setDepthBiasForCascade(cascadesCount - 1, static_shadows_depth_bias.get() + 0.005f * staticShadows->getTexSize());
  }
  else
  {
    depthBiasChanged = staticShadows->setDepthBias(static_shadows_depth_bias.get());
  }

  // static shadow map is always bound in forward,
  // due to fact that we can't change most of blocks by interval/variable
  // so we must set invalid texture for update
  int shadowMapVarId = -1;
  if (shadowInfoProvider.isForward())
  {
    shadowMapVarId = staticShadows->getTex().getVarId();
    ShaderGlobal::set_texture(shadowMapVarId, BAD_TEXTUREID);
  }

  bool tpJobsAdded = false;
  auto updateOriginAndDispatchCullingThreads = [&]() {
    ToroidalStaticShadowCascade::BeforeRenderReturned ret =
      staticShadows->updateOrigin(pos, 0.05f, 0.1f, staticShadowUniformUpdate, staticShadowMaxUpdateAmount, update_only_last_cascade);
    for (int i = 0, cn = staticShadows->cascadesCount(); i < cn; i++)
      if (int rn = staticShadows->getRegionToRenderCount(i))
      {
        if (DAGOR_UNLIKELY(rn > static_shadow_jobs[i].size()))
          static_shadow_jobs[i].resize(rn);
        for (int j = 0; j < rn; ++j)
        {
          if (!static_shadow_jobs[i][j].visibility)
            static_shadow_jobs[i][j].visibility = initOneStaticShadowsVisibility();

          TMatrix4_vec4 mat = staticShadows->getRegionToRenderCullTm(i, j);
          static_shadow_jobs[i][j].cullTm = (mat44f &)mat;
          static_shadow_jobs[i][j].viewPos = pos;
          threadpool::add(&static_shadow_jobs[i][j], threadpool::PRIO_LOW, /*wake*/ false);
          tpJobsAdded = true;
        }
      }
    return ret;
  };

  if (!render_static_shadows_every_frame)
  {
    ToroidalStaticShadowCascade::BeforeRenderReturned ret = updateOriginAndDispatchCullingThreads();
    staticShadowsSetShaderVars = depthBiasChanged || sunDirChanged || ret.varsChanged;
    if (ret.scrolled)
    {
      staticShadows->setShaderVars();
      staticShadowsSetShaderVars = false;
    }
  }
  else
  {
    staticShadows->invalidate(true);

    for (int i = 0, n = staticShadows->cascadesCount(); i < n; i++)
    {
      staticShadows->enableOptimizeBigQuadsRender(i, false);
      staticShadows->enableTransitionCascade(i, false);

      updateOriginAndDispatchCullingThreads();

      // Render immediately when debugging to avoid state issues with multiple updateOrigin calls in a frame
      StaticShadowCallback cb(*this);
      staticShadows->render(cb);
    }
    staticShadows->setShaderVars();
  }

  if (shadowInfoProvider.isForward())
    ShaderGlobal::set_texture(shadowMapVarId, staticShadows->getTex().getTexId());

  if (!tpJobsAdded) // No regions to render
    static_shadows_lmesh_cull_job.clear();

  return tpJobsAdded;
}

void ShadowsManager::renderStaticShadows()
{
  if (!render_static_shadows_every_frame) // The debug code path renders immediately after culling.
  {
    StaticShadowCallback cb(*this);
    staticShadows->render(cb);
    if (staticShadowsSetShaderVars)
      staticShadows->setShaderVars();
    free_overflowed_static_shadow_cull_jobs();
  }
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
  const float staticDist = staticShadows ? staticShadows->getDistance() : 1e6f;
  // Adjust shadow distance based on FOV (roughly keep top-down frustum area constant)
  const float adjustedMaxDist = p.wk * csmShadowsMaxDist;
  csm_mode.maxDist = clamp(adjustedMaxDist, (csm_mode.numCascades > 1) ? 16.f : 0.f, staticDist * 0.5f);
  csm_mode.powWeight = pow_weight_override.get() >= 0 ? pow_weight_override.get() : cvt(csm_mode.maxDist, 30.f, 50.f, 0.7f, 0.9f);
  csm_mode.cascade0Dist = cascade0_dist_override.get() >= 0 ? cascade0_dist_override.get() : cascade0_dist;
  csm_mode.shadowStart = p.zn + csmStartOffsetDistance;
  G_ASSERT(shadowInfoProvider.getRendinstShadowVisibilities().size() >= csm_mode.numCascades);
  csm->prepareShadowCascades(csm_mode, shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::CSM),
    orthonormalized_inverse(itm), itm.getcol(3), proj_tm, frustum, Point2(p.zn, p.zf), p.zn);
}

void ShadowsManager::initStaticShadow()
{
  const int cascades = shadowsQuality < ShadowsQuality::SHADOWS_LOW ? 1 : 2;
  if (staticShadows && cascades == staticShadows->cascadesCount())
    return;
  closeStaticShadow();
  shaders::OverrideState state = get_static_shadows_override_state();
  staticShadowsOverride = shaders::overrides::create(state);
  state.set(shaders::OverrideState::FLIP_CULL);
  state.zBias = 0.000000001;
  staticShadowsOverrideFlipCull = shaders::overrides::create(state);

  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();

  const int toroidalShadowResDefault = 4096;
  int toroidalShadowsRes = lvl_override::getInt(lvlSettings, "staticShadowsResolution", toroidalShadowResDefault);
  G_ASSERT(toroidalShadowsRes > 0);
  if (toroidalShadowsRes <= 0)
    toroidalShadowsRes = toroidalShadowResDefault;

  const float staticShadowDistance = lvl_override::getReal(lvlSettings, "staticShadowsDistance", 700.f);
  static_shadows_occlusion.set(
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("staticShadowsOcclusion", static_shadows_occlusion.get()));
  static_shadows_invalidation_grouping_us.set(
    dgs_get_settings()
      ->getBlockByNameEx("graphics")
      ->getInt("staticShadowsInvalidationGroupingUs", static_shadows_invalidation_grouping_us.get()));
  debug("static shadowsQuality = %d, cascades = %d, isTimeDynamic = %d", eastl::to_underlying(shadowsQuality), cascades,
    (int)isTimeDynamic);
  staticShadows = eastl::make_unique<ToroidalStaticShadows>(toroidalShadowsRes, cascades, staticShadowDistance, 512.f, true);
  staticShadows->enableTextureSpaceAlignment(true);

  if (isTimeDynamic)
  {
    staticShadows->enableTransitionCascade(cascades - 1, true);
    staticShadows->enableOptimizeBigQuadsRender(cascades - 1, true);
    staticShadows->enableStopRenderingAfterRegionSplit(cascades - 1, true);
    staticShadows->enableTransitionCascade(0, false);
    staticShadows->enableOptimizeBigQuadsRender(0, false);
    staticShadows->enableStopRenderingAfterRegionSplit(0, false);
  }

  staticShadowRenderNode = dabfg::register_node("static_shadow_render_node", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
    registry.orderMeAfter("after_world_render_node");
    registry.multiplex(dabfg::multiplexing::Mode::None);
    registry.executionHas(dabfg::SideEffects::External);
    return [this]() { renderStaticShadows(); };
  });

  rendinst::shadow_invalidate_cb = shadow_invalidate;
  rendinst::get_shadows_bbox_cb = get_shadows_bbox;
}

void ShadowsManager::staticShadowsSetWorldSize()
{
  BBox3 shadowBox = shadowInfoProvider.getStaticShadowsBBox();
  if (staticShadows && !shadowBox.isempty())
  {
    shadowBox[0].y = max(shadowBox[0].y, shadowInfoProvider.getWaterLevel() - 10.f);
    shadowBox[1].y = shadowBox[1].y + staticShadowsAdditionalHeight;
    // extend bbox for high geometry
    // geometry at the edge of the world box (especially with high y) will be outside of the box in (skewed) shadow space
    auto dirToSun = shadowInfoProvider.getDirToSun(IShadowInfoProvider::DirToSunType::STATIC);
    if (dirToSun.y >= ToroidalStaticShadows::ORTHOGONAL_THRESHOLD)
    {
      Point3 bboxWidth = shadowBox.width();
      Point3 bboxMax = shadowBox.boxMax();
      Point3 bboxMin = shadowBox.boxMin();
      float projectStep = bboxWidth.y / dirToSun.y;
      float x = (dirToSun.x > 0) ? bboxMin.x : bboxMax.x;
      float z = (dirToSun.z > 0) ? bboxMin.z : bboxMax.z;
      Point3 projPoint = Point3(x, bboxMax.y, z) - dirToSun * projectStep;
      shadowBox += projPoint;
    }
    staticShadows->setWorldBox(shadowBox);
    // no point
    const float htRange = min(ceilf((shadowBox[1].y - shadowBox[0].y) / 8.f) * 8., 1024.); // no more than 1km
    staticShadows->setMaxHtRange(htRange);                                                 // that is only for skewed matrix
  }
}

void ShadowsManager::markWorldBBoxDirty() { worldBBoxDirty = true; }

void ShadowsManager::shadowsInvalidate(bool force)
{
  TIME_PROFILE_DEV(shadows_invalidate);
  if (staticShadows)
    staticShadows->invalidate(force);
  clusteredLights.invalidateAllShadows();
  if (auto *ao = WRDispatcher::getDepthAOAboveCtx())
    ao->invalidateAO(force);
  resetShadowsVisibilityTesting();
  invalidateAnimCharShadowOcclusion();
  if (hasRIShadowOcclusion())
    rendinst::render::invalidateRIGenExtraShadowsVisibility();
}

void ShadowsManager::shadowsInvalidate(const BBox3 *boxes, uint32_t cnt)
{
  TIME_PROFILE_DEV(shadows_invalidate_boxes);
  if (staticShadows)
    staticShadows->invalidateBoxes(boxes, cnt);
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
}

void ShadowsManager::shadowsAddInvalidBBox(const BBox3 &box)
{
  std::lock_guard<std::mutex> scopedLock(staticShadowInvalidBboxes.lock); // Note: does this really needed? Could it be replaced
                                                                          // with G_ASSERT(is_main_thread()) instead?
  staticShadowInvalidBboxes.list.push_back(box);
  if (staticShadowInvalidBboxes.list.size() == staticShadowInvalidBboxes.list.capacity()) // Keep 1 spare for v_ldu(&bbox[1].x) to work
  {
    staticShadowInvalidBboxes.list.push_back_uninitialized();
    staticShadowInvalidBboxes.list.pop_back();
  }
}

void ShadowsManager::shadowsInvalidateGatheredBBoxes()
{
  if (!shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::STATIC_SHADOWS))
    return;

  // group up invalidations for defined time period, to avoid excessive updates
  // when same bboxes are slowly added in list over long (compared to frame time) period
  uint64_t timeGap = static_shadows_invalidation_grouping_us.get();
  if (timeGap)
  {
    if (staticShadowInvalidBboxes.lastApplyTicks == 0)
    {
      std::lock_guard<std::mutex> scopedLock(staticShadowInvalidBboxes.lock);
      if (staticShadowInvalidBboxes.list.size())
        staticShadowInvalidBboxes.lastApplyTicks = ref_time_ticks();
      return;
    }

    if (ref_time_delta_to_usec(ref_time_ticks() - staticShadowInvalidBboxes.lastApplyTicks) < timeGap)
      return;

    staticShadowInvalidBboxes.lastApplyTicks = 0;
  }

  decltype(staticShadowInvalidBboxes.list) boxes;
  {
    std::lock_guard<std::mutex> scopedLock(staticShadowInvalidBboxes.lock);
    staticShadowInvalidBboxes.list.swap(boxes); // To consider: double buffering? framemem?
  }

  if (boxes.empty())
    return;

  TIME_PROFILE(shadowsInvalidateGatheredBBoxes);
  bool merge = boxes.size() >= static_shadows_invalidation_merge_boxes_threshold.get();
  DA_PROFILE_TAG(shadowsInvalidateGatheredBBoxes, "boxes.size=%d merged=%d", boxes.size(), merge);

  if (!merge)
  {
    for (const BBox3 &box : boxes)
      shadowsInvalidate(box);
  }
  else
  {
    BBox3 mergedBox;
    for (const BBox3 &box : boxes)
      mergedBox += box;
    shadowsInvalidate(mergedBox);
  }
}

void ShadowsManager::closeStaticShadow()
{
  shaders::overrides::destroy(staticShadowsOverride);
  shaders::overrides::destroy(staticShadowsOverrideFlipCull);
  staticShadows.reset();
  closeAllStaticShadowsVisibility();
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
  if (shadowInfoProvider.isForward())
    return;

  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  changeSpotShadowBiasBasedOnQuality();

  CascadeShadows::Settings csmSettings;

  csmStartOffsetDistance = lvl_override::getReal(lvlSettings, "dynamicShadowsStartOffset", 0.0f);

  csmShadowsMaxDist = lvl_override::getReal(lvlSettings, "dynamicShadowsDistance", 50.f);
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

void ShadowsManager::initShadows()
{
  shadowsDownsample.init("downsample_shadows_depth_4x");

  initShadowsSettings();
  initDynamicShadows();
  if (shadowInfoProvider.hasRenderFeature(FeatureRenderFlags::STATIC_SHADOWS))
    initStaticShadow();

  // As CSM state might have changed
  initShadowsDownsampleNode();
}

void ShadowsManager::initShadowsSettings()
{
  const DataBlock *graphicsBlock = dgs_get_settings()->getBlockByNameEx("graphics");
  staticShadowMaxUpdateAmount = graphicsBlock->getReal("staticShadowMaxUpdateAmount", 0.1);
  staticShadowUniformUpdate = graphicsBlock->getBool("staticShadowUniformUpdate", false);
  const char *quality = graphicsBlock->getStr("shadowsQuality", "low");

  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();

  const char *qualityOverride = lvl_override::getStr(lvlSettings, "shadowQuality", quality);
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
  del_it(csm);
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
void ShadowsManager::renderGroundShadows(const Point3 &origin, int displacement_subdiv, const Frustum &culling_frustum)
{
  auto *lmeshMgr = WRDispatcher::getLandMeshManager();

  if (!lmeshMgr)
    return;

  auto *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  G_ASSERT(lmeshRenderer != nullptr);

  TIME_D3D_PROFILE(render_ground);
  LandMeshCullingState lmeshState;
  LandMeshCullingData cullingData(framemem_ptr());
  lmeshState.copyLandmeshState(*lmeshMgr, *lmeshRenderer);
  lmeshState.frustumCulling(*lmeshMgr, culling_frustum, NULL, cullingData, NULL, 0, origin, shadowInfoProvider.getCameraHeight(),
    shadowInfoProvider.getWaterLevel(), 0, displacement_subdiv);

  set_lmesh_rendering_mode(LMeshRenderingMode::RENDERING_DEPTH);
  lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER, cullingData, origin);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
}

void ShadowsManager::closeAllStaticShadowsVisibility()
{
  for (auto &jobs : static_shadow_jobs)
  {
    for (auto &job : jobs)
      threadpool::wait(&job);
    jobs.clear();
  }
}

RiGenVisibility *ShadowsManager::initOneStaticShadowsVisibility()
{
  RiGenVisibility *visibility = rendinst::createRIGenVisibility(midmem);
  rendinst::setRIGenVisibilityMinLod(visibility, rendinst::MAX_LOD_COUNT, 1);
  rendinst::setRIGenVisibilityRendering(visibility, rendinst::VisibilityRenderingFlag::Static);
  return visibility;
}

void ShadowsManager::renderStaticShadowsRegion(
  const mat44f &culling_view_proj, const TMatrix4 &shadow_glob_tm, const TMatrix &view_itm, int cascade, int region)
{
  G_ASSERT(staticShadows);
  TIME_D3D_PROFILE(static_shadow_region);
  FRAME_LAYER_GUARD(globalFrameBlockId);
  Point3 camera_pos = ::grs_cur_view.pos; // todo: should be like Point3(0,0,0)

  Frustum cullingFrustum(culling_view_proj);

  LandMeshManager *lmeshMgr = WRDispatcher::getLandMeshManager();
  LandMeshRenderer *lmeshRenderer = WRDispatcher::getLandMeshRenderer();
  float oldInvGeomDist = 0;
  int oldHmapLodDist = 0;
  if (lmeshMgr)
  {
    oldInvGeomDist = lmeshRenderer->getInvGeomLodDist();
    lmeshRenderer->setInvGeomLodDist(1. / 30000);
    oldHmapLodDist = lmeshMgr->getHmapLodDistance();
    lmeshMgr->setHmapLodDistance(16);

    // For ortho shadows cull with culling matrix, because it may not render the whole depth range.
    Frustum lmeshCullingFrustum = staticShadows->isOrthoShadow() ? cullingFrustum : Frustum(shadow_glob_tm);
    static_shadows_lmesh_cull_job.start(lmeshCullingFrustum, lmeshMgr, lmeshRenderer, lmeshMgr->getHmapHandler());
  }

  shaders::overrides::set(staticShadowsOverride.get());
  shadowInfoProvider.renderStaticSceneForShadowPass(RENDER_STATIC_SHADOW, camera_pos, view_itm, cullingFrustum);

  {
    static int rendinstDepthSceneBlockId = ShaderGlobal::getBlockId("rendinst_depth_scene");
    SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);

    auto &job = static_shadow_jobs[cascade][region];
    threadpool::wait(&job);
    RiGenVisibility *rendinstStaticShadowVisibility = job.visibility;

    // ccw + bias
    rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, rendinstStaticShadowVisibility, view_itm,
      rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No);

    // cw
    shaders::overrides::reset();
    shaders::overrides::set(staticShadowsOverrideFlipCull.get());
    rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, rendinstStaticShadowVisibility, view_itm,
      rendinst::LayerFlag::Opaque, rendinst::OptimizeDepthPass::No);
    shaders::overrides::reset();
    shaders::overrides::set(staticShadowsOverride.get());
  }

  if (lmeshMgr)
  {
    TIME_D3D_PROFILE(render_ground);
    if (staticShadows->isOrthoShadow())
      set_lmesh_rendering_mode(LMeshRenderingMode::RENDERING_DEPTH);
    else
      set_lmesh_rendering_mode(LMeshRenderingMode::RENDERING_SHADOW);

    TMatrix4 globtmTr = shadow_glob_tm.transpose();
    ShaderGlobal::set_color4(globtm_psf_0VarId, Color4(globtmTr[0]));
    ShaderGlobal::set_color4(globtm_psf_1VarId, Color4(globtmTr[1]));
    ShaderGlobal::set_color4(globtm_psf_2VarId, Color4(globtmTr[2]));
    ShaderGlobal::set_color4(globtm_psf_3VarId, Color4(globtmTr[3]));

    threadpool::wait(&static_shadows_lmesh_cull_job, 0, threadpool::PRIO_HIGH);
    lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_ONE_SHADER, static_shadows_lmesh_cull_job.cullingData, camera_pos);

    lmeshMgr->setHmapLodDistance(oldHmapLodDist);
    lmeshRenderer->setInvGeomLodDist(oldInvGeomDist);
  }

  shaders::overrides::reset();
}

void ShadowsManager::updateShadowsQFromSun()
{
  shadowsQuality = settingsShadowQuality;
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
  WinAutoLock lock(shadowsGpuOcclusionMgrMutex);
  shadowsGpuOcclusionMgr.afterDeviceReset();
}

void ShadowsManager::changeSpotShadowBiasBasedOnQuality()
{
  const DataBlock *lvlSettings = shadowInfoProvider.getLevelSettings();
  Point4 defaultSpotShadowBias = lvl_override::getPoint4(lvlSettings, "spotShadowBias", Point4(-0.00006f, -0.1f, 0.01f, 0.015f));
  Point4 highSpotShadowBias = lvl_override::getPoint4(lvlSettings, "highSpotShadowBias", Point4(-0.00006f, -0.1f, 0.001f, 0.005f));
  Point4 spotBias = shadowsQuality <= ShadowsQuality::SHADOWS_MEDIUM ? defaultSpotShadowBias : highSpotShadowBias;
  clusteredLights.setShadowBias(spotBias.x, spotBias.y, spotBias.z, spotBias.w);
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
  return found;
}

REGISTER_CONSOLE_HANDLER(shadows_console_handler);
