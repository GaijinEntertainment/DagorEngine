// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam

#include "private_worldRenderer.h"
#include "global_vars.h"

#include "hideNodesEvent.h"

#include <3d/dag_render.h>
#include <daECS/core/entityManager.h>
#include <daRg/dag_panelRenderer.h>
#include <frustumCulling/frustumPlanes.h>
#include <math/dag_frustum.h>
#include <fftWater/fftWater.h>
#include <perfMon/dag_statDrv.h>
#include <debug/dag_assert.h>
#include <3d/dag_texPackMgr2.h>
#include <webui/editVarPlugin.h>
#include <render/dag_cur_view.h>

#include <render/world/dynModelRenderer.h>
#include <render/world/frameGraphHelpers.h>
#include <render/world/frameGraphNodes/frameGraphNodes.h>
#include <render/world/frameGraphNodesMobile/nodes.h>
#include "render/fx/fx.h"
#include <render/debugMesh.h>
#include <render/deferredRenderer.h>
#include <ecs/render/renderPasses.h>
#include "render/screencap.h"
#include <render/viewVecs.h>
#include "render/skies.h"
#include "render/renderEvent.h"
#include <render/antiAliasing.h>
#include <render/dynamicQuality.h>
#include <render/dynamicResolution.h>
#include <render/downsampleDepth.h>
#include <ecs/render/updateStageRender.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstGen.h>

#include <render/noiseTex.h>
#include <render/clipmapDecals.h>
#include <render/world/gbufferConsts.h>
#include <render/world/occlusionLandMeshManager.h>
#include <render/world/postfxRender.h>
#include <render/weather/fluidWind.h>
#include <render/daBfg/bfg.h>
#include <render/resourceSlot/registerAccess.h>


#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>

#include "lmesh_modes.h"
#include <landMesh/lmeshManager.h>
#include <landMesh/clipMap.h>

#include <ui/uiRender.h>
#include <util/dag_convar.h>

#include "renderDynamicCube.h"

#include <shaders/subpass_registers.hlsli>
#include <render/vertexDensityOverlay.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_renderTarget.h>


extern int show_shadows;

#define EXTERN_CONSOLE_BOOL_VAL(name) extern ConVarT<bool, false> name

#define EXTERN_CONSOLE_INT_VAL(name) extern ConVarT<int, 1> name

#define EXTERN_CONSOLE_FLOAT_VAL_MINMAX(name) extern ConVarT<float, 1> name

EXTERN_CONSOLE_BOOL_VAL(resolveHZBBeforeDynamic);
EXTERN_CONSOLE_BOOL_VAL(use_occlusion_for_shadows);
EXTERN_CONSOLE_BOOL_VAL(use_occlusion_for_gpu_objs);

EXTERN_CONSOLE_BOOL_VAL(use_occlusion);
EXTERN_CONSOLE_BOOL_VAL(prepass);

EXTERN_CONSOLE_BOOL_VAL(shouldRenderWaterRipples);

EXTERN_CONSOLE_FLOAT_VAL_MINMAX(subdivCellSize);

EXTERN_CONSOLE_BOOL_VAL(async_animchars_main);

extern int globalFrameBlockId;

void start_async_animchar_main_render(const Frustum &fr, uint32_t hints, TexStreamingContext texCtx);


void WorldRenderer::renderGroundForward()
{
  G_ASSERT(isForwardRender());

  if (!lmeshMgr)
    return;
  if (!clipmap)
    initClipmap();

  TIME_D3D_PROFILE(render_ground_forward);

  ShaderGlobal::set_int(autodetect_land_selfillum_enabledVarId,
    autodetect_land_selfillum_colorVarId >= 0 ? ShaderGlobal::get_color4(autodetect_land_selfillum_colorVarId).a > 0.0f : 0);
  set_lmesh_rendering_mode(LMeshRenderingMode::RENDERING_LANDMESH);

  waitGroundVisibility();
  lmeshRenderer->setUseHmapTankSubDiv(displacementSubDiv);
  lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_WITH_CLIPMAP, cullingDataMain, ::grs_cur_view.pos);
}

void WorldRenderer::renderStaticOpaqueForward(const TMatrix &itm)
{
  TIME_D3D_PROFILE(static_opaque_forward)

  const Point3 viewPos = itm.getcol(3);

  struct ScopedUAVFeedback
  {
    Clipmap *clipmap;
    ScopedUAVFeedback(Clipmap *in_clipmap) : clipmap(in_clipmap)
    {
      if (clipmap)
        clipmap->startUAVFeedback();
    }
    ~ScopedUAVFeedback()
    {
      if (clipmap)
        clipmap->endUAVFeedback();
    }
  };

  {
    ScopedUAVFeedback UAVFeedback(clipmap);
    renderGroundForward();
    renderStaticSceneOpaque(RENDER_MAIN, viewPos, itm, currentFrameCamera.jitterFrustum);
  }
}

void WorldRenderer::renderStaticDecalsForward(
  ManagedTexView depth, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point3 &camera_world_pos)
{
  TIME_D3D_PROFILE(decals_on_static_forward)

  // we can't break RP on VR platform
  if (hasFeature(FeatureRenderFlags::DECALS))
  {
    d3d::set_depth(depth.getTex2D(), DepthAccess::SampledRO);
    d3d::resource_barrier({depth.getTex2D(), RB_RO_CONSTANT_DEPTH_STENCIL_TARGET | RB_STAGE_PIXEL, 0, 0});

    g_entity_mgr->broadcastEventImmediate(OnRenderDecals(view_tm, proj_tm, camera_world_pos));

    d3d::set_depth(depth.getTex2D(), DepthAccess::RW);
  }

  auto uiScenes = uirender::get_all_scenes();
  for (darg::IGuiScene *scn : uiScenes)
    darg_panel_renderer::render_panels_in_world(*scn, currentFrameCamera.viewItm.getcol(3), darg_panel_renderer::RenderPass::GBuffer);
}

void WorldRenderer::renderDynamicOpaqueForward(const TMatrix &itm)
{
  TIME_D3D_PROFILE(dynamic_opaque_forward)

  const Point3 viewPos = itm.getcol(3);
  renderDynamicOpaque(RENDER_MAIN, itm, currentFrameCamera.viewTm, currentFrameCamera.jitterProjTm, viewPos);

  extern void render_grass();
  render_grass();
}

void WorldRenderer::ctorForward()
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");

  // we can't conflicts slots & RT/DS targets, as we using BB
  // and also it breaks bindings of global frame block, so relaxed slot clearing
  acesfx::use_relaxed_tex_slot_clears();

  resolveHZBBeforeDynamic.set(false);
  use_occlusion_for_shadows.set(false);
  use_occlusion_for_gpu_objs.set(false);

  if (VariableMap::isGlobVariablePresent(compatibility_modeVarId))
    ShaderGlobal::set_int(compatibility_modeVarId, hasFeature(FeatureRenderFlags::FULL_DEFERRED) ? 0 : 1);

  subdivCellSize.set(graphicsBlk->getReal("hmapSubdivCellSize", subdivCellSize.get()));

  G_ASSERT(d3d::get_driver_desc().shaderModel >= 5.0_sm);
  riOcclusionData = rendinst::createOcclusionData();

  getShaderBlockIds();

  prepass.set(false);

  enviProbe = NULL;

  rendinst::render::tmInstColored = false; // rendinst::render::tmInstColored = true; it is used only on one stone in landing
  rendinst::rendinstClipmapShadows = false;
  rendinst::rendinstGlobalShadows = true;

  rendinst::set_billboards_vertical(false);

  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading(); // or it will crash here

  underWater.init("underwater_fog");
  const GuiControlDescWebUi landDesc[] = {
    DECLARE_BOOL_BUTTON(land_panel, generate_shore, false),
  };
  de3_webui_build(landDesc);

  {
    d3d::GpuAutoLock gpuLock;
    initResetable();

    if (::dgs_get_game_params()->getBool("use_da_skies", true))
      init_daskies();
    if (get_daskies())
      setSkies(get_daskies());
    else
    {
      ShaderGlobal::set_color4(get_shader_variable_id("skies_froxels_resolution", true), 1, 1, 1, 1);
      ShaderGlobal::set_real(get_shader_variable_id("clouds_thickness2", true), 1);
    }
  }

  if (::dgs_get_game_params()->getBool("use_fluid_wind", true))
    init_fluid_wind();

  clipmap_decals_mgr::init();

  occlusionLandMeshManager = eastl::make_unique<OcclusionLandMeshManager>();

  initOverrideStates();
  init_and_get_hash_128_noise();
  Point3 min_r, max_r;
  init_and_get_perlin_noise_3d(min_r, max_r);
  shouldRenderWaterRipples.set(false);

  updateImpostorSettings();
  resetLatencyMode();
  resetPerformanceMetrics();
  resetDynamicQuality();

  if (current_occlusion)
  {
    uint32_t occlWidth, occlHeight;
    current_occlusion->getMaskedResolution(occlWidth, occlHeight);
    occlusionRasterizer = eastl::make_unique<ParallelOcclusionRasterizer>(occlWidth, occlHeight);
  }

  if (hasFeature(FeatureRenderFlags::MOBILE_DEFERRED))
    mobileRp = MobileDeferredResources(get_frame_render_target_format(), get_gbuffer_depth_format(),
      hasFeature(FeatureRenderFlags::MOBILE_SIMPLIFIED_MATERIALS));

  createNodesForward();
  dynmodel_renderer::init_dynmodel_rendering(ShadowsManager::CSM_MAX_CASCADES);
}

using NodeHandleFactory = dabfg::NodeHandle (*)();
template <NodeHandleFactory... CreateNodes>
inline void emplace_back_to(eastl::vector<dabfg::NodeHandle> &nodes)
{
  (nodes.emplace_back(CreateNodes()), ...);
}

static dabfg::NodeHandle mk_after_world_render_node()
{
  return dabfg::register_node("after_world_render_node", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
    registry.orderMeAfter("finalize_frame_mobile");
#if DAGOR_DBGLEVEL > 0
    registry.orderMeAfter("debug_render_mobile");
#endif
    registry.executionHas(dabfg::SideEffects::External);
    return [] {};
  });
}

void WorldRenderer::createNodesForward()
{
  fgNodeHandles.clear();
  resSlotHandles.clear();

  if (isMobileDeferred())
  {
    emplace_back_to<makeGenAimRenderingDataNode, mk_opaque_setup_mobile_node, mk_opaque_begin_rp_node, mk_opaque_mobile_node,
      mk_opaque_resolve_mobile_node, mk_opaque_end_rp_mobile_node, mk_decals_mobile_node, mk_panorama_apply_mobile_node>(
      fgNodeHandles);
  }
  else
  {
    emplace_back_to<mk_opaque_setup_forward_node, mk_static_opaque_forward_node, mk_rename_depth_opaque_forward_node,
      mk_dynamic_opaque_forward_node, mk_decals_on_dynamic_forward_node, mk_panorama_apply_forward_node>(fgNodeHandles);
  }

  emplace_back_to<mk_after_world_render_node, mk_water_prepare_mobile_node, mk_water_mobile_node, mk_under_water_fog_mobile_node,
    mk_frame_data_setup_node, makeTransparentEcsNode, makeRendinstTransparentNode, makeTransparentSceneLateNode, makeMainHeroTransNode,
    makeTranslucentInWorldPanelsNode, mk_transparent_particles_mobile_node, mk_transparent_effects_setup_mobile_node,
    mk_occlusion_preparing_mobile_node, mk_postfx_target_producer_mobile_node, mk_postfx_mobile_node, mk_finalize_frame_mobile_node,
#if DAGOR_DBGLEVEL > 0
    mk_debug_render_mobile_node,
#endif
    mk_panorama_prepare_mobile_node>(fgNodeHandles);
}

void WorldRenderer::initResetableForward()
{
  initPostFx();
  initUiBlurFallback();

  cube.reset(new RenderDynamicCube());

  setResolutionForward();
  initRendinstVisibility();
}

void WorldRenderer::setResolutionForward()
{
  int w, h;
  getDisplayResolution(w, h);

  createDebugTexOverlay(w, h);

  // loadAntiAliasingSettings
  {
    const DataBlock *videoBlk = ::dgs_get_settings()->getBlockByNameEx("video");
    currentAntiAliasingMode = static_cast<AntiAliasingMode>(videoBlk->getInt("antiAliasingMode", int(AntiAliasingMode::OFF)));

    if (videoBlk->getBool("forceMSAA", false))
      currentAntiAliasingMode = AntiAliasingMode::MSAA;

    if (currentAntiAliasingMode != AntiAliasingMode::OFF && currentAntiAliasingMode != AntiAliasingMode::MSAA)
      DAG_FATAL("%d is not supported with forward rendering!", int(currentAntiAliasingMode));

    debug("Selected antialiasing mode is %d", int(currentAntiAliasingMode));
  }

  IPoint2 displayResolution(w, h);
  IPoint2 postFxResolution(w, h);

  getPostFxInternalResolution(postFxResolution.x, postFxResolution.y);

  dynamicResolution.reset();

  IPoint2 renderingResolution = postFxResolution;
  w = renderingResolution.x;
  h = renderingResolution.y;

  uint32_t rtFmt = get_frame_render_target_format();
  renderFramePool = ResizableRTargetPool::get(w, h, rtFmt | TEXCF_RTARGET | TEXCF_UNORDERED, 1);

  target.reset();

  setPostFxResolution(postFxResolution.x, postFxResolution.y);
  g_entity_mgr->broadcastEventImmediate(
    SetResolutionEvent(SetResolutionEvent::Type::SETTINGS_CHANGED, displayResolution, renderingResolution, postFxResolution));
  initSubDivSettings();

  initTarget();


  setFxQuality();

  ShaderGlobal::set_color4(rendering_resVarId, w, h, 1.0f / w, 1.0f / h);

  int halfW = w / 2, halfH = h / 2;
  ShaderGlobal::set_color4(lowres_rt_paramsVarId, halfW, halfH, 0, 0);

  ShaderGlobal::set_color4(lowres_tex_sizeVarId, w / 2, h / 2, 1.0f / (w / 2), 1.0f / (h / 2));

  ShaderGlobal::set_texture(water_refraction_texVarId, BAD_TEXTUREID);

  if (use_occlusion.get())
  {
    storeDownsampledTexturesInEsram = h <= 944;
    downsampledTexturesMipCount = max(min(get_log2w(w / 2), get_log2w(h / 2)) - 1, 4);
    ShaderGlobal::set_int(downsampled_depth_mip_countVarId, downsampledTexturesMipCount);
  }

  resetBackBufferTex();
  if (d3d::get_backbuffer_tex() != nullptr)
  {
    rtControl = RtControl::BACKBUFFER;
    updateBackBufferTex();
  }
  else
  {
    rtControl = RtControl::OWNED_RT;
    ownedBackbufferTex = dag::create_tex(NULL, displayResolution.x, displayResolution.y, TEXCF_RTARGET, 1, "final_target_frame");

    setFinalTargetTex(&ownedBackbufferTex);
  }

  ShaderGlobal::set_int(get_shader_glob_var_id("fsr_on", true), 0);

  G_ASSERT(target);
  changeStateFOMShadows();

  g_entity_mgr->broadcastEventImmediate(
    SetResolutionEvent(SetResolutionEvent::Type::SETTINGS_CHANGED, displayResolution, renderingResolution, postFxResolution));

  auto vrsDims = getDimsForVrsTexture(w, h);
  dabfg::set_resolution("main_view", IPoint2{w, h});
  dabfg::set_resolution("display", displayResolution);
  dabfg::set_resolution("texel_per_vrs_tile", vrsDims ? vrsDims.value() : IPoint2());
}

void WorldRenderer::initMobileTerrainSettings() const
{
  const eastl::string_view q = dgs_get_settings()->getBlockByNameEx("graphics")->getStr("mobileTerrainQuality", "medium");
  if (q == "minimum")
    setMobileTerrainQuality(MobileTerrainQuality::Minimum);
  else if (q == "low")
    setMobileTerrainQuality(MobileTerrainQuality::Low);
  else if (q == "medium")
    setMobileTerrainQuality(MobileTerrainQuality::Medium);
  else if (q == "high")
    setMobileTerrainQuality(MobileTerrainQuality::High);
}

void WorldRenderer::setMobileTerrainQuality(const MobileTerrainQuality quality) const
{
  bool simplifiedMicroDetails = false;
  bool fakeVertNormals = false;
  bool fullRender = false;
  bool puddle = false;

  switch (quality)
  {
    case MobileTerrainQuality::Minimum:
    {
      simplifiedMicroDetails = true;
      fakeVertNormals = true;
      fullRender = false;
      puddle = false;
      break;
    }
    case MobileTerrainQuality::Low:
    {
      simplifiedMicroDetails = false;
      fakeVertNormals = true;
      fullRender = false;
      puddle = false;
      break;
    }
    case MobileTerrainQuality::Medium:
    {
      simplifiedMicroDetails = false;
      fakeVertNormals = false;
      fullRender = true;
      puddle = false;
      break;
    }
    case MobileTerrainQuality::High:
    {
      simplifiedMicroDetails = false;
      fakeVertNormals = false;
      fullRender = true;
      puddle = true;
      break;
    }
  }

  ShaderGlobal::set_int(get_shader_variable_id("micro_details_quality", true), simplifiedMicroDetails ? 0 : 1);
  ShaderGlobal::set_int(get_shader_variable_id("hmap_use_fake_vert_normals", true), fakeVertNormals);
  ShaderGlobal::set_int(get_shader_variable_id("hmap_use_full_render", true), fullRender);
  ShaderGlobal::set_int(get_shader_variable_id("hmap_puddle_enabled", true), puddle);
}

void WorldRenderer::initMobileShadowSettings() const
{
  const eastl::string_view q = dgs_get_settings()->getBlockByNameEx("graphics")->getStr("mobileShadowsQuality", "medium");

  const int varId = get_shader_variable_id("mobile_deferred_shadow_quality");

  if (q == "off")
    ShaderGlobal::set_int(varId, (int)MobileStaticShadowQuality::Off);
  else if (q == "medium")
    ShaderGlobal::set_int(varId, (int)MobileStaticShadowQuality::Fast);
  else if (q == "high")
    ShaderGlobal::set_int(varId, (int)MobileStaticShadowQuality::FXAA);
}

void WorldRenderer::initMobileShadersSettings() const
{
  const eastl::string_view q = dgs_get_settings()->getBlockByNameEx("graphics")->getStr("mobileShadersQuality", "high");

  const int varId = get_shader_variable_id("mobile_simplified_materials");
  ShaderGlobal::set_int(varId, q == "medium" ? 1 : 0);
}
