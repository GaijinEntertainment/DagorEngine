// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "render/world/antiAliasingMode.h"
#define INSIDE_RENDERER 1 // fixme: move to jam

#include "render/renderEvent.h"
#include <render/lruCollision.h>
#include <gameRes/dag_collisionResource.h>
#include <render/shaderCacheWarmup/shaderCacheWarmup.h>
#include <render/antialiasing.h>
#include <daGI2/treesAboveDepth.h>
#include <daGI2/daGI2.h>
#include <GIWindows/GIWindows.h>

#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/gpuObjects.h>
#include <rendInst/visibility.h>

#include <landMesh/landRayTracer.h>
#include <landMesh/clipMap.h>
#include <landMesh/lastClip.h>
#include <landMesh/biomeQuery.h>
#include <landMesh/heightmapQuery.h>
#include <landMesh/lmeshMirroring.h>
#include <landMesh/lmeshRenderer.h>
#include <render/world/landMeshToHeightmapRenderer.h>

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <ecs/render/updateStageRender.h>
#include <ecs/camera/getActiveCameraSetup.h>
#include <ecs/lights/light.h>

#include <osApiWrappers/dag_messageBox.h>
#include <osApiWrappers/dag_direct.h>

#include <3d/dag_lowLatency.h>
#include <3d/dag_texMgrTags.h>
#include <3d/dag_gpuConfig.h>
#include <drv/3d/dag_rwResource.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_shaderConstants.h>
#include <render/dag_cur_view.h>
#include <workCycle/dag_workCycle.h> // dagor_work_cycle_is_need_to_draw

#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_renderScene.h>
#include <shaders/dag_rendInstRes.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_shaderVarsUtils.h>
#include <shaders/dag_shAssert.h>

#include <visualConsole/dag_visualConsole.h>

#include "aimRender.h"

#include <util/dag_delayedAction.h>
#include <util/dag_localization.h>
#include <util/dag_parseResolution.h>

#include <render/cables.h>

#include <webui/editVarPlugin.h>
#include <streaming/dag_streamingBase.h>
#include "game/player.h"
#include "render/skies.h"
#include "main/gameLoad.h"
#include "main/settings.h"
#include "main/main.h"
#include "main/level.h"
#include "main/water.h"
#include "main/version.h"

#include <fx/dag_commonFx.h>

#include <render/debugTexOverlay.h>
#include <render/debugTonemapOverlay.h>
#include <render/preIntegratedGF.h>
#include <render/downsampleDepth.h>
#include <render/screenSpaceReflections.h>
#include <render/rendererFeatures.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <render/volumetricLights/volumetricLights.h>
#include <render/weather/fluidWind.h>
#include <render/hdrRender.h>
#include <render/emissionColorMaps.h>
#include <render/dynamicQuality.h>
#include <render/dynamicResolution.h>
#include <render/cinematicMode.h>
#include <render/vertexDensityOverlay.h>
#include <render/imGuiProfiler.h>
#include <render/tdrGpu.h>

#include <gamePhys/phys/rendinstDestr.h>


#include <webui/shaderEditors.h>

#include "private_worldRenderer.h"
#include "global_vars.h"
#include "depthBounds.h"

#include "render/screencap.h"

#include "frameGraphNodes/frameGraphNodes.h"
#include <render/world/frameGraphHelpers.h>

#include <render/deepLearningSuperSampling.h>
#include <render/XeSuperSampling.h>
#include <render/FSR.h>
#include <render/temporalSuperResolution.h>
#if _TARGET_C2

#endif
#include <render/lightProbeSpecularCubesContainer.h>
#include <scene/dag_visibility.h>
#include <scene/dag_tiledScene.h> //for destructor
#include <render/indoorProbeManager.h>
#include <eventLog/eventLog.h>
#include <breakpad/binder.h>
#include <json/json.h>
#include <main/appProfile.h>

#include <render/gaussMipRenderer.h>
#include <camera/sceneCam.h>

#include <gui/dag_visualLog.h>
#include <render/noiseTex.h>
#include <render/world/dynModelRenderer.h>
#include <render/clipmapDecals.h>
#include <render/grass/grassRender.h>
#include <render/lightShadowParams.h>

#include <3d/dag_ICrashFallback.h>
#include <3d/dag_profilerTracker.h>
#include <3d/dag_texPackMgr2.h>
#include <render/resolution.h>
#include <render/renderSettings.h>

#include <daRg/bhvFpsBar.h>
#include <daRg/bhvLatencyBar.h>
#include <frameTimeMetrics/aggregator.h>
#include <render/resourceSlot/resolveAccess.h>

#include <render/debugMesh.h>

#include <ui/uiRender.h>

#include <gui/dag_stdGuiRender.h>
#include <render/debugBoxRenderer.h>
#include <render/world/cameraInCamera.h>
#include <render/world/dynModelRenderPass.h>
#include "render/world/overridden_params.h"
#include "render/world/worldRendererQueries.h"
#include "render/world/wrDispatcher.h"
#include "render/world/defaultVrsSettings.h"
#include "render/world/renderDynamicCube.h"
#include "render/world/reprojectionTm.h"
#include "render/world/depthAOAbove.h"
#include "render/world/renderPrecise.h"
#include "render/world/dynamicShadowRenderExtender.h"
#include "render/world/sunParams.h"
#include <render/viewVecs.h>

#include <ecs/anim/animchar_visbits.h>
#include <perfMon/dag_cpuFreq.h>
#include <frustumCulling/frustumPlanes.h>

#include <render/indoorProbeScenes.h>
#include <render/indoorProbeNodes.h>
#include <render/debugLightProbeShapeRenderer.h>
#include <render/debugCollisionDensityRenderer.h>

#include <render/world/portalRenderer.h>

#include <render/texDebug.h>
#include <render/levelProfiler.h>
#include <render/driverNetworkManager.h>
#include <render/world/bvh.h>
#include <render/priorityManagedShadervar.h>
#include <render/psoCacheLoader/psoCacheLoader.h>
#include <fftWater/fftWater.h>
#include <gpuMemoryDumper/gpuMemoryDumper.h>
#include <render/enviCover/enviCover.h>
#include <main/settingsValidator.h>

#include <quirrel/bindQuirrelEx/autoBind.h>
#include <ecs/scripts/sqBindEvent.h>
#include <sqrat.h>

#if _TARGET_ANDROID || _TARGET_IOS
#include <crashlytics/firebase_crashlytics.h>
#endif

extern void init_fx();
extern void term_fx();
extern void add_volfog_optional_graphs();
extern void add_envi_cover_optional_graphs();
extern void set_nightly_spot_lights();

void compute_csm_visibility(const Occlusion &occlusion, const Point3 &dir_from_sun);
void update_csm_length(const Frustum &frustum, const Point3 &dir_from_sun, float csm_shadows_max_dist);
bool can_change_altitude_unexpectedly();
void get_underground_zones_data(Tab<Point3_vec4> &bboxes);
float get_default_static_resolution_scale();

#define DEF_RENDER_EVENT      ECS_REGISTER_EVENT
#define DEF_RENDER_PROF_EVENT ECS_REGISTER_EVENT
DEF_RENDER_PROFILE_EVENTS
DEF_RENDER_EVENTS
#undef DEF_RENDER_PROF_EVENT
#undef DEF_RENDER_EVENT
ECS_REGISTER_EVENT(AfterDeviceReset)

#define VAR(a) ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_LIST
GLOBAL_VARS_OPTIONAL_LIST
#undef VAR

namespace var
{
ShaderVariableInfo sky_polarization_filter_term("sky_polarization_filter_term", true);
}

SQ_DEF_AUTO_BINDING_MODULE_EX(bind_render_events, "renderEvents", sq::VM_UI_ALL)
{
  Sqrat::Table tbl = ecs::sq::EventsBind<AfterDeviceReset>::bindall(vm);

  return tbl;
}

extern eastl::unique_ptr<DebugBoxRenderer> debugBoxRenderer;
extern bool grs_draw_wire;
extern uint32_t global_frame_id;
enum
{
  NUM_ROUGHNESS = 10,
  NUM_METALLIC = 10
};

static AntiAliasingMode convertAntialiasingMethod(render::antialiasing::AntialiasingMethod method)
{
  using render::antialiasing::AntialiasingMethod;
  switch (method)
  {
    case AntialiasingMethod::None:
    case AntialiasingMethod::MSAA:
    case AntialiasingMethod::MOBILE_MSAA: return AntiAliasingMode::OFF;
    case AntialiasingMethod::FXAALow:
    case AntialiasingMethod::FXAAHigh:
    case AntialiasingMethod::SMAA: return AntiAliasingMode::FXAA;
    case AntialiasingMethod::DLSS: return AntiAliasingMode::DLSS;
    case AntialiasingMethod::XeSS: return AntiAliasingMode::XESS;
    case AntialiasingMethod::FSR: return AntiAliasingMode::FSR;
    case AntialiasingMethod::TSR:
    case AntialiasingMethod::TAA:
    case AntialiasingMethod::MobileTAA:
    case AntialiasingMethod::MobileTAALow:
    case AntialiasingMethod::SGSR:
    case AntialiasingMethod::SGSR2:
    case AntialiasingMethod::METALFX: return AntiAliasingMode::TSR;
#if _TARGET_C2

#endif
    case AntialiasingMethod::SSAA: return AntiAliasingMode::SSAA;
  }

  return AntiAliasingMode::OFF;
}

static constexpr int DISPLACEMENT_TEX_SIZE = 2048;
static constexpr float DISPLACEMENT_DIST = 16.0f;

extern void set_add_lod_bias(float add, const char *name); // TODO: This should NOT be here
extern void set_add_lod_bias_cb(void (*cb)());
extern void reset_bindless_samplers();

ShaderBlockIdHolder globalConstBlockId{"global_const_block"};
ShaderBlockIdHolder globalFrameBlockId{"global_frame"};
ShaderBlockIdHolder rendinstDepthSceneBlockId{"rendinst_depth_scene"};
ShaderBlockIdHolder rendinstTransSceneBlockId{"rendinst_trans_scene"};
ShaderBlockIdHolder rendinstVoxelizeSceneBlockId{"rendinst_voxelize_scene"};
ShaderBlockIdHolder dynamicSceneBlockId{"dynamic_scene"};
ShaderBlockIdHolder dynamicDepthSceneBlockId{"dynamic_depth_scene"};
ShaderBlockIdHolder dynamicSceneTransBlockId{"dynamic_trans_scene"};
ShaderBlockIdHolder landMeshPrepareClipmapBlockId{"land_mesh_prepare_clipmap"};
ShaderBlockIdHolder water3dBlockId{"water3d_block"};

CONSOLE_BOOL_VAL("render", hide_heightmap_on_planes, false);
CONSOLE_BOOL_VAL("render", no_sun, false);
CONSOLE_BOOL_VAL("render", vehicle_cockpit_in_depth_prepass, true);

CONSOLE_BOOL_VAL("render", volfog_enabled, true);

CONSOLE_BOOL_VAL("distant_fog", use_checkerboard_reprojection, true);

CONSOLE_BOOL_VAL("render", prepass, true);
// assume total of 64 lights visible in clusters
CONSOLE_INT_VAL("render", dynamic_lights_initial_count, 64, 0, 1024);
CONSOLE_BOOL_VAL("render", dynamic_lights, true);

CONSOLE_BOOL_VAL("render", async_animchars_shadows, true);
CONSOLE_BOOL_VAL("render", async_animchars_main, true);
CONSOLE_BOOL_VAL("render", async_riex_opaque, true);

CONSOLE_BOOL_VAL("occlusion", stop_occlusion, false);
CONSOLE_BOOL_VAL("occlusion", use_occlusion, true);
extern ConVarB sw_occlusion;
CONSOLE_BOOL_VAL("occlusion", resolveHZBBeforeDynamic, true);

extern ConVarI volfog_force_invalidate;

CONSOLE_FLOAT_VAL_MINMAX("render", taa_base_mip_scale, 0.0f, -5, 4);

CONSOLE_FLOAT_VAL("render", probes_sky_prepare_altitude_tolerance, 100.0f);

CONSOLE_INT_VAL("render", waterAnisotropy, 2, 0, 5);

CONSOLE_FLOAT_VAL_MINMAX("render", subdivCellSize, 0.16, 0.02, 1);

CONSOLE_BOOL_VAL("render", vrs_dof, true);
enum
{
  MAX_SUBDIV = 5,
  MAX_SETTINGS_SUBDIV = 2
};

CONSOLE_BOOL_VAL("render", vrsEnable, true);

CONSOLE_INT_VAL("render", water_quality, 0, 0, 2);
CONSOLE_BOOL_VAL("render", use_occlusion_for_shadows, true);
CONSOLE_BOOL_VAL("render", use_occlusion_for_gpu_objs, true);

CONSOLE_INT_VAL("render", ssr_quality, 0, 0, 2);
CONSOLE_BOOL_VAL("render", ssr_enable, true);
CONSOLE_BOOL_VAL("render", ssr_fullres, false);
CONSOLE_INT_VAL("render", ssr_denoiser, 0, 0, 2);

CONSOLE_FLOAT_VAL("render", switch_ri_gen_mode_threshold, 2000);

// by default each 2meters per second results in 1m of additional height
CONSOLE_FLOAT_VAL_MINMAX("render", camera_speed_to_additional_height, 0.5f, -1000.f, 5.f);
CONSOLE_FLOAT_VAL("render", fast_move_camera_height_offset, 21.0f);
CONSOLE_BOOL_VAL("skies", sky_is_unreachable, true);

CONSOLE_BOOL_VAL("render", immediate_flush, false);
CONSOLE_BOOL_VAL("render", use_24bit_depth, false);

#if DAGOR_DBGLEVEL > 0
CONSOLE_BOOL_VAL("fg", test_dynamic_node_allocation, false);
#else
static constexpr bool test_dynamic_node_allocation = false;
#endif

CONSOLE_FLOAT_VAL_MINMAX("skies", sun_direction_update_threshold_deg, 1, 0, 180);
CONSOLE_FLOAT_VAL_MINMAX("skies", dynamic_time_scale, 0, -10000, 10000);

CONSOLE_INT_VAL("render", force_sub_pixels, 0, 0, 4);
CONSOLE_INT_VAL("render", force_super_pixels, 0, 0, 4);

CONSOLE_BOOL_VAL("render", sort_transparent_riex_instances, false);

CONSOLE_BOOL_VAL("render", envi_cover_disable_fg_nodes, false);
CONSOLE_BOOL_VAL("render", envi_cover_use_NBS, false);
CONSOLE_BOOL_VAL("render", toggle_normal_packing, false);

CONSOLE_BOOL_VAL("render", camcam_force_world_fov, false);

CONSOLE_BOOL_VAL("render", process_reactive_gbuffer_data, false);

inline bool is_gbuffer_cascade(int cascade) { return cascade == RENDER_MAIN || cascade == RENDER_CUBE; }
static String root_fog_graph;
static String root_envi_cover_graph;
#if DA_PROFILER_ENABLED
static carray<da_profiler::desc_id_t, CascadeShadows::MAX_CASCADES> animchar_csm_desc;
#endif

#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
webui::HttpPlugin http_renderer_plugins[128] = {nullptr};
#define HTTP_RENDERER_PLUGINS_SLICE make_span(http_renderer_plugins)
#else
#define HTTP_RENDERER_PLUGINS_SLICE dag::Span<webui::HttpPlugin>()
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT

namespace var
{
static ShaderVariableInfo layered_material_detail_quality("layered_material_detail_quality", true);
static ShaderVariableInfo packed_gbuf_normals("packed_gbuf_normals");
static ShaderVariableInfo packed_gbuf_1_tex("packed_gbuf_1_tex");
} // namespace var

uint32_t get_water_quality()
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const char *waterQuality = graphicsBlk->getStr("waterQuality", "low");
  return stricmp(waterQuality, "high") == 0 ? 2 : (stricmp(waterQuality, "medium") == 0 ? 1 : 0);
}

float get_mipmap_bias()
{
  if (get_world_renderer())
    return static_cast<WorldRenderer *>(get_world_renderer())->getMipmapBias();
  else
    return 0.f;
}

static void set_water_quality_from_settings() { water_quality.set(get_water_quality()); }

void WorldRenderer::startRenderLightProbe(const Point3 &view_pos, const TMatrix &view_tm, const TMatrix4 &proj_tm)
{
  if (volumeLight)
    volumeLight->switchOff();
  if (!hqProbesReflection)
    shaders::overrides::set(nocullState);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  if (get_daskies())
    get_daskies()->useFogNoScattering(view_pos, cube_pov_data, view_tm, proj_tm, UpdateSky::On,
      probes_sky_prepare_altitude_tolerance.get());
}

void WorldRenderer::endRenderLightProbe()
{
  if (volumeLight)
    volumeLight->switchOn();
  if (!hqProbesReflection)
    shaders::overrides::reset();
}

class LandmeshCMRenderer : public ClipmapRenderer
{
public:
  LandMeshRenderer &renderer;
  LandMeshManager &provider;
  float minZ, maxZ;
  LMeshRenderingMode omode;
  shaders::UniqueOverrideStateId flipCullStateId;
  ViewProjMatrixContainer oviewproj;
  TMatrix viewTm;

  LandmeshCMRenderer(LandMeshRenderer &r, LandMeshManager &p) :
    renderer(r), provider(p), minZ(-5), maxZ(100), omode(LMeshRenderingMode::RENDERING_LANDMESH)
  {
    shaders::OverrideState state;
    state.set(shaders::OverrideState::FLIP_CULL);
    flipCullStateId.reset(shaders::overrides::create(state));
    viewTm = TMatrix::IDENT;
    viewTm.setcol(0, 1, 0, 0);
    viewTm.setcol(1, 0, 0, 1);
    viewTm.setcol(2, 0, 1, 0);
  }

  virtual void startRenderTiles(const Point2 &center)
  {
    Point3 pos(center.x, ::grs_cur_view.pos.y, center.y);
    ((WorldRenderer *)get_world_renderer())->getMinMaxZ(minZ, maxZ);
    BBox3 landBox = provider.getBBox();
    maxZ = max(maxZ, landBox[1].y + 1);

    omode = renderer.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
    renderer.prepare(provider, pos, 2.f);
    d3d_get_view_proj(oviewproj);
    d3d::settm(TM_VIEW, viewTm);
    shaders::overrides::set(flipCullStateId);
  }
  virtual void endRenderTiles()
  {
    renderer.setRenderInBBox(BBox3());
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    renderer.setLMeshRenderingMode(omode);
    d3d_set_view_proj(oviewproj);
    shaders::overrides::reset();
  }

  virtual void renderTile(const BBox2 &region)
  {
    TMatrix4 proj;
    proj = matrix_ortho_off_center_lh(region[0].x, region[1].x, region[1].y, region[0].y, minZ, maxZ);
    d3d::settm(TM_PROJ, &proj);
    BBox3 region3(Point3(region[0].x, minZ, region[0].y), Point3(region[1].x, maxZ + 500, region[1].y));
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(landMeshPrepareClipmapBlockId, ShaderGlobal::LAYER_SCENE);
    renderer.setRenderInBBox(region3);
    TMatrix4_vec4 globTm = TMatrix4_vec4(viewTm) * proj;
    renderer.render(reinterpret_cast<mat44f_cref>(globTm), proj, Frustum{globTm}, provider, renderer.RENDER_CLIPMAP,
      ::grs_cur_view.pos);
    clipmap_decals_mgr::render(false);

    ViewProjMatrixContainer viewProj;
    d3d_get_view_proj(viewProj);
    g_entity_mgr->broadcastEventImmediate(OnClipmapTileRender((Frustum(TMatrix4(viewProj.getViewTm()) * viewProj.getProjTm()))));
  }
  virtual void renderFeedback(const TMatrix4 & /*globtm*/) { G_ASSERT(0); }
};

class FeedbackRenderer : public ClipmapRenderer
{
public:
  virtual void startRenderTiles(const Point2 &) { G_ASSERT(0); }
  virtual void endRenderTiles() { G_ASSERT(0); }
  virtual void renderTile(const BBox2 &) {}
  virtual void renderFeedback(const TMatrix4 & /*globtm*/) { G_ASSERT(0); }
};

void WorldRenderer::clearLegacyGPUObjectsVisibility()
{
  if (legacyGPUObjectsVisibilityInitialized)
  {
    rendinst::gpuobjects::disable_for_visibility(mainCameraVisibilityMgr.getRiMainVisibility());
    if (hasFeature(CAMERA_IN_CAMERA))
      rendinst::gpuobjects::disable_for_visibility(camcamVisibilityMgr.getRiMainVisibility());
    shadowsManager.disableGPUObjsCSM();
    rendinst::gpuobjects::disable_for_visibility(rendinst_dynamic_shadow_visibility);
    legacyGPUObjectsVisibilityInitialized = false;
  }
}

void WorldRenderer::closeNBSShaders()
{
  if (volumeLight)
    volumeLight->closeShaders();

  if (enviCover)
  {
    enviCover->setValidNBS(false);
    envi_cover_use_NBS.set(false);
    enviCover->closeShaders();
  }
}
void WorldRenderer::beforeResetNBS()
{
  if (volumeLight)
    volumeLight->beforeReset();

  if (enviCover)
    enviCover->beforeReset();
}

void WorldRenderer::afterResetNBS()
{
  if (volumeLight)
    volumeLight->afterReset();

  if (enviCover)
    enviCover->afterReset();
}

void WorldRenderer::unloadLevel()
{
  // Should be first, because BVH holds texture references to items released below
  g_entity_mgr->broadcastEventImmediate(UnloadLevel());

  staticSceneCollisionResource.reset();
  shadowsManager.resetShadowsVisibilityTesting();
  clearLegacyGPUObjectsVisibility();

  if (auto *skies = get_daskies())
    skies->closePanorama();

  cloudsVolume.close();

  closeNBSShaders();

  closeWaterPlanarReflection();

  lowresWaterHeightmap.close();
  clipmap_decals_mgr::clear();
  binSceneBbox.setempty();
  closeGround();
  closeClipmap();
  closeDeforms();
  closeDeformHeightmap();
  shoreRenderer.closeShoreAndWaterFlowmap(water);
  if (water)
    fft_water::shore_enable(water, false);
  if (lruCollision)
    lruCollision->clearRiInfo();

  closeDistantHeightmap();

  binScene = NULL;
  lmeshMgr = NULL;
  water = NULL;
  if (water_ssr_id >= 0)
  {
    d3d::free_predicate(water_ssr_id);
    water_ssr_id = -1;
  }
  if (depthAOAboveCtx)
    depthAOAboveCtx->invalidateAO(true);

  indoorProbesScene.reset();
  defrag_shaders_stateblocks(true);

  globalPaintTex.close();
  localPaintTex.close();

  reset_fx_textures_used();
  paintColorsTex.close();

  requestFgRecreation("unloadLevel");
}
void WorldRenderer::setWater(FFTWater *fftWater)
{
  water = fftWater;
  waterDistant[fft_water::RenderMode::WATER_SHADER] = PostFxRenderer("water_distant");
  waterDistant[fft_water::RenderMode::WATER_DEPTH_SHADER] = PostFxRenderer("water_depth_distant");
  waterLevel = 0;
  if (water)
  {
    const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
    const char *fftWaterQualitySettingStr = graphics->getStr("fftWaterQuality", "high");
    fftWaterQualitySetting = fft_water::RENDER_GOOD;
    if (stricmp(fftWaterQualitySettingStr, "low") == 0)
      fftWaterQualitySetting = fft_water::RENDER_VERY_LOW;
    else if (stricmp(fftWaterQualitySettingStr, "medium") == 0)
      fftWaterQualitySetting = fft_water::RENDER_LOW;
    else if (stricmp(fftWaterQualitySettingStr, "high") == 0)
      fftWaterQualitySetting = fft_water::RENDER_GOOD; // -V1048
    else if (stricmp(fftWaterQualitySettingStr, "ultra") == 0)
      fftWaterQualitySetting = fft_water::RENDER_EXCELLENT;
    else
      logerr("unknown water render quality <%s>", fftWaterQualitySettingStr);

    const bool ssrEnabled = isWaterSSREnabled();
    const bool oneToFourCascades = graphics->getBool("fftWaterOneToFourCascades", false);
    fft_water::init_render(water, fftWaterQualitySetting, ssrEnabled, ssrEnabled, oneToFourCascades);
    waterLevel = fft_water::get_level(water);
    water_ssr_id = d3d::create_predicate();

    const bool ripples = graphics->getBool("shouldRenderWaterRipples", true) && hasFeature(FeatureRenderFlags::RIPPLES);

    if (ripples && hasFeature(FeatureRenderFlags::WAKE))
    {
      fft_water::WaterFlowmap *waterFlowmap = fft_water::get_flowmap(water);
      if (waterFlowmap)
      {
        waterFlowmap->usingFoamFx = use_foam_tex();
        fft_water::set_flowmap_params(water);
      }
    }

    static int wetness_paramsVarId = ::get_shader_variable_id("wetness_params", true);
    Color4 wetnessParams = ShaderGlobal::get_float4(wetness_paramsVarId);
    wetnessParams.g = waterLevel;
    ShaderGlobal::set_float4(wetness_paramsVarId, wetnessParams);
  }
  else
  {
    ShaderGlobal::set_float(water_levelVarId, waterLevel = HeightmapHeightCulling::NO_WATER_ON_LEVEL);
  }

  setupWaterQuality();

  fft_water::close_flowmap(water);
}

FFTWater *WorldRenderer::getWater() { return water; }

void WorldRenderer::setMaxWaterTessellation(int value)
{
  value = clamp(value, 0, 7);
  if (!water || maxWaterTessellation == value)
    return;

  maxWaterTessellation = value;
  fft_water::set_grid_lod0_additional_tesselation(water, water_quality > 0 ? maxWaterTessellation : 0);
}

ShoreRenderer *WorldRenderer::getShore() { return &shoreRenderer; }

bool WorldRenderer::isWaterPlanarReflectionTerrainEnabled() const { return water_quality == 2; }

bool WorldRenderer::hasWaterSSRAlternateReflections() const { return water_quality == 2; }

void WorldRenderer::setupWaterQuality()
{
  waterPlanarReflectionTerrainNode = {};
  if (!water)
    return;

  static const int water_qualityVarId = get_shader_glob_var_id("water_quality", true);
  static const int water_refraction_base_mipVarId = get_shader_glob_var_id("water_refraction_base_mip", true);
  ShaderGlobal::set_int(water_qualityVarId, water_quality);
  ShaderGlobal::set_float(water_refraction_base_mipVarId, water_quality == 2 ? 0.f : 1.f);

  fft_water::set_grid_lod0_additional_tesselation(water, water_quality > 0 ? maxWaterTessellation : 0);
  fft_water::force_actual_waves(water, water_quality > 0);
  shoreRenderer.isForcingWaterWaves = water_quality > 0;
  if (isWaterPlanarReflectionTerrainEnabled())
    initWaterPlanarReflectionTerrainNode();
}

template <typename Collection>
int parse_quality_option(const char *chosen, const Collection &options, int default_value)
{
  for (uint32_t i = 0; i < options.size(); ++i)
    if (strcmp(chosen, options[i]) == 0)
      return i;
  return default_value;
}

void WorldRenderer::resetMainSkies(DaSkies *skies)
{
  if (main_pov_data)
  {
    get_daskies()->destroy_skies_data(main_pov_data);
    main_pov_data = nullptr;
  }

  enum
  {
    SKIES_QUALITY_LOW,
    SKIES_QUALITY_MEDIUM,
    SKIES_QUALITY_HIGH,
    SKIES_QUALITY_COUNT
  };
  const eastl::array<uint32_t, SKIES_QUALITY_COUNT> qualities = {1, 1, 2};
  const eastl::array<uint32_t, SKIES_QUALITY_COUNT> scatteringDepth = {32, 48, 64};
  const eastl::array<const char *, SKIES_QUALITY_COUNT> qualityNames = {"low", "medium", "high"};

  const uint32_t skiesQuality = parse_quality_option(
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("skiesQuality", "medium"), qualityNames, SKIES_QUALITY_MEDIUM);
  const uint32_t chosenQuality = qualities[skiesQuality];

  PreparedSkiesParams mainSkiesParams;
  mainSkiesParams.panoramic = PreparedSkiesParams::Panoramic::OFF;
  mainSkiesParams.reprojection = PreparedSkiesParams::Reprojection::ON;
  mainSkiesParams.skiesLUTQuality = chosenQuality;
  mainSkiesParams.scatteringScreenQuality = chosenQuality;
  mainSkiesParams.scatteringDepthSlices = scatteringDepth[skiesQuality];
  mainSkiesParams.transmittanceColorQuality = chosenQuality;
  mainSkiesParams.scatteringRangeScale = 1;
  mainSkiesParams.minScatteringRange = 80000;
  mainSkiesParams.strataInLowres = false;

  main_pov_data = skies->createSkiesData("main_bounding", mainSkiesParams);
}

void WorldRenderer::setSkies(DaSkies *skies)
{
  G_ASSERT(!main_pov_data && !cube_pov_data);

  resetMainSkies(skies);

  PreparedSkiesParams cubicSkiesParams;
  cubicSkiesParams.panoramic = PreparedSkiesParams::Panoramic::ON;
  cubicSkiesParams.reprojection = PreparedSkiesParams::Reprojection::OFF;
  cubicSkiesParams.skiesLUTQuality = 1;
  cubicSkiesParams.scatteringScreenQuality = 1;
  cubicSkiesParams.scatteringDepthSlices = 8;
  cubicSkiesParams.transmittanceColorQuality = 1;
  cubicSkiesParams.scatteringRangeScale = 1.0f; // zfar is extremely low
  cubicSkiesParams.minScatteringRange = 80000;
  cube_pov_data = skies->createSkiesData("cube", cubicSkiesParams); // required for each point of view
                                                                    // (or the only one if panorama used, for panorama center)
}

void WorldRenderer::onSceneLoaded(BaseStreamingSceneHolder *scn)
{
  binScene = scn;
  requestFgRecreation("onSceneLoaded");

  if (scn)
  {
    BBox3 box;
    for (auto &s : scn->getRsm().getScenes())
      box += s->calcBoundingBox();
    binSceneBbox = box;
  }
  if (!water)
    ShaderGlobal::set_float(water_levelVarId, waterLevel = HeightmapHeightCulling::NO_WATER_ON_LEVEL);
  shadowsManager.shadowsInvalidate(false);
  shadowsManager.staticShadowsSetWorldSize();
  invalidateGI(true);

#if DAGOR_DBGLEVEL == 0
  Color4 validationDebugColor = params_validation_debug_colorVarId.get_float4();
  validationDebugColor.a = 0; // disable debug visualization in release
  params_validation_debug_colorVarId.set_float4(validationDebugColor);
#endif
}

void WorldRenderer::updateLevelGraphicsSettings(const DataBlock &level_blk)
{
  levelSettings.reset(new DataBlock(*(level_blk.getBlockByNameEx("graphics"))));
  update_settings_entity(levelSettings.get());
  initIndoorProbesIfNecessary();
}

void WorldRenderer::prefetchPartsOfPaintingTexture()
{
  localPaintTex = SharedTex(localPaintTexId);
  globalPaintTex = dag::get_tex_gameres("assets_color_global_tex_palette");
  G_ASSERT(globalPaintTex);
  if (!localPaintTex)
  {
    localPaintTex = globalPaintTex;
  }
  if (globalPaintTex)
  {
    prefetch_managed_texture(globalPaintTex.getTexId());
    prefetch_managed_texture(localPaintTex.getTexId());
  }
}

void WorldRenderer::generatePaintingTexture()
{
  if (paintColorsTex)
    return;
  if (!prefetch_and_check_managed_texture_loaded(localPaintTex.getTexId(), true) ||
      !prefetch_and_check_managed_texture_loaded(globalPaintTex.getTexId(), true))
    return; // return because some textures not loaded yet, otherwise predefined 1 pixel texture will be used

  StaticTab<TEXTUREID, 2> tex_ids;
  tex_ids.push_back(globalPaintTex.getTexId());
  tex_ids.push_back(localPaintTex.getTexId());
  TexPtr combinedPaintTex = texture_util::stitch_textures_horizontal(tex_ids, "combined_paint_tex",
    TEXFMT_DEFAULT | TEXCF_SRGBREAD | TEXCF_UPDATE_DESTINATION);
  if (!combinedPaintTex)
    return;
  paintColorsTex.close();
  paintColorsTex = UniqueTexHolder(eastl::move(combinedPaintTex), "paint_details_tex");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.filter_mode = d3d::FilterMode::Point;
    ShaderGlobal::set_sampler(::get_shader_variable_id("paint_details_tex_samplerstate"), d3d::request_sampler(smpInfo));
  }
  paintColorsTex.setVar();
  globalPaintTex.close();
  localPaintTex.close();
}
void WorldRenderer::onLevelLoaded(const DataBlock &level_blk)
{
#if DAGOR_DBGLEVEL > 0
  settings_validator::ensure_no_duplicates(dgs_get_settings()->getBlockByName("graphics"), "", false);
  settings_validator::ensure_no_duplicates(dgs_get_settings()->getBlockByName("video"), "", true);
#endif
  // this is workaround for race in texture manager
  if (characterMicrodetailsId != BAD_TEXTUREID)
  {
    const bool loaded = get_managed_res_cur_tql(characterMicrodetailsId) != TQL_stub;
    if (!loaded)
    {
      logerr("character_micro_details were not loaded!");
      loadCharacterMicroDetails(::dgs_get_game_params()->getBlockByName("character_micro_details"));
    }
  }

  sort_transparent_riex_instances.set(
    ::dgs_get_game_params()->getBool("rendinstExtraSortTransparentInstances", sort_transparent_riex_instances.get()));
  process_reactive_gbuffer_data.set(level_blk.getBlockByNameEx("graphics")->getBool("process_reactive_gbuffer_data", false));

  requestFgRecreation("onLevelLoaded");

  createFinalOpaqueControlNodes();

  updateLevelGraphicsSettings(level_blk);
  g_entity_mgr->broadcastEventImmediate(OnLevelLoaded(level_blk));
  defrag_shaders_stateblocks(true);
  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  if (DngSkies *daskies = get_daskies())
  {
    daskies->prepare(get_daskies()->getSunDir(), false, 0);
    dir_to_sun.realTime = daskies->getPrimarySunDir();
  }
  dirToSunFlush();
  cameraSpeed = 0;
  invalidateClipmap(false);
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);
  prepareLastClip();
  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);
  if (lmeshMgr)
    lmeshMgr->setHmapLodDistance(2);
  else
    loadLandMicroDetails(level_blk.getBlockByName("micro_details"));

  if (lmeshMgr)
  {
    const DataBlock *levelClipBlk = level_blk.getBlockByNameEx("clipmap");
    const DataBlock *clipBlk = ::dgs_get_settings()->getBlockByNameEx("clipmap");
    float texelSize = clipBlk->getReal("texelSize", 4.0 / 1024);
    if (hasFeature(FeatureRenderFlags::LEVEL_LAND_TEXEL_SIZE))
      texelSize = levelClipBlk->getReal("texelSize", texelSize);
    if (!clipmap)
      initClipmap();
    debug("clipmap texelSize = %f", texelSize);
    clipmap->setStartTexelSize(texelSize);
  }
  shadowsManager.shadowsInvalidate(true);
  shadowsManager.staticShadowsAdditionalHeight = level_blk.getReal("staticShadowsAdditionalHeight", 0.f);
  shadowsManager.staticShadowsSetWorldSize();

  updateDistantHeightmap();

  giInvalidateDeferred = 0;
  setGIQualityFromSettings();
  setSettingsSSR();
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  localPaintTexId = ShaderGlobal::get_tex(get_shader_variable_id("paint_details_tex", true));
  prefetchPartsOfPaintingTexture();
  const DataBlock *overrideRiLodRangeLevel = level_blk.getBlockByNameEx("overrideRiLodRange");
  rendinst::overrideLodRanges(*overrideRiLodRangeLevel);
  rendinst::scheduleRIGenPrepare(make_span(&grs_cur_view.pos, 1)); //==

  changeGrassShadowQuality();
  changeDynamicShadowResolution();

  invalidateGI(true);
  if (getEnviProbeRenderFlags() & ENVI_PROBE_USE_GEOMETRY)
    invalidateCube();

  d3d::driver_command(Drv3dCommand::SAVE_PIPELINE_CACHE);

  initPortalRendererCallbacks();
  camera_in_camera::setup(hasFeature(CAMERA_IN_CAMERA));

  renderedFrames = 0;
}

void WorldRenderer::initPortalRendererCallbacks()
{
  auto portalRenderer = portal_renderer_mgr::query_portal_renderer();
  if (!portalRenderer)
    return;

  auto portalParamsGetterCb = [this]() -> PortalRenderer::CallbackParams {
    PortalRenderer::CallbackParams cb;
    cb.renderWater = [this](Texture *color_target, const CameraParams &camera, Texture *depth) -> void {
      d3d::set_render_target({depth, 0}, DepthAccess::SampledRO, {{color_target, 0}});
      renderWater(camera, WorldRenderer::DistantWater::No, false);
    };
    cb.renderLandmesh = [this](mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum, const Point3 &view_pos) -> void {
      if (lmeshRenderer && clipmap)
      {
        lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH);
        clipmap->startSecondaryFeedback();
        lmeshRenderer->render(globtm, proj, frustum, *lmeshMgr, LandMeshRenderer::RENDER_WITH_CLIPMAP, view_pos);
        clipmap->endSecondaryFeedback();
      }
    };
    cb.prepareClipmap = [this](const Point3 &origin) -> void {
      if (clipmap)
        clipmap->setSecondaryFeedbackCenter(origin);
    };
    cb.renderRiPrepass = [](const TMatrix &view_itm, const RiGenVisibility &visibility, const TexStreamingContext &tex_ctx,
                           const rendinst::render::RiExtraRenderer *) -> void {
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGenOptimizationDepth(rendinst::RenderPass::Depth, &visibility, view_itm,
        rendinst::IgnoreOptimizationLimits::No, rendinst::SkipTrees::No, rendinst::RenderGpuObjects::Yes, 1U, tex_ctx);
    };
    cb.renderRiNormal = [](const TMatrix &view_itm, const RiGenVisibility &visibility, const TexStreamingContext &tex_ctx,
                          const rendinst::render::RiExtraRenderer *riex_renderer) -> void {
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, &visibility, view_itm, rendinst::LayerFlag::Opaque,
        rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, riex_renderer, tex_ctx);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, &visibility, view_itm, rendinst::LayerFlag::NotExtra,
        rendinst::OptimizeDepthPass::Yes, 1, rendinst::AtestStage::All, riex_renderer, tex_ctx);
    };
    cb.renderRiTrans = [](const TMatrix &view_itm, const RiGenVisibility &visibility, const TexStreamingContext &tex_ctx,
                         const rendinst::render::RiExtraRenderer *riex_renderer) -> void {
      SCENE_LAYER_GUARD(rendinstTransSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, &visibility, view_itm, rendinst::LayerFlag::Transparent,
        rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All, riex_renderer, tex_ctx);
    };
    return cb;
  };
  portalRenderer->initCallbacks(portalParamsGetterCb);
}

void WorldRenderer::changeGrassShadowQuality()
{
  const uint32_t GRASS_SHADOWS_QUALITY_COUNT = 5;
  const uint32_t GRASS_SHADOWS_DEFAULT_QUALITY = 2;
  const eastl::array<const char *, GRASS_SHADOWS_QUALITY_COUNT> grassShadowsQualityNames = {"off", "low", "medium", "high", "ultra"};
  const eastl::array<int, GRASS_SHADOWS_QUALITY_COUNT> grassSelfShadowSamples = {0, 2, 4, 8, 16};

  const char *str = ::dgs_get_settings()
                      ->getBlockByNameEx("graphics")
                      ->getStr("grassShadowsQuality", grassShadowsQualityNames[GRASS_SHADOWS_DEFAULT_QUALITY]);
  uint32_t grassShadowQuality = parse_quality_option(str, grassShadowsQualityNames, GRASS_SHADOWS_DEFAULT_QUALITY);
  static int fast_grass_self_shadow_samplesVarId = ::get_shader_variable_id("fast_grass_self_shadow_samples", true);
  ShaderGlobal::set_int(fast_grass_self_shadow_samplesVarId, grassSelfShadowSamples[grassShadowQuality]);
}

int WorldRenderer::getDynamicShadowQuality()
{
  if (!hasFeature(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
    return 0;

  const uint32_t DYN_SHADOWS_QUALITY_COUNT = 5;
  const uint32_t DYN_SHADOWS_DEFAULT_QUALITY = 2;
  const eastl::array<const char *, DYN_SHADOWS_QUALITY_COUNT> dynShadowsQualityNames = {"off", "low", "medium", "high", "ultra"};

  const char *str = ::dgs_get_settings()
                      ->getBlockByNameEx("graphics")
                      ->getStr("dynamicShadowsQuality", dynShadowsQualityNames[DYN_SHADOWS_DEFAULT_QUALITY]);
  uint32_t dynamicShadowQuality = parse_quality_option(str, dynShadowsQualityNames, DYN_SHADOWS_DEFAULT_QUALITY);
  float qualityScale = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("dynamicShadowsQualityScale", 1.0f);
  dynamicShadowQuality = static_cast<uint32_t>(round(max(dynamicShadowQuality * qualityScale, 0.0f)));
  return dynamicShadowQuality;
}

void WorldRenderer::changeDynamicShadowResolution()
{
  const bool dynamicShadow32bit = lvl_override::getBool(levelSettings.get(), "dynamicShadow32bit", false);
  lights.changeShadowResolution(getDynamicShadowQuality(), dynamicShadow32bit);
  shadowsManager.changeSpotShadowBiasBasedOnQuality();
}

void WorldRenderer::setDynamicShadowsMaxUpdatePerFrame()
{
  lights.setMaxShadowsToUpdateOnFrame(
    ::dgs_get_settings()
      ->getBlockByNameEx("graphics")
      ->getInt("dynamicShadowsMaxUpdatePerFrame", ClusteredLights::DEFAULT_MAX_SHADOWS_TO_UPDATE_PER_FRAME));
}

void WorldRenderer::beforeLoadLevel(const DataBlock &level_blk)
{
  // tend to be long action, executed in blocking manner, note it for reports
#if _TARGET_ANDROID || _TARGET_IOS
  crashlytics::AppState appState("render_beforeLoadLevel");
#endif

  worldBBox = BBox3();
  originalLevelBlk = DataBlock(level_blk);
  giMaxUpdateHeightAboveFloor = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("maxHeightGIUpdateAboveFloor", 32.0f);
  giHmapOffsetFromUp = giMaxUpdateHeightAboveFloor - 8.0f; // default 32-8 = 24
  daGdpRangeScale = 1;

  const DataBlock *clustered_lights = originalLevelBlk.getBlockByNameEx("clustered_lights");
  float maxClusteredDist = clustered_lights->getReal("maxClusteredDist", 500.0f);
  lights.setMaxClusteredDist(maxClusteredDist);

  // we should create grass before apply settings
  init_grass(level_blk.getBlockByName("grass"));

  changeFeatures(getOverridenRenderFeatures());
  onBareMinimumSettingsChanged();

#if _TARGET_ANDROID
  ShaderGlobal::set_int(get_shader_variable_id("use_glass_dual_source_blending", true),
    d3d::get_driver_desc().caps.hasDualSourceBlending);
#endif

  // update settings entity before all other systems, because that requare to know about settings
  if (!update_settings_entity(level_blk.getBlockByNameEx("graphics")))
    logerr("Render settings entity must exist.");

  // here we can initialize all systems which depends on world renderer
  g_entity_mgr->broadcastEventImmediate(BeforeLoadLevel());

  d3d::driver_command(Drv3dCommand::ACQUIRE_OWNERSHIP);

  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading(); // or it will crash here
  d3d::set_render_target();

  shadowsManager.initShadows();
  setSpecularCubesContainerForReinit();
  reinitCube(level_blk.getPoint3("enviProbePos", Point3(0, 100, 0)));
  if (auto *skies = get_daskies())
  {
    const DataBlock &blk = *level_blk.getBlockByNameEx("skies");
    timeDynamic = blk.getBool("isTimeDynamic", false);
    sky_is_unreachable.set(blk.getBool("skyIsUnreachable", true));
    setupSkyPanoramaAndReflectionFromSetting();
    rendinst::setDirFromSun(-skies->getPrimarySunDir());
  }
  const DataBlock &depthAOAboveBlk = *level_blk.getBlockByNameEx("depthAOAbove");
  needTransparentDepthAOAbove = depthAOAboveBlk.getBool("render_transparent", false);
  if (depthAOAboveCtx)
  {
    depthAOAboveCtx.reset(nullptr);
    depthAOAboveCtx = eastl::make_unique<DepthAOAboveContext>(DEPTH_AROUND_TEX_SIZE, DEPTH_AROUND_DISTANCE,
      needTransparentDepthAOAbove && transparentDepthAOAboveRequests);
  }
  // ECS entities related with AO should be created again after level is changed.
  resetSSAOImpl();

  TMatrix saveItm = ::grs_cur_view.itm;
  ::grs_cur_view.itm = saveItm;
  d3d::driver_command(Drv3dCommand::RELEASE_OWNERSHIP);

  prerunFx();

  set_water_quality_from_settings();

  resetLatencyMode();
}

void WorldRenderer::onLightmapSet(TEXTUREID lmap_tid)
{
  G_ASSERTF(lightmapTexId == BAD_TEXTUREID, "lightmapTexId=%d", lightmapTexId);
  lightmapTexId = lmap_tid;
  acquire_managed_tex(lightmapTexId);
}
void WorldRenderer::onLandmeshLoaded(const DataBlock &level_blk, const char *fn, LandMeshManager *lmesh)
{
  loadLandMicroDetails(level_blk.getBlockByName("micro_details"));
  if (!lmesh)
    return;

  LandMeshRenderer *lmRenderer = lmesh->createRenderer();

  if (lightmapTexId == BAD_TEXTUREID)
  {
    char levelFileNameWithoutExt[DAGOR_MAX_PATH];
    String lightmapFileName(DAGOR_MAX_PATH, "%s_lightmap",
      dd_get_fname_without_path_and_ext(levelFileNameWithoutExt, sizeof(levelFileNameWithoutExt), fn));

    if (!is_managed_textures_streaming_load_on_demand())
      ddsx::tex_pack2_perform_delayed_data_loading();

    lightmapTexId = ::get_tex_gameres(lightmapFileName);
    if (lightmapTexId == BAD_TEXTUREID)
      logerr("lightmapFileName =<%s> NOT found ", lightmapFileName);
  }
  constexpr int HMAP_DEFAULT_LOD_COUNT = 5;
  int hmapLodCnt = level_blk.getInt("hmapLodCount", ::dgs_get_game_params()->getInt("hmapDefaultLodCount", HMAP_DEFAULT_LOD_COUNT));
  LmeshMirroringParams lmeshMirror = load_lmesh_mirroring(level_blk);
  execute_delayed_action_on_main_thread(make_delayed_action([this, lmeshMirror, lmesh, lmRenderer, hmapLodCnt]() {
    if (lmesh->getHmapHandler())
      lmesh->getHmapHandler()->setLodCount(hmapLodCnt);

    BBox2 lightmapBbox;
    lightmapBbox.lim[0].x = lmesh->getCellOrigin().x * lmesh->getLandCellSize();
    lightmapBbox.lim[0].y = lmesh->getCellOrigin().y * lmesh->getLandCellSize();
    lightmapBbox.lim[1].x = (lmesh->getNumCellsX() + lmesh->getCellOrigin().x) * lmesh->getLandCellSize();
    lightmapBbox.lim[1].y = (lmesh->getNumCellsY() + lmesh->getCellOrigin().y) * lmesh->getLandCellSize();

    ShaderGlobal::set_texture(get_shader_variable_id("lightmap_tex"), lightmapTexId);
    ShaderGlobal::set_sampler(get_shader_variable_id("lightmap_tex_samplerstate"), d3d::request_sampler({}));
    Color4 world_to_lightmap(1.f / (lmesh->getNumCellsX() * lmesh->getLandCellSize()),
      1.f / (lmesh->getNumCellsY() * lmesh->getLandCellSize()), -(float)lmesh->getCellOrigin().x / lmesh->getNumCellsX(),
      -(float)lmesh->getCellOrigin().y / lmesh->getNumCellsY());

    ShaderGlobal::set_float4(get_shader_variable_id("world_to_lightmap"), world_to_lightmap);

    // init mirroring
    lmRenderer->setMirroring(*lmesh, lmeshMirror.numBorderCellsXPos, lmeshMirror.numBorderCellsXNeg, lmeshMirror.numBorderCellsZPos,
      lmeshMirror.numBorderCellsZNeg);
    lmeshRenderer = lmRenderer;
    lmeshMgr = lmesh;

    initDeformHeightmap();
    debug("lmesh set in main thread");

    if (!clipmap)
      initClipmap();
  }));

  debug("lmesh loaded");
}

void WorldRenderer::initDeformHeightmap()
{
  closeDeformHeightmap();

  const char *groundDeformationsSettingStr =
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("groundDeformations", "medium");

  enum
  {
    OFF,
    LOW,
    MEDIUM,
    HIGH,
    GROUND_QUALITY_COUNT
  };

  const eastl::array<const char *, GROUND_QUALITY_COUNT> qualityNames = {"off", "low", "medium", "high"};

  int groundDeformationsSetting = parse_quality_option(groundDeformationsSettingStr, qualityNames, MEDIUM);


  if (groundDeformationsSetting == OFF)
  {
    debug("DeformHeightmap disabled in graphics settings.");
    return;
  }
  if (lmeshMgr == nullptr)
  {
    debug("DeformHeightmap initialization failed. lmeshMgr == nullptr");
    return;
  }
  if (lmeshMgr->getHmapHandler() == nullptr)
  {
    debug("DeformHeightmap initialization failed. lmeshMgr->getHmapHandler() == nullptr");
    return;
  }

  if (biome_query::get_biome_group_id("snow") < 0 && groundDeformationsSetting < MEDIUM)
  {
    // Heightmap deformation is visible either on deep snow or on tire tracks. Deformation on tire tracks is usually
    // less visible and we disable it on low settings and levels where there isn't snow to save performance.
    // We can still enable it if we decide it's worth the performance hit (~0.5-0.6ms on Xbox One S).
    debug("DeformHeightmap disabled for this level, groundDeformationsSetting is low and heightmap does not have landclass with "
          "\"snow\" biome group.");
    return;
  }

  DeformHeightmapDesc deformHmapDesc;
  deformHmapDesc.texSize = groundDeformationsSetting == HIGH ? 2048 : 1024;
  deformHmapDesc.rectSize = groundDeformationsSetting == HIGH ? 90.f : 60.f;
  deformHmapDesc.raiseEdges = groundDeformationsSetting >= MEDIUM;
  deformHmapDesc.minTerrainHeight = lmeshMgr->getHmapHandler()->getWorldBox().lim[0].y;
  deformHmapDesc.maxTerrainHeight = lmeshMgr->getHmapHandler()->getWorldBox().lim[1].y;
  if (is_relative_equal_float(deformHmapDesc.maxTerrainHeight, deformHmapDesc.minTerrainHeight))
  {
    debug("DeformHeightmap wasn't initialized because heightmap is flat");
    return;
  }

  deformHmap.reset(new DeformHeightmap(deformHmapDesc));
  if (!deformHmap->isValid())
  {
    logerr("DeformHeightmap initialization failed. Game should work properly without DeformHeightmap feature.");
    deformHmap.reset();
  }
  else
  {
    debug("DeformHeightmap initialized. Quality: %s", groundDeformationsSettingStr);
    if (g_entity_mgr->getTemplateDB().getTemplateByName("physmap_patch_data_creation_request"))
      g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("physmap_patch_data_creation_request"));
    if (ecs::EntityId toggleEid = g_entity_mgr->getSingletonEntity(ECS_HASH("hmap_displacement_invalidation_toggle")))
      g_entity_mgr->set(toggleEid, ECS_HASH("hmap_displacement_invalidation__enabled"), true);
  }
}

void WorldRenderer::closeDeformHeightmap()
{
  deformHmap.reset();
  ShaderGlobal::set_int(deform_hmap_enabledVarId, 0);
  if (ecs::EntityId toggleEid = g_entity_mgr->getSingletonEntity(ECS_HASH("hmap_displacement_invalidation_toggle")))
    g_entity_mgr->set(toggleEid, ECS_HASH("hmap_displacement_invalidation__enabled"), false);
}

bool WorldRenderer::getLandHeight(const Point2 &p, float &ht, Point3 *normal) const
{
  if (!lmeshMgr || !lmeshMgr->getHmapHandler())
    return false;
  if (lmeshMgr->getHolesManager() && lmeshMgr->getHolesManager()->check(p))
    return false;
  return lmeshMgr->getHmapHandler()->getHeight(p, ht, normal);
}

void WorldRenderer::updateImpostorSettings()
{
  const int impostorQuality = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("impostor", 1);
  rendinst_impostor_set_parallax_mode(impostorQuality > 0 ? 1 : 0);
  rendinst_impostor_set_view_mode(impostorQuality > 1 ? 1 : 0);
}

void WorldRenderer::setSharpeningFromSettings()
{
  if (!hasFeature(FeatureRenderFlags::POSTFX))
    return;

  const float sharpening = ::dgs_get_settings()->getBlockByNameEx("graphics")->getReal("sharpening", 0.f);
  ShaderGlobal::set_float(contrast_adaptive_sharpening_strengthVarId, sharpening / 100.f);
}

void WorldRenderer::createDebugTexOverlay(const int w, const int h)
{
#if DAGOR_DBGLEVEL > 0
  del_it(debug_tex_overlay);
  debug_tex_overlay = new DebugTexOverlay;
  debug_tex_overlay->setTargetSize(Point2(w, h));
  debug_tonemap_overlay.reset();
  debug_tonemap_overlay = eastl::make_unique<DebugTonemapOverlay>();
  debug_tonemap_overlay->setTargetSize(Point2(w, h));
#else
  G_UNUSED(w);
  G_UNUSED(h);
#endif
}

void WorldRenderer::resetLatencyMode() { lowlatency::set_latency_mode(lowlatency::get_from_blk(), 0); }

void WorldRenderer::resetVsyncMode()
{
  d3d::enable_vsync(::dgs_get_settings()->getBlockByNameEx("video")->getBool("vsync", false) && lowlatency::is_vsync_allowed());
}

void WorldRenderer::resetPerformanceMetrics()
{
  PerfDisplayMode mode = static_cast<PerfDisplayMode>(
    ::dgs_get_settings()->getBlockByNameEx("video")->getInt("perfMetrics", static_cast<int>(PerfDisplayMode::FPS)));
  if (!lowlatency::is_available() && mode != PerfDisplayMode::OFF && mode != PerfDisplayMode::FPS)
    mode = PerfDisplayMode::FPS;
  darg::bhv_fps_bar.setDisplayMode(mode);
  darg::bhv_latency_bar.setDisplayMode(mode);
}

void WorldRenderer::resetDynamicQuality()
{
  dynamicQuality.reset();
  if (::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("dynamicQuality", false))
    dynamicQuality = eastl::make_unique<DynamicQuality>(*g_entity_mgr, ::dgs_get_settings()->getBlockByName("dynamicQuality"));
}


void WorldRenderer::onSettingsChanged(const FastNameMap &changed_fields, bool apply_after_reset)
{
  dafg::invalidate_history();

  d3d::GpuAutoLock gpu_al;

  cachedStaticResolutionScale =
    dgs_get_settings()->getBlockByNameEx("video")->getReal("staticResolutionScale", get_default_static_resolution_scale());
  useFullresClouds = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("HQVolumetricClouds", false);
  ShaderGlobal::set_int(get_shader_variable_id("ssr_alternate_reflections_ignore_roughness", true),
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("HQSSR", false) ? 1 : 0);

  if (changed_fields.getNameId("graphics/preset") >= 0 || changed_fields.getNameId("graphics/consolePreset") >= 0)
    changePreset();

  if (changed_fields.getNameId("graphics/anisotropy") >= 0)
  {
    load_anisotropy_from_settings();
    reset_anisotropy("*");
    if (lmeshRenderer && lmeshMgr)
      lmeshRenderer->updateCustomSamplers(*lmeshMgr);
    ShaderGlobal::set_int(dgs_tex_anisotropyVarId, ::dgs_tex_anisotropy);
  }

  // we need call this function after changePreset, because it change RenderFeatures
  update_settings_entity(levelSettings.get());

  if (changed_fields.getNameId("graphics/dynamicQuality") >= 0)
    resetDynamicQuality();

  // Update latency before updating vsync
  if (changed_fields.getNameId("video/dlssFrameGenerationCount") >= 0 ||
      (changed_fields.getNameId("video/nvidia_latency") >= 0 || changed_fields.getNameId("video/amd_latency") >= 0 ||
        changed_fields.getNameId("video/intel_latency") >= 0))
    resetLatencyMode();

  if (do_settings_changes_need_videomode_change(changed_fields))
  {
    if (!apply_after_reset)
      applySettingsChanged();
    else
      applySettingsAfterResetDevice = true;
  }

  bool shouldApplySettingsChanged = false;

  if (changed_fields.getNameId("graphics/effectsShadows") >= 0)
  {
    changeStateFOMShadows();
    shouldApplySettingsChanged = true;
  }

  if (changed_fields.getNameId("graphics/groundDisplacementQuality") >= 0)
    initSubDivSettings();

  if (changed_fields.getNameId("graphics/groundDeformations") >= 0)
    initDeformHeightmap();

  if (changed_fields.getNameId("graphics/anisotropy") >= 0)
  {
    if (!apply_after_reset)
      initClipmap();
    else
      initClipmapAfterResetDevice = true;
  }

  if (changed_fields.getNameId("graphics/impostor") >= 0)
    updateImpostorSettings();

  if (changed_fields.getNameId("graphics/rendinstTesselation") >= 0)
  {
    setRendinstTesselation();
    initDisplacement();
  }

  if (changed_fields.getNameId("graphics/lodsShiftDistMul") >= 0)
    rendinst::setLodsShiftDistMult();

  if (changed_fields.getNameId("graphics/rendinstDistMul") >= 0)
    rendinst::updateSettingsDistMul();

  if (changed_fields.getNameId("graphics/riExtraMulScale") >= 0)
    rendinst::updateRIExtraMulScale();

  if (changed_fields.getNameId("graphics/IndoorReflectionProbes") >= 0)
    initIndoorProbesIfNecessary();

  if (changed_fields.getNameId("graphics/aoQuality") >= 0)
    resetSSAOImpl();

  if (changed_fields.getNameId("graphics/giAlgorithm") >= 0 || changed_fields.getNameId("graphics/giAlgorithmQuality") >= 0)
    setGIQualityFromSettings();

  if (changed_fields.getNameId("graphics/cloudsQuality") >= 0)
    setupSkyPanoramaAndReflectionFromSetting();

  if (changed_fields.getNameId("graphics/waterQuality") >= 0)
  {
    set_water_quality_from_settings();
    shouldApplySettingsChanged = true;
  }

  // Need to reapply settings when dynamic resolution was turned on or off because rendering
  // resolution may change and render targets should be recreated with proper size.
  if (changed_fields.getNameId("video/dynamicResolution/enabled") >= 0)
    shouldApplySettingsChanged = true;
  else if (dynamicResolution && (changed_fields.getNameId("video/dynamicResolution/minResolutionScale") >= 0 ||
                                  changed_fields.getNameId("video/dynamicResolution/targetFPS") >= 0))
    dynamicResolution->applySettings();

  if (shouldApplySettingsChanged)
    applySettingsChanged();

  if ((changed_fields.getNameId("graphics/waterQuality") >= 0) || (changed_fields.getNameId("graphics/fftWaterQuality") >= 0))
    setWater(water);

  if (changed_fields.getNameId("graphics/environmentDetailsQuality") >= 0)
  {
    setEnviromentDetailsQuality();
    setNbsQuality();
  }

  if (changed_fields.getNameId("graphics/fxTarget") >= 0)
    setFxQuality();

  if (changed_fields.getNameId("graphics/ssrQuality") >= 0)
    setSettingsSSR();

  if (changed_fields.getNameId("renderFeatures/cameraInCamera") >= 0)
  {
    clearLegacyGPUObjectsVisibility();
    setFeatureFromSettings(CAMERA_IN_CAMERA);
  }

  if (hasMotionVectors != needMotionVectors())
    toggleMotionVectors();

  if (changed_fields.getNameId("graphics/skiesQuality") >= 0)
    resetMainSkies(get_daskies());

  if (changed_fields.getNameId("screenshots/format") >= 0)
    screencap::init(dgs_get_settings());

  if (changed_fields.getNameId("video/perfMetrics") >= 0)
    resetPerformanceMetrics();

  if (changed_fields.getNameId("graphics/HQProbeReflections") >= 0)
    toggleProbeReflectionQuality();

  if (changed_fields.getNameId("graphics/staticUpsampleQuality") >= 0)
    applyStaticUpsampleQuality();

  if (changed_fields.getNameId("graphics/grassShadowsQuality") >= 0)
    changeGrassShadowQuality();

  if (changed_fields.getNameId("graphics/dynamicShadowsQuality") >= 0)
    changeDynamicShadowResolution();
  if (changed_fields.getNameId("graphics/dynamicShadowsMaxUpdatePerFrame") >= 0)
    setDynamicShadowsMaxUpdatePerFrame();

  if (changed_fields.getNameId("graphics/sharpening") >= 0)
    setSharpeningFromSettings();

  if (changed_fields.getNameId("graphics/fxaaQuality") >= 0)
    applyFXAASettings();

  if (changed_fields.getNameId("graphics/chromaticAberration") >= 0 ||
      changed_fields.getNameId("cinematicEffects/chromaticAberration") >= 0 || changed_fields.getNameId("graphics/preset") >= 0 ||
      changed_fields.getNameId("graphics/consolePreset") >= 0)
    setChromaticAberrationFromSettings();

  if (changed_fields.getNameId("graphics/filmGrain") >= 0 || changed_fields.getNameId("cinematicEffects/filmGrain") >= 0 ||
      changed_fields.getNameId("graphics/preset") >= 0 || changed_fields.getNameId("graphics/consolePreset") >= 0)
    setFilmGrainFromSettings();
}

void WorldRenderer::beforeDeviceReset(bool full_reset)
{
  beforeResetNBS();

  if (full_reset)
  {
    if (clipmap)
      clipmap->beforeReset();

    ddsx::cease_delayed_data_loading(ddsx::hq_tex_priority);
    ddsx::cease_delayed_data_loading(0);
    DEBUG_CP();
  }
  else
  {
    if (clipmap)
      clipmap->finalizeFeedback(); // just for good measure.
  }

  acesfx::before_reset();
  HeightmapRenderer::beforeResetDevice();
  clearLegacyGPUObjectsVisibility();

  if (full_reset)
    lights.beforeResetDevice();
}

void WorldRenderer::afterDeviceReset(bool full_reset)
{
  if (emissionColorMaps)
    emissionColorMaps->onReset();

  if (applySettingsAfterResetDevice)
  {
    applySettingsAfterResetDevice = false;
    applySettingsChanged();
  }

  if (initClipmapAfterResetDevice)
  {
    initClipmapAfterResetDevice = false;
    initClipmap();
  }

  {
    d3d::GpuAutoLock gpuLock;
    HeightmapRenderer::afterResetDevice();
    rendinst::gpuobjects::after_device_reset();
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    acesfx::after_reset();

    if (auto *skies = get_daskies())
    {
      skies->reset();
      skies->skiesDataScatteringVolumeBarriers(main_pov_data);
    }
  }

  afterResetNBS();

  if (auto portalRenderer = portal_renderer_mgr::query_portal_renderer())
    portalRenderer->afterDeviceReset();

  if (full_reset)
  {
    static const char *ERROR_MSG = nullptr;
    if (::dgs_tex_quality < 2)
    {
      ::dgs_tex_quality++;
      debug("switched to dgs_tex_quality=%d", ::dgs_tex_quality);
      ERROR_MSG = "Video driver hung and was restarted. Texture quality has been reduced.";
    }
    else
    {
      debug("dgs_tex_quality=%d remained the same", ::dgs_tex_quality);
      ERROR_MSG = "Video driver hung and was restarted.";
    }

    logmessage(DAGOR_DBGLEVEL > 0 ? _MAKE4C('D3DW') : _MAKE4C('D3DE'), ERROR_MSG);
    visuallog::logmsg(ERROR_MSG);

    ddsx::reload_active_textures(0);

    if (lmeshMgr)
    {
      lmeshMgr->afterDeviceReset(lmeshRenderer, full_reset);
      if (lmeshMgr->getHmapHandler())
        lmeshMgr->getHmapHandler()->fillHmapTextures();
    }
    if (clipmap)
      clipmap->afterReset();
    dirToSunFlush();
    shadowsManager.initShadows();
    closeResetable();
    initResetable();
    resetGI();
    delayedRenderCtx.invalidate();
    invalidateCube();
    reinitSpecularCubesContainerIfNeeded();
    onSceneLoaded(binScene);
    g_entity_mgr->broadcastEventImmediate(AfterDeviceReset(full_reset));

    clipmap_decals_mgr::after_reset();

    ddsx::tex_pack2_perform_delayed_data_loading(0);
    prepareLastClip();
    fft_water::reset_render(water);
    if (water_ssr_id == -1)
      water_ssr_id = d3d::create_predicate();
    lights.afterResetDevice();
    if (paintColorsTex)
    {
      paintColorsTex.close();
      prefetchPartsOfPaintingTexture();

      // re-attach initial paint details texture, to avoid OOB access to null texture
      ShaderGlobal::set_texture(get_shader_variable_id("paint_details_tex", true), localPaintTexId);
    }
  }
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  shadowsManager.afterDeviceReset();

  createDeforms();

  backbufferTex.close();
  updateBackBufferTex();
}

void WorldRenderer::windowResized() { applySettingsAfterResetDevice = true; }

void WorldRenderer::updateFinalTargetFrame() { updateBackBufferTex(); }

void WorldRenderer::preloadLevelTextures()
{
  rendinst::preloadTexturesToBuildRendinstShadows();
  preload_textures_for_last_clip();
  if (characterMicrodetailsId != BAD_TEXTUREID)
    prefetch_and_wait_managed_textures_loaded(make_span_const(&characterMicrodetailsId, 1), false);
}

void WorldRenderer::applySettingsChanged()
{
  d3d::GpuAutoLock gpuLock;
  setResolution();
  requestFgRecreation("applySettingsChanged");
}

void WorldRenderer::processPendingFgRecreation()
{
  if (hasPendingFgRecreation)
  {
    TIME_PROFILE(createFramegraphNodes);
    createNodes();
    hasPendingFgRecreation = false;
  }
}

void WorldRenderer::changeStateFOMShadows()
{
  fomShadowManager.reset();
  if (!hasFeature(FOM_SHADOWS))
    return;
  const bool fom = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("effectsShadows", true);
  if (fom)
  {
    const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
    fomShadowManager = eastl::make_unique<FomShadowsManager>(graphicsBlk->getInt("fom_tex_sz", 256),
      lvl_override::getReal(levelSettings.get(), "fom_z_distance", 80),
      lvl_override::getReal(levelSettings.get(), "fom_xy_distance", 128),
      lvl_override::getReal(levelSettings.get(), "fom_height", 80));
  }
}

void WorldRenderer::resetSSAOImpl()
{
  aoFGNodes = {};

  int aoW, aoH;
  getMaxPossibleRenderingResolution(aoW, aoH);
  aoW /= 2;
  aoH /= 2;
  const bool ssaoOn = hasFeature(SSAO) && lvl_override::getBool(levelSettings.get(), "ssao_enabled", true);

  if (!ssaoOn)
  {
    g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("capsules_ao"))); // destroy capsules AO
    g_entity_mgr->broadcastEventImmediate(ResetAoEvent(IPoint2(aoW, aoH), ResetAoEvent::CLOSE));
    return;
  }

  const char *aoQualityStr = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("aoQuality", "medium");
  if (strcmp(aoQualityStr, "medium") == 0)
    aoQuality = AoQuality::MEDIUM;
  else if (strcmp(aoQualityStr, "low") == 0)
    aoQuality = AoQuality::LOW;
  else if (strcmp(aoQualityStr, "high") == 0)
    aoQuality = AoQuality::HIGH;
  else
    aoQuality = AoQuality::MEDIUM;

  if (!hasFeature(DOWNSAMPLED_NORMALS))
    aoQuality = AoQuality::LOW;
  if (is_rtao_enabled() || is_ptgi_enabled())
    aoQuality = AoQuality::LOW;

  if (aoQuality == AoQuality::HIGH)
  {
    g_entity_mgr->getOrCreateSingletonEntity(ECS_HASH("capsules_ao")); // create capsules AO
  }
  else
  {
    g_entity_mgr->destroyEntity(g_entity_mgr->getSingletonEntity(ECS_HASH("capsules_ao"))); // destroy capsules AO
  }
  g_entity_mgr->broadcastEventImmediate(
    ResetAoEvent(IPoint2(aoW, aoH), aoQuality == AoQuality::HIGH ? ResetAoEvent::INIT : ResetAoEvent::CLOSE));

  uint32_t creationFlags = SSAO_NONE;
  if (hasFeature(CONTACT_SHADOWS))
    creationFlags |= SSAO_USE_CONTACT_SHADOWS;
  if (getGiQuality().algorithm == GiAlgorithm::LOW && bareMinimumPreset)
    creationFlags |= SSAO_USE_WSAO;
  if (getGiQuality().algorithm == GiAlgorithm::LOW && hasFeature(CONTACT_SHADOWS))
    creationFlags |= SSAO_USE_CONTACT_SHADOWS_SIMPLIFIED;
  if (isThinGBuffer())
    creationFlags |= SSAO_IMMEDIATE;

  bool hq = aoQuality != AoQuality::LOW;
  aoFGNodes = makeAmbientOcclusionNodes(hq ? AoAlgo::GTAO : AoAlgo::SSAO, aoW, aoH, creationFlags);

  if (!hq)
  {
    int quality = hasFeature(DOWNSAMPLED_NORMALS) ? 1 : (getGiQuality().algorithm == GiAlgorithm::LOW ? -1 : 0);
    ShaderGlobal::set_int(ssao_qualityVarId, quality);
    ShaderGlobal::set_int(blur_qualityVarId, quality);
    ShaderGlobal::set_float(ssao_strengthVarId, 1.0f);
  }
}

void WorldRenderer::setFinalTargetTex(const ManagedTex *tex, const char *ctx_mark)
{
  const auto prevFinalTarget = eastl::exchange(finalTargetFrame, tex);
  if (DAGOR_UNLIKELY(prevFinalTarget != finalTargetFrame))
    debug("%sWR:finalTex:setFinalTargetTex from %p to %p (RTControl %d)", ctx_mark, prevFinalTarget, finalTargetFrame,
      eastl::to_underlying(rtControl));
}

void WorldRenderer::resetBackBufferTex()
{
  if (rtControl == RtControl::OWNED_RT)
    ownedBackbufferTex.close();

  setFinalTargetTex(nullptr, "[reset]");
  backbufferTex.close(); // update backbuffer pointer
}

void WorldRenderer::updateBackBufferTex()
{
  if (rtControl == RtControl::BACKBUFFER)
  {
    if (!backbufferTex || backbufferTex.getBaseTex() != d3d::get_backbuffer_tex())
    {
      backbufferTex.close();
      backbufferTex = dag::get_backbuffer();
      debug("WR:finalTex: backbufferTex is updated to %p", &backbufferTex);
    }
    setFinalTargetTex(&backbufferTex, "[BB]");
  }
}

void WorldRenderer::setSettingsSSR()
{
  eastl::string_view ssrQuality = lvl_override::getStr(getLevelSettings(), "ssrQuality", "low");
  if (ssrQuality == eastl::string_view("low"))
    ssr_quality.set(0);
  else if (ssrQuality == eastl::string_view("medium"))
    ssr_quality.set(1);
  else if (ssrQuality == eastl::string_view("high") || ssrQuality == eastl::string_view("movie"))
    ssr_quality.set(2);
  else
  {
    logwarn("Invalid SSR quality <%s>", ssrQuality);
    ssr_quality.set(0);
  }

  ShaderGlobal::set_int(glass_dynamic_lightVarId, ssr_quality >= 1 ? 1 : 0);

  ssr_enable.set(!is_rtr_enabled());
}

void WorldRenderer::getMaxPossibleRenderingResolution(int &width, int &height) const
{
  if (dynamicResolution)
    dynamicResolution->getMaxPossibleResolution(width, height);
  else
    getRenderingResolution(width, height);
}

void WorldRenderer::updateSettingsSSR(int width, int height)
{
  ssrFGNodes.clear();

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  if (isSSREnabled())
  {
    bool ssrHQ = graphicsBlk->getBool("hqSSR", false);
    if (width < 0 || height < 0)
      getMaxPossibleRenderingResolution(width, height);

    struct
    {
      bool alternateReflections;
      int qualityShaderVar;
      SSRQuality quality;
    } qualityPresets[] = {
      // SDFrefl, ShaderVar, SSRQuality
      {false, 0, SSRQuality::Compute}, // 0 Compute Low
      {false, 2, SSRQuality::Compute}, // 2 Compute High
      {true, 3, SSRQuality::Compute},  // 3 Compute Highest
    };
    G_ASSERT(ssr_quality.getMax() == eastl::size(qualityPresets) - 1);
    const auto qualityPreset = qualityPresets[ssr_quality.get()];

    ShaderGlobal::set_int(get_shader_variable_id("ssr_resolution", true), ssr_fullres.get());
    ShaderGlobal::set_int(get_shader_variable_id("ssr_quality", true), qualityPreset.qualityShaderVar);
    ssrWantsAlternateReflections = qualityPreset.alternateReflections;
    ssrFGNodes = makeScreenSpaceReflectionNodes(width, height, ssr_fullres.get(), ssr_denoiser.get(), ssrHQ ? TEXFMT_A16B16G16R16F : 0,
      qualityPreset.quality);
  }
  else
    ssrWantsAlternateReflections = false;
}

#if !_TARGET_PC && !_TARGET_ANDROID && !_TARGET_IOS && !_TARGET_C3
IPoint2 WorldRenderer::getFixedPostfxResolution(const IPoint2 &orig_resolution) const
{
  const char *resStr = ::dgs_get_settings()->getBlockByNameEx("video")->getStr("resolution", "1920x1080");
  if (strcmp(resStr, "auto") == 0)
    return orig_resolution;
  IPoint2 stringResolution;
  if (!get_resolution_from_str(resStr, stringResolution.x, stringResolution.y))
  {
    logerr("get_resolution_from_str('%s') failed. defaulting to original resolution: %dx%d", resStr, orig_resolution.x,
      orig_resolution.y);
    return orig_resolution;
  }
  IPoint2 scaledResolution = stringResolution;
  if (fabs(scaledResolution.x / float(orig_resolution.x) - scaledResolution.y / float(orig_resolution.y)) > 0.001f)
  {
    IPoint2 correctedResolution = scaledResolution.x > 0 && scaledResolution.y > 0
                                    ? IPoint2(scaledResolution.x, orig_resolution.y * scaledResolution.x / orig_resolution.x)
                                    : orig_resolution;
    logerr("Scaled resolution has a different aspect ratio than original resolution! Check \"video/resolution\" in config!\n"
           "scaledResolution: %ix%i, orig_resolution: %ix%i, resolution string from settings: %s, corrected resolution: %ix%i",
      scaledResolution.x, scaledResolution.y, orig_resolution.x, orig_resolution.y, resStr, correctedResolution.x,
      correctedResolution.y);
    scaledResolution = correctedResolution;
  }
  if (!(scaledResolution.x <= orig_resolution.x && scaledResolution.y <= orig_resolution.y))
  {
    logwarn("Scaled resolution was higher than original resolution, setting scaled resolution to original.\n"
            "scaledResolution: %ix%i, orig_resolution: %ix%i, resolution string from settings: %s",
      scaledResolution.x, scaledResolution.y, orig_resolution.x, orig_resolution.y, resStr);
    scaledResolution = orig_resolution;
  }
  return scaledResolution;
}
#endif

void WorldRenderer::getRenderingResolution(int &w, int &h) const
{
  if (antiAliasing)
  {
    IPoint2 resolution = antiAliasing->getInputResolution();
    w = resolution.x;
    h = resolution.y;
  }
  else
  {
    getPostFxInternalResolution(w, h);
  }
}

void WorldRenderer::getPostFxInternalResolution(int &w, int &h) const
{
  getDisplayResolution(w, h);
  IPoint2 displayResolution = IPoint2(w, h);

// TODO make this data driven
#if !_TARGET_PC && !_TARGET_ANDROID && !_TARGET_IOS && !_TARGET_C3
  w = fixedPostfxResolution.x;
  h = fixedPostfxResolution.y;
  return;
#endif

  if (currentAntiAliasingMode == AntiAliasingMode::DLSS || currentAntiAliasingMode == AntiAliasingMode::XESS ||
      currentAntiAliasingMode == AntiAliasingMode::FSR
#if _TARGET_C2

#endif
  )
  {
    // DLSS/XeSS/TSR run before the postfx pass, and upscale the image, so the postfx will be done at display resolution.
    return;
  }

  IPoint2 resolution = displayResolution;
  if (isSSAAEnabled())
  {
    resolution = getSSAAResolution(displayResolution);
  }
  else if (currentAntiAliasingMode == AntiAliasingMode::TSR)
  {
    // TAA cannot be used to scale resolution by itself, but we can scale after the postfx pass
    if (isFsrEnabled())
    {
      resolution = getFsrScaledResolution(displayResolution);
    }
  }
  else
  {
    // FXAA and OFF scale up after postfx
    resolution = getStaticScaledResolution(displayResolution);
  }
  w = resolution.x;
  h = resolution.y;
}

void WorldRenderer::getDisplayResolution(int &w, int &h) const
{
  if (overrideDisplayRes)
  {
    w = overridenDisplayRes.x;
    h = overridenDisplayRes.y;
  }
  else
    d3d::get_screen_size(w, h);
}

void WorldRenderer::overrideResolution(IPoint2 res)
{
  overridenDisplayRes = res;
  overrideDisplayRes = true;
  setResolution();
}

void WorldRenderer::resetResolutionOverride()
{
  overrideDisplayRes = false;
  setResolution();
}

float WorldRenderer::getMipmapBias() const
{
  float lodBias = antiAliasing ? antiAliasing->getLodBias() : 0.f;
  lodBias += isFsrEnabled() ? fsrMipBias : 0.0f;
  lodBias += isSSAAEnabled() ? ssaaMipBias : 0.0f;
  return taa_base_mip_scale.get() + lodBias;
}

eastl::optional<IPoint2> WorldRenderer::getDimsForVrsTexture(int screenWidth, int screenHeight)
{
  auto &drvDesc = d3d::get_driver_desc();
  if (!drvDesc.caps.hasVariableRateShadingTexture)
    return eastl::nullopt;

  int tileSizeX = static_cast<int>(drvDesc.variableRateTextureTileSizeX);
  int tileSizeY = static_cast<int>(drvDesc.variableRateTextureTileSizeY);
  G_ASSERT(tileSizeX > 0 && tileSizeY > 0);

  return IPoint2((screenWidth - 1) / tileSizeX + 1, (screenHeight - 1) / tileSizeY + 1);
}

void WorldRenderer::setAntialiasing()
{
  prepareForPostfxNoAANode = {};
  fxaaNode = {};
  ssaaNode = {};

  antiAliasing.reset();

  render::antialiasing::reinit();

  const auto antialiasingMethod = render::antialiasing::get_method();
  currentAntiAliasingMode = convertAntialiasingMethod(antialiasingMethod);

  IPoint2 postFxResolution;
  getPostFxInternalResolution(postFxResolution.x, postFxResolution.y);
  ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TEMPORAL); // default

  // TODO: Workaround for dynamic grass being noisy with TSR low if AA is forced on
  // We should drop support for TSR low except on consoles
  ShaderGlobal::set_int(grass_dynamic_flagVarId, 1);

  switch (currentAntiAliasingMode)
  {
    case AntiAliasingMode::DLSS:
      antiAliasing = eastl::make_unique<DeepLearningSuperSampling>(postFxResolution);
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TEMPORAL);
      break;
    case AntiAliasingMode::XESS:
      antiAliasing = eastl::make_unique<XeSuperSampling>(postFxResolution);
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TEMPORAL);
      break;
    case AntiAliasingMode::TSR:
      antiAliasing = eastl::make_unique<TemporalSuperResolution>(postFxResolution);
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TSR);
      ShaderGlobal::set_int(grass_dynamic_flagVarId, antiAliasing.get()->needMotionVectors());
      break;
    case AntiAliasingMode::FSR:
      antiAliasing = eastl::make_unique<FSR>(postFxResolution);
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TEMPORAL);
      break;
    case AntiAliasingMode::FXAA:
      fxaaNode = makeFXAANode(isStaticUpsampleEnabled() ? "frame_to_upscale" : "frame_after_postfx", !isStaticUpsampleEnabled());
      prepareForPostfxNoAANode = makePrepareForPostfxNoAANode();
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::NON_TEMPORAL);
      break;
    case AntiAliasingMode::SSAA:
      ssaaNode = makeSSAANode();
      ssaaMipBias = log2(float(getSSAAResolution(postFxResolution).y) / float(postFxResolution.y));
      prepareForPostfxNoAANode = makePrepareForPostfxNoAANode();
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::NON_TEMPORAL);
      break;
#if _TARGET_C2




#endif
    default: prepareForPostfxNoAANode = makePrepareForPostfxNoAANode(); break;
  }

  // if for some reason selected AA mode fails to init, use TSR as fallback and no AA if TSR fails too
  if (antiAliasing && !antiAliasing->isAvailable())
  {
    if (currentAntiAliasingMode != AntiAliasingMode::TSR)
    {
      currentAntiAliasingMode = AntiAliasingMode::TSR;
      render::antialiasing::set_method(render::antialiasing::AntialiasingMethod::TSR);
      antiAliasing = eastl::make_unique<TemporalSuperResolution>(postFxResolution);
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::TSR);
      ShaderGlobal::set_int(grass_dynamic_flagVarId, antiAliasing.get()->needMotionVectors());
    }
    if (!antiAliasing->isAvailable())
    {
      render::antialiasing::set_method(render::antialiasing::AntialiasingMethod::None);
      currentAntiAliasingMode = AntiAliasingMode::OFF;
      prepareForPostfxNoAANode = makePrepareForPostfxNoAANode();
    }
  }
}

void WorldRenderer::setResolution()
{
  int w, h;
  WorldRenderer::getDisplayResolution(w, h);

  createDebugTexOverlay(w, h);

  loadFsrSettings();

  antiAliasing.reset();
  closeStaticUpsample();
  closeFsr();
  prepareForPostfxNoAANode = {};

  IPoint2 displayResolution(w, h);
#if !_TARGET_PC && !_TARGET_ANDROID && !_TARGET_IOS && !_TARGET_C3
  fixedPostfxResolution = getFixedPostfxResolution(displayResolution);
#endif

  if (hasFeature(FeatureRenderFlags::TAA))
  {
    setAntialiasing();
  }
  else
  {
    currentAntiAliasingMode = AntiAliasingMode::FXAA;
    ssaaNode = {};
    prepareForPostfxNoAANode = makePrepareForPostfxNoAANode();
  }

  IPoint2 postFxResolution(w, h);
  getPostFxInternalResolution(postFxResolution.x, postFxResolution.y);
  dafg::set_resolution("post_fx", postFxResolution);

  motion_vector_access::set_motion_vector_type(
    needMotionVectors() ? motion_vector_access::MotionVectorType::DynamicUVZ : motion_vector_access::MotionVectorType::StaticUVZ);
  initStaticUpsample(displayResolution, postFxResolution); // Used on consoles and bareMinimum
  G_ASSERTF(!isFXAAEnabled() || resolutionScaleMode != RESOLUTION_SCALE_POSTFX,
    "FXAA must run in render resolution! video/upscaleInPostfx should be off");

  dynamicResolution.reset();
  if (::dgs_get_settings()->getBlockByNameEx("video")->getBlockByNameEx("dynamicResolution")->getInt("targetFPS", 0) > 0)
  {
    if (auto &caps = d3d::get_driver_desc().caps;
        antiAliasing && antiAliasing->supportsDynamicResolution() && caps.hasAliasedTextures && caps.hasResourceHeaps)
    {
      antiAliasing->setInputResolution(postFxResolution);
      dynamicResolution = eastl::make_unique<DynamicResolution>(postFxResolution.x, postFxResolution.y);
    }
    else
      debug("Dynamic Resolution was enabled in settings but not available due to %s",
        currentAntiAliasingMode != AntiAliasingMode::TSR
#if _TARGET_C2

#endif
          ? "anti-aliasing (need TSR)"
          : !caps.hasAliasedTextures ? "driver (need alias support)"
            : !caps.hasResourceHeaps ? "driver (need heaps support)"
                                     : "unknown");
  }

  darg::bhv_fps_bar.setRenderingResolution(dynamicResolution ? eastl::make_optional(postFxResolution) : eastl::nullopt);

  IPoint2 renderingResolution = antiAliasing ? antiAliasing->getInputResolution() : postFxResolution; //-V574
  w = renderingResolution.x;
  h = renderingResolution.y;

  target.reset();

  toggleMotionVectors(); // Create RT here, because it depends on motion vectors usage
  // water reflection
  const int reflW = w / 2, reflH = h / 2;

  if (isWaterSSREnabled())
    ShaderGlobal::set_float4(get_shader_variable_id("water_reflection_size", true), reflW, reflH, 1.0f / reflW, 1.0f / reflH);
  // initCloseDepth();

  // water-

  setFxQuality();

  initClipmap();

  ShaderGlobal::set_float4(rendering_resVarId, w, h, 1.0f / w, 1.0f / h);

  int halfW = w / 2, halfH = h / 2;
  ShaderGlobal::set_float4(lowres_rt_paramsVarId, halfW, halfH, 0, 0);

  ShaderGlobal::set_float4(lowres_tex_sizeVarId, w / 2, h / 2, 1.0f / (w / 2), 1.0f / (h / 2));

  // bool ssrCompute = ::dgs_get_settings()->getBool("ssrCompute", false);       // compute ssr turned off for now
  setSettingsSSR();
  updateSettingsSSR(w, h);

  resetSSAOImpl();

  {
    const char *downsample_cs_shader = nullptr;
    if (d3d::get_driver_code().is(d3d::dx12 || d3d::ps4 || d3d::ps5))
      downsample_cs_shader = "depth_hierarchy";
    downsample_depth::init("downsample_depth2x", downsample_cs_shader);

    storeDownsampledTexturesInEsram = h <= 944;
    downsampledTexturesMipCount = max(min(get_log2w(w / 2), get_log2w(h / 2)) - 1, 4);
    ShaderGlobal::set_int(downsampled_depth_mip_countVarId, downsampledTexturesMipCount);
  }

  copyDepth.init("copy_depth");


  emissionColorMaps.reset();
  emissionColorMaps = eastl::make_unique<EmissionColorMaps>();

  if (!hasFeature(FeatureRenderFlags::PREV_OPAQUE_TEX))
    ShaderGlobal::set_texture(water_refraction_texVarId, BAD_TEXTUREID);

  auto setRtControl = [this](RtControl nrt, bool force = false) {
    if (eastl::exchange(rtControl, nrt) != nrt || force)
      resetBackBufferTex();
  };
  if (hdrrender::is_hdr_enabled())
  {
    auto prevt = hdrrender::get_render_target_tex();
    hdrrender::set_resolution(displayResolution.x, displayResolution.y, isFsrEnabled()); // This might re-create tex
    setRtControl(RtControl::RT, /*force*/ prevt != hdrrender::get_render_target_tex());
    setFinalTargetTex(&hdrrender::get_render_target(), "[HDR, RT]");
  }
  else if (!d3d::get_backbuffer_tex() || overrideDisplayRes)
  {
    setRtControl(RtControl::OWNED_RT, /*force*/ true);
    // FIXME: finalTargetTex registered in FG as external texture
    // somehow if its format changes when switching from BB mode to OwnedRT mode
    // FG check is triggered about "changed formats in rendering", while it should not
    // here is workaround - use BB format to make check happy
    int bbFmt = TEXFMT_DEFAULT;
    if (d3d::get_backbuffer_tex())
    {
      TextureInfo info;
      d3d::get_backbuffer_tex()->getinfo(info);
      bbFmt = info.cflg & TEXFMT_MASK;
    }
    ownedBackbufferTex = dag::create_tex(NULL, displayResolution.x, displayResolution.y,
      TEXCF_RTARGET | bbFmt | (isFsrEnabled() ? TEXCF_UNORDERED : 0), 1, "final_target_frame");
    setFinalTargetTex(&ownedBackbufferTex, "[OwnedRT]");
  }
  else
  {
    setRtControl(RtControl::BACKBUFFER, /*force*/ !backbufferTex || backbufferTex.getBaseTex() != d3d::get_backbuffer_tex());
    updateBackBufferTex();
  }

  if (isFsrEnabled())
  {
    initFsr(postFxResolution, displayResolution);
  }
  ShaderGlobal::set_int(get_shader_glob_var_id("fsr_on", true), isFsrEnabled());

  changeStateFOMShadows();

  if (volumeLight)
  {
    const int volResW = (postFxResolution.x / 16 + 7) & ~7, volResH = (postFxResolution.y / 16 + 7) & ~7;
    const int volResD = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("volResDepth", 64);
    volumeLight->setResolution(volResW, volResH, volResD, w, h);
  }
  setPostFxResolution(postFxResolution.x, postFxResolution.y);
  IPoint2 maxPossibleRenderResolution;
  getMaxPossibleRenderingResolution(maxPossibleRenderResolution.x, maxPossibleRenderResolution.y);
  g_entity_mgr->broadcastEventImmediate(SetResolutionEvent(SetResolutionEvent::Type::SETTINGS_CHANGED, displayResolution,
    renderingResolution, postFxResolution, maxPossibleRenderResolution));
  initSubDivSettings();

  lights.setResolution(w, h);

  auto vrsDims = getDimsForVrsTexture(w, h);
  dafg::set_resolution("main_view", IPoint2{w, h});
  dafg::set_resolution("display", displayResolution);
  dafg::set_resolution("texel_per_vrs_tile", vrsDims ? vrsDims.value() : IPoint2());

  if (hdrrender::is_hdr_enabled())
    hdrrender::init(displayResolution.x, displayResolution.y, true, isFsrEnabled());
  else
    hdrrender::shutdown();

  resetVsyncMode();

  requestFgRecreation("setResolution");
}

void WorldRenderer::changeSingleTexturesResolution(int width, int height)
{
  int halfW = width / 2, halfH = height / 2;

  ShaderGlobal::set_float4(rendering_resVarId, width, height, 1.0f / width, 1.0f / height);
  ShaderGlobal::set_float4(lowres_rt_paramsVarId, halfW, halfH, 0, 0);
  ShaderGlobal::set_float4(lowres_tex_sizeVarId, halfW, halfH, 1.0f / halfW, 1.0f / halfH);

  if (target)
    target->changeResolution(width, height);
  lights.changeResolution(width, height);

  IPoint2 displayResolution, postFxResolution;
  getDisplayResolution(displayResolution.x, displayResolution.y);
  getPostFxInternalResolution(postFxResolution.x, postFxResolution.y);
  IPoint2 maxPossibleRenderResolution;
  getMaxPossibleRenderingResolution(maxPossibleRenderResolution.x, maxPossibleRenderResolution.y);
  g_entity_mgr->broadcastEventImmediate(SetResolutionEvent(SetResolutionEvent::Type::DYNAMIC_RESOLUTION, displayResolution,
    IPoint2(width, height), postFxResolution, maxPossibleRenderResolution));
}

void WorldRenderer::initSubDivSettings()
{
  subdivSettings = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("groundDisplacementQuality", 0);
}

void WorldRenderer::initIndoorProbesIfNecessary()
{
  if (!dgs_get_settings()->getBlockByNameEx("graphics")->getBool("supportsIndoorProbes", true))
  {
    indoorProbesScene.reset();
  }
  else if (lvl_override::getBool(levelSettings.get(), "IndoorReflectionProbes", true) && specularCubesContainer)
  {
    if (indoorProbesScene)
    {
      indoorProbeMgr.reset();
      indoorProbeNodes.reset();
      indoorProbeNodes = eastl::make_unique<IndoorProbeNodes>();
      indoorProbeMgr = eastl::make_unique<IndoorProbeManager>(specularCubesContainer.get(), indoorProbeNodes.get());
      indoorProbeMgr->resetScene(eastl::move(indoorProbesScene));
    }
  }
  else if (indoorProbeMgr)
  {
    indoorProbesScene = indoorProbeMgr->unloadScene();
    indoorProbeMgr.reset();
  }
}

void WorldRenderer::initResetable()
{
  initPostFx();
  const bool compatibilityMode = !hasFeature(FeatureRenderFlags::FULL_DEFERRED);
  if (!compatibilityMode)
  {
    preIntegratedGF.reset(new MultiFramePGF(PREINTEGRATE_SPECULAR_DIFFUSE, 256));
  }
  initDisplacement();
  initHmapPatches(512);
  depthAOAboveCtx = eastl::make_unique<DepthAOAboveContext>(DEPTH_AROUND_TEX_SIZE, DEPTH_AROUND_DISTANCE,
    needTransparentDepthAOAbove && transparentDepthAOAboveRequests);

// sadly no cubemap arrays on mobiles
// TODO: remove platform def here by using compatibility render on iOS
#if !_TARGET_APPLE
  // Light probes managers should be recreated before specular cubes container. TODO: Rework it to have more safe code.
  if (hasFeature(FeatureRenderFlags::SPECULAR_CUBES))
    specularCubesContainer = eastl::make_unique<LightProbeSpecularCubesContainer>();
#endif

  cube.reset(new RenderDynamicCube);

  WorldRenderer::setResolution();
  initRendinstVisibility();
}

void WorldRenderer::closeResetable()
{
  closeRendinstVisibility();
  cube.reset(nullptr);
  depthAOAboveCtx.reset(nullptr);
  preIntegratedGF.reset();
  if (indoorProbeMgr)
  {
    indoorProbesScene = indoorProbeMgr->unloadScene();
    indoorProbeMgr.reset();
  }
  specularCubesContainer.reset();
  closePostFx();
  uiBlurFallbackTexId.close();
  lowresWaterHeightmap.close();
}

bool WorldRenderer::forceEnableMotionVectors() const
{
  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
  return graphics->getBool("forceEnableMotionVectors", false) || is_denoiser_enabled() || can_use_motion_vrs();
}

bool WorldRenderer::needMotionVectors() const
{
  return (antiAliasing && antiAliasing->needMotionVectors()) || (!isThinGBuffer() && forceEnableMotionVectors());
}

bool WorldRenderer::needSeparatedUI() const { return antiAliasing && antiAliasing->isFrameGenerationEnabled(); }

bool WorldRenderer::needUIBlendingForScreenshot() const
{
  return antiAliasing && antiAliasing->isFrameGenerationEnabled() && !antiAliasing->needsUIBlending();
}

void WorldRenderer::toggleMotionVectors()
{
  hasMotionVectors = needMotionVectors();
  initTarget();

  requestFgRecreation("toggleMotionVectors");

  ShaderGlobal::set_int(has_motion_vectorsVarId, hasMotionVectors);
}

bool WorldRenderer::isUpsampling() const
{
  if (dynamicResolution)
    return true;
  IPoint2 renderingResolution;
  getRenderingResolution(renderingResolution.x, renderingResolution.y);
  IPoint2 displayResolution;
  getDisplayResolution(displayResolution.x, displayResolution.y);
  return renderingResolution != displayResolution;
}

bool WorldRenderer::isUpsamplingBeforePostfx() const
{
  IPoint2 renderingResolution;
  getRenderingResolution(renderingResolution.x, renderingResolution.y);
  IPoint2 displayResolution;
  getPostFxInternalResolution(displayResolution.x, displayResolution.y);
  return renderingResolution != displayResolution;
}

bool WorldRenderer::isGeneratingFrames() const { return antiAliasing && antiAliasing->isFrameGenerationEnabled(); }

void WorldRenderer::printResolutionScaleInfo() const
{
  int w, h;
  getRenderingResolution(w, h);
  console::print_d("Render resolution: %dx%d", w, h);
  getPostFxInternalResolution(w, h);
  console::print_d("PostFx Internal resolution: %dx%d", w, h);
  getDisplayResolution(w, h);
  console::print_d("Display resolution: %dx%d", w, h);

  if (dynamicResolution)
  {
    console::print_d("Dynamic resolution target: %d FPS", dynamicResolution->getTargetFrameRate());
    console::print_d("FPS limit: %d FPS", get_fps_limit());
    double vsyncFrameRate;
    d3d::driver_command(Drv3dCommand::GET_VSYNC_REFRESH_RATE, &vsyncFrameRate);
    console::print_d("Vsync refresh rate: %.2f Hz", vsyncFrameRate);
  }
  else
    console::print_d("Dynamic resolution: disabled");

  const char *aaMode = nullptr;
  switch (currentAntiAliasingMode)
  {
    case AntiAliasingMode::OFF: aaMode = "OFF"; break;
    case AntiAliasingMode::FXAA: aaMode = "FXAA"; break;
    case AntiAliasingMode::TSR: aaMode = "TSR"; break;
    case AntiAliasingMode::DLSS: aaMode = "DLSS"; break;
    case AntiAliasingMode::XESS: aaMode = "XESS"; break;
    case AntiAliasingMode::FSR: aaMode = "FSR"; break;
    case AntiAliasingMode::SSAA: aaMode = "SSAA"; break;
#if _TARGET_C2

#endif
    default: aaMode = "Invalid"; break;
  }
  console::print_d("AA mode: %s", aaMode);
  auto bool_to_string = [](bool b) { return b ? "true" : "false"; };
  console::print_d("Fsr enabled: %s", bool_to_string(isFsrEnabled()));
  console::print_d("Static Upsampling enabled: %s", bool_to_string(isStaticUpsampleEnabled()));
}

int WorldRenderer::getSubPixels() const
{
  // Temporary woraround for screenshots crashing with camcam
  if (camera_in_camera::is_lens_render_active())
    return 1;
  if (!VariableMap::isGlobVariablePresent(sub_pixelsVarId) || isUpsampling() || applySettingsAfterResetDevice ||
      isGeneratingFrames() || is_bvh_enabled())
    return 1;
  // TODO:: temporarily ignore resource hog settings in order to reduce audio out - of - sync / glitching issues
  if (is_recording())
    return 1;
  if (force_sub_pixels > 0)
    return force_sub_pixels;
  if (screencap::is_screenshot_scheduled())
    return is_cinematic_mode_enabled() ? get_cinematic_mode()->getSettings()->sub_pixels : screencap::subsamples();
  return 1;
}

int WorldRenderer::getSuperPixels() const
{
  // Temporary woraround for screenshots crashing with camcam
  if (camera_in_camera::is_lens_render_active())
    return 1;
  if (!VariableMap::isGlobVariablePresent(super_pixelsVarId) || !VariableMap::isGlobVariablePresent(interleave_pixelsVarId) ||
      !VariableMap::isGlobVariablePresent(super_screenshot_texVarId) || isUpsampling() || applySettingsAfterResetDevice ||
      isGeneratingFrames() || is_bvh_enabled())
    return 1;
  // TODO:: temporarily ignore resource hog settings in order to reduce audio out - of - sync / glitching issues
  if (is_recording())
    return 1;
  if (force_super_pixels > 0)
    return force_super_pixels;
  if (!screencap::should_hide_gui() && !is_recording())
    return 1;
  return is_cinematic_mode_enabled() ? get_cinematic_mode()->getSettings()->super_pixels : screencap::supersamples();
}

void WorldRenderer::forceStaticResolutionScaleOff(const bool force)
{
  debug("forcing static resolution scale to %s", force ? "off" : "on");
  forceStaticResolutionOff = force;

  const float staticResolutionScale = cachedStaticResolutionScale;
  const bool needUpdateResolution = staticResolutionScale != 0 && staticResolutionScale != 100.0;
  if (needUpdateResolution)
    setResolution();
}

void WorldRenderer::updateEnviCoverCompatibility()
{
  if (!hasFeature(FeatureRenderFlags::FULL_DEFERRED))
  {
    isEnviCoverCompatible = false;
    return;
  }

  const int gbuffFmtCount = needMotionVectors() ? FULL_GBUFFER_RT_COUNT : NO_MOTION_GBUFFER_RT_COUNT;
  for (int i = 0; i < gbuffFmtCount; i++)
  {
    if (!(d3d::get_texformat_usage(FULL_GBUFFER_FORMATS[i] | TEXCF_UNORDERED) & d3d::USAGE_UNORDERED_LOAD))
    {
      isEnviCoverCompatible = false;
      return;
    }
  }

  isEnviCoverCompatible = true;
}

void WorldRenderer::initTarget()
{
  updateEnviCoverCompatibility();

  dag::RelocatableFixedVector<uint32_t, FULL_GBUFFER_RT_COUNT, false> main_gbuf_fmts{};
  uint32_t gbuf_cnt = FULL_GBUFFER_RT_COUNT, globalFlags = GBUF_TARGET_GLOBAL_FLAGS;
  if (!isThinGBuffer())
  {
    gbuf_cnt = needMotionVectors() ? FULL_GBUFFER_RT_COUNT : NO_MOTION_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < FULL_GBUFFER_RT_COUNT; ++i)
      main_gbuf_fmts.push_back(FULL_GBUFFER_FORMATS[i]);
  }
  else
  {
    G_ASSERTF(!needMotionVectors(), "Motion vectors are not supported with thin gbuffer!");
    gbuf_cnt = THIN_GBUFFER_RT_COUNT;
    for (uint32_t i = 0; i < THIN_GBUFFER_RT_COUNT; ++i)
      main_gbuf_fmts.push_back(THIN_GBUFFER_RT_FORMATS[i]);
  }

  {
    if (isEnviCoverCompatible && isEnviCover)
      globalFlags |= TEXCF_UNORDERED;

    prepareGbufferFGNode = makePrepareGbufferNode(globalFlags, gbuf_cnt, main_gbuf_fmts, hasMotionVectors, is_rr_enabled());
  }

  requestFgRecreation("initTarget");
}

void WorldRenderer::resetVolumeLights()
{
  if (VolumeLight::IS_SUPPORTED && hasFeature(FeatureRenderFlags::VOLUME_LIGHTS))
  {
    volumeLight.reset(new VolumeLight);
    volumeLight->init();
  }
  else
  {
    volumeLight.reset();
  }

  requestFgRecreation("resetVolumeLights");
}

static UniqueTex gpu_hang_uav;
static eastl::unique_ptr<ComputeShaderElement> gpu_hang_cs;
static void hang_gpu()
{
  // Here we are activating the TDR mechanism inside the driver
  if (!gpu_hang_uav)
    gpu_hang_uav = dag::create_tex(NULL, 1, 1, TEXFMT_R32UI | TEXCF_UNORDERED, 1, "gpu_hang_uav");
  if (!gpu_hang_cs)
    gpu_hang_cs.reset((new_compute_shader("hang_gpu_cs", false)));

  static int gpu_hang_uav_no = ShaderGlobal::get_slot_by_name("gpu_hang_uav_no");

  d3d::set_rwtex(STAGE_CS, gpu_hang_uav_no, gpu_hang_uav.getBaseTex(), 0, 0, true);

  gpu_hang_cs->dispatch(10, 10, 1);
}

static void free_gpu_hang_resources()
{
  gpu_hang_uav.close();
  gpu_hang_cs.reset();
}

void WorldRenderer::ctorCommon()
{
  extern bool add_occluders; // do not add rendinst occluders, we don't have good yet
  add_occluders = false;

  d3dhang::register_gpu_hanger(hang_gpu);

  if (d3d::get_driver_code().is(d3d::dx12))
    d3d::driver_command(Drv3dCommand::SET_DRIVER_NETWORD_MANAGER, new DriverNetworkManager(get_exe_version_str()));

  ShaderGlobal::set_int(dgs_tex_anisotropyVarId, ::dgs_tex_anisotropy);

#if DA_PROFILER_ENABLED
  char dapMarkerName[32] = "prepare_render_csm_0";
  for (int i = 0; i < animchar_csm_desc.size(); ++i)
  {
    dapMarkerName[sizeof("prepare_render_csm_0") - 2] = '0' + i; // Faster then snprintf
    animchar_csm_desc[i] = DA_PROFILE_ADD_LOCAL_DESCRIPTION(0, dapMarkerName);
  }
#endif

  set_add_lod_bias_cb(reset_bindless_samplers);
  ShaderGlobal::set_int(ShaderVariableInfo{"can_use_half_precision", true},
    d3d::get_driver_desc().caps.hasShaderFloat16Support ? 1 : 0);

  FeatureRenderFlagMask featureRenderFlags = getPresetFeatures();
  defaultFeatureRenderFlags = featureRenderFlags;
  set_current_render_features(featureRenderFlags);

  FeatureRenderFlagMask changedFeatures = FeatureRenderFlagMask().set();

  g_entity_mgr->broadcastEventImmediate(ChangeRenderFeatures(featureRenderFlags, changedFeatures));

  logdbg("Running with featureRenderFlags: %s", render_features_to_string(featureRenderFlags));

  if (::dgs_get_settings()->getBlockByNameEx("ui")->getBool("hide", false))
    toggle_hide_gui();

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");

  tiledRenderArch = graphicsBlk->getBool("tiledRenderArch", d3d::get_driver_desc().caps.hasTileBasedArchitecture);

  if (tiledRenderArch)
    debug("use tiled render architecture");

  use_occlusion.set(graphicsBlk->getBool("use_occlusion", use_occlusion.get()));

  init_draw_cached_debug_twocolored_shader();

  auto satelliteParamsGetterCb = [this]() -> SatelliteRenderer::CallbackParams {
    SatelliteRenderer::CallbackParams satelliteCb;
    satelliteCb.renderWater = [this](Texture *color_target, const CameraParams &camera, Texture *depth) -> void {
      d3d::set_render_target({depth, 0}, DepthAccess::SampledRO, {{color_target, 0}});
      renderWater(camera, WorldRenderer::DistantWater::No, false);
    };
    satelliteCb.renderLandmesh = [this](mat44f_cref globtm, const TMatrix4 &proj, const Frustum &frustum,
                                   const Point3 &view_pos) -> void {
      if (lmeshRenderer && clipmap)
      {
        lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH);
        clipmap->startUAVFeedback();
        lmeshRenderer->render(globtm, proj, frustum, *lmeshMgr, LandMeshRenderer::RENDER_WITH_CLIPMAP, view_pos);
        clipmap->endUAVFeedback();
      }
    };
    satelliteCb.clipmapGetLastUpdatedTileCount = [this]() -> int { return clipmap ? clipmap->getLastUpdatedTileCount() : 0; };
    satelliteCb.landmeshHeightGetter = [this](const Point2 &pos2d, float &height) -> bool {
      return lmeshMgr->getHeight(pos2d, height, NULL);
    };
    return satelliteCb;
  };
  satelliteRenderer.initParamsGetter(satelliteParamsGetterCb);

  setWater(nullptr);
}

void WorldRenderer::ctorDeferred()
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");

  resolveHZBBeforeDynamic.set(graphicsBlk->getBool("resolveHZBBeforeDynamic", !tiledRenderArch));
  applyFXAASettings();
  use_occlusion_for_shadows.set(graphicsBlk->getBool("use_occlusion_for_shadows", use_occlusion_for_shadows.get()));
  use_occlusion_for_gpu_objs.set(graphicsBlk->getBool("use_occlusion_for_gpu_objs", use_occlusion_for_gpu_objs.get()));
  lodDistanceScaleBase = graphicsBlk->getReal("lodDistanceScaleBase", lodDistanceScaleBase);
  cachedStaticResolutionScale =
    dgs_get_settings()->getBlockByNameEx("video")->getReal("staticResolutionScale", get_default_static_resolution_scale());
  useFullresClouds = graphicsBlk->getBool("HQVolumetricClouds", false);
  waterReflectionEnabled = graphicsBlk->getBool("water_reflection_enabled", true);
  ShaderGlobal::set_int(get_shader_variable_id("ssr_alternate_reflections_ignore_roughness", true),
    ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("HQSSR", false) ? 1 : 0);

  toggle_normal_packing.set(hasFeature(GBUFFER_PACKED_NORMALS));

  if (VariableMap::isGlobVariablePresent(compatibility_modeVarId))
    ShaderGlobal::set_int(compatibility_modeVarId, hasFeature(FeatureRenderFlags::FULL_DEFERRED) ? 0 : 1);

  subdivCellSize.set(graphicsBlk->getReal("hmapSubdivCellSize", subdivCellSize.get()));

  G_ASSERT(d3d::get_driver_desc().shaderModel >= 5.0_sm);
  riOcclusionData = rendinst::createOcclusionData();

  dynamic_lights.set(hasFeature(FeatureRenderFlags::CLUSTERED_LIGHTS));
  if (dynamic_lights.get())
  {
    lights.init(::dgs_get_game_params()->getInt("lightsCount", dynamic_lights_initial_count.get()), getDynamicShadowQuality(),
      hasFeature(TILED_LIGHTS));
    shadowsManager.changeSpotShadowBiasBasedOnQuality();
    setDynamicShadowsMaxUpdatePerFrame();
    shadowRenderExtender.reset(new DynamicShadowRenderExtender([this] {
      // TODO: if many extensions are registered one by one, this clearly wastes work.
      // However, currently there is only 1 (or zero).
      this->requestFgRecreation("DynamicShadowRenderExtender callback");
    }));
  }
  prepass.set(graphicsBlk->getBool("prepass", prepass.get()));
  rendinst::render::useRiCellsDepthPrepass(prepass.get());
  rendinst::render::useRiDepthPrepass(prepass.get());

  loadCharacterMicroDetails(::dgs_get_game_params()->getBlockByName("character_micro_details"));

  enviProbe = NULL;

  if (::dgs_get_game_params()->getBool("use_water_render", true))
  {
    const char *foamTexName = "water_surface_foam_tex";
    TEXTUREID waterFoamTexId = ::get_tex_gameres(foamTexName); // mem leak
    G_ASSERTF(waterFoamTexId != BAD_TEXTUREID, "water foam texture '%s' not found.", foamTexName);
    ShaderGlobal::set_texture(get_shader_variable_id("foam_tex"), waterFoamTexId);
    release_managed_tex(waterFoamTexId);

    if (hasFeature(FeatureRenderFlags::WATER_PERLIN))
    {
      const char *perlinName = "water_perlin";
      TEXTUREID perlinTexId = ::get_tex_gameres(perlinName); // mem leak
      G_ASSERTF(perlinTexId != BAD_TEXTUREID, "water perlin noise texture '%s' not found.", perlinName);
      ShaderGlobal::set_texture(get_shader_variable_id("perlin_noise"), perlinTexId);
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.mip_map_mode = d3d::MipMapMode::Linear;
        smpInfo.filter_mode = d3d::FilterMode::Linear;
        ShaderGlobal::set_sampler(get_shader_variable_id("perlin_noise_samplerstate"), d3d::request_sampler(smpInfo));
      }
      release_managed_tex(perlinTexId);
    }
  }

  if (::dgs_get_game_params()->getBool("use_glass_scratch_render", true))
  {
    const DataBlock *glassScratchesBlk = ::dgs_get_game_params()->getBlockByNameEx("glassScratches");
    ShaderGlobal::set_float4(scratch_paramsVarId, glassScratchesBlk->getReal("texcoord_mul", 10.f),
      glassScratchesBlk->getReal("alpha_mul", 0.f), glassScratchesBlk->getReal("radius_pow", 45.f),
      glassScratchesBlk->getReal("mul", 4.f));
    glassShadowK = ::dgs_get_game_params()->getReal("glassShadowK", 0.7f);
    ShaderGlobal::set_float(glass_shadow_kVarId, glassShadowK);
    const char *scratchTexName = "glass_scratch_n";
    TEXTUREID scratchTexId = ::get_tex_gameres(scratchTexName);
    const char *scratchTexVarName = "scratch_tex";
    int scratchTexVarId = get_shader_variable_id(scratchTexVarName, true);
    if (scratchTexVarId >= 0 && scratchTexId != BAD_TEXTUREID)
    {
      ShaderGlobal::set_texture(scratchTexVarId, scratchTexId);
      ShaderGlobal::set_sampler(get_shader_variable_id("scratch_tex_samplerstate", false), d3d::request_sampler({}));
      release_managed_tex(scratchTexId);
    }
    else
    {
      if (scratchTexVarId < 0)
        logerr("WorldRenderer::init: Couldn't find shaderVar '%s'", scratchTexVarName);
      if (scratchTexId == BAD_TEXTUREID)
        logerr("WorldRenderer::init: Couldn't find gameres tex '%s", scratchTexName);
    }
  }

  rendinst::render::tmInstColored = false; // rendinst::render::tmInstColored = true; it is used only on one stone in landing
  rendinst::rendinstClipmapShadows = false;
  rendinst::rendinstGlobalShadows = true;

  rendinst::set_billboards_vertical(false);
  setRendinstTesselation();

  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading(); // or it will crash here

  underWater.init("underwater_fog");
  shoreRenderer.buildUI();
  closeDistantHeightmap();
  resetVolumeLights();
  enviCover.reset(new EnviCover);

  {
    d3d::GpuAutoLock gpuLock;
    initResetable();

    if (::dgs_get_game_params()->getBool("use_da_skies", true))
      init_daskies();
    if (get_daskies())
    {
      setSkies(get_daskies());
      if (hasFeature(FeatureRenderFlags::VOLUME_LIGHTS))
        get_daskies()->projectUses2DShadows(true); // volfog can only use 2d cloud shadows
    }
    else
    {
      ShaderGlobal::set_int4(get_shader_variable_id("skies_froxels_resolution", true), 1, 1, 1, 1);
      ShaderGlobal::set_float(get_shader_variable_id("clouds_thickness2", true), 1);
    }
  }

  if (::dgs_get_game_params()->getBool("use_fluid_wind", true))
    init_fluid_wind();

  clipmap_decals_mgr::init();

  initOverrideStates();
  init_and_get_hash_128_noise();
  Point3 min_r, max_r;
  init_and_get_perlin_noise_3d(min_r, max_r);


  updateImpostorSettings();
  resetLatencyMode();
  resetVsyncMode();
  resetPerformanceMetrics();
  resetDynamicQuality();

  requestFgRecreation("ctorDeferred");
  dynmodel_renderer::init_dynmodel_rendering(ShadowsManager::CSM_MAX_CASCADES);

  setSharpeningFromSettings();
  setChromaticAberrationFromSettings();
  setFilmGrainFromSettings();
  setEnviromentDetailsQuality();

  camera_in_camera::setup(hasFeature(CAMERA_IN_CAMERA));

  render::antialiasing::close(); // in case if we have initialized it before already
  render::antialiasing::init(&aaGlue);

#if DAGOR_DBGLEVEL > 0
  gpu_mem_dumper::init();
#endif
}

void WorldRenderer::setChromaticAberrationFromSettings()
{
  static constexpr int CHROMATIC_ABERRATION_PRIORITY = 0;
  bool chromaticAberration =
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("chromaticAberration", false) && !bareMinimumPreset;
  Point3 value = dgs_get_settings()->getBlockByNameEx("cinematicEffects")->getPoint3("chromaticAberration", Point3(0.02, 0.01, 0.75));
  if (chromaticAberration)
    PriorityShadervar::set_float4(chromatic_aberration_paramsVarId.get_var_id(), CHROMATIC_ABERRATION_PRIORITY, Point4::xyz0(value));
  else
    PriorityShadervar::clear(chromatic_aberration_paramsVarId.get_var_id(), CHROMATIC_ABERRATION_PRIORITY);
}

void WorldRenderer::setFilmGrainFromSettings()
{
  static constexpr int FILM_GRAIN_PRIORITY = 0;
  bool filmGrain = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("filmGrain", false) && !bareMinimumPreset;
  Point3 value = dgs_get_settings()->getBlockByNameEx("cinematicEffects")->getPoint3("filmGrain", Point3(0.2, 1.0, 0.75));
  if (filmGrain)
    PriorityShadervar::set_float4(film_grain_paramsVarId.get_var_id(), FILM_GRAIN_PRIORITY, Point4::xyz0(value));
  else
    PriorityShadervar::clear(film_grain_paramsVarId.get_var_id(), FILM_GRAIN_PRIORITY);
}

template <typename T>
using BlobHandle = dafg::VirtualResourceHandle<T, false, false>;

// TODO: put it in FG
void WorldRenderer::recreateCloudsVolume()
{
  enum
  {
    CLOUD_VOLUME_W = 32,
    CLOUD_VOLUME_H = 16,
    CLOUD_VOLUME_D = 32
  };

  cloudsVolume.close();
  cloudsVolume = dag::create_voltex(CLOUD_VOLUME_W, CLOUD_VOLUME_H, CLOUD_VOLUME_D, TEXFMT_R8 | TEXCF_UNORDERED, 1, "cloud_volume");

  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
    ShaderGlobal::set_sampler(::get_shader_variable_id("cloud_volume_samplerstate", true), d3d::request_sampler(smpInfo));
  }
}

void WorldRenderer::createNodes()
{
  // ██████████████████████████████████████████████████████████████
  // ╔════════════════════════════════════════════════════════════╗
  // ║                       (ﾉಥ益ಥ)ﾉ彡┻━┻                       ║
  // ║                                                            ║
  // ║         !!!DO NOT CREATE ANY MORE FG NODES!!!              ║
  // ║         !!!DO NOT CREATE ANY MORE FG NODES!!!              ║
  // ║         !!!DO NOT CREATE ANY MORE FG NODES!!!              ║
  // ║               !!!!IN WORLD RENDERER!!!!                    ║
  // ╚════════════════════════════════════════════════════════════╝
  // ██████████████████████████████████████████████████████████████


  fgNodeHandles.clear();
  resSlotHandles.clear();

  dafg::set_multiplexing_default_mode(
    dafg::multiplexing::Mode::SubSampling | dafg::multiplexing::Mode::SuperSampling | dafg::multiplexing::Mode::Viewport,
    dafg::multiplexing::Mode::Viewport | dafg::multiplexing::Mode::CameraInCamera);

  if (isFsrEnabled())
  {
    auto fsrNodes = makeFsrNodes();
    for (auto &node : fsrNodes)
      fgNodeHandles.push_back(eastl::move(node));
  }
  else
  {
    if (isFXAAEnabled())
      fxaaNode = makeFXAANode(isStaticUpsampleEnabled() ? "frame_to_upscale" : "frame_after_postfx", !isStaticUpsampleEnabled());

    if (isStaticUpsampleEnabled())
      fgNodeHandles.emplace_back(makeStaticUpsampleNode(isFXAAEnabled() ? "frame_to_upscale" : "postfxed_frame"));
  }

  shadowsManager.initVisibilityNode();
  shadowsManager.initShadowsDownsampleNode();

  // Clearing this will force postfxTargetProducerNode and externalFinalFrameControlNodes to be recreated next frame
  subSuperSamplingNodes.clear();

  recreate_motion_blur_nodes({&motionBlurAccumulateNode, &motionBlurApplyNode, &motionBlurStatus});

  const bool aaNeedsHistory = antiAliasing && antiAliasing->needDepthHistory();
  const bool giNeedsHistory = giNeedsReprojection();
  const bool needDepthHistory = aaNeedsHistory || giNeedsHistory;
  g_entity_mgr->broadcastEventImmediate(OnCameraNodeConstruction{
    &fgNodeHandles, &resSlotHandles, prepass.get(), giNeedsReprojection(), needDepthHistory, bareMinimumPreset, hasMotionVectors});
}

bool WorldRenderer::isFsrEnabled() const
{
  return fsrEnabled && currentAntiAliasingMode == AntiAliasingMode::TSR &&
         TemporalSuperResolution::parse_preset() == TemporalSuperResolution::Preset::Low;
}

void WorldRenderer::debugRecreateNodes()
{
  if (resolveHZBBeforeDynamic.pullValueChange())
  {
    requestFgRecreation("resolveHZBBeforeDynamic value changed");
  }

  if (process_reactive_gbuffer_data.pullValueChange())
    requestFgRecreation("process_reactive_gbuffer_data value changed");

  if (toggle_normal_packing.pullValueChange())
  {
    toggleFeatures(FeatureRenderFlagMask{}.set(FeatureRenderFlags::GBUFFER_PACKED_NORMALS), toggle_normal_packing.get());
  }

  if (volfog_enabled.pullValueChange())
  {
    shadowsManager.initShadowsDownsampleNode();
    requestFgRecreation("volfog_enabled value changed");
    // TODO: Why is this here? Why is this siwtchOff needed?
    // Who turns it back on?
    if (volumeLight != nullptr && !volfog_enabled.get())
      volumeLight->switchOff();
  }

  if (envi_cover_disable_fg_nodes.pullValueChange() || envi_cover_use_NBS.pullValueChange())
  {
    requestFgRecreation("Envi Cover setting change");
  }

  if (::pull_depth_bounds_enabled())
  {
    requestFgRecreation("pull_depth_bounds_enabled");
    shadowsManager.initCombineShadowsNode();
  }

  if (dynamic_lights.pullValueChange())
    requestFgRecreation("dynamic_lights value changed");

  if (async_animchars_main.pullValueChange())
    requestFgRecreation("async_animchars_main value changed");
}

void WorldRenderer::createFinalOpaqueControlNodes()
{
  finalOpaqueControlNodes.clear();

  // We do a special dance here to get robust ordering of various layers
  // of semi-transparent-but-not-really objects: sky and water.
  // We also optionally do various blurs on the opaque color like for SSSS.
  // TODO: this should all probably be part of the transparent namespace,
  // but right now refactoring it that way is very hard.

  finalOpaqueControlNodes.push_back(dafg::register_node("start_water_before_clouds", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.rename("opaque_resolved", "opaque_with_water_before_clouds").texture();
    registry.rename("gbuf_depth_after_resolve", "opaque_depth_with_water_before_clouds").texture();
  }));

  finalOpaqueControlNodes.push_back(dafg::register_node("start_envi", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.rename("opaque_with_water_before_clouds", "opaque_with_envi").texture(); }));

  finalOpaqueControlNodes.push_back(dafg::register_node("start_opaque_postprocessing", DAFG_PP_NODE_SRC,
    [](dafg::Registry registry) { registry.rename("opaque_with_envi", "opaque_processed").texture(); }));

  finalOpaqueControlNodes.push_back(dafg::register_node("start_water_after_clouds", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.rename("opaque_processed", "opaque_with_envi_and_water").texture();
    registry.rename("opaque_depth_with_water_before_clouds", "opaque_depth_with_water").texture();
  }));
}

void WorldRenderer::setEnviCover(bool envi_cover)
{
  isEnviCover = envi_cover && !bareMinimumPreset;
  initTarget(); // Need target recreation for optional UAV flag
  requestFgRecreation("Envi Cover ecs loaded");
}

WaterRenderMode WorldRenderer::determineWaterRenderMode(bool underWater, bool belowClouds)
{
  return underWater ? WaterRenderMode::LATE : belowClouds ? WaterRenderMode::EARLY_AFTER_ENVI : WaterRenderMode::EARLY_BEFORE_ENVI;
}

// Note: unfortunately can't be put in header due to dtor req of incomplete types (e.g. unique_ptr) for exception handlers
WorldRenderer::WorldRenderer()
{
  // Note: do not put anything here, use one of the methods below
  ctorCommon();
  ctorDeferred();
}

WorldRenderer::~WorldRenderer()
{
  close();
  defrag_shaders_stateblocks(true);
}

void WorldRenderer::close()
{
  satelliteRenderer.cleanupResources();
  shader_assert::close();
  free_gpu_hang_resources();
  release_hash_128_noise();
  release_perline_noise_3d();
  waitAllJobs();
  flipCullStateId.reset();
  shaders::overrides::destroy(diffuseOverride);
  shaders::overrides::destroy(specularOverride);
  closeResetable();
  closeGround();
  closeClipmap();
  closeDeforms();
  closeCharacterMicroDetails();
  get_daskies()->destroy_skies_data(main_pov_data);
  get_daskies()->destroy_skies_data(cube_pov_data);
  closeWaterPlanarReflection();
  aoFGNodes = {};
  ssrFGNodes.clear();
  shadowsManager.closeShadows();
  fomShadowManager.reset();
  closeOverrideStates();
  binScene = NULL;
  // closeGround();
  close_draw_cached_debug();

  downsample_depth::close();


  light_probe::destroy(enviProbe);
  enviProbe = nullptr;


  ShaderGlobal::reset_textures();

  del_it(debug_tex_overlay);
  target.reset();
  closeGI();
  giBlk.reset();
  lights.close();

  indoorProbeMgr.reset();
  specularCubesContainer.reset();
  term_daskies();

  close_fluid_wind();
  clipmap_decals_mgr::release();
  rendinst::destroyOcclusionData(riOcclusionData);

  hdrrender::shutdown();
  resetBackBufferTex();
  reset_fx_textures_used();

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_GLOBAL_CONST);
  dynmodel_renderer::close_dynmodel_rendering();
  giWindows.reset();
  hzbUploadStagingTex.close();
  render::antialiasing::close();
}

void WorldRenderer::initOverrideStates()
{
  shaders::OverrideState state;
  state.set(shaders::OverrideState::Z_CLAMP_ENABLED);
  depthClipState = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_FUNC);
  state.zFunc = CMPF_ALWAYS;
  zFuncAlwaysStateId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_FUNC | shaders::OverrideState::Z_WRITE_DISABLE);
  state.zFunc = CMPF_EQUAL;
  zFuncEqualStateId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::FLIP_CULL);
  flipCullStateId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::BLEND_SRC_DEST);
  state.sblend = BLEND_ONE;
  state.dblend = BLEND_ONE;
  additiveBlendStateId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::Z_BOUNDS_ENABLED);
  enabledDepthBoundsId = shaders::overrides::create(state);

  state = shaders::OverrideState();
  state.set(shaders::OverrideState::CULL_NONE);
  nocullState = shaders::overrides::create(state);
}

WRDispatcher::CommonOverrideStates WRDispatcher::getCommonOverrideStates()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  return {wr->depthClipState, wr->flipCullStateId, wr->zFuncAlwaysStateId, wr->additiveBlendStateId, wr->enabledDepthBoundsId,
    wr->zFuncEqualStateId};
}

void WorldRenderer::closeOverrideStates() { shaders::overrides::destroy(depthClipState); }

void WorldRenderer::updateLightShadow(light_id light, const LightShadowParams &shadow_params)
{
  bool need_shadows = shadow_params.isEnabled;
  ShadowCastersFlag old_casters;
  bool old_hint_dynamic;
  uint8_t old_priority, old_shadow_size_srl;
  uint16_t old_quality;
  DynamicShadowRenderGPUObjects render_gpu_objects, old_render_gpu_objects;
  render_gpu_objects = shadow_params.supportsGpuObjects ? DynamicShadowRenderGPUObjects::YES : DynamicShadowRenderGPUObjects::NO;
  bool had_shadows =
    getShadowProperties(light, old_casters, old_hint_dynamic, old_quality, old_priority, old_shadow_size_srl, old_render_gpu_objects);
  bool shadows_changed = (had_shadows != need_shadows);
  ShadowCastersFlag casters = (shadow_params.supportsDynamicObjects ? ShadowCastersFlag::Dynamic : ShadowCastersFlag::None) |
                              (shadow_params.approximateStatic ? ShadowCastersFlag::ApproximateStatic : ShadowCastersFlag::None) |
                              (shadow_params.isTwoSided ? ShadowCastersFlag::TwoSided : ShadowCastersFlag::None);
  if (!shadows_changed && had_shadows)
    shadows_changed = casters != old_casters || old_hint_dynamic != shadow_params.isDynamic || old_quality != shadow_params.quality ||
                      old_priority != shadow_params.priority || old_shadow_size_srl != shadow_params.shadowShrink ||
                      old_render_gpu_objects != render_gpu_objects;

  if (shadows_changed)
  {
    if (had_shadows)
      removeShadow(light);
    if (need_shadows)
    {
      addShadowToLight(light, casters, shadow_params.isDynamic, static_cast<uint16_t>(shadow_params.quality),
        static_cast<uint8_t>(shadow_params.priority), static_cast<uint16_t>(shadow_params.shadowShrink), render_gpu_objects);
    }
  }
}

void WorldRenderer::debugLogFrameTiming()
{
  constexpr int logAmount = 2;
  Drv3dTimings frameTiming;

  for (intptr_t i = 0; i < logAmount; ++i)
  {
    int loggedFrames = d3d::driver_command(Drv3dCommand::GET_TIMINGS, (void *)&frameTiming, (void *)i);
    if (i >= loggedFrames)
    {
      visuallog::logmsg(String(0, "Driver provided timings for %u frames", i));
      return;
    }

    float frameTime = ref_time_delta_to_usec(frameTiming.frontendUpdateScreenInterval) / 1000.0f;
    float frameLatency = ref_time_delta_to_usec(frameTiming.frontendToBackendUpdateScreenLatency) / 1000.0f;
    float frameFrontBackWait = ref_time_delta_to_usec(frameTiming.frontendBackendWaitDuration) / 1000.0f;
    float frameBackFrontWait = ref_time_delta_to_usec(frameTiming.backendFrontendWaitDuration) / 1000.0f;
    float frameGPUWait = ref_time_delta_to_usec(frameTiming.gpuWaitDuration) / 1000.0f;
    float swapSwapWait = ref_time_delta_to_usec(frameTiming.presentDuration) / 1000.0f;
    float getSwapWait = ref_time_delta_to_usec(frameTiming.backbufferAcquireDuration) / 1000.0f;
    float frontSwapchainWait = ref_time_delta_to_usec(frameTiming.frontendWaitForSwapchainDuration) / 1000.f;

    visuallog::logmsg(String(0, "Frame %u (ms): total %.2f, latency %.2f, GPU wait %.2f", i, frameTime, frameLatency, frameGPUWait));
    visuallog::logmsg(String(0, "  f->b %.2f, b->f %.2f, present %.2f, back buffer get %.2f, ll wait %.2f", frameFrontBackWait,
      frameBackFrontWait, swapSwapWait, getSwapWait, frontSwapchainWait));
  }
}


void WorldRenderer::update(float dt, float, const TMatrix &itm)
{
  profiler_tracker::start_frame();

  {
    if (sw_occlusion)
    {
      mainCameraVisibilityMgr.runOcclusionRasterizerCleaners();
      if (camera_in_camera::is_lens_render_active())
        camcamVisibilityMgr.runOcclusionRasterizerCleaners();
    }

    IPoint2 res = get_display_resolution(); // used in sparks to approximate display pixel size.
    acesfx::setup_camera_and_debug(dt, currentFrameCamera.viewItm, currentFrameCamera.jitterPersp, res.x, res.y);

    acesfx::start_fx_managers_update_job(dt);
    threadpool::wake_up_all_delayed(threadpool::PRIO_HIGH);
  }

  setDeformsOrigin(itm.getcol(3));

  if (auto portalRenderer = portal_renderer_mgr::query_portal_renderer())
    portalRenderer->update(dt);

  if (isDebugLogFrameTiming)
    debugLogFrameTiming();

  generatePaintingTexture(); // in update because needed textures is loading after level loaded
  update_and_prefetch_fx_textures_used();
#if TIME_PROFILER_ENABLED
  render::imgui_profiler::update_profiler(dagor_frame_no());
#endif
}

void update_world_renderer(float dt, float rdt, const TMatrix &itm, bool scene_loading)
{
  TIME_PROFILE(update_world_renderer);
  if (auto wr = !scene_loading ? static_cast<WorldRenderer *>(get_world_renderer()) : nullptr)
    wr->update(dt, rdt, itm);
  if (!dagor_work_cycle_is_need_to_draw()) // Note: we still have to update frp for it's side effects
    uirender::update_all_gui_scenes_mainthread(dt);
}

void WorldRenderer::cullFrustumLights(
  Occlusion *occlusion, vec3f viewPos, mat44f_cref globtm, mat44f_cref view, mat44f_cref proj, float zn)
{
  lights.cullFrustumLights(viewPos, globtm, view, proj, zn, occlusion, SpotLightMaskType::SPOT_LIGHT_MASK_NONE,
    OmniLightMaskType::OMNI_LIGHT_MASK_NONE); // least important Job, needed only at resolve pass
}

inline DPoint3 floor(const DPoint3 &a) { return DPoint3(floor(a.x), floor(a.y), floor(a.z)); }

CameraParams get_camera_params(TMatrix view_itm, const DPoint3 &view_pos, const Driver3dPerspective &persp, const TMatrix4D &projTm)
{
  TIME_PROFILE(get_camera_params);

  if (!(fabsf(dot(view_itm.getcol(0), view_itm.getcol(1))) < 1e-6f && fabsf(dot(view_itm.getcol(0), view_itm.getcol(2))) < 1e-6f &&
        fabsf(dot(view_itm.getcol(1), view_itm.getcol(2))) < 1e-6f))
  {
#if DAGOR_DBGLEVEL > 0
    logerr("view matrix should be orthonormalized %@ %@ %@", view_itm.getcol(0), view_itm.getcol(1), view_itm.getcol(2));
#endif
    view_itm.orthonormalize();
    if (check_nan(view_itm))
      view_itm = TMatrix::IDENT;
  }
  else if (!(fabsf(lengthSq(view_itm.getcol(0)) - 1) < 1e-5f && fabsf(lengthSq(view_itm.getcol(1)) - 1) < 1e-5f &&
             fabsf(lengthSq(view_itm.getcol(2)) - 1) < 1e-5f))
  {
#if DAGOR_DBGLEVEL > 0
    logerr("view matrix should be normalized %@ %@ %@", view_itm.getcol(0), view_itm.getcol(1), view_itm.getcol(2));
#endif
    view_itm.setcol(0, normalize(view_itm.getcol(0)));
    view_itm.setcol(1, normalize(view_itm.getcol(1)));
    view_itm.setcol(2, normalize(view_itm.getcol(2)));
  }

  TMatrix4D viewTm = calc_camera_view(view_itm, view_pos);
  TMatrix4D globtm = viewTm * projTm;
  // globtm in doubles

  const DPoint3 roundedPosD =
    /* better this to match calc_optimal_wofs, but not required */ 8.0 * floor(view_pos / 8.0 + DPoint3(0.5, 0.5, 0.5));
  const DPoint3 remainderPosD = view_pos - roundedPosD;
  const vec4f negRoundedPos = v_make_vec4f(-roundedPosD.x, -roundedPosD.y, -roundedPosD.z, 0);
  const vec4f negRemainderPos = v_make_vec4f(-remainderPosD.x, -remainderPosD.y, -remainderPosD.z, 0);

  TMatrix4D viewRotTm = viewTm;
  viewRotTm.setrow(3, 0.0f, 0.0f, 0.0f, 1.0f);

  CameraParams cameraParams;
  cameraParams.znear = persp.zn;
  cameraParams.zfar = persp.zf;
  cameraParams.cameraWorldPos = view_pos;
  cameraParams.negRoundedCamPos = negRoundedPos;
  cameraParams.negRemainderCamPos = negRemainderPos;
  cameraParams.noJitterPersp = persp;
  cameraParams.viewRotProjTm = TMatrix4(viewRotTm * projTm);
  cameraParams.viewRotJitterProjTm = cameraParams.viewRotProjTm;
  cameraParams.viewRotTm = TMatrix(viewRotTm);
  cameraParams.viewItm = view_itm;
  cameraParams.viewTm = TMatrix(viewTm);
  cameraParams.noJitterGlobtm = (const TMatrix4 &)TMatrix4(globtm); //(const TMatrix4&) is for LLVM on mac
  cameraParams.noJitterProjTm = (const TMatrix4 &)TMatrix4(projTm);
  cameraParams.noJitterFrustum = cameraParams.noJitterGlobtm;
  cameraParams.jitterPersp = cameraParams.noJitterPersp;
  cameraParams.jitterProjTm = cameraParams.noJitterProjTm;
  cameraParams.jitterGlobtm = cameraParams.noJitterGlobtm;
  cameraParams.jitterFrustum = cameraParams.noJitterFrustum;

  return cameraParams;
}

void WorldRenderer::setUpView(
  const TMatrix &view_itm, const DPoint3 &view_pos, const Driver3dPerspective &persp, const TMatrix4D &proj_tm)
{
  TIME_D3D_PROFILE(setUpView);

  prevFrameCamera = currentFrameCamera;

  if (camera_in_camera::is_lens_only_zoom_enabled())
  {
    int w, h;
    d3d::get_screen_size(w, h);
    const CameraSetup &camSetup = get_active_camera_setup();
    const Driver3dPerspective unzoomedPerspective =
      calc_camera_perspective(camSetup.fovSettings, camSetup.fovMode, persp.zn, persp.zf, w, h);

    const TMatrix4D unzoomedProjTm = dmatrix_perspective_reverse(unzoomedPerspective.wk, unzoomedPerspective.hk,
      unzoomedPerspective.zn, unzoomedPerspective.zf, unzoomedPerspective.ox, unzoomedPerspective.oy);
    const Driver3dPerspective &lensAreaPerspective = persp;

    prevCamcamParams = camcamParams;
    camcamParams = get_camera_params(view_itm, view_pos, camcam_force_world_fov ? unzoomedPerspective : lensAreaPerspective, proj_tm);
    currentFrameCamera = get_camera_params(view_itm, view_pos, unzoomedPerspective, unzoomedProjTm);
  }
  else
  {
    currentFrameCamera = get_camera_params(view_itm, view_pos, persp, proj_tm);
    camcamParams = prevCamcamParams = eastl::nullopt;
  }


  d3d::settm(TM_WORLD, TMatrix::IDENT);
  d3d::settm(TM_VIEW, currentFrameCamera.viewTm);
  d3d::settm(TM_PROJ, &currentFrameCamera.noJitterProjTm);

  ShaderGlobal::set_float4(world_view_posVarId, view_pos.x, view_pos.y, view_pos.z, 1);

  ShaderGlobal::set_float4(projection_centerVarId, persp.ox, persp.oy, 0, 0);

  ShaderGlobal::set_float4(camera_rightVarId, view_itm.getcol(0));
  ShaderGlobal::set_float4(camera_upVarId, view_itm.getcol(1));
}

void WorldRenderer::updateLodsScaling()
{
  const float defRiDistMul = rendinst::getDefaultDistAddMul();
  const float defImpostorDistMul = rendinst::getDefaultImpostorsDistAddMul();

  float riDistMul = defRiDistMul;
  float impostorsDistMul = defImpostorDistMul;

  g_entity_mgr->broadcastEventImmediate(QueryShooterCamDistanceMultipliers{&riDistMul, &impostorsDistMul});
  riDistMul = max(riDistMul, defRiDistMul);
  impostorsDistMul = max(impostorsDistMul, defImpostorDistMul);

  if (camera_in_camera::is_lens_only_zoom_enabled())
  {
    rendinst::setImpostorsDistAddMul(defImpostorDistMul);
    rendinst::setDistMul(defRiDistMul, 0.0f);
  }
  else
  {
    rendinst::setImpostorsDistAddMul(impostorsDistMul);
    rendinst::setDistMul(riDistMul, 0.0f);
  }

  if (camera_in_camera::is_lens_render_active())
    camcamVisibilityMgr.updateLodsScaling(riDistMul, impostorsDistMul);

  bool impostorRangeIncreased = impostorsDistMul > defImpostorDistMul;
  bool rendinstRangeIncreased = riDistMul > defRiDistMul;
  g_entity_mgr->broadcastEventImmediate(RendinstLodRangeIncreasedEvent(impostorRangeIncreased, rendinstRangeIncreased));
}

namespace
{
struct DriverFallbackCallback : public FrameEvents
{
  static constexpr int TEST_FRAMES_TO_RENDER = 10;
  int callbackCount = 0;
  void beginFrameEvent() override {}
  void endFrameEvent() override
  {
    if (++callbackCount >= TEST_FRAMES_TO_RENDER)
      crash_fallback_helper->successfullyLoaded();
  }
  void registerSelf()
  {
    if (callbackCount < TEST_FRAMES_TO_RENDER)
      d3d::driver_command(Drv3dCommand::REGISTER_ONE_TIME_FRAME_EXECUTION_EVENT_CALLBACKS, this);
  }
};
DriverFallbackCallback driverFallbackCallback{};
} // namespace

void WorldRenderer::beforeRender(float scaled_dt,
  float act_dt,
  float real_dt,
  float game_time,
  const TMatrix &view_itm_,
  const DPoint3 &view_pos,
  const Driver3dPerspective &persp,
  const TMatrix4D &proj_tm)
{
  if (sceneload::is_scene_switch_in_progress())
    return;

  ShaderGlobal::set_float(var::sky_polarization_filter_term, get_sky_polarization_filter_term());

  realDeltaTime = real_dt;
  gameTime = game_time;
  set_shader_global_time(gameTime);

  ShaderGlobal::set_float(foam_timeVarId, gameTime);
  ShaderGlobal::set_float(scroll_timeVarId, gameTime);

  if (crash_fallback_helper)
    driverFallbackCallback.registerSelf();

  if (immediate_flush.pullValueChange())
  {
    if (immediate_flush.get())
    {
      if (d3d::driver_command(Drv3dCommand::ENABLE_IMMEDIATE_FLUSH))
        debug("immediate_flush mode is enabled.");
    }
    else
    {
      d3d::driver_command(Drv3dCommand::DISABLE_IMMEDIATE_FLUSH);
      debug("immediate_flush mode is disabled.");
    }
  }

  rendinst::render::useRiCellsDepthPrepass(prepass.get());
  rendinst::render::useRiDepthPrepass(prepass.get());

  TIME_D3D_PROFILE(world_render__beforeRender);

  if (dynamicQuality)
    dynamicQuality->onFrameStart(*g_entity_mgr);
  if (dynamicResolution)
  {
    dynamicResolution->setMinimumMsPerFrame(safediv(1000.f, get_fps_limit()));
    dynamicResolution->beginFrame();
    int w, h;
    dynamicResolution->getCurrentResolution(w, h);
    IPoint2 currentResolution = IPoint2(w, h);
    IPoint2 prevResolution = antiAliasing->getInputResolution();

    TIME_D3D_PROFILE(changeRenderingResolution);
    if (currentResolution != prevResolution)
    {
      antiAliasing->setInputResolution(currentResolution);
      changeSingleTexturesResolution(w, h);
      dafg::set_dynamic_resolution("main_view", currentResolution);
      darg::bhv_fps_bar.setRenderingResolution(currentResolution);

      // VRS texture doesn't need to change resolution dynamically (?)
    }
  }

  setUpView(view_itm_, view_pos, persp, proj_tm);

  processHeroTeleportation();
  processGIInvalidationRequests();

  updateBackBufferTex();

  DynamicRenderableSceneInstance::lodDistanceScale =
    safediv(lodDistanceScaleBase, persp.wk * d3d::get_screen_aspect_ratio() / (16.0f / 9.0f));

  if (deformHmap)
    deformHmap->beforeRenderWorld(view_pos);

  const TMatrix &itm = currentFrameCamera.viewItm;
  const Point3_vec4 viewPos = itm.getcol(3);
  const Point3 prevViewPos = prevFrameCamera.viewItm.getcol(3);


  cameraHeight = 4.0f; // fixme: get real height
  if (lmeshMgr)
  {
    waterLevel = water ? fft_water::get_level(water) : HeightmapHeightCulling::NO_WATER_ON_LEVEL;
    float ht = waterLevel, htb = waterLevel;
    bool htSuccess = lmeshMgr->getHeight(Point2::xz(viewPos), ht, NULL);
    bool htBSuccess = false;
    if (lmeshMgr->getLandTracer())
      htBSuccess = lmeshMgr->getLandTracer()->getHeightBounding(Point2::xz(viewPos), htb);

    if (!htBSuccess)
      htb = ht;
    if (!htSuccess)
      ht = htb;

    if (!htBSuccess)
    {
      if (lmeshMgr->getHmapHandler() && lmeshMgr->getHmapHandler()->heightmapHeightCulling)
      {
        int lod = floorf(lmeshMgr->getHmapHandler()->heightmapHeightCulling->getLod(128.f));
        float hmin;
        lmeshMgr->getHmapHandler()->heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(viewPos), hmin, htb);
      }
    }
    if (htBSuccess || htSuccess)
    {
      ht = max(waterLevel, ht);
      htb = max(waterLevel, htb);
      cameraHeight = max(cameraHeight, viewPos.y - 0.5f * (ht + htb));
    }
    lmeshRenderer->setUseHmapTankSubDiv(displacementSubDiv);
  }
  cameraActualHeight = cameraHeight;

  canChangeAltitudeUnexpectedly = can_change_altitude_unexpectedly();
  if (canChangeAltitudeUnexpectedly)
    cameraHeight += fast_move_camera_height_offset.get();

  constexpr uint32_t cameraInitialPositionRelaxationFrames = 5;
  const float speed = length(prevViewPos - itm.getcol(3)) / max(0.001f, real_dt);
  const bool cameraMovementSimilarToTeleportation =
    speed > 330.f && fabsf(cameraSpeed - speed) / max(0.001f, real_dt) > 1000.f; // new speed exceeds speed of sound, and acceleration
                                                                                 // exceed 100g, so teleportation
  if (renderedFrames < cameraInitialPositionRelaxationFrames || cameraMovementSimilarToTeleportation)
    cameraSpeed = 0;
  else
    cameraSpeed = lerp(cameraSpeed, speed, 1.0f - expf(-real_dt)); // exponential average

  // 10.f here is g-force.
  bool highEnergy = (cameraHeight * 10.f + cameraSpeed * cameraSpeed * 0.5f) > switch_ri_gen_mode_threshold.get();
  rendinst::render::setRIGenRenderMode(!canChangeAltitudeUnexpectedly || !highEnergy ? -1 : 0);

  {
    Occlusion *occlusion = getMainCameraOcclusion();
    Frustum mainCullingFrustum = Frustum(currentFrameCamera.noJitterGlobtm);
    Frustum froxelFogFrustum = (volfog_enabled && volumeLight)
                                 ? volumeLight->calcFrustum(currentFrameCamera.viewTm, currentFrameCamera.noJitterPersp)
                                 : mainCullingFrustum;
    g_entity_mgr->broadcastEventImmediate(UpdateStageInfoBeforeRender(scaled_dt, act_dt, real_dt, mainCullingFrustum, froxelFogFrustum,
      {persp.wk, persp.hk}, persp.wk > 1e-3f ? 1.f / (persp.wk * persp.wk) : 1.0f, occlusion, currentFrameCamera.viewItm.getcol(3),
      currentFrameCamera.negRoundedCamPos, currentFrameCamera.negRemainderCamPos, -getDirToSun(DirToSun::FINISHED),
      currentFrameCamera.viewTm, currentFrameCamera.jitterProjTm, currentFrameCamera.viewItm, currentFrameCamera.jitterPersp));
  }

  // should be called in WorldRenderer::beforeRender to avoid data race with ParallelUpdateFrame
  if (get_cables_mgr())
  {
    bool cablesChanged = get_cables_mgr()->beforeRender();
    if (cablesChanged)
      bvh_cables_changed();
  }

  // dispatch performed in this function, so it should be done in beforeRender
  {
    TIME_D3D_PROFILE(heightmap_query_update)
    heightmap_query::update();
  }

  if (ecs::EntityId zoneEid = g_entity_mgr->getSingletonEntity(ECS_HASH("transparent_partition")))
  {
    transparentPartitionSphere = g_entity_mgr->getOr<PartitionSphere>(zoneEid, ECS_HASH("partition_sphere"), PartitionSphere());
  }
  else
  {
    transparentPartitionSphere = PartitionSphere();
  }

  updateHeroData();

  if (fomShadowManager)
  {
    float minHt, maxHt;
    getMinMaxZ(minHt, maxHt);
    fomShadowManager->prepareFOMShadows(::grs_cur_view.itm, waterLevel, minHt, getDirToSun(DirToSun::FINISHED));
    d3d::settm(TM_PROJ, &currentFrameCamera.jitterProjTm);
    d3d::settm(TM_VIEW, currentFrameCamera.viewTm);
  }
  acesfx::prepare_main_culling(currentFrameCamera.jitterGlobtm);
  prepareFXForBVH(Point3::xyz(currentFrameCamera.cameraWorldPos));

  update_fluid_wind(scaled_dt, view_pos);

  if (water_quality.pullValueChange())
    setupWaterQuality();

  if (ssr_quality.pullValueChange() || ssr_fullres.pullValueChange() || ssr_enable.pullValueChange() || ssr_denoiser.pullValueChange())
    updateSettingsSSR(-1, -1);

  rendinst::before_culling();
  rendinst::before_render();

  if (prepass.pullValueChange())
    requestFgRecreation("prepass shvar changed");
}

enum
{
  AGT_NONE = 0,
  AGT_ADDITIONAL = 1,
  AGT_UI = 2,
  AGT_DAFX = 4,
  AGT_ALL = 7
};
extern void start_async_game_tasks(uint32_t frame_id, int agt = AGT_ALL, bool wake = true);

template <typename F>
#if defined(__ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__) && __ENVIRONMENT_MAC_OS_X_VERSION_MIN_REQUIRED__ < 101400
// workaround for 'error: aligned deallocation function of type 'void (void *, std::align_val_t) noexcept'
// is only available on macOS 10.14 or newer'
struct AnimcharRenderShadowsJob final : public cpujobs::IJob
#else
// align on 2 cache lines to avoid false sharing on setting 'done' within threadpool
struct alignas(EA_CACHE_LINE_SIZE * 2) AnimcharRenderShadowsJob final : public cpujobs::IJob
#endif
{
  const F &cb;
  int i;
  AnimcharRenderAsyncFilter eidMask;
  AnimcharRenderShadowsJob(const F &cb_, int i_, da_profiler::desc_id_t d = 0) : cb(cb_), i(i_) { IJob::jobNameProfDesc = d; }
  ~AnimcharRenderShadowsJob() { G_FAST_ASSERT(0); } // isn't called
  const char *getJobName(bool &) const override
  {
    G_FAST_ASSERT(0); // Note: should not be called since `jobNameProfDesc` is set
    return nullptr;
  }
  void doJob() override { cb(i); }
};

const char *fmt_csm_render_pass_name(int cascade, char tmps[], int csm_task_id = 0)
{
  // Note: formatting this way is much faster then snprintf or string's ctor
  tmps[sizeof("csm#") - 1] = '0' + cascade;
  tmps[sizeof("csm#0") - 1] = csm_task_id > 0 ? '#' : '\0';
  if (csm_task_id > 0)
    tmps[sizeof("csm#0#") - 1] = '0' + csm_task_id;
  return tmps;
}

void WorldRenderer::updateTransformations(const DPoint3 &move,
  const TMatrix4_vec4 &jittered_cam_pos_to_unjittered_history_clip,
  const TMatrix4_vec4 &prev_origo_relative_view_proj_tm) // move currentWorldPos - prevWorldPos
{
  TMatrix4_vec4 globtm = (const TMatrix4_vec4 &)currentFrameCamera.jitterGlobtm;
  globtm = globtm.transpose();
  ShaderGlobal::set_float4(globtm_psf_0VarId, Color4(globtm[0]));
  ShaderGlobal::set_float4(globtm_psf_1VarId, Color4(globtm[1]));
  ShaderGlobal::set_float4(globtm_psf_2VarId, Color4(globtm[2]));
  ShaderGlobal::set_float4(globtm_psf_3VarId, Color4(globtm[3]));

  TMatrix4_vec4 prevGlobTm = (const TMatrix4_vec4 &)prevFrameCamera.jitterGlobtm;
  prevGlobTm = prevGlobTm.transpose();
  ShaderGlobal::set_float4(prev_globtm_psf_0VarId, Color4(prevGlobTm[0]));
  ShaderGlobal::set_float4(prev_globtm_psf_1VarId, Color4(prevGlobTm[1]));
  ShaderGlobal::set_float4(prev_globtm_psf_2VarId, Color4(prevGlobTm[2]));
  ShaderGlobal::set_float4(prev_globtm_psf_3VarId, Color4(prevGlobTm[3]));

  ShaderGlobal::set_float4x4(jitteredCamPosToUnjitteredHistoryClipVarId, jittered_cam_pos_to_unjittered_history_clip);
  ShaderGlobal::set_float4(prev_to_cur_origin_moveVarId, move.x, move.y, move.z, length(move));

  ShaderGlobal::set_float4(prevOrigoRelativeViewProjTm0VarId, Color4(prev_origo_relative_view_proj_tm[0]));
  ShaderGlobal::set_float4(prevOrigoRelativeViewProjTm1VarId, Color4(prev_origo_relative_view_proj_tm[1]));
  ShaderGlobal::set_float4(prevOrigoRelativeViewProjTm2VarId, Color4(prev_origo_relative_view_proj_tm[2]));
  ShaderGlobal::set_float4(prevOrigoRelativeViewProjTm3VarId, Color4(prev_origo_relative_view_proj_tm[3]));
}

static void set_mip_bias(float bias_value)
{
  TIME_PROFILE(set_mip_bias);
  acesfx::set_default_mip_bias(bias_value);
  const LODBiasRule lodBiasRules[] = {
    {"*", bias_value}, {"impostor", bias_value}, {"character_micro_details*", 0}, {"gen_cloud_detail", 0}, {"gen_cloud_shape", 0}};
  set_add_lod_bias_batch(make_span(lodBiasRules));
  ShaderGlobal::set_float(mip_biasVarId, bias_value);
}

void WorldRenderer::draw(uint32_t frame_id, float realDt)
{
  int renderResW, renderResH;
  getRenderingResolution(renderResW, renderResH);
  int displayResW, displayResH;
  getDisplayResolution(displayResW, displayResH);
  DA_PROFILE_TAG(resolution, "render:%d x %d, display:%d x %d", renderResW, renderResH, displayResW, displayResH);

  debugRecreateNodes();
  processPendingFgRecreation();

  ShaderGlobal::set_int(var::packed_gbuf_normals, hasFeature(GBUFFER_PACKED_NORMALS));
  auto callBeforePUFD = []() { // `PUFD` is abbr ParallelUpdateFrameDelayed
    rendinst::applyTiledScenesUpdateForRIGenExtra(2000, 1000);
  };
  callBeforePUFD();            // Must be called before additional job (aka PUFD) to avoid data races & lock order inversions
  if (!delay_PUFD_after_bvh()) // It'll be started after bvh_update_instances instead, runs parallel with bvh_update node
    start_async_game_tasks(frame_id, AGT_ADDITIONAL, /*wake*/ true); // Note: long job - start as early as possible

  const TMatrix &itm = currentFrameCamera.viewItm;
  const Point3_vec4 viewPos = itm.getcol(3);
  const Point3 prevViewPos = prevFrameCamera.viewItm.getcol(3);
  const mat44f &globtm = (const mat44f &)currentFrameCamera.noJitterGlobtm;
  const mat44f &proj = (const mat44f &)currentFrameCamera.noJitterProjTm;
  const TMatrix &viewTm = currentFrameCamera.viewTm;
  const TMatrix4 &projJitter = currentFrameCamera.jitterProjTm;
  const Driver3dPerspective &persp = currentFrameCamera.noJitterPersp;
  Frustum frustum(globtm);
  RiGenVisibility *mainCameraRiMainVisibility = mainCameraVisibilityMgr.getRiMainVisibility();
  Occlusion *mainCameraOcclusion = getMainCameraOcclusion();

  updateSky(realDt); // Needs to be done before startVisibility to avoid a data race in csm

  calcWaterPlanarReflectionMatrix();

  // initialize visibility for legacy gpu objects on demand
  if (DAGOR_UNLIKELY(rendinst::gpuobjects::has_manager() && !legacyGPUObjectsVisibilityInitialized))
  {
    rendinst::gpuobjects::enable_for_visibility(mainCameraRiMainVisibility);
    if (hasFeature(CAMERA_IN_CAMERA))
      rendinst::gpuobjects::enable_for_visibility(camcamVisibilityMgr.getRiMainVisibility());
    shadowsManager.conditionalEnableGPUObjsCSM();
    rendinst::gpuobjects::enable_for_visibility(rendinst_dynamic_shadow_visibility);
    legacyGPUObjectsVisibilityInitialized = true;
  }
  debug_mesh::Type currentType = debug_mesh::debug_gbuffer_mode_to_type(show_gbuffer);
#if DAGOR_DBGLEVEL == 0
  if (!app_profile::get().devMode)
    currentType = debug_mesh::Type::NONE;
#endif
  bool needDepthReinit = debug_mesh::enable_debug(currentType);

#if DAGOR_DBGLEVEL > 0
  needDepthReinit |= use_24bit_depth.pullValueChange();
  updateDistantHeightmap(); // for convars
#endif

  if (needDepthReinit)
  {
    requestFgRecreation("depth format change or debug mesh mode change during the draw");
    initTarget();
  }

  if (attemptsToUpdateSkySph == 0 && !!enviProbe && light_probe::hasNans(enviProbe))
  {
    logerr("EnviProbe has NaNs, attempting to update it.");
    attemptsToUpdateSkySph = 1;
  }

  if (attemptsToUpdateSkySph != 0) // It means that we have failed attempts to compute sky spherical harmonic
    updateSkyProbeDiffuse();

  if (lmeshMgr)
  {
    bool renderHmap = true;
    if (hide_heightmap_on_planes.get() && canChangeAltitudeUnexpectedly)
      renderHmap = false; // low poly landmesh will be rendered instead
    if (lmeshMgr->mayRenderHmap != renderHmap)
    {
      lmeshMgr->mayRenderHmap = renderHmap;
      shadowsManager.shadowsInvalidate(false);
    }
  }

  float firstPersonShadowDist = get_fpv_shadow_dist();
  shadowsManager.prepareShadowsMatrices(itm, persp, reinterpret_cast<const TMatrix4 &>(proj), frustum, firstPersonShadowDist);

  startVisibility();
  // debug("parallel for %d", get_time_usec(reft));
  // parallel jobs done


  // rendinst::applyTiledScenesUpdateForRIGenExtra(2000, 1000); // if we will call it here, program halts!
  if (lmeshMgr && lmeshMgr->getHmapHandler())
  {
    const float hmapCellSize = lmeshMgr->getHmapHandler()->getHeightmapCellSize();
    if (subdivCellSize.get() < hmapCellSize)
      displacementSubDiv = floorf(log2(hmapCellSize / subdivCellSize.get()) + 0.5f);
    else
      displacementSubDiv = 0;
    displacementSubDiv = clamp((int)displacementSubDiv + min(subdivSettings, (int)MAX_SETTINGS_SUBDIV), (int)0, (int)MAX_SUBDIV);
  }

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  if (preIntegratedGF)
    preIntegratedGF->update();

  // Update heightmap.
  if (lmeshMgr)
  {
    HeightmapHandler *hmapHandler = lmeshMgr->getHmapHandler();
    if (hmapHandler)
    {
      hmapHandler->prepareHmapModificaton();
    }
  }

  prepareDeformHmap();

  shoreRenderer.updateShore(water, water_quality.get(), currentFrameCamera.cameraWorldPos);
  if (water)
    waterLevel = fft_water::get_level(water); // TODO: can we remove this?
  renderWaterHeightmapLowres();
  if (water)
  {
    TIME_D3D_PROFILE(water_render_simulate);
    fft_water::setAnisotropy(water, waterAnisotropy.get());
    fft_water::before_render(water);
  }

  beforeDrawPostFx();
  {
    TIME_D3D_PROFILE(postfx_before_draw);
    g_entity_mgr->broadcastEventImmediate(BeforeDrawPostFx());
  }

  {
    TIME_D3D_PROFILE(impostors);
    rendinst::updateRIGenImpostors(1000, -getDirToSun(DirToSun::IMPOSTOR_PRESHADOWS), itm, proj);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  }

  g_entity_mgr->broadcastEventImmediate(BeforeDraw(persp, frustum, itm.getcol(3), realDt));

  // debug("regen = %d force_update = %d", sky_panel.regen, force_update);
  // sky_panel.regen  = false;

  if (indoorProbeMgr)
    indoorProbeMgr->update(itm.getcol(3));

  ShaderGlobal::set_float4(world_view_posVarId, Color4(viewPos.x, viewPos.y, viewPos.z, 1));
  ShaderGlobal::set_float4(prev_world_view_posVarId, Color4(prevViewPos.x, prevViewPos.y, prevViewPos.z, 1));


  int renderingWidth, renderingHeight;
  getRenderingResolution(renderingWidth, renderingHeight);
  ShaderGlobal::set_float(pixel_scaleVarId, 1.0 / (persp.hk * renderingHeight));

  // TODO: figure out why this causes gray flickering on screen when put before probes update
  ShaderGlobal::set_float4(zn_zfarVarId, persp.zn, persp.zf, 0, 0);

  {
    // walkers should be beforeRender'ed here
  }

  d3d::set_render_target();

  createDeforms();

  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);

  // The faster we move, the less important clipmap texel size is.
  // In the same time, the faster we move, the more likely we can gain altitude
  float additionalHeight = camera_speed_to_additional_height.get() < 0 ? -camera_speed_to_additional_height.get()
                                                                       : camera_speed_to_additional_height.get() * cameraSpeed;
  const float cameraSpeedDependHeight = cameraHeight + additionalHeight;
  const float cameraHeightLimit = giMaxUpdateHeightAboveFloor; // default: 32 meters above ground.
  const bool requiresGroundDetails = (cameraSpeedDependHeight < cameraHeightLimit);

  // must be called before prepareClipmap
  renderDisplacementRiLandclasses(viewPos);

  if (requiresGroundDetails)
  {
    Point3 displacementCenter = itm.getcol(3);
    Point3 viewDirection = itm.getcol(2);
    if (abs(viewDirection.y) < 0.99f)
    {
      // -1 is a safe constant here. If you look down, you can see a bit behind the
      // character, so we need to cover that area too. It could be more complicated
      // by using the fov to calculate exact values, but it is unnecessary and costy
      // as we would need to sample the terrain to get the camera height from it.
      float tessellationOffset = DISPLACEMENT_DIST - 1;

      viewDirection.y = 0;
      viewDirection.normalize();
      displacementCenter += viewDirection * tessellationOffset;
    }

    renderDisplacementAndHmapPatches(displacementCenter, DISPLACEMENT_DIST);
  }

  // based on old virtualtexture zoom calculations, assuming it was tuned for 4/512 texel size.
  const int cameraSpeedDependMinimumClipmapZoom =
    get_closest_pow2(clipmap ? (int)(((4.f / 512.f) / clipmap->getStartTexelSize()) * additionalHeight / 10.f) : 1);
  const int minimumClipmapZoom = max(cameraSpeedDependMinimumClipmapZoom, can_change_altitude_unexpectedly() ? 2 : 1);
  prepareClipmap(itm.getcol(3), itm, currentFrameCamera.noJitterGlobtm, currentFrameCamera.noJitterPersp.wk, minimumClipmapZoom);

  {
    TIME_D3D_PROFILE(biome_query_update)
    biome_query::update(); // depends on prepareClipmap because sample it.
  }

  Point3 giPos = itm.getcol(3);
  float hmin = -100000., hmax = 100000.;
  if (lmeshMgr && lmeshMgr->getHmapHandler() && lmeshMgr->getHmapHandler()->heightmapHeightCulling)
  {
    // 16 meters is hardcoded. it actually should be 32 meters, but we care about quality around us more, so we accept a bit of
    // undetailed data there.
    const float giXZHalfExtents = 32.f;
    int lod = floorf(lmeshMgr->getHmapHandler()->heightmapHeightCulling->getLod(giXZHalfExtents));
    float of = 4.0f;
    float hmin1, hmin2, hmin3, hmin4;
    // interpolated min is not min. but it is smooth. So is min of 4 interpolated mins, but it is actually more correct
    lmeshMgr->getHmapHandler()->heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(giPos) + Point2(-of, -of), hmin1, hmax);
    lmeshMgr->getHmapHandler()->heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(giPos) + Point2(+of, -of), hmin2, hmax);
    lmeshMgr->getHmapHandler()->heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(giPos) + Point2(-of, +of), hmin3, hmax);
    lmeshMgr->getHmapHandler()->heightmapHeightCulling->getMinMaxInterpolated(lod, Point2::xz(giPos) + Point2(+of, +of), hmin4, hmax);
    hmin = min(min(hmin1, hmin2), min(hmin3, hmin4));

    BBox3 giBbox(giPos - giXZHalfExtents, giPos + giXZHalfExtents);
    Tab<Point3_vec4> groundHolesBBoxes(framemem_ptr());
    get_underground_zones_data(groundHolesBBoxes);
    for (int i = 0; i < groundHolesBBoxes.size() / 2; i++)
    {
      BBox3 groundHolesZone = BBox3(groundHolesBBoxes[2 * i], groundHolesBBoxes[2 * i + 1]);
      if (groundHolesZone & giBbox)
        hmin = min(hmin, groundHolesZone.boxMin().y);
    }
  }

  giBeforeRender();
  if (requiresGroundDetails || isTimeDynamic())
    updateGIPos(giPos, itm, hmin, hmax);

  BBox3 riBox;
  if (rendinst::render::notRenderedStaticShadowsBBox(riBox))
    shadowsInvalidate(riBox);

  shadowsManager.shadowsInvalidateGatheredBBoxes();

  satelliteRenderer.renderScripted(currentFrameCamera);

  if (clipmap)
    clipmap->resetSecondaryFeedbackCenter();
  if (auto portalRenderer = portal_renderer_mgr::query_portal_renderer()) // TODO: maybe not the best place
    portalRenderer->render(currentFrameCamera);

  if (get_daskies())
  {
    TIME_D3D_PROFILE(skiesPrepare)
    int oldBlock = ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    {
      TIME_D3D_PROFILE(changeData)
      if (main_pov_data)
      {
        get_daskies()->changeSkiesData(1, 1, 1, renderingWidth, renderingHeight, main_pov_data,
          useFullresClouds ? CloudsResolution::ForceFullresClouds : CloudsResolution::Default);
      }
    }

    if (get_daskies()->panoramaEnabled())
    {
      cloudsVolume.close();
    }
    else if (!cloudsVolume)
    {
      recreateCloudsVolume();
    }
    else
    {
      TIME_D3D_PROFILE(buildCloudVolume);
      static constexpr float CLOUD_VOLUME_RANGE = 10000;
      get_daskies()->renderCloudVolume(cloudsVolume.getVolTex(), BAD_TEXTUREID, CLOUD_VOLUME_RANGE, viewTm, projJitter,
        TMatrix::IDENT);
      cloudsVolume.setVar();
      d3d::resource_barrier({cloudsVolume.getVolTex(), RB_RO_SRV | RB_STAGE_PIXEL | RB_STAGE_COMPUTE, 0, 0});
    }

    {
      TIME_D3D_PROFILE(useFog)
      get_daskies()->useFog(itm.getcol(3), main_pov_data, viewTm, projJitter);
    }
    if (ShaderGlobal::getBlock(ShaderGlobal::LAYER_FRAME) != oldBlock)
      ShaderGlobal::setBlock(oldBlock, ShaderGlobal::LAYER_FRAME);
  }

  auto waitMainVisibility = [this]() {
    mainCameraVisibilityMgr.waitMainVisibility();
    if (hasFeature(CAMERA_IN_CAMERA))
      camcamVisibilityMgr.waitMainVisibility();
  };

  // TODO: move this code block into ShadowsManager
  if (CascadeShadows *csm = shadowsManager.getCascadeShadows())
  {
    if (use_occlusion_for_shadows.get())
      waitMainVisibility();

    if (mainCameraOcclusion && use_occlusion_for_shadows.get())
    {
      update_csm_length(csm->getWholeCoveredFrustum(), -getDirToSun(DirToSun::FINISHED), shadowsManager.getCsmShadowsMaxDist());
      compute_csm_visibility(*mainCameraOcclusion, -getDirToSun(DirToSun::FINISHED));
    }
    if (int numCascades = async_animchars_shadows.get() ? min(csm->getNumCascadesToRender(), (int)CascadeShadows::MAX_CASCADES) : 0)
    {
      TIME_PROFILE(animchar_shadows_parallel);
      G_ASSERT(g_entity_mgr->isConstrainedMTMode());
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Depth));
      dynmodel_renderer::DynModelRenderingState *dStates[AnimcharRenderAsyncFilter::ARF_IDX_COUNT][CascadeShadows::MAX_CASCADES];
      Frustum frustums[CascadeShadows::MAX_CASCADES];
      // Pad to 2 cache lines to avoid false sharing
      volatile int workLeft[CascadeShadows::MAX_CASCADES][(EA_CACHE_LINE_SIZE * 2) / sizeof(int)];
#define CSM_CASCADE_JOBS_STARTED(i_) (*(eastl::end(workLeft[i_]) - 1)) // Via macros to avoid blow up sizeof capture context of labmda
      for (int i = 0; i < numCascades; ++i)
      {
        for (int j = 0; j < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; j++)
        {
          char tmps[] = "csm#000";
          dStates[j][i] = dynmodel_renderer::create_state(fmt_csm_render_pass_name(RENDER_SHADOWS_CSM + i, tmps, j));
        }
        frustums[i] = csm->getFrustum(i);
        interlocked_relaxed_store(workLeft[i][0], AnimcharRenderAsyncFilter::ARF_IDX_COUNT);
        interlocked_relaxed_store(CSM_CASCADE_JOBS_STARTED(i), AnimcharRenderAsyncFilter::ARF_IDX_COUNT);
      }
      GlobalVariableStates globalVarsState(framemem_ptr());
      copy_current_global_variables_states(globalVarsState);
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      auto render_shadow_cb = [&](int i) {
        int cascade = i / AnimcharRenderAsyncFilter::ARF_IDX_COUNT;
        interlocked_decrement(CSM_CASCADE_JOBS_STARTED(cascade));
        auto eidFilter = AnimcharRenderAsyncFilter(i & AnimcharRenderAsyncFilter::ARF_IDX_MASK);
        dynmodel_renderer::DynModelRenderingState *pState = dStates[eidFilter][cascade];
        // todo: interlocked_or uint8_t
        const animchar_visbits_t add_vis_bits = VISFLG_CSM_SHADOW_RENDERED;   //(VISFLG_MAIN_VISIBLE | VISFLG_MAIN_CAMERA_RENDERED)
        const animchar_visbits_t check_bits = VISFLG_MAIN_AND_SHADOW_VISIBLE; //(VISFLG_MAIN_AND_SHADOW_VISIBLE|VISFLG_MAIN_VISIBLE)
        const uint8_t filterMask = UpdateStageInfoRender::RENDER_SHADOW;
        g_entity_mgr->broadcastEventImmediate(AnimcharRenderAsyncEvent(*pState, &globalVarsState,
          NULL,              //&occlusion
          frustums[cascade], // : Frustum(currentFrameCamera.noJitterGlobtm),
          add_vis_bits, check_bits, filterMask, false /*hasMotionVectors*/, eidFilter));
        if (interlocked_decrement(workLeft[cascade][0]) == 0) // Last one finalizes and prepares for render
        {
          TIME_PROFILE_DEV(finalize_render_csm);
          dynmodel_renderer::DynModelRenderingState *dstState = dStates[0][cascade];
          for (int j = 1; j < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; j++)
            dstState->addStateFrom(eastl::move(*dStates[j][cascade]));
          dstState->prepareForRender();
        }
      };
      using JobType = AnimcharRenderShadowsJob<decltype(render_shadow_cb)>;
      alignas(JobType) char jobs_storage[sizeof(JobType) * CascadeShadows::MAX_CASCADES * AnimcharRenderAsyncFilter::ARF_IDX_COUNT];
      auto addCascadeJobs = [&](int ci) {
        for (int i = 0; i < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; ++i)
        {
          int ji = ci * AnimcharRenderAsyncFilter::ARF_IDX_COUNT + i;
          auto jptr = jobs_storage + sizeof(JobType) * ji;
          auto job = new (jptr, _NEW_INPLACE) JobType(render_shadow_cb, ji
#if DA_PROFILER_ENABLED
            ,
            animchar_csm_desc[ci]
#endif
          );
          threadpool::add(job, threadpool::PRIO_HIGH, /*wake*/ false);
        }
      };
      if (DAGOR_LIKELY(numCascades > 1))
        addCascadeJobs(numCascades - 1); // Note: last cascade seems to be the longest, so add it first
      for (int ci = 1; ci < numCascades - 1; ++ci)
        addCascadeJobs(ci);
      threadpool::wake_up_all();

      // Usually 0th cascade is quickest to render, so do it in main thread
      for (int i = 0; i < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; ++i)
      {
        auto j = new (jobs_storage + sizeof(JobType) * i, _NEW_INPLACE) JobType(render_shadow_cb, i);
        DA_PROFILE_EVENT_DESC(animchar_csm_desc[0]);
        j->doJob();
        j->done = 1;
      }

      // wait and render done jobs (in unspecified order)
      csm->renderShadowsCascadesCb([&](int num_cascades_to_render, bool clear_per_view) {
        G_ASSERT(numCascades <= num_cascades_to_render);
        G_UNUSED(num_cascades_to_render);
        unsigned cascadesNotRenderedMask = (1 << numCascades) - 1;
        intptr_t spinsLeftBeforeWait = SPINS_BEFORE_SLEEP / 2; // ~0.25ms on prevgen
        do
        {
          for (unsigned i = 0, bit = 1, joffs = 0; i < numCascades;
               ++i, bit <<= 1, joffs += sizeof(JobType) * AnimcharRenderAsyncFilter::ARF_IDX_COUNT)
            if (bit & cascadesNotRenderedMask)
            {
              bool haveUndoneJobs = interlocked_acquire_load(workLeft[i][0]) != 0;
              if (!haveUndoneJobs)
                for (unsigned j = 0; j < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; j++)
                {
                  auto job = reinterpret_cast<JobType *>(jobs_storage + joffs + j * sizeof(JobType));
                  if (!interlocked_acquire_load(job->done))
                  {
                    haveUndoneJobs = true;
                    break;
                  }
                }
              if (haveUndoneJobs)
              {
                // Note: Last cascade is added first and usually quite big so try to avoid active wait until
                // all of it's jobs are started
                if (spinsLeftBeforeWait > 0 && interlocked_acquire_load(CSM_CASCADE_JOBS_STARTED(num_cascades_to_render - 1)))
                  continue;
                TIME_PROFILE_DEV(wait_cascade);
                for (unsigned j = 0; j < AnimcharRenderAsyncFilter::ARF_IDX_COUNT; j++)
                {
                  auto job = reinterpret_cast<JobType *>(jobs_storage + joffs + j * sizeof(JobType));
                  threadpool::wait(job, 0, threadpool::PRIO_HIGH);
                }
              }
              if (__popcount(cascadesNotRenderedMask) == 3) // Before render of 2nd (#1) cascade
                start_async_game_tasks(frame_id, AGT_DAFX);
              cascadesNotRenderedMask &= ~bit;
              csm->renderShadowCascadeDepth((int)i, clear_per_view);
            }
          if (!cascadesNotRenderedMask)
            break;
          else
          {
            cpu_yield();
            --spinsLeftBeforeWait;
          }
        } while (1);
      });
#undef CSM_CASCADE_JOBS_STARTED
    }
    else
    {
      start_async_game_tasks(frame_id);
      csm->renderShadowsCascades();
    }
  }
  else
  {
    // start async tasks here when CSM is disabled
    start_async_game_tasks(frame_id);
    waitMainVisibility();
  }

  if (lmeshMgr)
  {
    const bool isUpsampling = antiAliasing && antiAliasing->isUpsampling();
    const int sub_pixels = (!isUpsampling && VariableMap::isGlobVariablePresent(sub_pixelsVarId)) ? screencap::subsamples() : 1;
    int lod = clamp((int)floor(currentFrameCamera.noJitterPersp.wk + 0.5f) + sub_pixels - 1, 2, 3);
    lmeshMgr->setHmapLodDistance(lod);
  }
  bool hasGpuObjs = rendinst::gpuobjects::has_manager();
  startGroundVisibility();           // we have to call it after all other ground renders are complete
  startGroundReflectionVisibility(); // we have to call it after all other ground renders are complete
  startLightsCullingJob();
  bvh_update_instances(viewPos, get_daskies() ? -get_daskies()->getPrimarySunDir() : Point3(0, -1, 0),
    currentFrameCamera.noJitterFrustum);
  start_async_game_tasks(frame_id, AGT_ALL, /*wake*/ false); // Start the rest of tasks in case it wasnt started earlier
  bool tpEarlyWakeUp = hasGpuObjs || rendinst::render::pendingRebuild() || enviProbeInvalid || enviProbeNeedsReload;
  if (tpEarlyWakeUp)
    threadpool::wake_up_all_delayed(threadpool::PRIO_HIGH); // Wake threadpool for all previously added jobs

  if (hasGpuObjs)
  {
    if (mainCameraOcclusion && use_occlusion_for_gpu_objs)
      waitMainVisibility();
    rendinst::gpuobjects::update(itm.getcol(3));
  }

  {
    const bool useOcclusion = (hasGpuObjs && use_occlusion_for_gpu_objs);
    eastl::fixed_vector<rendinst::render::RiBeforeDrawPerViewParams, 2, false> perViewParams;

    perViewParams.push_back({mainCameraRiMainVisibility, &frustum, useOcclusion ? mainCameraOcclusion : nullptr});
    if (hasFeature(CAMERA_IN_CAMERA) && camcamParams.has_value())
    {
      perViewParams.push_back({camcamVisibilityMgr.getRiMainVisibility(), &camcamParams->jitterFrustum,
        useOcclusion ? camcamVisibilityMgr.getOcclusion() : nullptr});
    }

    rendinst::render::before_draw(rendinst::RenderPass::Normal, perViewParams);
  }

  reinitCubeIfInvalid();

  if (enviProbeNeedsReload)
  {
    reloadCube(true);
    enviProbeNeedsReload = false;
  }

  bool tpLateWakeUp = !tpEarlyWakeUp;
  if (hasFeature(FeatureRenderFlags::STATIC_SHADOWS))
  {
    FRAME_LAYER_GUARD(globalFrameBlockId);
    bool updateOnlyLastCascade = !requiresGroundDetails && !is_free_camera_enabled();
    tpLateWakeUp |= shadowsManager.updateStaticShadowAround(itm.getcol(3), updateOnlyLastCascade);
  }
  shadowsManager.prepareVSM(viewPos);
  shadowsManager.prepareVoxelShadows(viewPos);
  tpLateWakeUp |= prepareDelayedRender(itm);
  if (tpLateWakeUp)
    threadpool::wake_up_all_delayed(threadpool::PRIO_HIGH); // Wake threadpool for all previously added jobs

  if (currentAntiAliasingMode == AntiAliasingMode::FSR && antiAliasing)
    ((FSR *)antiAliasing.get())->setDt(realDt);

  // NOTE: Adding new code is HIGHLY UNDESIRABLE!!!
  // Use framegraph nodes instead!
  int w, h;
  getDisplayResolution(w, h);
  currentTexCtx = TexStreamingContext(currentFrameCamera.noJitterPersp, w);

  AimRenderingData aimRenderingData = get_aim_rendering_data();

  dafg::NodeHandle testEmptyNode;
  if (test_dynamic_node_allocation)
    testEmptyNode = dafg::register_node("empty_test_node", DAFG_PP_NODE_SRC, [](dafg::Registry) { return [] {}; });

  const bool requiresSubsamplingThisFrame = (subPixels = getSubPixels()) > 1;
  const bool requiresSuperSamplingThisFrame = (superPixels = getSuperPixels()) > 1;
  if (requiresSuperSamplingThisFrame)
  {
    int superW = w * superPixels, superH = h * superPixels;
    if (screenshotSuperFrame)
    {
      TextureInfo ti;
      screenshotSuperFrame->getinfo(ti);
      if (ti.w != superW || ti.h != superH)
        screenshotSuperFrame.close();
    }
    if (!screenshotSuperFrame)
      screenshotSuperFrame = dag::create_tex(nullptr, superW, superH,
        TEXCF_RTARGET | (hdrrender::is_hdr_enabled() ? TEXFMT_A16B16G16R16F : 0), 1, "screenshotsuperframe");
  }
  else
    screenshotSuperFrame.close();

  const bool subSuperSamplingAvailable = true;

  if (requiresSubsampling != requiresSubsamplingThisFrame || requiresSupersampling != requiresSuperSamplingThisFrame ||
      (subSuperSamplingNodes.empty() && subSuperSamplingAvailable))
  {
    requiresSubsampling = requiresSubsamplingThisFrame;
    requiresSupersampling = requiresSuperSamplingThisFrame;
    const bool multisampling = requiresSubsampling || requiresSupersampling;
    if (multisampling)
    {
      antiAliasing.reset();
      // Does the coc calculation in the postfx shader
      ShaderGlobal::set_int(antialiasing_typeVarId, AntiAliasingType::NON_TEMPORAL);
      prepareForPostfxNoAANode = makePrepareForPostfxNoAANode();
      fxaaNode = {};
    }
    else
    {
      setAntialiasing();
    }
    subSuperSamplingNodes = makeSubsamplingNodes(requiresSubsampling, requiresSupersampling);
    if (multisampling)
      frameToPresentProducerNode = makeFrameToPresentProducerNode();
    else
      frameToPresentProducerNode = dafg::NodeHandle();
    externalFinalFrameControlNodes = makeExternalFinalFrameControlNodes(multisampling);
    postfxTargetProducerNode = makePostfxTargetProducerNode(multisampling);
  }

  dafg::ExternalState state;
  state.wireframeModeEnabled = ::grs_draw_wire;
  state.vrsMaskEnabled = shouldToggleVRS(aimRenderingData);
  dafg::update_external_state(state);
  if (state.vrsMaskEnabled != (vrsNodeHandles.size() > 2) || vrsNodeHandles.empty())
  {
    vrsNodeHandles = makeCreateVrsTextureNode(!state.vrsMaskEnabled);
  }

  const uint32_t cameraMultiplexCount = camera_in_camera::is_lens_render_active() ? 2 : 1;
  dafg::set_multiplexing_extents(dafg::multiplexing::Extents{
    static_cast<uint32_t>(superPixels * superPixels), static_cast<uint32_t>(subPixels * subPixels), 1, cameraMultiplexCount});

  // todo: FG should clear block itself, or at least logerr that blocks should be cleared before run_nodes
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  float cur_mip_scale = getMipmapBias();
  static float last_mip_scale = 0;
  if (last_mip_scale != cur_mip_scale)
  {
    set_mip_bias(cur_mip_scale);
    last_mip_scale = cur_mip_scale;
  }

  const int allsamples = subPixels * superPixels;
  if (allsamples > 1)
    set_mip_bias(-get_log2w(allsamples));

  // TODO frame graph assumes this is off by default, we should get rid of grs_draw_wire changing wireframe mode everywhere
  d3d::setwire(false);
  ShaderGlobal::set_float4(zn_zfarVarId, persp.zn, persp.zf, 0, 0);

  resource_slot::resolve_access();
  [[maybe_unused]] bool fgWasRun = dafg::run_nodes();

  if (allsamples > 1)
    set_mip_bias(last_mip_scale);
  if (::grs_draw_wire)
    d3d::setwire(::grs_draw_wire);

  if (auto ds = get_daskies())
    ds->unsetSkyLightParams();

#if _TARGET_PC   // Assume that only PC can reset
  if (!fgWasRun) // Dev lost?
    waitAllJobs();
#endif

  // Stuff after this point is GUI and debug visualization, they don't
  // support the `global_frame` block.
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  ++renderedFrames;
}

void on_ui_finished()
{
  if (WorldRenderer *wr = static_cast<WorldRenderer *>(get_world_renderer()))
    wr->onUIFinished();
}

void WorldRenderer::onUIFinished()
{
  if (dynamicQuality)
    dynamicQuality->onFrameEnd();
  if (dynamicResolution)
    dynamicResolution->endFrame();
}

void WorldRenderer::debugDraw()
{
  if (debug_tex_overlay)
    debug_tex_overlay->render();
  if (debug_tonemap_overlay)
    debug_tonemap_overlay->render();
  draw_rtr_validation();
  shadowsManager.debugRenderVoxelShadows();
}

void WorldRenderer::getSunSph0Color(float /*height*/, Color3 &oSun, Color3 &sph0)
{
  oSun = sun;
  sph0 = Color3(sphValues[0].a, sphValues[1].a, sphValues[2].a);
}

void WorldRenderer::setDirToSun()
{
  Point3 newSunDir = get_daskies() ? get_daskies()->getPrimarySunDir() : Point3(0.400, -0.610, 0.684);
  dir_to_sun.realTime = newSunDir;

  rendinst::setDirFromSun(-newSunDir); // it can be outdated, so we update it no matter what

  auto useCompression = [] { return ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("globalShadowCompression", true); };

  const bool sunDirChanged =
    dir_to_sun.hasPendingForceUpdate || dot(newSunDir, dir_to_sun.curr) < cos(sun_direction_update_threshold_deg * DEG_TO_RAD);

  if (sunDirChanged)
    dir_to_sun.sunDirectionUpdateStage = DirToSun::NO_UPDATE;

  dir_to_sun.hasPendingForceUpdate = false;

  switch (dir_to_sun.sunDirectionUpdateStage)
  {
    case DirToSun::NO_UPDATE:
    {
      if (sunDirChanged)
      {
        dir_to_sun.sunDirectionUpdateStage++;
        dir_to_sun.curr = newSunDir;
        if (rendinst::rendinstGlobalShadows)
          rendinst::startUpdateRIGenGlobalShadows();
        rendinst::render::invalidateRIGenExtraShadowsVisibility();
        g_entity_mgr->broadcastEventImmediate(CmdRecreateSpotLights());
      }
    }
    break;
    case DirToSun::RENDINST_GLOBAL_SHADOWS:
      if (!rendinst::rendinstGlobalShadows ||
          (rendinst::render::are_impostors_ready_for_depth_shadows() &&
            rendinst::render::renderRIGenGlobalShadowsToTextures(-dir_to_sun.curr, false, useCompression(), !isTimeDynamic())))
        dir_to_sun.sunDirectionUpdateStage++;
      break;
    case DirToSun::STATIC_SHADOW:
    {
      auto staticShadows = shadowsManager.getStaticShadows();
      if (!staticShadows || !staticShadows->isAnyCascadeInTransition())
        dir_to_sun.sunDirectionUpdateStage++;
      break;
    }
    case DirToSun::RELOAD_CUBE:
      dir_to_sun.sunDirectionUpdateStage++;
      reloadCube(true);
      break;
    case DirToSun::IMPOSTOR_PRESHADOWS:
      rendinst::render::impostorPreshadowNeedUpdate = true;
      dir_to_sun.sunDirectionUpdateStage++;
      break;
    case DirToSun::FINISHED:
      dir_to_sun.prev = dir_to_sun.curr;
      dir_to_sun.sunDirectionUpdateStage = DirToSun::NO_UPDATE;
      break;
    default: dir_to_sun.sunDirectionUpdateStage++; break;
  }
  if (dir_to_sun.sunDirectionUpdateStage < DirToSun::NO_UPDATE || dir_to_sun.sunDirectionUpdateStage > DirToSun::FINISHED)
  {
    logerr("invalid DirToSun update stage! dir_to_sun.sunDirectionUpdateStage = %@", dir_to_sun.sunDirectionUpdateStage);
    dir_to_sun.sunDirectionUpdateStage = DirToSun::NO_UPDATE;
  }

  ShaderGlobal::set_float4(from_sun_directionVarId, -dir_to_sun.realTime);
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);

  shadowsManager.updateShadowsQFromSunAndReinit();
}

void WorldRenderer::loadFogNodes(const String &graph_name, float low_range, float high_range, float low_height, float high_height)
{
  G_ASSERT(is_main_thread());

  root_fog_graph = graph_name;
  debug("root_fog_graph (rootFogGraph) = '%s'", root_fog_graph.str());
  if (!volumeLight)
    return;

  volfog_force_invalidate = 1; // Invalidate on next frame

  volumeLightLowRange = low_range;
  volumeLightHighRange = high_range;
  volumeLightLowHeight = low_height;
  volumeLightHighHeight = high_height;

  volumeLight->setRange(volumeLightLowRange);
  volumeLight->initShaders(graph_name);
  setNbsQuality();
  add_volfog_optional_graphs();
}

void WorldRenderer::loadEnviCoverNodes(const String &graph_name)
{
  G_ASSERT(is_main_thread());

  // Disable NBS on cards not supporting UAVs for necessary formats
  updateEnviCoverCompatibility();
  if (!isEnviCoverCompatible)
    return;

  root_envi_cover_graph = graph_name;

  if (!enviCover)
    return;

  if (root_envi_cover_graph.empty())
    return;

  setEnviCover(true);
  enviCover->setValidNBS(true);
  enviCover->initShader(graph_name);
  setNbsQuality();
  envi_cover_use_NBS.set(true);
  add_envi_cover_optional_graphs();
  // Need here for safety is this is called after node initializations because of async load
  requestFgRecreation("Envi Cover NBS loaded");
}

void WorldRenderer::closeGround()
{
  closeLandMicroDetails();
  del_it(lmeshRenderer);
  ShaderGlobal::reset_from_vars(lightmapTexId);
  release_managed_tex(lightmapTexId);
  lightmapTexId = BAD_TEXTUREID;
}

bool WorldRenderer::useClipmapFeedbackOversampling() { return Clipmap::is_uav_supported(); }

void WorldRenderer::closeClipmap() { del_it(clipmap); }
void WorldRenderer::initClipmap()
{
  if (!lmeshMgr)
    return;

  bvh_release_bindlessly_held_textures();

  // Init clipmap
  if (!clipmap)
    clipmap = new Clipmap(true); // TEXFMT_R5G6B5

  // Get feedback properties
  FeedbackProperties feedbackProperties;
  if (useClipmapFeedbackOversampling())
  {
    IPoint2 targetRes;
    getRenderingResolution(targetRes.x, targetRes.y);

    feedbackProperties = Clipmap::getSuggestedOversampledFeedbackProperties(targetRes);

    IPoint2 feedbackLargeSize = feedbackProperties.feedbackSize * feedbackProperties.feedbackOversampling;
    logdbg("clipmap feedback: Using large UAV feedback, %i x %i resolution. Feedback is downsampled x%i, down to %i x %i.",
      feedbackLargeSize.x, feedbackLargeSize.y, feedbackProperties.feedbackOversampling, feedbackProperties.feedbackSize.x,
      feedbackProperties.feedbackSize.y);
  }
  else
  {
    feedbackProperties = Clipmap::getDefaultFeedbackProperties();

    logdbg("clipmap feedback: Using default UAV feedback, %i x %i resolution", feedbackProperties.feedbackSize.x,
      feedbackProperties.feedbackSize.y);
  }

  int lastTexMips = clipmap ? clipmap->getMaxTexMips() : -1;
  const DataBlock *clipBlk = ::dgs_get_settings()->getBlockByNameEx("clipmap");
  if (lastTexMips <= 0)
    lastTexMips = clipBlk->getInt("texMips", 7);

#if _TARGET_IOS || _TARGET_ANDROID || _TARGET_C3
  int cw = 2048;
  int ch = 2048;
  int defaultUpdateCount = TEX_DEFAULT_CLIPMAP_UPDATES_PER_FRAME_MOBILE;
  bool allowETC2 = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("allowETC2", true);
#else
  int cw = 4096;
  int ch = 4096;
  int defaultUpdateCount = TEX_DEFAULT_CLIPMAP_UPDATES_PER_FRAME;
  bool allowETC2 = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("allowETC2", false);
#endif

  bool hasETC2 = allowETC2 && BcCompressor::isAvailable(BcCompressor::ECompressionType::COMPRESSION_ETC2_RGBA);
  bool useCompression = (BcCompressor::isAvailable(BcCompressor::ECompressionType::COMPRESSION_BC3) &&
                          BcCompressor::isAvailable(BcCompressor::ECompressionType::COMPRESSION_BC5)) ||
                        hasETC2;
  useCompression = clipBlk->getBool("useCompression", useCompression);
  float texelSize = clipBlk->getReal("texelSize", 4.0 / 1024);

  cw = clipBlk->getInt("cacheWidth", cw);
  ch = clipBlk->getInt("cacheHeight", ch);
  ch *= useCompression ? 2 : 1;

  // Anisotropic x16 and x8 request more tiles of smaller mip level and bigger boarders are needed.
  // Anisotropic x16 can make use of 224^2 texels per tile (76.5% of area), anisotropic x8 240^2 (87.8%), anisotropic x4 248^2 (93.8%).
  int clipmapAnisotropic = min(::dgs_tex_anisotropy, 4);

  // There is no a special pass for non uav hardware feedback so switch to the cpu one instead
  clipmap->init(texelSize, Clipmap::is_uav_supported() ? Clipmap::CPU_HW_FEEDBACK : Clipmap::SOFTWARE_FEEDBACK, feedbackProperties,
    lastTexMips);

  struct CacheFormats
  {
    uint32_t full[3];

    struct FormatsAndCount
    {
      const uint32_t *fmt;
      const size_t count;
    };
    FormatsAndCount get() const { return FormatsAndCount{full, 3}; }
  };

  CacheFormats compressedFormats{{TEXFMT_DXT5 | TEXCF_SRGBREAD, TEXFMT_ATI2N, TEXFMT_DXT5}};
  CacheFormats compressedEtcFormats{{TEXFMT_ETC2_RGBA | TEXCF_SRGBREAD, TEXFMT_ETC2_RG, TEXFMT_ETC2_RGBA}};
  CacheFormats uncompressedFormats{{TEXFMT_DEFAULT | TEXCF_SRGBREAD, TEXFMT_R8G8, TEXFMT_DEFAULT}};

  const auto &cacheFmtInfo = useCompression ? hasETC2 ? compressedEtcFormats : compressedFormats : uncompressedFormats;

  const auto [cacheFormats, cachesCount] = cacheFmtInfo.get();
  const auto [cacheFormatsFallback, cachesCountFallback] = uncompressedFormats.get();
  G_ASSERT(cachesCount == cachesCountFallback);

  const uint32_t buf_formats[3] = {TEXFMT_DEFAULT | TEXCF_SRGBWRITE, TEXFMT_DEFAULT, TEXFMT_DEFAULT};
  const uint32_t buf_formats_compat[3] = {TEXFMT_A16B16G16R16F, TEXFMT_A16B16G16R16F, TEXFMT_A16B16G16R16F};

  // use ARGB16F, as hw race workaround
  bool useCompatBufFormats = d3d::get_driver_desc().issues.hasRenderPassClearDataRace;
  if (useCompatBufFormats)
    ShaderGlobal::set_int(get_shader_variable_id("uncompressed_clipmap_buffer_srgb_mode"), 1);

  const float maxEffectiveTargetResolution = 2 * 1920 * 1080;
  clipmap->initVirtualTexture(cw, ch, 256, 1, clipmapAnisotropic, maxEffectiveTargetResolution);

  clipmap->createCaches(cacheFormats, cacheFormatsFallback, cachesCount, useCompatBufFormats ? buf_formats_compat : buf_formats, 3);
  clipmap->setMaxTileUpdateCount(clipBlk->getInt("updateTileCount", defaultUpdateCount));
}

void WorldRenderer::updateSky(float realDt)
{
  TIME_PROFILE(updateSky);
  if (!get_daskies())
  {
    setDirToSun();
    return;
  }
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);

  if (isTimeDynamic())
  {
    if (dynamic_time_scale != 0)
    {
      // Only for debug purposes
      double time = get_daskies()->getStarsJulianDay();
      time += realDt * dynamic_time_scale / 3600 / 24;
      get_daskies()->setStarsJulianDay(time);
    }
    get_daskies()->setAstronomicalSunMoon();
  }

  get_daskies()->updateSkyLightParams();
  get_daskies()->prepare(get_daskies()->getSunDir(), false, 0);
  setDirToSun();

  Color3 amb, moon, moonamb;
  float sunCos, moonCos;
  if (get_daskies()->currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
  {
    if (get_daskies()->getMoonIsPrimary())
    {
      sun = moon;
      amb = moonamb;
    }
    sun *= get_daskies()->getSkyLightSunAttenuation();
    amb *= get_daskies()->getSkyLightEnviAttenuation();
    sun = sun / PI;
    // optimize
    ShaderGlobal::set_float4(sun_light_colorVarId, color4(sun, 0));
    ShaderGlobal::set_float4(sun_color_0VarId, no_sun.get() ? Color4(0, 0, 0, 0) : color4(sun, 0));
    ShaderGlobal::set_float4(sky_colorVarId, color4(amb, 0));
    ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);
    acesfx::setSkyParams(-get_daskies()->getPrimarySunDir(), sun, amb);
  }
  else
  {
    G_ASSERT(0);
  }
  // moving skies
  // get_daskies()->setCloudsOrigin(getDaSkies()->getCloudsOrigin().x + windX, getDaSkies()->getCloudsOrigin().y + windZ);
  // get_daskies()->setStrataCloudsOrigin(getDaSkies()->getStrataCloudsOrigin().x + render_panel.strataCloudsSpeed*windX,
  //                               get_daskies()->getStrataCloudsOrigin().y+render_panel.strataCloudsSpeed*windZ);
}

bool is_rendinst_tessellation_supported() { return d3d::get_driver_desc().caps.hasQuadTessellation; }

static bool isRendeinstTesselationEnabled()
{
  return is_rendinst_tessellation_supported() &&
         ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("rendinstTesselation", false);
}

void WorldRenderer::initDisplacement()
{
  heightmapAround.close();
  bool rendinstTesselation = isRendeinstTesselationEnabled();
  heightmapAround = dag::create_tex(NULL, DISPLACEMENT_TEX_SIZE, DISPLACEMENT_TEX_SIZE,
    TEXCF_RTARGET | TEXFMT_R8 | (rendinstTesselation ? TEXCF_GENERATEMIPS : 0), rendinstTesselation ? 0 : 1, "hmap_ofs_tex");
  d3d::SamplerInfo smpInfo;
  smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
  ShaderGlobal::set_sampler(get_shader_variable_id("hmap_ofs_tex_samplerstate"), d3d::request_sampler(smpInfo));
  d3d::resource_barrier({heightmapAround.getTex2D(), RB_RO_SRV | RB_STAGE_COMPUTE, 0, 0});
  displacementData.resetOrigin();
  displacementData.texSize = DISPLACEMENT_TEX_SIZE;
}

void WorldRenderer::initHmapPatches(int texSize)
{
  hmapPatchesEnabled = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("hmapPatchesEnabled", true);
  if (!hmapPatchesEnabled)
    return;
  hmapPatchesDepthTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "hmap_patches_depth_tex");
  hmapPatchesTex = dag::create_tex(NULL, texSize, texSize, TEXCF_RTARGET | TEXFMT_L16, 1, "hmap_patches_tex");
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    smpInfo.filter_mode = d3d::FilterMode::Linear;
    ShaderGlobal::set_sampler(get_shader_variable_id("hmap_patches_tex_samplerstate"), d3d::request_sampler(smpInfo));
  }
  d3d::resource_barrier({hmapPatchesTex.getTex2D(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});
  processHmapPatchesDepth.init("process_hmap_patches_depth");
  hmapPatchesData.resetOrigin();
  hmapPatchesData.texSize = texSize;
}

struct DisplacementCallback
{
  float texelSize;
  LandmeshCMRenderer cm;

  DisplacementCallback(float texel, LandMeshRenderer &lmRenderer, LandMeshManager &lmMgr) : texelSize(texel), cm(lmRenderer, lmMgr) {}

  void start(const IPoint2 &texelOrigin) { cm.startRenderTiles(point2(texelOrigin) * texelSize); }
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
  {
    d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
    d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    BBox2 box(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    cm.renderTile(box);
  }
  void end() { cm.endRenderTiles(); }
};

struct HmapPatchesCallback
{
  LandMeshManager &provider;
  LandMeshRenderer &renderer;
  ViewProjMatrixContainer oviewproj;
  float texelSize;
  float viewPosY;
  int texSize;
  IPoint2 newOrigin;
  float minZ, maxZ;
  TMatrix ivtm;
  RiGenVisibility *visibility;
  LMeshRenderingMode prevLandmeshRenderingMode;
  TMatrix4 viewTm;

  HmapPatchesCallback(float texel,
    LandMeshRenderer &lm_renderer,
    LandMeshManager &lm_mgr,
    const IPoint2 &new_origin,
    int tex_size,
    float minz,
    float maxz,
    float view_y,
    RiGenVisibility *vis) :
    renderer(lm_renderer),
    provider(lm_mgr),
    oviewproj(),
    texelSize(texel),
    texSize(tex_size),
    viewPosY(view_y),
    newOrigin(new_origin),
    minZ(minz),
    maxZ(maxz),
    visibility(vis),
    prevLandmeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH)
  {
    viewTm = TMatrix4::IDENT;
    viewTm.setcol(0, 1, 0, 0, 0);
    viewTm.setcol(1, 0, 0, 1, 0);
    viewTm.setcol(2, 0, 1, 0, 0);
  }
  void start(const IPoint2 &texelOrigin)
  {
    d3d_get_view_proj(oviewproj);
    d3d::settm(TM_VIEW, &viewTm);
    ivtm = orthonormalized_inverse(tmatrix(viewTm));
    BBox3 landBox = provider.getBBox();
    maxZ = max(maxZ, landBox[1].y + 1);
    ShaderGlobal::set_float4(hmap_patches_min_max_zVarId, minZ, maxZ, 0.0, 1.0);

    prevLandmeshRenderingMode = renderer.setLMeshRenderingMode(LMeshRenderingMode::RENDERING_CLIPMAP);
    renderer.prepare(provider, Point3(texelOrigin.x * texelSize, viewPosY, texelOrigin.y * texelSize), 2.f);
    IPoint2 texelsFrom = IPoint2(newOrigin.x - texSize / 2, newOrigin.y - texSize / 2);
    IPoint2 wd = IPoint2(texSize, texSize);
    BBox2 box(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    BBox3 box3(Point3(box[0].x, minZ, box[0].y), Point3(box[1].x, maxZ, box[1].y));
    bbox3f boxCull = v_ldu_bbox3(box3);
    rendinst::prepareRIGenExtraVisibilityBox(boxCull, 0, 0.1, 100, *visibility);
  }
  void renderQuad(const IPoint2 &lt, const IPoint2 &wd, const IPoint2 &texelsFrom)
  {
    d3d::setview(lt.x, lt.y, wd.x, wd.y, 0, 1);
    BBox2 box(point2(texelsFrom) * texelSize, point2(texelsFrom + wd) * texelSize);
    SCOPE_VIEW_PROJ_MATRIX;
    d3d::clearview(CLEAR_ZBUFFER, E3DCOLOR(), 0, 0);
    TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, minZ, maxZ);
    d3d::settm(TM_PROJ, &proj);
    SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
    rendinst::render::renderRIGen(rendinst::RenderPass::Depth, visibility, ivtm, rendinst::LayerFlag::RendinstHeightmapPatch,
      rendinst::OptimizeDepthPass::No);

    BBox3 region(Point3(box[0].x, minZ, box[0].y), Point3(box[1].x, maxZ + 500, box[1].y));
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    ShaderGlobal::setBlock(landMeshPrepareClipmapBlockId, ShaderGlobal::LAYER_SCENE);
    renderer.setRenderInBBox(region);
    TMatrix4_vec4 globTm = viewTm * proj;
    renderer.render(reinterpret_cast<mat44f_cref>(globTm), proj, Frustum{globTm}, provider, renderer.RENDER_PATCHES, ivtm.getcol(3));
  }
  void end()
  {
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
    renderer.setLMeshRenderingMode(prevLandmeshRenderingMode);
    d3d_set_view_proj(oviewproj);
    renderer.setRenderInBBox(BBox3());
  }
};

static void render_toroidal_update_helper(Point2 alignedOrigin,
  float fullDistance,
  ToroidalHelper &displacementData,
  eastl::fixed_function<sizeof(void *) * 4, void(float, const IPoint2 &, ToroidalHelper &, int)> rcb,
  bool force = false)
{
  float texelSize = (fullDistance / displacementData.texSize);
  enum
  {
    TEXEL_ALIGN = 4
  };
  IPoint2 newTexelsOrigin = (ipoint2(floor(alignedOrigin / (texelSize))) + IPoint2(TEXEL_ALIGN / 2, TEXEL_ALIGN / 2));
  newTexelsOrigin = newTexelsOrigin - (newTexelsOrigin % TEXEL_ALIGN);
  enum
  {
    THRESHOLD = TEXEL_ALIGN * 4
  };
  IPoint2 move = abs(displacementData.curOrigin - newTexelsOrigin);
  if (move.x >= THRESHOLD || move.y >= THRESHOLD || force)
  {
    const float fullUpdateThreshold = 0.45;
    const int fullUpdateThresholdTexels = fullUpdateThreshold * displacementData.texSize;
    // if distance travelled is too big, there is no need to update movement in two steps
    if (max(move.x, move.y) < fullUpdateThresholdTexels)
    {
      if (move.x < move.y)
        newTexelsOrigin.x = displacementData.curOrigin.x;
      else
        newTexelsOrigin.y = displacementData.curOrigin.y;
    }
    rcb(texelSize, newTexelsOrigin, displacementData, fullUpdateThresholdTexels);
  }
}

void WorldRenderer::renderDisplacementRiLandclasses(const Point3 &camera_pos)
{
  if (!lmeshMgr)
    return;

  clipmap->getRiLandclassIndices(riLandclassIndices);
  if (riLandclassIndices.size() == 0)
    return;

  TIME_D3D_PROFILE(riLandclassDisplacement);

  // invalidate/insert new ri landclasses
  while (riLandclassDisplacementDataArr.size() < riLandclassIndices.size())
  {
    auto &toroidalHelper = riLandclassDisplacementDataArr.push_back();
    toroidalHelper.resetOrigin();
    toroidalHelper.texSize = DISPLACEMENT_TEX_SIZE;
  }

  // invalidate changed ri landclasses
  for (int i = 0; i < riLandclassIndices.size(); ++i)
  {
    if (i >= riLandclassIndicesPrev.size() || riLandclassIndices[i] != riLandclassIndicesPrev[i])
      riLandclassDisplacementDataArr[i].resetOrigin();
  }
  // lazy init
  if (!riLandclassDepthTextureArr || riLandclassIndices.size() > riLandclassIndicesPrev.size())
  {
    riLandclassDepthTextureArr.close();
    riLandclassDepthTextureArr = dag::create_array_tex(DISPLACEMENT_TEX_SIZE, DISPLACEMENT_TEX_SIZE, riLandclassIndices.size(),
      TEXCF_RTARGET | TEXFMT_R8, 1, "deform_hmap_ri_landclass_arr");
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Wrap;
    ShaderGlobal::set_sampler(get_shader_variable_id("deform_hmap_ri_landclass_arr_samplerstate", true),
      d3d::request_sampler(smpInfo));
  }
  riLandclassIndicesPrev = riLandclassIndices;

  if (::grs_draw_wire)
    d3d::setwire(0);

  for (int ri_index = 0; ri_index < riLandclassIndices.size(); ++ri_index)
  {
    ToroidalHelper &toroidal_helper = riLandclassDisplacementDataArr[ri_index];
    float distToGround;
    int riOffset = ri_index + 1;
    Point3 origin = clipmap->getMappedRelativeRiPos(riOffset, camera_pos, distToGround);

    static constexpr float DISPLACEMENT_CULLING_DIST_BIAS = 1.0f; // must be > ShaderGlobal::get_float(hmap_displacement_upVarId))
    static constexpr float DISPLACEMENT_CULLING_DIST = DISPLACEMENT_DIST + DISPLACEMENT_CULLING_DIST_BIAS;
    if (distToGround > DISPLACEMENT_CULLING_DIST)
      continue;

    Point2 alignedOrigin = Point2::xz(origin);
    float fullDistance = 2 * DISPLACEMENT_DIST;

    float targetTexelSize = 0;
    auto renderDisplacement = [&](float texelSize, const IPoint2 &newTexelsOrigin, ToroidalHelper &displacementData,
                                int fullUpdateThresholdTexels) {
      TIME_D3D_PROFILE(updateDisplacement);
      SCOPE_RENDER_TARGET;
      ShaderGlobal::set_int(clipmap_writes_height_onlyVarId, 1);
      d3d::set_render_target({}, DepthAccess::RW, {{riLandclassDepthTextureArr.getArrayTex(), 0, static_cast<uint32_t>(ri_index)}});
      DisplacementCallback cb(texelSize, *lmeshRenderer, *lmeshMgr);
      toroidal_update(newTexelsOrigin, displacementData, fullUpdateThresholdTexels, cb);
      d3d::resource_barrier({riLandclassDepthTextureArr.getArrayTex(), RB_RO_SRV | RB_STAGE_VERTEX, (unsigned)ri_index, 1});
      ShaderGlobal::set_int(clipmap_writes_height_onlyVarId, 0);
      targetTexelSize = texelSize;
    };
    const bool forceInvalidate = false;
    render_toroidal_update_helper(alignedOrigin, fullDistance, toroidal_helper, renderDisplacement, forceInvalidate);
    if (targetTexelSize != 0) // if not changed, just set the previous state to the shadervars
    {
      Point2 ofs =
        point2((toroidal_helper.mainOrigin - toroidal_helper.curOrigin) % toroidal_helper.texSize) / toroidal_helper.texSize;
      Point2 aligned = point2(toroidal_helper.curOrigin) * targetTexelSize;

      clipmap->setHmapOfsAndTexOfs(ri_index,
        Point4(1.0f / fullDistance, 1.0f / fullDistance, -aligned.x / fullDistance + 0.5f, -aligned.y / fullDistance + 0.5f),
        Point4(ofs.x, ofs.y, 0, 0));
    }
  }

  clipmap->updateLandclassData();

  if (::grs_draw_wire)
    d3d::setwire(1);
}

void WorldRenderer::renderDisplacementAndHmapPatches(const Point3 &origin, float distance)
{
  if (!lmeshMgr)
    return;
  Point2 alignedOrigin = Point2::xz(origin);
  float fullDistance = 2 * distance;
  auto renderDisplacement = [&](float texelSize, const IPoint2 &newTexelsOrigin, ToroidalHelper &displacementData,
                              int fullUpdateThresholdTexels) {
    TIME_D3D_PROFILE(updateDisplacement);
    if (::grs_draw_wire)
      d3d::setwire(0);
    SCOPE_RENDER_TARGET;

    ShaderGlobal::set_int(clipmap_writes_height_onlyVarId, 1);

    d3d::set_render_target({}, DepthAccess::RW, {{heightmapAround.getTex2D(), 0, 0}});
    DisplacementCallback cb(texelSize, *lmeshRenderer, *lmeshMgr);
    if (displacementInvalidationBoxes.size())
    {
      TIME_D3D_PROFILE(invalidateDisplacement);
      toroidal_invalidate_boxes(displacementData, texelSize, displacementInvalidationBoxes, cb);
      clear_and_shrink(displacementInvalidationBoxes);
    }
    toroidal_update(newTexelsOrigin, displacementData, fullUpdateThresholdTexels, cb);
    if (heightmapAround)
    {
      // we need to genmips only when rendinstTesselation is enabled, but it is not tracked anywhere, so use level count as hint
      if (heightmapAround.getTex2D()->level_count() > 1)
        heightmapAround.getTex2D()->generateMips();
      d3d::resource_barrier({heightmapAround.getTex2D(), RB_RO_SRV | RB_STAGE_VERTEX, 0, 0});
    }
    Point2 ofs =
      point2((displacementData.mainOrigin - displacementData.curOrigin) % displacementData.texSize) / displacementData.texSize;
    ShaderGlobal::set_float4(world_to_hmap_tex_ofsVarId, ofs.x, ofs.y, 0, 0);
    Point2 aligned = point2(displacementData.curOrigin) * texelSize;
    ShaderGlobal::set_float4(world_to_hmap_ofsVarId, 1.0f / fullDistance, 1.0f / fullDistance, -aligned.x / fullDistance + 0.5,
      -aligned.y / fullDistance + 0.5);
    if (::grs_draw_wire)
      d3d::setwire(1);

    ShaderGlobal::set_int(clipmap_writes_height_onlyVarId, 0);
  };
  bool invalidateDisplacement = displacementInvalidationBoxes.size() > 0;
  // when heightmapAround is not initialized, there should be no invalidations
  // to avoid growing buffer
  G_ASSERTF((invalidateDisplacement && heightmapAround.getTex2D()) || !invalidateDisplacement,
    "updating displacement without hmap_ofs_tex");
  if (heightmapAround.getTex2D())
    render_toroidal_update_helper(alignedOrigin, fullDistance, displacementData, renderDisplacement, invalidateDisplacement);
  if (!hmapPatchesEnabled)
    return;
  auto renderPatches = [&](float texelSize, const IPoint2 &newTexelsOrigin, ToroidalHelper &displacementData,
                         int fullUpdateThresholdTexels) {
    SCOPE_RENDER_TARGET;
    TIME_D3D_PROFILE(updateHeightmapPatches);
    d3d::set_render_target({hmapPatchesDepthTex.getTex2D(), 0, 0}, DepthAccess::RW, {});
    float minZ, maxZ;
    getMinMaxZ(minZ, maxZ);
    HmapPatchesCallback patchCb(texelSize, *lmeshRenderer, *lmeshMgr, newTexelsOrigin, hmapPatchesData.texSize, minZ, maxZ, origin.y,
      rendinstHmapPatchesVisibility);
    toroidal_update(newTexelsOrigin, displacementData, fullUpdateThresholdTexels, patchCb);
    TIME_D3D_PROFILE(updateHeightmapPatchesFiltering);
    d3d::resource_barrier({hmapPatchesDepthTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});
    d3d::set_render_target({}, DepthAccess::RW, {{hmapPatchesTex.getTex2D(), 0, 0}});
    processHmapPatchesDepth.render();
    d3d::resource_barrier({hmapPatchesTex.getTex2D(), RB_RO_SRV | RB_STAGE_VERTEX | RB_STAGE_COMPUTE, 0, 0});
    Point2 ofs =
      point2((displacementData.mainOrigin - displacementData.curOrigin) % displacementData.texSize) / displacementData.texSize;
    ShaderGlobal::set_float4(world_to_hmap_patches_tex_ofsVarId, ofs.x, ofs.y, 0, 0);
    alignedOrigin = point2(displacementData.curOrigin) * texelSize;
    ShaderGlobal::set_float4(world_to_hmap_patches_ofsVarId, 1.0f / fullDistance, 1.0f / fullDistance,
      -alignedOrigin.x / fullDistance + 0.5, -alignedOrigin.y / fullDistance + 0.5);
  };
  render_toroidal_update_helper(alignedOrigin, fullDistance, hmapPatchesData, renderPatches);
}

void WorldRenderer::prepareClipmap(const Point3 &origin, const TMatrix &view_itm, const TMatrix4 &globtm, float wk, int minimum_zoom)
{
  if (!lmeshMgr)
    return;

  TIME_D3D_PROFILE(prepare_clipmap);
  if (::grs_draw_wire)
    d3d::setwire(0);

  const float water_level = water ? fft_water::get_level(water) : HeightmapHeightCulling::NO_WATER_ON_LEVEL;
  lmeshRenderer->prepare(*lmeshMgr, origin, cameraHeight, water_level);

  if (lmeshMgr->getHmapHandler())
  {
    int newTerrainState = lmeshMgr->getHmapHandler()->getTerrainStateVersion();
    if (invalidationBoxesAfterHeightmapChange.size() && hmapTerrainStateVersion == newTerrainState)
    {
      grass_invalidate(invalidationBoxesAfterHeightmapChange);
      invalidateAfterHeightmapChange();
    }
    hmapTerrainStateVersion = newTerrainState;
  }

  clipmap->finalizeFeedback();
  static int land_load_generation = 0;
  if (land_load_generation != interlocked_relaxed_load(textag_get_info(TEXTAG_LAND).loadGeneration))
  {
    land_load_generation = interlocked_acquire_load(textag_get_info(TEXTAG_LAND).loadGeneration);
    clipmap->invalidate(false);
  }
  const Tab<BBox2> &invalidBboxes = clipmap_decals_mgr::get_updated_regions();
  if (invalidBboxes.size() && clipmap_decals_mgr::check_decal_textures_loaded())
  {
    for (const BBox2 &bbox : invalidBboxes)
      clipmap->invalidateBox(bbox);
    // invalidate displacement only if it is enabled
    if (heightmapAround.getTex2D())
      displacementInvalidationBoxes = invalidBboxes;
    clipmap_decals_mgr::clear_updated_regions();
  }

#if DAGOR_DBGLEVEL > 0
  static bool debugAlwaysInvalidateClipmap = dgs_get_settings()->getBlockByNameEx("debug")->getBool("alwaysInvalidateClipmap", false);
  if (DAGOR_UNLIKELY(debugAlwaysInvalidateClipmap))
    clipmap->invalidate(true);
#endif

  int w, h;
  getRenderingResolution(w, h);
  clipmap->setTargetSize(w, h, getMipmapBias());

  LandmeshCMRenderer landmeshCMRenderer(*lmeshRenderer, *lmeshMgr);
  clipmap->prepareRender(landmeshCMRenderer);

  ZoomFeeedbackParams zoomParams = {cameraActualHeight, wk, minimum_zoom, {clipmapCutoffZoom, clipmapCutoffMipsCnt}};
  SoftwareFeedbackParams swFbParams = {view_itm, 0.0, 0.0f, cameraHeight /* should be < maxDist for planes! */};
  clipmap->prepareFeedback(origin, globtm, nullptr, zoomParams, swFbParams);

  const float fallBackTexelSize =
    /* 8 times bigger than last aligned clip */ (1 << (clipmap->getMaxTexMips() - 1)) * clipmap->getPixelRatio() * 8.f;

  // (1 << (clipmap->getMaxTexMips() - 1)) * (sqrtf(clipmap->getZoom()) * clipmap->getStartTexelSize() * 8.f);
  // 8 times bigger than last aligned clip, with sqrt on current zoom

  // if texel size is 64 metes, we seeing almost all location anyway
  clipmap->initFallbackPage((fallBackTexelSize < 64.f) ? 2 : 0, max(fallBackTexelSize, 3.f));

  if (::grs_draw_wire)
    d3d::setwire(1);
}

void WorldRenderer::prepareDeformHmap()
{
  if (deformHmap != nullptr && deformHmap->isEnabled())
  {
    TIME_D3D_PROFILE(DeformableHeightmap_render);
    if (::grs_draw_wire)
      d3d::setwire(0);

    deformHmap->beforeRenderDepth();
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::DeformHmap));
    ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_BEGIN);
    g_entity_mgr->broadcastEventImmediate(
      RenderHmapDeform(deformHmap->getDeformRect(), currentFrameCamera.negRoundedCamPos, currentFrameCamera.negRemainderCamPos,
        currentFrameCamera.viewTm, deformHmap->getInverseViewTm(), currentFrameCamera.viewItm.getcol(3)));

    d3d::driver_command(Drv3dCommand::ASYNC_PIPELINE_COMPILE_RANGE_END);

    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    deformHmap->afterRenderDepth();

    if (::grs_draw_wire)
      d3d::setwire(1);
  }
}

void WorldRenderer::renderGround(const LandMeshCullingData &lmesh_culling_data, bool is_first_iter)
{
  if (!lmeshMgr)
    return;

  if (!clipmap)
    initClipmap();

  TIME_D3D_PROFILE(render_ground);

  ShaderGlobal::set_int(autodetect_land_selfillum_enabledVarId,
    autodetect_land_selfillum_colorVarId >= 0 ? ShaderGlobal::get_float4(autodetect_land_selfillum_colorVarId).a > 0.0f : 0);

  if (clipmap && is_first_iter)
    clipmap->startUAVFeedback();
  lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_LANDMESH);

  lmeshRenderer->setUseHmapTankSubDiv(displacementSubDiv);
  lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_WITH_CLIPMAP, lmesh_culling_data, currentFrameCamera.cameraWorldPos);

  if (clipmap && is_first_iter)
    clipmap->endUAVFeedback();
}
#if _TARGET_XBOX
namespace d3d
{
void resummarize_htile(BaseTexture *tex);
}
#endif
void WorldRenderer::renderStaticSceneOpaque(
  int cascade, const Point3 &camera_pos, const TMatrix &view_itm, const Frustum &culling_frustum)
{
  TIME_D3D_PROFILE(renderStaticSceneOpaque);

  Texture *currentDepth = nullptr;

  // static scene!
  if (is_gbuffer_cascade(cascade) && cascade != RENDER_CUBE)
  {
    RiGenVisibility *mainCameraRiMainVisibility = mainCameraVisibilityMgr.getRiMainVisibility();

    G_ASSERT(mainCameraRiMainVisibility && rendinst_cube_visibility);
    if (prepass.get())
    {
      TIME_D3D_PROFILE(ri_prepass);
      Driver3dRenderTarget prevRT;
      d3d::get_render_target(prevRT);
      currentDepth = static_cast<Texture *>(prevRT.getDepth().tex);

      d3d::set_render_target({currentDepth, 0, 0}, DepthAccess::RW, {});
      mainCameraVisibilityMgr.waitVisibility(cascade);
      if (async_riex_opaque.get()) // wait until vb is filled by async opaque render (prepass re-uses same buf struct)
        mainCameraVisibilityMgr.waitAsyncRIGenExtraOpaqueRenderVbFill();
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGenOptimizationDepth(rendinst::RenderPass::Depth,
        cascade == RENDER_MAIN ? mainCameraRiMainVisibility : rendinst_cube_visibility, view_itm,
        rendinst::IgnoreOptimizationLimits::No, rendinst::SkipTrees::Yes, rendinst::RenderGpuObjects::Yes, 1U, currentTexCtx);
      d3d::set_render_target(prevRT);

      renderRITreePrepass(view_itm);
      if (vehicle_cockpit_in_depth_prepass.get())
      {
        g_entity_mgr->broadcastEventImmediate(VehicleCockpitPrepass(currentFrameCamera.viewTm, currentTexCtx));
      }
    }
  }

  if (binScene)
  {
    VisibilityFinder vf;
    // fixme: use same object_to_sceen_ratio as in main scene for csm shadows and main
    vf.set(v_ldu(&camera_pos.x), culling_frustum, 0.0f, 0.0f, 1.0f, 1.0f, cascade == RENDER_MAIN ? getMainCameraOcclusion() : nullptr);
    // todo: we should use object_to_sceen_ratio!

    if (cascade >= RENDER_SHADOWS_CSM)
    {
      binScene->getRsm().allocPrepareVisCtx(cascade + 1);
      binScene->getRsm().doPrepareVisCtx(vf, cascade + 1, 0xFFFFFFFF);
      binScene->getRsm().setPrepareVisCtxToRender(cascade + 1);
      binScene->getRsm().render(vf, cascade + 1, 0xFFFFFFFF);

      binScene->getRsm().setPrepareVisCtxToRender(-1);
    }
    else
    {
      binScene->render(vf, 0, 0xFFFFFFFF);
    }
  }
  renderRendinst(cascade, view_itm);

  if (is_gbuffer_cascade(cascade) && cascade != RENDER_CUBE)
    renderGiCollision(view_itm, culling_frustum);

  if (cascade == RENDER_MAIN)
    renderRITree();

  if (get_cables_mgr())
  {
    if (cascade >= RENDER_SHADOWS_CSM)
      get_cables_mgr()->render(Cables::RENDER_PASS_SHADOW);
    else if (cascade == RENDER_MAIN)
      get_cables_mgr()->render(Cables::RENDER_PASS_OPAQUE);
  }
  if (is_gbuffer_cascade(cascade) && prepass.get() && cascade != RENDER_CUBE)
  {
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    d3d::set_render_target({prevRT.getDepth().tex, 0, 0}, DepthAccess::RW, {});
    render_grass_prepass(GrassView::Main);
    d3d::set_render_target(prevRT);
    d3d::resummarize_htile(currentDepth);
  }

  g_entity_mgr->broadcastEventImmediate(RenderStaticSceneEvent(culling_frustum, camera_pos, cascade));
}

void WorldRenderer::renderDynamicOpaque(
  int cascade, const TMatrix &view_itm, const TMatrix &view_tm, const TMatrix4 &proj_tm, const Point3 &cam_pos)
{
  ShaderGlobal::set_int(dyn_model_render_passVarId, is_gbuffer_cascade(cascade) ? eastl::to_underlying(dynmodel::RenderPass::Color)
                                                                                : eastl::to_underlying(dynmodel::RenderPass::Depth));
  TexStreamingContext texCtx = TexStreamingContext(0);
  TIME_D3D_PROFILE(entities)
  Frustum frustum;
  uint32_t hints = 0;

  CascadeShadows *csm = shadowsManager.getCascadeShadows();
  if (cascade >= RENDER_SHADOWS_CSM)
  {
    if (!csm)
      return;

    frustum = csm->getFrustum(cascade);
    hints |= UpdateStageInfoRender::RENDER_SHADOW | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
  }
  else
  {
    frustum = TMatrix4(view_tm) * proj_tm;
    switch (cascade)
    {
      case RENDER_MAIN:
        hints |= UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH | UpdateStageInfoRender::RENDER_MAIN;
        texCtx = currentTexCtx;
        if (hasMotionVectors)
          hints |= UpdateStageInfoRender::RENDER_MOTION_VECS;
        break;
      case RENDER_CUBE: hints |= UpdateStageInfoRender::RENDER_COLOR | UpdateStageInfoRender::RENDER_DEPTH; break;
      case RENDER_STATIC_SHADOW:
      case RENDER_DYNAMIC_SHADOW: hints |= UpdateStageInfoRender::RENDER_SHADOW | UpdateStageInfoRender::RENDER_DEPTH; break;
      default: G_ASSERTF(0, "unknown render stage"); break;
    };
  }

  Occlusion *occl = cascade == RENDER_MAIN ? getMainCameraOcclusion() : nullptr;
  const dynmodel_renderer::DynModelRenderingState *pState = NULL;
  char tmps[] = "csm#000";
  if (async_animchars_shadows.get() && cascade >= RENDER_SHADOWS_CSM && cascade - RENDER_SHADOWS_CSM < csm->getNumCascadesToRender())
  {
    pState = dynmodel_renderer::get_state(fmt_csm_render_pass_name(cascade, tmps));
  }
  else if (async_animchars_main.get())
    if (cascade == RENDER_MAIN)
    {
      mainCameraVisibilityMgr.waitAsyncAnimcharMainRender();
      pState = mainCameraVisibilityMgr.getAsyncAnimcharMainRenderState();
    }

  {
    TIME_D3D_PROFILE(ecs_render);
    g_entity_mgr->broadcastEventImmediate(UpdateStageInfoRender(hints, frustum, view_itm, view_tm, proj_tm, cam_pos,
      currentFrameCamera.negRoundedCamPos, currentFrameCamera.negRemainderCamPos, occl, cascade, pState, texCtx));
  }

  ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
}

dynamic_shadow_render::QualityParams WorldRenderer::getShadowRenderQualityParams() const { return lights.getQualityParams(); }

DynamicShadowRenderExtender::Handle WorldRenderer::registerShadowRenderExtension(DynamicShadowRenderExtender::Extension &&extension)
{
  return shadowRenderExtender->registerExtension(eastl::move(extension));
}

void WorldRenderer::renderDynamicsForShadowPass(int cascade, const TMatrix &itm, const Point3 &cam_pos)
{
  G_ASSERT_RETURN(cascade >= RENDER_SHADOWS_CSM, );

  CascadeShadows *csm = shadowsManager.getCascadeShadows();
  if (!csm)
    return;

  TMatrix viewTm = tmatrix(csm->getRenderViewMatrix(cascade));
  TMatrix4 projTm = csm->getRenderProjMatrix(cascade);
  renderDynamicOpaque(cascade, itm, viewTm, projTm, cam_pos);
}

void WorldRenderer::renderRendinst(int cascade, const TMatrix &view_itm)
{
  TIME_D3D_PROFILE(rendinst);

  if (cascade >= RENDER_SHADOWS_CSM)
  {
    CascadeShadows *csm = shadowsManager.getCascadeShadows();

    mainCameraVisibilityMgr.waitVisibility(cascade);
    rendinst::render::before_draw(rendinst::RenderPass::ToShadow, rendinst_shadows_visibility[cascade], csm->getFrustum(cascade),
      nullptr);
    SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
    rendinst::render::renderRIGen(rendinst::RenderPass::ToShadow, rendinst_shadows_visibility[cascade], csm->getShadowViewItm(cascade),
      rendinst::LayerFlag::Opaque |
        (shadowsManager.shouldRenderCsmStatic(cascade) ? rendinst::LayerFlag::NotExtra : rendinst::LayerFlag{}),
      rendinst::OptimizeDepthPass::No);
  }
  else
  {
    if (cascade == RENDER_DYNAMIC_SHADOW)
    {
      SCENE_LAYER_GUARD(rendinstDepthSceneBlockId);
      rendinst::render::renderRIGen(rendinst::RenderPass::Depth, rendinst_dynamic_shadow_visibility, view_itm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No);
    }
    else if (cascade == RENDER_MAIN)
    {
      RiGenVisibility *mainCameraRiMainVisibility = mainCameraVisibilityMgr.getRiMainVisibility();
      mainCameraVisibilityMgr.waitVisibility(RENDER_MAIN);
      rendinst::render::RiExtraRenderer *staticRiExtraRenderer = mainCameraVisibilityMgr.waitAsyncRIGenExtraOpaqueRender();
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, mainCameraRiMainVisibility, view_itm, rendinst::LayerFlag::Opaque,
        rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All, staticRiExtraRenderer, currentTexCtx);
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, mainCameraRiMainVisibility, view_itm, rendinst::LayerFlag::Opaque,
        rendinst::OptimizeDepthPass::No, /*count_multiply*/ 1, rendinst::AtestStage::All, nullptr, currentTexCtx,
        rendinst::RiExtraRenderingSubset::OnlyDynamic);
    }
    else if (cascade == RENDER_CUBE)
    {
      mainCameraVisibilityMgr.waitLightProbeVisibility();
      rendinst::render::renderRIGen(rendinst::RenderPass::Normal, rendinst_cube_visibility, view_itm,
        rendinst::LayerFlag::Opaque | rendinst::LayerFlag::NotExtra, rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::All,
        nullptr, currentTexCtx);
    }
  }
}

void WorldRenderer::renderRITreePrepass(const TMatrix &view_itm)
{
  TIME_D3D_PROFILE(tree_prepass);

  mainCameraVisibilityMgr.waitVisibility(RENDER_MAIN);
  renderFullresRITreePrepass(view_itm);
}

void WorldRenderer::renderFullresRITreePrepass(const TMatrix &view_itm)
{
  ScopeRenderTarget scopeRt;

  d3d::set_render_target({scopeRt.prevRT.getDepth().tex, 0, 0}, DepthAccess::RW, {});

  rendinst::render::renderRITreeDepth(mainCameraVisibilityMgr.getRiMainVisibility(), view_itm);
}


void WorldRenderer::renderRITree()
{
  TIME_D3D_PROFILE(rendinst_tree);
  renderFullresRITree();
}

void WorldRenderer::renderFullresRITree()
{
  rendinst::render::renderRIGen(rendinst::RenderPass::Normal, mainCameraVisibilityMgr.getRiMainVisibility(),
    currentFrameCamera.viewItm, rendinst::LayerFlag::NotExtra,
    prepass.get() ? rendinst::OptimizeDepthPass::Yes : rendinst::OptimizeDepthPass::No, 1, rendinst::AtestStage::NoAtest, nullptr,
    currentTexCtx);
}

void WorldRenderer::setRendinstTesselation()
{
  bool rendinstTesselation = isRendeinstTesselationEnabled();
  ShaderGlobal::set_float(object_tess_factorVarId, rendinstTesselation ? 1.0f : 0.0f);
}

bool WorldRenderer::prepareDelayedRender(const TMatrix &itm)
{
  // If using panorama, wait for it to be properly rendered, otherwise probe rendered on first frame will contain garbage
  bool skiesReady = has_custom_sky() || !get_daskies() || !get_daskies()->panoramaEnabled() || get_daskies()->isPanoramaValid();
  bool tpJobsAdded = false;
  if (specularCubesContainer && skiesReady)
  {
    LightProbeSpecularCubesContainer::CubeUpdater cubeUpdater = [this, &tpJobsAdded](const ManagedTex *tex_ptr, const Point3 &position,
                                                                  const int face_number) {
      TIME_D3D_PROFILE(lightProbesCubePrepare);
      TMatrix4_vec4 glotm = cube->getGlobTmForFace(face_number, position);
      prepareLightProbeRIVisibilityAsync(reinterpret_cast<mat44f_cref>(glotm), position);
      tpJobsAdded = true;
      delayedRenderCtx.lightProbeData.texPtr = tex_ptr;
      delayedRenderCtx.lightProbeData.position = position;
      delayedRenderCtx.lightProbeData.faceNumber = face_number;
      delayedRenderCtx.lightProbeData.render = true;
    };
    FRAME_LAYER_GUARD(-1);
    specularCubesContainer->update(cubeUpdater, itm.getcol(3));
  }
  if (hasFeature(FeatureRenderFlags::STATIC_SHADOWS) && depthAOAboveCtx)
  {
    float minZ, maxZ;
    getMinMaxZ(minZ, maxZ);
    tpJobsAdded = depthAOAboveCtx->prepare(currentFrameCamera.viewItm.getcol(3), minZ, maxZ);
    delayedRenderCtx.depthAOAboveData.itm = itm;
    delayedRenderCtx.depthAOAboveData.render = true;
  }
  return tpJobsAdded;
}

void WorldRenderer::renderDelayedCube()
{
  FRAME_LAYER_GUARD(globalFrameBlockId);
  auto &data = delayedRenderCtx.lightProbeData;
  if (eastl::exchange(data.render, false))
    cube->update(data.texPtr, data.position, data.faceNumber, RenderDynamicCube::RenderMode::WHOLE_SCENE);
}

void WorldRenderer::renderDelayedDepthAbove()
{
  auto &data = delayedRenderCtx.depthAOAboveData;
  if (eastl::exchange(data.render, false))
    depthAOAboveCtx->render(*this, data.itm);
}

void WorldRenderer::reinitCubeIfInvalid()
{
  if (!enviProbeInvalid)
    return;

  uint32_t enviProbeRenderFlags = getEnviProbeRenderFlags();
  uint32_t checkFlags = ENVI_PROBE_SUN_ENABLED | ENVI_PROBE_USE_GEOMETRY;
  bool needeReinitCube = true;
  // In case we use geometry and sun, the shadows must be rendered at the time of probe rendering.
  if ((enviProbeRenderFlags & checkFlags) == checkFlags)
  {
    BBox3 worldBox = getWorldBBox3();
    float checkRadius = length(worldBox.width()) * 0.5f;
    needeReinitCube &= shadowsManager.fullyUpdatedStaticShadows(worldBox.center(), checkRadius);
  }
  // Do not reinit envi probe until skies are not ready, otherwise we will have wrong sky color and missed clouds
  if (auto *skies = get_daskies())
    needeReinitCube &= skies->isCloudsReady();
  if (needeReinitCube)
  {
    reinitCube();
    enviProbeInvalid = false;
    enviProbeNeedsReload = false;
  }
  else
    debug("WR:cube reinit skipped because dependent systems are not ready");
}

static int get_envi_cube_size() { return dgs_get_settings()->getBlockByNameEx("graphics")->getInt("envi_cube_size", 128); }

void WorldRenderer::reinitCube(const Point3 &at)
{
  cubeResolution = get_envi_cube_size();
  reinitCube(cubeResolution, at);
}

void WorldRenderer::reinitSpecularCubesContainerIfNeeded(int cube_size)
{
  if (!specularCubesContainerNeedsReinit)
    return;

  if (specularCubesContainer)
  {
    specularCubesContainer->init(cube_size < 0 ? get_envi_cube_size() : cube_size, TEXFMT_A16B16G16R16F);
    specularCubesContainerNeedsReinit = false;
  }
}

void WorldRenderer::reinitCube(int ew, const Point3 &at)
{
  const float oldSkyFilterTerm = ShaderGlobal::get_float(var::sky_polarization_filter_term);
  ShaderGlobal::set_float(var::sky_polarization_filter_term, 1.0f);

  enviProbePos = at;
  cube->init(ew);
  light_probe::destroy(enviProbe);
  enviProbe = light_probe::create("envi", ew, TEXFMT_A16B16G16R16F);
  ShaderGlobal::set_texture(envi_probe_specularVarId, *light_probe::getManagedTex(enviProbe));
  ShaderGlobal::set_sampler(envi_probe_specular_samplerstateVarId, d3d::request_sampler({}));
  reinitSpecularCubesContainerIfNeeded(ew);
  initIndoorProbesIfNecessary();
  g_entity_mgr->broadcastEventImmediate(RenderReinitCube{});
  beforeDrawPostFx();
  reloadCube(true);

  ShaderGlobal::set_float(var::sky_polarization_filter_term, oldSkyFilterTerm);
}

void WorldRenderer::updateSkyProbeDiffuse()
{
  TIME_D3D_PROFILE(lightProbeDiffuse)
  const Color4 *sphHarm = nullptr;
  eastl::vector<Color4> customEnviProbeHarmonics = get_custom_envi_probe_spherical_hamornics();
  if (!(customEnviProbeHarmonics.size() == 0 || customEnviProbeHarmonics.size() == SphHarmCalc::SPH_COUNT))
  {
    logerr("customEnviProbeHarmonics.size() must be either 0 or %d", SphHarmCalc::SPH_COUNT);
    customEnviProbeHarmonics.resize(SphHarmCalc::SPH_COUNT);
    for (Color4 &c : customEnviProbeHarmonics)
      c = Color4();
  }
  if (customEnviProbeHarmonics.size() == SphHarmCalc::SPH_COUNT)
  {
    sphHarm = customEnviProbeHarmonics.data();
  }
  else if (light_probe::calcDiffuse(enviProbe, NULL, 1, 1, true))
  {
    sphHarm = light_probe::getSphHarm(enviProbe);
    G_ASSERT(SphHarmCalc::SPH_COUNT == sphValues.size());
  }
  if (sphHarm)
  {
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
    {
      sphValues[i] = sphHarm[i];
      ShaderGlobal::set_float4(get_shader_variable_id(String(128, "enviSPH%d", i)), sphValues[i]);
    }

    Point4 normal1(0, 1, 0, 1);

    Point3 intermediate0, intermediate1, intermediate2;

    // sph.w - constant frequency band 0, sph.xyz - linear frequency band 1
    const Point4 *sphHarm4 = reinterpret_cast<const Point4 *>(sphHarm);

    intermediate0.x = sphHarm4[0] * normal1;
    intermediate0.y = sphHarm4[1] * normal1;
    intermediate0.z = sphHarm4[2] * normal1;

    // sph.xyzw and sph6 - quadratic polynomials frequency band 2

    Point4 r1(0, 0, 0, 0);
    r1.w = 3.0 * r1.w - 1;
    intermediate1.x = sphHarm4[3] * r1;
    intermediate1.y = sphHarm4[4] * r1;
    intermediate1.z = sphHarm4[5] * r1;

    float r2 = -1;
    intermediate2 = Point3::xyz(sphHarm4[6]) * r2;

    log_custom_envi_probe_spherical_harmonics(sphHarm);
    debug("sky_up = %@", intermediate0 + intermediate1 + intermediate2);
    attemptsToUpdateSkySph = 0;
  }
  else
  {
    G_ASSERT(0);
    attemptsToUpdateSkySph++;
    const uint32_t MAX_ATTEMPTS_TO_CALC_SKY_SPH_HARM = 5;
    if (attemptsToUpdateSkySph == MAX_ATTEMPTS_TO_CALC_SKY_SPH_HARM)
    {
      logerr("Can't update sky spherical harmonic after %d tries.", MAX_ATTEMPTS_TO_CALC_SKY_SPH_HARM);
      attemptsToUpdateSkySph = 0;
    }
  }
}

void WorldRenderer::reloadCube(bool first)
{
  const uint32_t enviProbeRenderFlags = getEnviProbeRenderFlags();
  const bool sunEnabled = enviProbeRenderFlags & ENVI_PROBE_SUN_ENABLED;
  const bool useGeometry = enviProbeRenderFlags & ENVI_PROBE_USE_GEOMETRY;

  d3d::GpuAutoLock gpu_al;
  static int local_light_probe_texVarId = get_shader_variable_id("local_light_probe_tex", true);
  updateSky(0);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  if (!sunEnabled)
    ShaderGlobal::set_float4(sun_color_0VarId, 0, 0, 0, 0);
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);
  if (first)
  {
    SCOPE_RENDER_TARGET;
    for (int i = 0; i < 6; ++i)
    {
      d3d::set_render_target({}, DepthAccess::RW,
        {{light_probe::getManagedTex(enviProbe)->getCubeTex(), 0, static_cast<uint32_t>(i)}});
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
    }
    for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
      ShaderGlobal::set_float4(get_shader_variable_id(String(128, "enviSPH%d", i)), 1e-9, 1e-9, 1e-9, 1e-9);
    light_probe::update(enviProbe, NULL);
  }
  Point3 origin = enviProbePos;
  if (lmeshMgr && lmeshMgr->getHmapHandler())
  {
    float landHeight;
    if (lmeshMgr->getHmapHandler()->getHeight(Point2::xz(enviProbePos), landHeight, nullptr))
      origin.y = max(enviProbePos.y, landHeight + 1.8f);
  }
  debug("render envi probe at %@", origin);
  {
    TIME_D3D_PROFILE(cubeRender)
    if (!render_custom_envi_probe(light_probe::getManagedTex(enviProbe), -1))
    {
      mainCameraVisibilityMgr.waitLightProbeVisibility(); // Async visibility shouldn't be running here, but wait for safety.
      if (!get_daskies()->isPanoramaValid())
        get_daskies()->temporarilyDisablePanorama(true);
      cube->update(light_probe::getManagedTex(enviProbe), origin, -1,
        useGeometry ? RenderDynamicCube::RenderMode::WHOLE_SCENE : RenderDynamicCube::RenderMode::ONLY_SKY);
      get_daskies()->temporarilyDisablePanorama(false);
    }
  }
  {
    TIME_D3D_PROFILE(lightProbeSpecular)
    light_probe::update(enviProbe, NULL);
  }
  updateSkyProbeDiffuse();
  if (!sunEnabled)
    ShaderGlobal::set_float4(sun_color_0VarId, color4(sun, 0));
  ShaderGlobal::setBlock(globalConstBlockId, ShaderGlobal::LAYER_GLOBAL_CONST);

  ShaderGlobal::set_texture(local_light_probe_texVarId, *light_probe::getManagedTex(enviProbe));
  ShaderGlobal::set_sampler(get_shader_variable_id("local_light_probe_tex_samplerstate", true), d3d::request_sampler({}));
  invalidateGI(true);
}

void WorldRenderer::prepareLightProbeRIVisibility(const mat44f &globtm, const Point3 &view_pos)
{
  TIME_PROFILE(prepareLightProbeRIVisibility);

  const float MIN_HEIGHT_IN_PIXELS = 10.f;
  const vec4f threshold = v_div_x(v_set_x(MIN_HEIGHT_IN_PIXELS), v_cvt_vec4f(v_seti_x(cubeResolution)));
  const vec4f camPosAndThresSq = v_perm_xyzd(v_ldu(&view_pos.x), v_rot_1(v_mul_x(threshold, threshold)));
  const auto angleSizeFilter = [=](vec4f bbmin, vec4f bbmax) {
    bbox3f bbox = {bbmin, bbmax};
    const vec4f to_box = v_length3_sq_x(v_sub(v_bbox3_center(bbox), camPosAndThresSq));
    const vec4f dia = v_bbox3_inner_diameter(bbox);
    return v_test_vec_x_gt(v_mul_x(dia, dia), v_mul_x(to_box, v_perm_wxyz(camPosAndThresSq))) != 0;
  };

  rendinst::prepareRIGenExtraVisibility(globtm, view_pos, *rendinst_cube_visibility, false, NULL, {},
    rendinst::RiExtraCullIntention::MAIN, false, false, false, angleSizeFilter);
  rendinst::prepareRIGenVisibility(Frustum(globtm), view_pos, rendinst_cube_visibility, false, NULL);
}

void WorldRenderer::renderLightProbeOpaque(const Point3 &view_pos, const TMatrix &view_itm, const Frustum &culling_frustum)
{
  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  renderStaticSceneOpaque(RENDER_CUBE, view_pos, view_itm, culling_frustum);
  //
  if (lmeshMgr)
  {
    lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_REFLECTION);
    lmeshRenderer->setUseHmapTankSubDiv(0);
    lmeshRenderer->render(*lmeshMgr, LandMeshRenderer::RENDER_REFLECTION, view_pos); // no async culling
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
  }
  ShaderGlobal::set_texture(ssao_texVarId, BAD_TEXTUREID);
}

void WorldRenderer::renderLightProbeEnvi(const TMatrix &view, const TMatrix4 &proj, const Driver3dPerspective &persp)
{
  TIME_D3D_PROFILE(cubeenvi)

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
  if (!get_daskies())
    return;
  TMatrix itm;
  itm = orthonormalized_inverse(view);
  if (DAGOR_LIKELY(!try_render_custom_sky(view, proj, persp)))
  {
    get_daskies()->renderEnvi(sky_is_unreachable, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 2, UniqueTex{}, UniqueTex{},
      nullptr, // fixme: it should be with cube depth buffer
      cube_pov_data, view, proj, persp, UpdateSky::Off, true, probes_sky_prepare_altitude_tolerance.get());
  }
}

void WorldRenderer::renderLmeshReflection(const LandMeshCullingData &lmesh_refl_culling_data)
{
  TIME_D3D_PROFILE(landmesh_reflection)
  if (!lmeshMgr)
    return;

  ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  lmeshRenderer->setLMeshRenderingMode(LMeshRenderingMode::RENDERING_REFLECTION);
  lmeshRenderer->setUseHmapTankSubDiv(0);
  lmeshRenderer->forceLowQuality(true);
  lmeshRenderer->renderCulled(*lmeshMgr, LandMeshRenderer::RENDER_REFLECTION, lmesh_refl_culling_data, ::grs_cur_view.pos);
  lmeshRenderer->forceLowQuality(false);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
}

void WorldRenderer::updateHeroData()
{
  QueryHeroWtmAndBoxForRender heroDataQuery{};
  if (const ecs::EntityId heroEid = game::get_controlled_hero())
    if (g_entity_mgr->getEntityTemplateId(heroEid) != ecs::INVALID_TEMPLATE_INDEX)
      g_entity_mgr->sendEventImmediate(heroEid, heroDataQuery);
  heroData = heroDataQuery;
}

Occlusion *WorldRenderer::getMainCameraOcclusion()
{
  return camera_in_camera::is_lens_render_active() ? nullptr : mainCameraVisibilityMgr.getOcclusion();
}

Occlusion *get_main_occlusion_safe()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  return wr ? wr->getMainCameraOcclusion() : nullptr;
}

void WorldRenderer::renderWaterSSR(const TMatrix &itm, const Driver3dPerspective &persp)
{
  if (!water)
    return;

  TIME_D3D_PROFILE(reflection)
  fft_water::render(water, itm.getcol(3), shoreRenderer.getDistanceFieldTexId(), currentFrameCamera.noJitterFrustum, persp,
    fft_water::GEOM_NORMAL, water_ssr_id, nullptr, fft_water::RenderMode::WATER_SSR_SHADER);
}

static ShaderVariableInfo far_water_transparencyVarId("far_water_transparency", true);
static ShaderVariableInfo water_fresnel_reflectanceVarId("water_fresnel_reflectance", true);

void WorldRenderer::renderWater(const CameraParams &camera, DistantWater render_distant_water, bool render_ssr)
{
  if (!water || is_water_hidden())
    return;

  TIME_D3D_PROFILE(water);
  if (render_ssr)
    d3d::begin_conditional_render(water_ssr_id);
  ShaderGlobal::set_int(far_water_transparencyVarId, 0);
  ShaderGlobal::set_float4(water_fresnel_reflectanceVarId, Color4(0.02, 0.2, 0.5, 0)); // TODO: make it configurable
  fft_water::render(water, camera.viewItm.getcol(3), shoreRenderer.getDistanceFieldTexId(), camera.noJitterFrustum, camera.jitterPersp,
    fft_water::GEOM_NORMAL);
  if (render_ssr)
    d3d::end_conditional_render(water_ssr_id);

  if (render_distant_water == DistantWater::Yes)
  {
    TIME_D3D_PROFILE(distant_water);
    FRAME_LAYER_GUARD(water3dBlockId);

    d3d::set_vs_constbuffer_register_count(522);
    waterDistant[fft_water::RenderMode::WATER_DEPTH_SHADER].render(); // depth prepass
    waterDistant[fft_water::RenderMode::WATER_SHADER].render();       // color pass
    d3d::set_vs_constbuffer_register_count(0);
  }
}

bool WorldRenderer::shouldToggleVRS(const AimRenderingData &aim_data)
{
  static ShaderVariableInfo motion_vrs_strength("motion_vrs_strength", true);
  if (::dgs_get_game_params()->getBool("forceDisableVrs", false)) // TODO: remove it after VRS is stable
    vrsEnable = false;
  if (render::antialiasing::is_ray_reconstruction_enabled())
    return false;
  const bool motionVrs = can_use_motion_vrs() && motion_vrs_strength && motion_vrs_strength.get_float() > 0;
  const bool dofVrs = aim_data.farDofEnabled && vrs_dof;
  return (motionVrs || dofVrs) && d3d::get_driver_desc().caps.hasVariableRateShadingTexture && vrsEnable;
}

void WorldRenderer::prerunFx()
{
  const float prerunDt = 0.05;
  const float prerunFxTime = dgs_get_settings()->getBlockByNameEx("prerun")->getReal("fxPrerun", 10.f);

  // NOTE: this function is only supposed to work with CPU particles.
  // Textures are reset to be explicit about dependencies of finish_update.
  acesfx::setDepthTex(nullptr);
  acesfx::setNormalsTex(nullptr);

  for (int i = 0, n = safediv(prerunFxTime, prerunDt); i < n; i++)
  {
    acesfx::update_fx_managers(prerunDt);
    acesfx::flush_dafx_commands();
    acesfx::start_dafx_update(prerunDt);
    acesfx::finish_update(TMatrix4::IDENT, getMainCameraOcclusion());
  }
}

void WorldRenderer::setFxQuality()
{
  acesfx::wait_fx_managers_update_and_allow_accum_cmds();
  FxResolutionSetting fxRes = acesfx::getFxResolutionSetting();
  fxRtOverride = FX_RT_OVERRIDE_DEFAULT; // no forced low res due to sparks
  if (fxRes == FX_HIGH_RESOLUTION)
    fxRtOverride = FX_RT_OVERRIDE_HIGHRES;

  acesfx::set_rt_override(fxRtOverride);

  FxQuality fxQuality = acesfx::getFxQualityMask();

  acesfx::set_quality_mask(fxQuality);

  requestFgRecreation("setFxQuality");
  g_entity_mgr->broadcastEventImmediate(SetFxQuality(fxQuality));
}

void WorldRenderer::setEnviromentDetailsQuality()
{
  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");

  const char *environmentDetailQualityStr = graphics->getStr("environmentDetailsQuality", "low");
  if (stricmp(environmentDetailQualityStr, "low") == 0)
  {
    ShaderGlobal::set_int(var::layered_material_detail_quality, 0);
    g_entity_mgr->broadcastEventImmediate(SetGrassSdfEareaser{false});
  }
  else if (stricmp(environmentDetailQualityStr, "medium") == 0)
  {
    ShaderGlobal::set_int(var::layered_material_detail_quality, 0);
    g_entity_mgr->broadcastEventImmediate(SetGrassSdfEareaser{false});
  }
  else if (stricmp(environmentDetailQualityStr, "high") == 0)
  {
    ShaderGlobal::set_int(var::layered_material_detail_quality, 1);
    g_entity_mgr->broadcastEventImmediate(SetGrassSdfEareaser{true});
  }
  else
  {
    logerr("unknown environment detail quality <%s>", environmentDetailQualityStr);
  }
}

void WorldRenderer::setNbsQuality()
{
  const DataBlock *graphics = ::dgs_get_settings()->getBlockByNameEx("graphics");
  const char *environmentDetailQualityStr = graphics->getStr("environmentDetailsQuality", "low");

  NodeBasedShaderQuality nbsQuality = NodeBasedShaderQuality::Low;

  if (stricmp(environmentDetailQualityStr, "medium") == 0)
    nbsQuality = NodeBasedShaderQuality::Medium;
  else if (stricmp(environmentDetailQualityStr, "high") == 0)
    nbsQuality = NodeBasedShaderQuality::High;
  else if (stricmp(environmentDetailQualityStr, "low") != 0)
    logerr("unknown environment detail quality <%s>", environmentDetailQualityStr);

  if (enviCover)
    enviCover->setNbsQuality(nbsQuality);

  if (volumeLight)
    volumeLight->setNbsQuality(nbsQuality);
}

void WorldRenderer::copyClipmapUAVFeedback()
{
  if (clipmap)
    clipmap->copyUAVFeedback();
}

String WorldRenderer::showTexCommand(const char *argv[], int argc)
{
  if (debug_tex_overlay)
    return debug_tex_overlay->processConsoleCmd(argv, argc);
  return String(64, "no debug_tex_overlay");
}

String WorldRenderer::showTonemapCommand(const char *argv[], int argc)
{
  if (debug_tonemap_overlay)
    return debug_tonemap_overlay->processConsoleCmd(argv, argc);
  return String(64, "no debug_tonemap_overlay");
}

void WorldRenderer::createDebugBoxRender()
{
  eastl::vector<const char *> boxesGroupNames = {"indoor_walls", "gi_windows", "envi_probes", "restriction_boxes"};
  eastl::vector<E3DCOLOR> groupColors = {E3DCOLOR(200, 0, 0), E3DCOLOR(200, 100, 0), E3DCOLOR(100, 255, 100), E3DCOLOR(0, 255, 100)};
  eastl::vector<const scene::TiledScene *> groupScenes = {
    getWallsScene(), getWindowsScene(), getEnviProbeBoxesScene(), getRestrictionBoxScene()};
  debugBoxRenderer =
    eastl::make_unique<DebugBoxRenderer>(eastl::move(boxesGroupNames), eastl::move(groupScenes), eastl::move(groupColors));
}

String WorldRenderer::showBoxesCommand(const char *argv[], int argc)
{
  if (!debugBoxRenderer)
    createDebugBoxRender();

  return debugBoxRenderer->processCommand(argv, argc);
}

void WorldRenderer::setRIVerifyDistance(float distance)
{
  if (!debugBoxRenderer)
    createDebugBoxRender();
  debugBoxRenderer->verifyRIDistance = distance;
  debugBoxRenderer->needLogText = true;
}

const scene::TiledScene *WorldRenderer::getRestrictionBoxScene() const { return restrictionBoxesScene.get(); }

const scene::TiledScene *WorldRenderer::getEnviProbeBoxesScene() const
{
  if (indoorProbeMgr)
    return indoorProbeMgr->getEnviProbeBoxes();
  if (indoorProbesScene)
    return indoorProbesScene->getScene(ECS_HASH("envi_probe_box").hash);
  return nullptr;
}

void WorldRenderer::invalidateLightProbes()
{
  if (indoorProbeMgr)
    indoorProbeMgr->invalidate();
}

void WorldRenderer::invalidateVolumeLight()
{
  if (volumeLight)
  {
    TIME_D3D_PROFILE(volume_light_invalidate);
    volumeLight->invalidate();
  }
}

static void close_acquired_texture(TEXTUREID &id)
{
  if (id == BAD_TEXTUREID)
    return;
  ShaderGlobal::reset_from_vars(id);
  release_managed_tex(id);
  id = BAD_TEXTUREID;
}

extern TEXTUREID load_land_micro_details(const DataBlock &micro);
extern void close_land_micro_details(TEXTUREID &micro);

void WorldRenderer::closeLandMicroDetails() { close_land_micro_details(landMicrodetailsId); }

void WorldRenderer::loadLandMicroDetails(const DataBlock *micro)
{
  closeLandMicroDetails();
  if (!micro)
    return;
  landMicrodetailsId = load_land_micro_details(*micro);
}

void WorldRenderer::closeCharacterMicroDetails() { close_acquired_texture(characterMicrodetailsId); }

extern TEXTUREID load_texture_array_immediate(const char *name, const char *param_name, const DataBlock &blk, int &count);

void WorldRenderer::loadCharacterMicroDetails(const DataBlock *micro)
{
  closeCharacterMicroDetails();

  bool useMicrodetails = hasFeature(FeatureRenderFlags::MICRODETAILS);

  if (!micro || !useMicrodetails)
    return;

  int microDetailCount = 0;
  characterMicrodetailsId = load_texture_array_immediate("character_micro_details*", "micro_detail", *micro, microDetailCount);
  ShaderGlobal::set_texture(get_shader_variable_id("character_micro_details"), characterMicrodetailsId);
  {
    d3d::SamplerInfo smpInfo;
    smpInfo.anisotropic_max = ::dgs_tex_anisotropy;
    ShaderGlobal::set_sampler(get_shader_variable_id("character_micro_details_samplerstate"), d3d::request_sampler(smpInfo));
  }
  ShaderGlobal::set_int(get_shader_variable_id("character_micro_details_count", true), microDetailCount);
}

void WorldRenderer::invalidateClipmap(bool force)
{
  if (!lmeshMgr || !clipmap)
    return;
  clipmap->invalidate(force);
}

void WorldRenderer::dumpClipmap()
{
  if (!lmeshMgr || !clipmap)
    return;
  clipmap->dump();
}

void WorldRenderer::prepareLastClip()
{
  if (!lmeshMgr)
    return;
  LandMeshData data;
  data.lmeshMgr = lmeshMgr;
  data.lmeshRenderer = lmeshRenderer;
  data.texture_size = ::dgs_get_settings()->getBlockByNameEx("clipmap")->getInt("lastClipTexSize", 2048);
  data.use_dxt = true;
  data.global_frame_id = globalFrameBlockId;
  data.flipCullStateId = flipCullStateId.get();
  data.land_mesh_prepare_clipmap_blockid = landMeshPrepareClipmapBlockId;
  // ShaderGlobal::setBlock(globalFrameBlockId, ShaderGlobal::LAYER_FRAME);
  prepare_fixed_clip(last_clip, last_clip_sampler, data, false, ::grs_cur_view.pos);
  last_clip.setVar();
}

void WorldRenderer::closeDistantHeightmap()
{
  distantHeightmapTex.close(); // to force reinit distant hmap
  static constexpr float INF_DIST = 10000000;
  ShaderGlobal::set_float4(distant_heightmap_target_boxVarId, Color4(0, 0, INF_DIST, INF_DIST));
  // distant heightmap is never sampled this way
}
void WorldRenderer::updateDistantHeightmap()
{
  bool hasDistantHeightmap = distantHeightmapTex.getTex2D();
  bool needsDistantHeightmap = volumeLight && volumeLight->isDistantFogEnabled();

  if (hasDistantHeightmap == needsDistantHeightmap)
    return;

  if (!needsDistantHeightmap)
  {
    closeDistantHeightmap();
    return;
  }

  if (!lmeshRenderer || !lmeshMgr || !lmeshMgr->getHmapHandler())
    return;

  static constexpr int DISTANT_HEIGHTMAP_RES = 1024;
  static constexpr float METER_PER_TEXEL = 16; // the same detail as hmap with 4K hmap res, 1 km^2 area, and LOD_6 hmap sampling in NBS
  float distantHeightmapRange = DISTANT_HEIGHTMAP_RES * METER_PER_TEXEL / 2;

  distantHeightmapTex.close();
  distantHeightmapTex =
    dag::create_tex(nullptr, DISTANT_HEIGHTMAP_RES, DISTANT_HEIGHTMAP_RES, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "distant_heightmap_tex");
  if (!distantHeightmapTex)
  {
    logerr("can't create distant heightmap texture");
    return;
  }

  BBox3 landmeshBBox = lmeshMgr->getBBox();
  float scale = landmeshBBox.width().y;
  float offset = landmeshBBox.boxMin().y;
  ShaderGlobal::set_float4(get_shader_variable_id("heightmap_min_max", true), Color4(1.0f / scale, -offset / scale, scale, offset));

  BBox3 hmapBBox = lmeshMgr->getHmapHandler()->getWorldBox();
  Point2 origin = Point2::xz(hmapBBox.center());
  Point2 extent = Point2(distantHeightmapRange, distantHeightmapRange);
  BBox2 targetBox = BBox2(origin - extent, origin + extent);

  Point4 world_to_heightmap;
  const float water_level = water ? fft_water::get_level(water) : HeightmapHeightCulling::NO_WATER_ON_LEVEL;
  render_landmesh_to_heightmap(distantHeightmapTex.getTex2D(), distantHeightmapRange, origin, &targetBox, water_level,
    world_to_heightmap);
  d3d::resource_barrier({distantHeightmapTex.getTex2D(), RB_RO_SRV | RB_STAGE_PIXEL, 0, 0});

  Point2 worldPosOfs = targetBox.getMin();
  Point2 worldSize = targetBox.size();
  ShaderGlobal::set_float4(distant_world_to_hmap_lowVarId,
    Color4(1.0f / worldSize.x, 1.0f / worldSize.y, -worldPosOfs.x / worldSize.x, -worldPosOfs.y / worldSize.y));

  ShaderGlobal::set_float4(distant_heightmap_scaleVarId, Color4(scale, offset, 0, 0));

  ShaderGlobal::set_float4(distant_heightmap_target_boxVarId,
    Color4(hmapBBox.center().x, hmapBBox.center().z, hmapBBox.width().x / 2, hmapBBox.width().z / 2));
}

void WorldRenderer::renderWaterHeightmapLowres()
{
  Color4 underwater_fade = ShaderGlobal::get_float4(underwater_fadeVarId);
  bool needsHeightmap = needs_water_heightmap(underwater_fade);
  needsHeightmap = needsHeightmap && water && fft_water::get_heightmap(water);
  if (needsHeightmap && !lowresWaterHeightmap)
  {
    lowresWaterHeightmap = dag::create_tex(nullptr, lowresWaterHeightmapSize, lowresWaterHeightmapSize, TEXCF_RTARGET | TEXFMT_R16F, 1,
      "water_heightmap_lowres");
    ShaderGlobal::set_sampler(get_shader_variable_id("water_heightmap_lowres_samplerstate", true), d3d::request_sampler({}));
    lowresWaterHeightmapRenderer.init("water_heightmap_lowres");
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({}, DepthAccess::RW, {{lowresWaterHeightmap.getTex2D(), 0, 0}});
    lowresWaterHeightmapRenderer.render();
  }
  else if (!needsHeightmap && lowresWaterHeightmap)
    lowresWaterHeightmap.close();
}

BBox3 WorldRenderer::getWorldBBox3() const
{
  BBox3 result = [this]() {
    if (!worldBBox.isempty())
      return worldBBox;
    BBox3 b;
    if (!binSceneBbox.isempty())
      b += binSceneBbox;
    if (lmeshMgr)
      b += lmeshMgr->getBBoxWithHMapWBBox();
    return b;
  }();
  result += additionalBBox;
  return result;
}

void WorldRenderer::getMinMaxZ(float &minHt, float &maxHt) const
{
  BBox3 worldBox = getWorldBBox3();
  if (worldBox[0].y > worldBox[1].y)
  {
    minHt = -5;
    maxHt = 100;
  }
  else
  {
    minHt = worldBox[0].y - 1;
    maxHt = worldBox[1].y + 10;
  }
}

int WorldRenderer::getTemporalShadowFramesCount() const
{
  if (!screencap::is_screenshot_scheduled())
  {
    if (antiAliasing)
    {
      int taaFrameCount = antiAliasing->getTemporalFrameCount();
      if (taaFrameCount > 0)
        return taaFrameCount;
    }
    return 1;
  }
  // Count how many samples will be taken to avoid repeating dithering pattern
  return subPixels * subPixels;
}

int WorldRenderer::getShadowFramesCount() const
{
  if (!screencap::is_screenshot_scheduled())
  {
    if (antiAliasing)
    {
      int taaFrameCount = antiAliasing->getTemporalFrameCount();
      if (taaFrameCount > 0)
        return taaFrameCount;
    }
    return 8;
  }
  // Count how many samples will be taken to avoid repeating dithering pattern
  return subPixels * subPixels;
}

bool WorldRenderer::isTimeDynamic() const { return timeDynamic || dynamic_time_scale != 0; }

void WorldRenderer::initIndoorProbeShapes(eastl::unique_ptr<IndoorProbeScenes> &&scene_ptr)
{
  indoorProbesScene = eastl::move(scene_ptr);
  initIndoorProbesIfNecessary();
  set_up_omni_lights();
  g_entity_mgr->broadcastEvent(UpdateEffectRestrictionBoxes());
}

void WorldRenderer::initRestrictionBoxes(eastl::unique_ptr<scene::TiledScene> &&boxes) { restrictionBoxesScene = eastl::move(boxes); }

bool WorldRenderer::getBoxAround(const Point3 &position, TMatrix &box) const
{
  TMatrix newBox;
  newBox.setcol(0, Point3{0, 0, 0});
  newBox.setcol(1, Point3{0, 0, 0});
  newBox.setcol(2, Point3{0, 0, 0});
  newBox.setcol(3, position);
  vec3f lightPos = v_make_vec4f(newBox.getcol(3).x, newBox.getcol(3).y, newBox.getcol(3).z, 0);
  bbox3f boundingBox;
  boundingBox.bmin = v_sub(lightPos, v_splats(0.0001f));
  boundingBox.bmax = v_add(lightPos, v_splats(0.0001f));
  float best = MAX_REAL;
  auto storeVisibleBox = [&newBox, &lightPos, &best](scene::node_index, mat44f_cref m) {
    // pick box where the light is closest to an axis
    mat44f invMat;
    v_mat44_inverse43(invMat, m);
    vec3f localPos = v_abs(v_mat44_mul_vec3p(invMat, lightPos));
    if (!v_check_xyz_all_true(v_cmp_gt(V_C_HALF, localPos)))
      return;
    vec4f vDist = v_min(localPos, v_min(v_splat_y(localPos), v_splat_z(localPos))); // result is in the x component
    float value = v_extract_x(vDist);

    // pick smallest box
    // vec3f vSize = v_mul(v_dot3(m.col0, m.col0), v_mul(v_dot3(m.col1, m.col1), v_dot3(m.col2, m.col2)));
    // float value = v_extract_x(vSize);

    if (value < best)
    {
      best = value;
      v_mat_43cu_from_mat44(newBox.m[0], m);
    }
  };
  if (getRestrictionBoxScene())
    getRestrictionBoxScene()->boxCull<false, false>(boundingBox, 0, 0, storeVisibleBox);
  if (getEnviProbeBoxesScene() && best == MAX_REAL)
    getEnviProbeBoxesScene()->boxCull<false, false>(boundingBox, 0, 0, storeVisibleBox);
  if (getWallsScene() && best == MAX_REAL)
    getWallsScene()->boxCull<false, false>(boundingBox, 0, 0, storeVisibleBox);
  bool found = best < MAX_REAL;
  if (found)
    box = newBox;
  return found;
}

void WorldRenderer::enableVolumeFogOptionalShader(const String &shader_name, bool enable)
{
  if (volumeLight)
    volumeLight->enableOptionalShader(shader_name, enable);
}

void WorldRenderer::enableEnviCoverOptionalShader(const String &shader_name, bool enable)
{
  if (enviCover)
    enviCover->enableOptionalShader(shader_name, enable);
}

bool WorldRenderer::isThinGBuffer() const
{
  const bool thinGBuffer = !hasFeature(FeatureRenderFlags::FULL_DEFERRED);
  return thinGBuffer;
}

bool WorldRenderer::isSSREnabled() const
{
  return ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("ssr_enabled", true) && hasFeature(FeatureRenderFlags::SSR) &&
         ssr_enable.get();
}

bool WorldRenderer::isWaterSSREnabled() const { return waterReflectionEnabled && hasFeature(FeatureRenderFlags::WATER_REFLECTION); }

bool WorldRenderer::isLowResLUT() const { return ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("low_res_lut", false); }

WorldRenderer::GiQuality WorldRenderer::getGiQuality() const
{
  auto getGiAlgorithm = [](const char *a, GiAlgorithm def = GiAlgorithm::MEDIUM) -> GiAlgorithm {
    if (!a)
      return def;
    eastl::string_view algorithm = eastl::string_view(a);
#if DAGOR_DBGLEVEL > 0
    if (algorithm == eastl::string_view("off"))
      return GiAlgorithm::OFF;
#endif
    if (algorithm == eastl::string_view("low"))
      return GiAlgorithm::LOW;
    if (algorithm == eastl::string_view("medium"))
      return GiAlgorithm::MEDIUM;
    if (algorithm == eastl::string_view("high"))
      return GiAlgorithm::HIGH;
    logwarn("Invalid GI quality <%s>", a);
    return GiAlgorithm::LOW;
  };

  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  GiAlgorithm settingsAlgorithm = getGiAlgorithm(graphicsBlk->getStr("giAlgorithm", NULL));
  GiAlgorithm algorithm =
    getGiAlgorithm(getLevelSettings() ? getLevelSettings()->getStr("giAlgorithm", NULL) : NULL, settingsAlgorithm);
  const float settingsAlgorithmQuality = graphicsBlk->getReal("giAlgorithmQuality", 1.0);
  float algorithmQuality =
    getLevelSettings() ? getLevelSettings()->getReal("giAlgorithmQuality", settingsAlgorithmQuality) : settingsAlgorithmQuality;
  if (int(algorithm) < int(settingsAlgorithm))
  {
    algorithm = settingsAlgorithm;
    algorithmQuality = settingsAlgorithmQuality;
  }
  else if (algorithm == settingsAlgorithm)
    algorithmQuality = max(algorithmQuality, settingsAlgorithmQuality);

  if (algorithm == GiAlgorithm::LOW || bareMinimumPreset || is_only_low_gi_supported())
    return {GiAlgorithm::LOW, is_only_low_gi_supported() ? 0 : algorithmQuality};
  return {algorithm, algorithmQuality};
}

void WorldRenderer::changePreset()
{
#if DAGOR_DBGLEVEL > 0
  const char *preset = ::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("preset", "medium");
  const bool isMediumPreset = strcmp(preset, "medium") == 0;
  // This shader interval is only assumed in _fast compile configs, so it's a good indicator.
  // If this changes in the future and this assert triggers false, positives, tested variable should be changed.
  const bool usesFastCompiledShaders = ShaderGlobal::is_var_assumed(antialiasing_typeVarId);
  // Fast compiled shaders are only usable with the medium preset.
  // If someone tries to modify graphics settings without compile fully shader configs, an error message can save a lot of headache
  G_ASSERTF(isMediumPreset || !usesFastCompiledShaders, "Fast compiled shaders only work with medium preset.\n"
                                                        "Based on shader interval assumptions, it looks like you have used the fast "
                                                        "compilation config. This is only compatible with the medium preset.");
#endif
#if !_TARGET_PC && !_TARGET_ANDROID && !_TARGET_IOS && !_TARGET_C3
  IPoint2 displayResolution;
  getDisplayResolution(displayResolution.x, displayResolution.y);
  fixedPostfxResolution = getFixedPostfxResolution(displayResolution);
#endif
  defaultFeatureRenderFlags = getPresetFeatures();
  changeFeatures(getOverridenRenderFeatures());
  onBareMinimumSettingsChanged();
}

FeatureRenderFlagMask WorldRenderer::getOverridenRenderFeatures()
{
  ecs::EntityId level_eid = ::get_current_level_eid();
  if (g_entity_mgr->getOr(level_eid, ECS_HASH("level__renderFeaturesOverrideFromLevelBlk"), false))
  {
    return load_render_features(originalLevelBlk.getBlockByNameEx("renderFeatures"));
  }
  else if (
    const ecs::Object *overrides = g_entity_mgr->getNullable<ecs::Object>(level_eid, ECS_HASH("level__renderFeaturesOverrides")))
  {
    if (!overrides->empty())
    {
      return apply_render_features_override(defaultFeatureRenderFlags, *overrides);
    }
  }

  return defaultFeatureRenderFlags;
}

void WorldRenderer::onBareMinimumSettingsChanged()
{
  bool bare_minimum = false;
#if _TARGET_PC || _TARGET_APPLE
  bare_minimum = strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("preset", "medium"), "bareMinimum") == 0;
#elif _TARGET_XBOXONE || _TARGET_C1
  bare_minimum = strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("consolePreset", "HighFPS"), "bareMinimum") == 0;
#endif
  if (bare_minimum == bareMinimumPreset)
    return;
  bareMinimumPreset = bare_minimum;
  setWater(water);
  setEnviromentDetailsQuality();
  setGIQualityFromSettings();
  resetSSAOImpl();
  initTarget();
  hide_heightmap_on_planes.set(bareMinimumPreset);
}

FeatureRenderFlagMask WorldRenderer::getPresetFeatures()
{
  const DataBlock *featuresBlk = ::dgs_get_settings()->getBlockByNameEx("renderFeatures");
  FeatureRenderFlagMask presetFeatures = load_render_features(featuresBlk);

#if _TARGET_PC || _TARGET_APPLE || _TARGET_XBOXONE || _TARGET_C1
#if _TARGET_PC || _TARGET_APPLE
  if (strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("preset", "medium"), "bareMinimum") == 0)
#else
  if (strcmp(::dgs_get_settings()->getBlockByNameEx("graphics")->getStr("consolePreset", "HighFPS"), "bareMinimum") == 0)
#endif
  {
    presetFeatures.reset();
    presetFeatures.set(FeatureRenderFlags::DECALS)
      .set(FeatureRenderFlags::VOLUME_LIGHTS)
      .set(FeatureRenderFlags::ADAPTATION)
      .set(FeatureRenderFlags::POSTFX)
      .set(FeatureRenderFlags::SSAO)
      .set(FeatureRenderFlags::STATIC_SHADOWS);
  }
#endif

  return presetFeatures;
}

void WorldRenderer::setFeatureFromSettings(const FeatureRenderFlags f)
{
  const bool enabled = load_render_feature(::dgs_get_settings()->getBlockByNameEx("renderFeatures"), f);
  toggleFeatures(FeatureRenderFlagMask{}.set(f), enabled);
}

void WorldRenderer::toggleFeatures(const FeatureRenderFlagMask &f, bool turn_on)
{
  FeatureRenderFlagMask featureRenderFlags = get_current_render_features();
  FeatureRenderFlagMask newFeatures = turn_on ? featureRenderFlags | f : featureRenderFlags & ~f;
  changeFeatures(newFeatures);
}

void WorldRenderer::validateFeatures(FeatureRenderFlagMask &features, const FeatureRenderFlagMask &)
{
  if (features.test(FeatureRenderFlags::SSR) && !features.test(FeatureRenderFlags::FULL_DEFERRED))
  {
    logerr("SSR currently not implemented for thinGBuffer");
    features.flip(FeatureRenderFlags::SSR);
  }
  // TODO: detect that other features toggling is not supported in special cases and disable them
}

void WorldRenderer::changeFeatures(const FeatureRenderFlagMask &f)
{
  FeatureRenderFlagMask oldFeatures = get_current_render_features();
  FeatureRenderFlagMask featureRenderFlags = f;
  validateFeatures(featureRenderFlags, oldFeatures ^ featureRenderFlags);

  const FeatureRenderFlagMask &newFeatures = featureRenderFlags;
  FeatureRenderFlagMask changedFeatures = oldFeatures ^ newFeatures;
  if (changedFeatures.none())
    return;

  set_current_render_features(featureRenderFlags);

  dafg::invalidate_history();

  debug("Changing featureRenderFlags: enable: %s disable: %s", render_features_to_string((changedFeatures & newFeatures)),
    render_features_to_string((changedFeatures & ~newFeatures)));

  bool reapplySettings = false;
  bool recreateResettable = false;
  bool recreateFramegraph = false;

  // most following methods need GPU acquire
  d3d::GpuAutoLock gpu_lock;

  if (changedFeatures.test(FeatureRenderFlags::STATIC_SHADOWS))
  {
    if (hasFeature(FeatureRenderFlags::STATIC_SHADOWS))
      shadowsManager.initStaticShadow();
    else
      shadowsManager.closeStaticShadow();
  }

  if (changedFeatures.test(FeatureRenderFlags::FULL_DEFERRED))
  {
    const char *sh_bindump_prefix = hasFeature(FeatureRenderFlags::FULL_DEFERRED)
                                      ? ::dgs_get_settings()->getStr("fullFeaturedShaders", "compiledShaders/game")
                                      : ::dgs_get_settings()->getStr("compatShaders", "compiledShaders/compatPC/game");
    rebuild_shaders_stateblocks();
    load_shaders_bindump_with_fence(sh_bindump_prefix, 5.0_sm);
    rendinst::render::reinitOnShadersReload();
    recreateResettable |= true;
    reapplySettings |= true;
  }

  if (changedFeatures.test(FeatureRenderFlags::CLUSTERED_LIGHTS) || changedFeatures.test(FeatureRenderFlags::DYNAMIC_LIGHTS_SHADOWS))
  {
    dynamic_lights.set(hasFeature(FeatureRenderFlags::CLUSTERED_LIGHTS));
    lights.close();
    if (dynamic_lights.get())
    {
      lights.init(::dgs_get_game_params()->getInt("lightsCount", dynamic_lights_initial_count.get()), getDynamicShadowQuality(),
        hasFeature(TILED_LIGHTS));
      shadowsManager.changeSpotShadowBiasBasedOnQuality();
    }
    reapplySettings |= true;
  }

  if (changedFeatures.test(FeatureRenderFlags::TILED_LIGHTS))
  {
    lights.toggleTiledLights(hasFeature(FeatureRenderFlags::TILED_LIGHTS));
    reapplySettings |= true;
  }

  if (changedFeatures.test(FeatureRenderFlags::VOLUME_LIGHTS))
  {
    resetVolumeLights();
    loadFogNodes(root_fog_graph, volumeLightLowRange, volumeLightHighRange, volumeLightLowHeight, volumeLightHighHeight);
    reapplySettings |= true;
  }

  if (changedFeatures.test(FeatureRenderFlags::MICRODETAILS))
    loadCharacterMicroDetails(::dgs_get_game_params()->getBlockByName("character_micro_details"));

  if (changedFeatures.test(FeatureRenderFlags::SPECULAR_CUBES) || changedFeatures.test(FeatureRenderFlags::BLOOM))
  {
    recreateResettable |= true;
    reapplySettings |= true;
  }

  if (changedFeatures.test(FeatureRenderFlags::DEFERRED_LIGHT) || changedFeatures.test(FeatureRenderFlags::UPSCALE_SAMPLING_TEX) ||
      changedFeatures.test(FeatureRenderFlags::COMBINED_SHADOWS) || changedFeatures.test(FeatureRenderFlags::PREV_OPAQUE_TEX) ||
      changedFeatures.test(FeatureRenderFlags::SSAO) || changedFeatures.test(FeatureRenderFlags::SSR) ||
      changedFeatures.test(FeatureRenderFlags::DOWNSAMPLED_NORMALS) || changedFeatures.test(FeatureRenderFlags::WATER_REFLECTION) ||
      changedFeatures.test(FeatureRenderFlags::DOWNSAMPLED_SHADOWS) || changedFeatures.test(FeatureRenderFlags::POSTFX))
    reapplySettings |= true;

  if (changedFeatures.test(FeatureRenderFlags::CAMERA_IN_CAMERA))
  {
    recreateResettable |= true;
    camera_in_camera::setup(hasFeature(CAMERA_IN_CAMERA));
  }

  if (changedFeatures.test(FeatureRenderFlags::GBUFFER_PACKED_NORMALS) || changedFeatures.test(FeatureRenderFlags::CAMERA_IN_CAMERA))
    recreateFramegraph |= true;

  // VolumetricLightsNode depends on UPSCALE_SAMPLING_TEX which changed by presets
  if (changedFeatures.test(FeatureRenderFlags::UPSCALE_SAMPLING_TEX))
    recreateFramegraph |= true;

  g_entity_mgr->broadcastEventImmediate(ChangeRenderFeaturesEarly{});

  if (recreateResettable)
  {
    closeResetable();
    initResetable();
    setSpecularCubesContainerForReinit();
    reinitCube();
  }

  if (reapplySettings)
    applySettingsChanged();
  else if (recreateFramegraph)
  {
    requestFgRecreation("changeFeatures");
  }

  updateDistantHeightmap(); // currently it depends on distant fog

  if (changedFeatures.test(FeatureRenderFlags::SPECULAR_CUBES))
    invalidateCube();

  if (changedFeatures.test(FeatureRenderFlags::UPSCALE_SAMPLING_TEX) || changedFeatures.test(FeatureRenderFlags::TAA))
    requestFgRecreation("changeFeatures: UPSCALE_SAMPLING_TEX or TAA");

  if (changedFeatures.test(FeatureRenderFlags::FOM_SHADOWS))
    changeStateFOMShadows();

  g_entity_mgr->broadcastEventImmediate(ChangeRenderFeatures(newFeatures, changedFeatures));

  // TODO: detect that feature toggle need some reinit and do it

  debug("Running with featureRenderFlags: %s", render_features_to_string(get_current_render_features()));
}

void WorldRenderer::requestDepthAboveRenderTransparent()
{
  if (depthAOAboveCtx && transparentDepthAOAboveRequests == 0)
  {
    depthAOAboveCtx.reset(nullptr);
    depthAOAboveCtx =
      eastl::make_unique<DepthAOAboveContext>(DEPTH_AROUND_TEX_SIZE, DEPTH_AROUND_DISTANCE, needTransparentDepthAOAbove);
  }
  transparentDepthAOAboveRequests++;
}

void WorldRenderer::revokeDepthAboveRenderTransparent()
{
  if (depthAOAboveCtx && transparentDepthAOAboveRequests == 1)
  {
    depthAOAboveCtx.reset(nullptr);
    depthAOAboveCtx = eastl::make_unique<DepthAOAboveContext>(DEPTH_AROUND_TEX_SIZE, DEPTH_AROUND_DISTANCE, false);
  }
  transparentDepthAOAboveRequests = max(0, transparentDepthAOAboveRequests - 1);
}

int WorldRenderer::getDynamicResolutionTargetFps() const { return dynamicResolution ? dynamicResolution->getTargetFrameRate() : 0; }

BaseTexture *WorldRenderer::initAndGetHZBUploadStagingTex()
{
  if (!hzbUploadStagingTex)
    hzbUploadStagingTex =
      dag::create_tex(nullptr, OCCLUSION_W + (OCCLUSION_W >> 1), OCCLUSION_H, TEXFMT_R32F | TEXCF_DYNAMIC, 1, "hzb_readback_staging");
  return hzbUploadStagingTex.getTex2D();
}

static InitOnDemand<WorldRenderer, false, volatile bool> world_renderer;

bool get_sun_sph0_color(float height, Color3 &sun, Color3 &sph0)
{
  world_renderer->getSunSph0Color(height, sun, sph0);
  return true;
}

#if HAS_SHADER_GRAPH_COMPILE_SUPPORT

webui::HttpPlugin *get_renderer_http_plugins()
{
  http_renderer_plugins[0] = get_fog_shader_graph_editor_http_plugin();
  http_renderer_plugins[1] = get_envi_cover_shader_graph_editor_http_plugin();
  return http_renderer_plugins;
}


bool fog_shader_compiler(uint32_t variant_id, const String &name, const String &code, const DataBlock &shader_blk, String &out_errors)
{
  G_UNUSED(variant_id);
  G_UNUSED(code);
  G_UNUSED(out_errors);

  char buf[300];
  const char *shaderBinaryDir = dd_get_fname_location(buf, root_fog_graph.str());

  return world_renderer->volumeLight->updateShaders(String(0, "%s/%s", shaderBinaryDir, name), shader_blk, out_errors);
}

bool envi_cover_shader_compiler(
  uint32_t variant_id, const String &name, const String &code, const DataBlock &shader_blk, String &out_errors)
{
  G_UNUSED(variant_id);
  G_UNUSED(code);
  G_UNUSED(out_errors);

  char buf[300];
  const char *shaderBinaryDir = dd_get_fname_location(buf, root_envi_cover_graph.str());

  return world_renderer->enviCover->updateShaders(String(0, "%s/%s", shaderBinaryDir, name), shader_blk, out_errors);
}
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT

bool has_renderer_http_plugins_requirements()
{
#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
  if (!dd_dir_exists("../develop/assets/loc_shaders"))
  {
    debug("has_renderer_http_plugins_requirements: develop/assets/loc_shaders does not exist, shader_recompiler disabled");
    return false;
  }

  if (!dd_dir_exists("../../tools/dagor_cdk/commonData/graphEditor"))
  {
    debug("has_renderer_http_plugins_requirements: tools/dagor_cdk/commonData/graphEditor does not exist, shader_recompiler disabled");
    return false;
  }

  return true;
#else
  return false;
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT
}

void init_fog_shader_graph_plugin()
{
#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
  if (!has_renderer_http_plugins_requirements())
    return;

  if (root_fog_graph.empty())
  {
    debug("init_fog_shader_graph_plugin: root_fog_graph (rootFogGraph) is empty");
    return;
  }

  char name_buf[260];
  const char *shadersFolderName = ::dgs_get_game_params()->getStr("nodeBasedShadersFolder", "common");
  String rootGraphFileName(0, "../develop/assets/loc_shaders/%s/%s.json", shadersFolderName,
    dd_get_fname_without_path_and_ext(name_buf, sizeof(name_buf), root_fog_graph));

  NodeBasedShaderManager::initCompilation();
  ShaderGraphRecompiler::initialize(NodeBasedShaderType::Fog, fog_shader_compiler, rootGraphFileName);
#endif // HAS_SHADER_GRAPH_COMPILE_SUPPORT
}

void init_envi_cover_graph_plugin()
{
#if HAS_SHADER_GRAPH_COMPILE_SUPPORT
  if (!has_renderer_http_plugins_requirements())
    return;

  if (root_envi_cover_graph.empty())
  {
    debug("init_envi_cover_shader_graph_plugin: root_envi_cover_graph is empty");
    return;
  }

  char name_buf[260];
  const char *shadersFolderName = ::dgs_get_game_params()->getStr("nodeBasedShadersFolder", "common");
  String rootGraphFileName(0, "../develop/assets/loc_shaders/%s/%s.json", shadersFolderName,
    dd_get_fname_without_path_and_ext(name_buf, sizeof(name_buf), root_envi_cover_graph));

  NodeBasedShaderManager::initCompilation();
  ShaderGraphRecompiler::initialize(NodeBasedShaderType::EnviCover, envi_cover_shader_compiler, rootGraphFileName);
#endif
}

IRenderWorld *create_world_renderer() { return world_renderer.demandInit(); }

void destroy_world_renderer() { world_renderer.demandDestroy(); }

IRenderWorld *get_world_renderer() { return world_renderer.get(); }

IRenderWorld *get_world_renderer_unsafe()
{
  G_FAST_ASSERT((bool)world_renderer);
  return &*world_renderer;
}

void init_renderer_console();
void close_renderer_console();


void init_renderer_per_game()
{
  {
    d3d::GpuAutoLock gpuLock; // don't get why we need ownership
    biome_query::init();
    heightmap_query::init();
    init_fx();
  }
  profiler_tracker::init();
}
void term_renderer_per_game()
{
  profiler_tracker::close();
  biome_query::close();
  heightmap_query::close();
  term_fx();
}

#if _TARGET_PC_WIN
void send_gpu_net_event(const char *event_name, GpuVendor gpu_vendor, const DriverVersion *drv_ver)
{
  static const char EVENT_LOG_COLLECTION_NAME[] = "enl_events"; // name is predefined on backend. Contact ops if you want to change it
  Json::Value meta;
  meta[Json::StaticString{"evt"}] = event_name;
  if (gpu_vendor != GpuVendor::UNKNOWN)
    meta[Json::StaticString{"vid_vndr"}] = eastl::to_underlying(gpu_vendor);
  if (drv_ver)
  {
    Json::Value &jver = meta[Json::StaticString{"vid_drv_ver"}];
    jver.resize(4);
    jver[0] = drv_ver->product;
    jver[1] = drv_ver->major;
    jver[2] = drv_ver->minor;
    jver[3] = drv_ver->build;
  }
  meta["event"] = "gpu_event";
  event_log::send_udp(EVENT_LOG_COLLECTION_NAME, nullptr, 0, &meta);
}
#endif

static void verify_driver_caps()
{
#if _TARGET_PC_WIN
  if (d3d::get_driver_code().is(d3d::dx12 || d3d::vulkan)) // these drivers did all the testing at init.
    return;

  auto &drvDesc = d3d::get_driver_desc();

  if (drvDesc.shaderModel < 5.0_sm)
  {
    send_gpu_net_event("not_dx11_hw", GpuVendor::UNKNOWN, nullptr);
    os_message_box("This game requires a DirectX 11 compatible video card.", "Initialization error", GUI_MB_OK | GUI_MB_ICON_ERROR);
    _exit(1);
  }

  const GpuUserConfig &gcfg = d3d_get_gpu_cfg();
  debug("vendor %s: driver version: %s", d3d_get_vendor_name(drvDesc.info.vendor), drvDesc.info.driverVersion.toString().c_str());

  if (drvDesc.info.vendor == GpuVendor::AMD &&                        //
      drvDesc.info.driverVersion <= DriverVersion{8, 17, 10, 1404} && // this driver version found empirically
      drvDesc.info.driverVersion != DriverVersion{})                  // driver version was not detected at all
  {
    send_gpu_net_event("old_amd_vid_drv", drvDesc.info.vendor, &drvDesc.info.driverVersion);
    if (!dgs_execute_quiet)
    {
      os_message_box("The driver for this AMD videocard is outdated. Install Catalyst 16.X.X+ driver (at least Beta version)",
        "Initialization error", GUI_MB_OK | GUI_MB_ICON_ERROR);
    }
    breakpad::shutdown(); // allow to continue, but don't report fatals in this case
  }
  if (gcfg.outdatedDriver || !drvDesc.caps.hasProperUAVSupport)
  {
    send_gpu_net_event("old_vid_drv", drvDesc.info.vendor, &drvDesc.info.driverVersion);
    if (!dgs_execute_quiet)
    {
      const char *address = "https://support.gaijin.net/hc/articles/4405867465489";
      String message(0, "%s\n<a href=\"%s\">%s</a>", get_localized_text("video/outdated_driver"), address, address);
      os_message_box(message.data(), get_localized_text("video/outdated_driver_hdr"), GUI_MB_OK | GUI_MB_ICON_ERROR);
    }
  }

  if (!dgs_execute_quiet && !d3d::is_stub_driver() &&
      !dgs_get_settings()->getBlockByNameEx("debug")->getBool("skipIntegratedGpuWarning", false))
  {
    if (gcfg.usedSlowIntegratedSwitchableGpu)
    {
      os_message_box(get_localized_text("video/integrated_gpu_selected"), get_localized_text("video/header512"),
        GUI_MB_ICON_INFORMATION);
    }
  }
#endif
}

shadercache::WarmupParams get_shader_warmup_params()
{
  const bool supportsRT = d3d::get_driver_desc().caps.hasRayQuery;
  const bool hasTess = d3d::get_driver_desc().caps.hasQuadTessellation;
  const uint32_t texFmt = TEXFMT_R8;
  bool supportUnorderedLoad = d3d::get_texformat_usage(texFmt | TEXCF_UNORDERED) & d3d::USAGE_UNORDERED_LOAD;
  const bool hasHalfPrecision = d3d::get_driver_desc().caps.hasShaderFloat16Support;

  shadercache::WarmupParams params{};
  params.invalidVars = {
    {"use_hw_rt_gi", supportsRT ? 0 : 1},                         //
    {"world_sdf_support_uav_load", supportUnorderedLoad ? 0 : 1}, //
    {"can_use_half_precision", hasHalfPrecision ? 0 : 1},         //
    {"use_satellite_rendering", 1},                               // not needed for release builds
    {"water_tess_factor", hasTess ? 0 : 1} // if we have tesselation support, by default it will be used in water
  };

  if (!supportsRT)
  {
    params.invalidVars.push_back({"water_rt_enabled", 1});
    params.invalidVars.push_back({"water_refraction_tex", 0});
  }

  if ((get_water_quality() >= 2) && dgs_get_settings()->getBlockByNameEx("graphics")->getBool("useFoamFx", false))
    params.invalidVars.push_back({"wfx_effects_tex_enabled", 0}); // will always have wfx_effects_tex under this condition

  return params;
}

void init_world_renderer()
{
#if _TARGET_PC_WIN
  dump_tdr_settings();
#endif

  verify_driver_caps();

#if _TARGET_PC
  // only PC integrated videocards tend to have low VRAM amounts
  // SoCs on mobile platforms not fall for this case
  if (d3d_get_gpu_cfg().integrated)
  {
    debug("switched texture quality to low on integrated videocards");
    ::dgs_tex_quality = max(::dgs_tex_quality, (int)2);
  }

  const int dedicatedGPUMemory = d3d::get_dedicated_gpu_memory_size_kb();
  if (dedicatedGPUMemory > 0) // 0 is unknown or unimplemeted
  {
    if (dedicatedGPUMemory <= (1024 << 10) && ::dgs_tex_quality < 2)
    {
      ::dgs_tex_quality = 2;
      debug("switched texture quality to low on videocards with <= 1Gb of VRAM");
    }
    if (dedicatedGPUMemory <= (2048 << 10) && ::dgs_tex_quality < 1)
    {
      debug("switched texture quality to medium on videocards with <= 2Gb of VRAM");
      ::dgs_tex_quality = 1;
    }
  }
#endif

  init_renderer_console();
#if DAGOR_DBGLEVEL == 0
  if (app_profile::get().devMode)
#endif

    if (console::IVisualConsoleDriver *consoleDriver = setup_visual_console_driver())
    {
      int monoFontId = StdGuiRender::get_font_id("mono");
      consoleDriver->setFontId(max(monoFontId, 0));
      if (::dd_file_exist("autoexec.txt"))
        console::process_file("autoexec.txt");
    }

  if (d3d::get_driver_code().is(d3d::metal))
  {
    // important - don't do acquire here otherewise it won't do loading animation
    d3d::driver_command(Drv3dCommand::LOAD_PIPELINE_CACHE);
  }
  else if (d3d::get_driver_code().is(d3d::dx12 && !d3d::anyXbox))
  {
    load_pso_cache(get_exe_version_str());
  }
  else
  {
    shadercache::WarmupParams params = get_shader_warmup_params();
    shadercache::warmup_shaders_from_settings(params, true /*is_loadindg_thread*/, false /*backgroundWarmup*/);
  }

  dafg::startup(); // Note: FG might be referenced before world rendered creation from ecs code

  texdebug::init();
  levelprofiler::init();
}

void close_world_renderer()
{
  texdebug::teardown();
  levelprofiler::teardown();

  dafg::shutdown();

  close_renderer_console();
  close_visual_console_driver();
}

void init_gpu_settings() { d3d_apply_gpu_settings(*::dgs_get_settings()); }

void init_tex_streaming()
{
  init_managed_textures_streaming_support();
  int coreCount = cpujobs::get_core_count();
  int defWorkersCount = coreCount > 4 ? coreCount - 2 : coreCount - 1; // Keep one free core for main HT thread if we have enough cores
  const DataBlock &cfg = *dgs_get_settings()->getBlockByNameEx("texStreaming");
  ddsx::set_decoder_workers_count(cfg.getInt("numTexWorkThreads", defWorkersCount));
  ddsx::hq_tex_priority = cfg.getInt("hqTexPrio", is_managed_textures_streaming_load_on_demand() ? 1 : 0);
  init_gpu_settings();
}

bool is_point_under_water(const Point3 &point)
{
  float outRes = 0.f;
  if (auto wr = get_world_renderer())
    if (auto water = wr->getWater())
      fft_water::getHeightAboveWater(water, point, outRes);
  return outRes < 0.f;
}


static constexpr char const *WR_WAS_NULL_ERR_MSG = "Tried to access WorldRenderer components "
                                                   "when WorldRenderer did not exist!";

bool WRDispatcher::needSeparatedUI()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->needSeparatedUI();
}

bool WRDispatcher::isUpsampling()
{
  if (isReadyToUse())
    return static_cast<WorldRenderer *>(get_world_renderer())->isUpsampling();
  return false;
}

bool WRDispatcher::isFsrEnabled()
{
  if (isReadyToUse())
    return static_cast<WorldRenderer *>(get_world_renderer())->isFsrEnabled();
  return false;
}

int WRDispatcher::getCurrentAntiAliasingMode()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return static_cast<int>(wr->getAntiAliasingMode()); //-V522
}

CameraParams &WRDispatcher::getCurrentCameraParams()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->currentFrameCamera; //-V522
}

const CameraParams &WRDispatcher::getPreviousCameraParams()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->prevFrameCamera; //-V522
}

int WRDispatcher::getTemporalShadowFramesCount()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->getTemporalShadowFramesCount(); //-V522
}

void WRDispatcher::updateTransformations(const DPoint3 &move,
  const TMatrix4_vec4 &jittered_cam_pos_to_unjittered_history_clip,
  const TMatrix4_vec4 &prev_origo_relative_view_proj_tm)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  wr->updateTransformations(move, jittered_cam_pos_to_unjittered_history_clip, prev_origo_relative_view_proj_tm);
}

CameraViewVisibilityMgr *WRDispatcher::getMainCameraVisibilityMgr()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return &wr->mainCameraVisibilityMgr; //-V522
}

HeroWtmAndBox &WRDispatcher::getHeroData()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->heroData; //-V522
}

float WRDispatcher::getDaGdpRangeScale()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->daGdpRangeScale; //-V522
}

SkiesData *WRDispatcher::getMainPovSkiesData()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->main_pov_data; //-V522
}

const ManagedTex &WRDispatcher::getFinalTargetFrame()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return *wr->finalTargetFrame; //-V522
}

int WRDispatcher::getGbufferTargetGlobalFlags() { return WorldRenderer::GBUF_TARGET_GLOBAL_FLAGS; }

bool WRDispatcher::isReadyToUse() { return get_world_renderer() != nullptr; }

bool WRDispatcher::hasHighResFx()
{
  return static_cast<WorldRenderer *>(get_world_renderer())->getFxRtOverride() == FX_RT_OVERRIDE_HIGHRES;
}

IPoint2 WRDispatcher::getSubSuperPixels()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return IPoint2(wr->superPixels, wr->subPixels); //-V522
}

Point3 WRDispatcher::getSunDirection()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->dir_to_sun.curr; //-V522
}

Color3 WRDispatcher::getSunColor()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->sun; //-V522
}

const DataBlock *WRDispatcher::getLevelSettings()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->getLevelSettings(); //-V522
}

float WRDispatcher::GetWaterLevel()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->water ? fft_water::get_level(wr->water) : HeightmapHeightCulling::NO_WATER_ON_LEVEL;
}

WaterRenderMode WRDispatcher::determineWaterRenderMode(bool underWater, bool belowClouds)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->determineWaterRenderMode(underWater, belowClouds); //-V522
}

void WRDispatcher::getDisplayResolution(int &w, int &h)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  wr->getDisplayResolution(w, h); //-V522
}

void WRDispatcher::getDownsampledDepthParams(uint32_t &downsampledTexturesMipCount, bool &storeDownsampledTexturesInEsram)
{
  downsampledTexturesMipCount = 1;
  storeDownsampledTexturesInEsram = false;

  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));

  downsampledTexturesMipCount = wr->downsampledTexturesMipCount;
  storeDownsampledTexturesInEsram = wr->storeDownsampledTexturesInEsram;
}

float WRDispatcher::getGameTime()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->gameTime; //-V522
}
float WRDispatcher::getRealDeltaTime()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->realDeltaTime; //-V522
}

EnviCover *WRDispatcher::getEnviCover()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->enviCover.get(); //-V522
}

bool WRDispatcher::isEnviCover()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->isEnviCover; //-V522
}

bool WRDispatcher::isEnviCoverCompatible()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->isEnviCoverCompatible; //-V522
}

AntiAliasing *WRDispatcher::getAntialiasing()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->antiAliasing.get(); //-V522
}

ShadowsManager &WRDispatcher::getShadowsManager()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->shadowsManager; //-V522
}

ClusteredLights &WRDispatcher::getClusteredLights()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->lights; //-V522
}

IShadowInfoProvider &WRDispatcher::getShadowInfoProvider()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return *wr;
}

uint32_t WRDispatcher::addSpotLight(const ClusteredLights::SpotLight &light, SpotLightMaskType mask)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->addSpotLight(light, mask); //-V522
}

void WRDispatcher::setLight(uint32_t id_, const ClusteredLights::SpotLight &light, SpotLightMaskType mask, bool invalidate_shadow)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  wr->setLight(id_, light, mask, invalidate_shadow); //-V522
}

void WRDispatcher::destroyLight(uint32_t id)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  wr->destroyLight(id); //-V522
}

VolumeLight *WRDispatcher::getVolumeLight()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERT_AND_DO(wr != nullptr, DAG_FATAL(WR_WAS_NULL_ERR_MSG));
  return wr->volumeLight.get(); //-V522
}

Clipmap *WRDispatcher::getClipmap()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->getClipmap(); //-V522
}

LandMeshManager *WRDispatcher::getLandMeshManager()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->lmeshMgr;
}

LandMeshRenderer *WRDispatcher::getLandMeshRenderer()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->lmeshRenderer;
}

BaseStreamingSceneHolder *WRDispatcher::getBinScene()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->binScene;
}

DepthAOAboveContext *WRDispatcher::getDepthAOAboveCtx()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->depthAOAboveCtx.get();
}

DeformHeightmap *WRDispatcher::getDeformHeightmap()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->deformHmap.get();
}

IndoorProbeManager *WRDispatcher::getIndoorProbeManager()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->indoorProbeMgr.get();
}

MotionBlurNodePointers WRDispatcher::getMotionBlurNodePointers()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, {}, WR_WAS_NULL_ERR_MSG);
  return {&wr->motionBlurAccumulateNode, &wr->motionBlurApplyNode, &wr->motionBlurStatus};
}

const scene::TiledScene *WRDispatcher::getEnviProbeBoxesScene()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, nullptr, WR_WAS_NULL_ERR_MSG);
  return wr->getEnviProbeBoxesScene();
}

void WRDispatcher::ensureIndoorProbeDebugBuffersExist()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, , WR_WAS_NULL_ERR_MSG);
  if (wr->indoorProbeMgr)
    wr->indoorProbeMgr->ensureDebugBuffersExist();
}

eastl::optional<Point4> WRDispatcher::getHmapDeformRect()
{
  auto wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr, {}, WR_WAS_NULL_ERR_MSG);
  if (wr->deformHmap && wr->deformHmap->isEnabled())
    return wr->deformHmap->getDeformRect();
  else
    return {};
}

void WRDispatcher::getMinMaxZ(float &minHt, float &maxHt)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, , WR_WAS_NULL_ERR_MSG);
  wr->getMinMaxZ(minHt, maxHt);
}

bool WRDispatcher::shouldHideGui()
{
  // Hide UI for one frame while UI FG nodes are recreated fully, UI asserts if preparing and rendering is not paired.
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  return wr && wr->applySettingsAfterResetDevice && wr->needSeparatedUI();
}

bool WRDispatcher::isBareMinimum()
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, false, WR_WAS_NULL_ERR_MSG);
  return wr->getBareMinimumPreset();
}

bool WRDispatcher::usesDepthPrepass() { return prepass.get(); }

void WRDispatcher::updateWorldBBox(const BBox3 &additional_bbox)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  G_ASSERTF_RETURN(wr != nullptr, , WR_WAS_NULL_ERR_MSG);
  wr->additionalBBox += additional_bbox;
}

void WRDispatcher::recreateRayTracingDependentNodes(uint32_t features_to_reset)
{
  auto *wr = static_cast<WorldRenderer *>(get_world_renderer());
  if (wr == nullptr)
    return;

  bool needSetResolution = false;
  if (wr->hasMotionVectors != wr->needMotionVectors())
    needSetResolution |= true;
  // DLSS RR needs a device reset anyway, so we should avoid doing it twice.
  // This code is still needed, because on startup DLSS RR might be on
  // but RT wasn't initialized yet, so nodes were created wrong.
  if (features_to_reset & WRDispatcher::REINIT_TARGET && !wr->applySettingsAfterResetDevice)
    needSetResolution |= true;

  if (needSetResolution)
    wr->setResolution();
  if (features_to_reset & WRDispatcher::WATER)
    wr->requestFgRecreation("recreateRayTracingDependentNodes: Water");
  if (features_to_reset & WRDispatcher::SSAO)
    wr->resetSSAOImpl();
  if (features_to_reset & WRDispatcher::GI)
    wr->setGIQualityFromSettings();
  if (features_to_reset & WRDispatcher::SSR)
    wr->setSettingsSSR();
}

ECS_REGISTER_EVENT(GatherSplinegenBVHDataEvent);
ECS_REGISTER_EVENT(RemoveSplinegenBVHEvent);
ECS_REGISTER_EVENT(AfterRenderWorld)
ECS_REGISTER_EVENT(RenderPostFx)
ECS_REGISTER_EVENT(BeforeDrawPostFx)
ECS_REGISTER_EVENT(SetFxQuality)
ECS_REGISTER_EVENT(SetResolutionEvent)
ECS_REGISTER_EVENT(ChangeRenderFeatures)
ECS_REGISTER_EVENT(UpdateEffectRestrictionBoxes)
ECS_REGISTER_EVENT(RenderHmapDeform)
ECS_REGISTER_EVENT(QueryUnexpectedAltitudeChange)
ECS_REGISTER_EVENT(CustomSkyRender);
ECS_REGISTER_EVENT(CustomDmPanelRender);
ECS_REGISTER_EVENT(ResetAoEvent);
ECS_REGISTER_EVENT(BeforeLoadLevel);
ECS_REGISTER_EVENT(OnLevelLoaded);
ECS_REGISTER_EVENT(UpdateStageInfoBeforeRender);
ECS_REGISTER_EVENT(UpdateStageInfoRender);
ECS_REGISTER_EVENT(UpdateStageInfoRenderTrans);
ECS_REGISTER_EVENT(QueryHeroWtmAndBoxForRender);
ECS_REGISTER_EVENT(AfterHeightmapChange);
ECS_REGISTER_EVENT(RenderReinitCube);
ECS_REGISTER_EVENT(BeforeDraw);

bool have_renderer() { return true; }
