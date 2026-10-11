// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <shaders/dag_shAssert.h>
#include <vecmath/dag_vecMathDecl.h>
#include <scene/dag_occlusionMap.h>
#include <3d/dag_quadIndexBuffer.h>
#include <render/temporalAA.h>
#include <render/lights/clusteredLights.h>
#include <math/dag_TMatrix4D.h>
#include <astronomy/astronomy.h>
#include <dag_noise/dag_uint_noise.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <ioSys/dag_zstdIo.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <drv/3d/dag_renderStates.h>
#include <drv/3d/dag_viewScissor.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_buffers.h>
#include <drv/3d/dag_texture.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_driverDesc.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_lock.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_postFxRenderer.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_texPackMgr2.h>
#include <drv/3d/dag_resetDevice.h>
#include <3d/dag_texMgrTags.h>
#include <3d/dag_texMgr.h>

#include "test_main.h"
#include <shaders/dag_computeShaders.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include "de3_freeCam.h"
#include <gui/dag_stdGuiRender.h>
#include <debug/dag_debug3d.h>
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_vromfs.h>
#include <time.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <util/dag_console.h>
#include <util/dag_convar.h>
#include <render/sphHarmCalc.h>
#include <osApiWrappers/dag_miscApi.h>
#include <render/cascadeShadows.h>
#include <ssao_public.h>
#include <gtao_public.h>
#include <render/lights/omniLightsManager.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_cube_matrix.h>
#include <math/dag_viewMatrix.h>
#include <math/dag_frustum.h>
#include <math/dag_math3d.h>
#include <math/dag_adjpow2.h>
#include <math/random/dag_random.h>
#include <gui/dag_visConsole.h>
#include <screenShotSystem/dag_screenShotSystem.h>
#include <perfMon/dag_statDrv.h>
#include <visualConsole/dag_visualConsole.h>
#include <generic/dag_carray.h>
#include <render/lightCube.h>
#include <drv/3d/dag_rwResource.h>
#include <daSWRT/swBVH.h>
#include <levelSwrt.h>
#include <drv/3d/dag_lock.h>
#include <memory/dag_framemem.h>

#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_delayedAction.h>
#include <util/dag_parallelForInline.h>
#include <landMesh/lmeshCulling.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_geomTree.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <render/deferredRenderer.h>
#include <render/downsampleDepth.h>
#include <render/whiteTex.h>
#include <screenSpaceReflections_api.h>
#include <render/preIntegratedGF.h>
#include <render/viewVecs.h>
#include <render/voxelization_target.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daSkiesToBlk.h>
#include "de3_gui.h"
#include <render/giVerifierCapture.h>
#include "de3_gui_dialogs.h"
#include "de3_hmapTex.h"
#include <render/motionVectorAccess.h>
#include <render/spheres_consts.hlsli>
#include <render/scopeRenderTarget.h>
#include <shaders/dag_shaderMatData.h>
#include <render/debugTexOverlay.h>
#include <render/cameraPositions.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/staticShadowsCB.h>
#include <render/viewTransformData.h>
#include <render/upscale/upscaleSampling.h>
#include <daEditorE/daEditorE.h>
#include "entEdit.h"
#include <render/debugGbuffer.h>

#include <math/dag_hlsl_floatx.h>
#include <daGI2/daGI2.h>
#include <voxelizedMedia/voxelizedMedia.h>
#include <voxelizedMedia/vegetationMediaVoxelizer.h>
#include <heightmapTrace/heightmapMaxGrid.h>
#include <render/voxelization_target.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/functional.h>
#include <EASTL/bit.h>
#include <EASTL/optional.h>
#include <generic/dag_enumerate.h>

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>
#include <render/denoiser.h>
#include <render/rtsm.h>

#include "de3_benchmark.h"
#include "lruCollision.h"
#include <render/toroidal_update.h>
#include <profileEventsGUI/profileEventsGUI.h>
#include <scene/dag_visibility.h>
#include <shaders/dag_renderScene.h>
#include <render/heightmapShadows/heightmapShadows.h>
#include <gameLibs/render/raytracedToroidalStaticShadow/raytracedToroidalStaticShadow_api.h>
#include "depthAround.h"

#include <SnapdragonSuperResolution/SnapdragonSuperResolution.h>

#include <EASTL/optional.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>

extern "C" const size_t PS4_USER_MALLOC_MEM_SIZE = 1ULL << 30; // 1GB

static int loading_job_mgr_id = -1;
#include "level.h"
typedef StrmSceneHolder scene_type_t;

#define SKY_GUI  1
#define TEST_GUI 0

#define GLOBAL_VARS_LIST                             \
  VAR(prev_ambient_age)                              \
  VAR(prev_ambient)                                  \
  VAR(prev_specular)                                 \
  VAR(ambient_has_history)                           \
  VAR(ambient_reproject_tm)                          \
  VAR(prev_gbuffer_depth)                            \
  VAR(screen_ambient)                                \
  VAR(screen_specular)                               \
  VAR(voxelize_world_to_rasterize_space_mul)         \
  VAR(voxelize_world_to_rasterize_space_add)         \
  VAR(water_level)                                   \
  VAR(dynamic_lights_count)                          \
  VAR(zn_zfar)                                       \
  VAR(view_vecLT)                                    \
  VAR(view_vecRT)                                    \
  VAR(view_vecLB)                                    \
  VAR(view_vecRB)                                    \
  VAR(world_view_pos)                                \
  VAR(lowres_tex_size)                               \
  VAR(downsampled_far_depth_tex)                     \
  VAR(downsampled_far_depth_tex_samplerstate)        \
  VAR(prev_downsampled_far_depth_tex)                \
  VAR(prev_downsampled_far_depth_tex_samplerstate)   \
  VAR(downsampled_close_depth_tex)                   \
  VAR(downsampled_close_depth_tex_samplerstate)      \
  VAR(prev_downsampled_close_depth_tex)              \
  VAR(prev_downsampled_close_depth_tex_samplerstate) \
  VAR(downsampled_normals)                           \
  VAR(downsampled_normals_samplerstate)              \
  VAR(prev_downsampled_normals)                      \
  VAR(from_sun_direction)                            \
  VAR(shadow_frame)                                  \
  VAR(rasterize_collision_type)                      \
  VAR(downsample_depth_type)                         \
  VAR(local_light_probe_tex)                         \
  VAR(envi_probe_specular)

#define GLOBAL_VARS_OPT_LIST                           \
  VAR(downsampled_checkerboard_depth_tex)              \
  VAR(downsampled_checkerboard_depth_tex_samplerstate) \
  VAR(upscale_half_res_depth_tex)                      \
  VAR(upscale_half_res_depth_tex_samplerstate)         \
  VAR(prev_downsampled_normals_samplerstate)           \
  VAR(sphere_time)                                     \
  VAR(dagi_sp_has_exposure_assume)                     \
  VAR(lmesh_height_tex)                                \
  VAR(lmesh_height_tex_samplerstate)                   \
  VAR(lmesh_height_encoding)                           \
  VAR(world_to_lmesh_height)                           \
  VAR(gi_debug_froxels)                                \
  VAR(media_shadow_dist)                               \
  VAR(swrt_shadow_target)                              \
  VAR(swrt_shadow_target_size)                         \
  VAR(prev_globtm_psf_0)                               \
  VAR(prev_globtm_psf_1)                               \
  VAR(prev_globtm_psf_2)                               \
  VAR(prev_globtm_psf_3)                               \
  VAR(globtm_psf_0)                                    \
  VAR(globtm_psf_1)                                    \
  VAR(globtm_psf_2)                                    \
  VAR(globtm_psf_3)                                    \
  VAR(projtm_psf_0)                                    \
  VAR(projtm_psf_1)                                    \
  VAR(projtm_psf_2)                                    \
  VAR(projtm_psf_3)                                    \
  VAR(globtm_no_ofs_psf_0)                             \
  VAR(globtm_no_ofs_psf_1)                             \
  VAR(globtm_no_ofs_psf_2)                             \
  VAR(globtm_no_ofs_psf_3)                             \
  VAR(use_hw_rt_gi)                                    \
  VAR(rtss_screen_filter_mode)                         \
  VAR(rtss_screen_radius)                              \
  VAR(rtss_reference)

#define VAR(a) static ShaderVariableInfo a##VarId(#a, true);
GLOBAL_VARS_OPT_LIST
#undef VAR
#define VAR(a) static ShaderVariableInfo a##VarId(#a);
GLOBAL_VARS_LIST
#undef VAR

enum
{
  SCENE_MODE_NORMAL = 0,
  SCENE_MODE_VOXELIZE_ALBEDO = 1,
  SCENE_MODE_VOXELIZE_SDF = 2
};
enum
{
  REFLECTIONS_OFF,
  REFLECTIONS_SSR,
  REFLECTIONS_ALL
};

extern bool grs_draw_wire;
bool exit_button_pressed = false;

static bool enable_taa_override = false;
static bool use_snapdragon_super_resolution = false;
static float snapdragon_super_resolution_scale = 0.75f;
static uint32_t gi_fmt = TEXFMT_R11G11B10F; // TEXFMT_R32UI; // TEXFMT_R11G11B10F; // TEXFMT_R32UI;//

namespace hmap_debug
{
extern Point3 debug_quad_points[4];
extern Point3 debug_water_edges[4];
extern int debug_water_edges_count;
} // namespace hmap_debug

static void init_webui(const DataBlock *debug_block)
{
  if (!debug_block)
    debug_block = dgs_get_settings()->getBlockByNameEx("debug");
  if (debug_block->getBool("http_server", true))
  {
    static webui::HttpPlugin test_http_plugins[] = {webui::profiler_http_plugin, webui::shader_http_plugin, NULL};
    // webui::plugin_lists[1] = aces_http_plugins;
    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = test_http_plugins;
    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.bindPort = debug_block->getInt("http_port", 0);
    webui::startup(&cfg);
  }
}

void add_dynrend_resource_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneLodsResource *resource, uint64_t bvhId);
void add_dynrend_instance_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneInstance *resource, uint64_t bvh_id, int count,
  const Point3 &view_pos, float cull_radius);

void set_test_tm(int inst, TMatrix &tm) { tm.setcol(3, Point3(20 + (inst / 10) * 14, 0, 20 + (inst % 10) * 14)); }

static const int gbuf_rt = 3;
static unsigned gbuf_fmts[gbuf_rt] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_A8R8G8B8};
enum
{
  GBUF_ALBEDO_AO = 0,
  GBUF_NORMAL_ROUGH_MET = 1,
  GBUF_MATERIAL = 2,
  GBUF_NUM = 4
};

CONSOLE_BOOL_VAL("render", sw_rt, false);
CONSOLE_BOOL_VAL("render", hw_rt_gi, true);
CONSOLE_BOOL_VAL("render", debug_hmap_trace, false); // traced heightfield into the gbuffer
// hides raster trees only; the media volume instances stay = the destroyed tree look for GI.
// the land panel checkbox also invalidates the media, this convar deliberately does not
CONSOLE_BOOL_VAL("render", trees, true);
CONSOLE_FLOAT_VAL_MINMAX("render", swrt_rebuild_around, -0.001, -0.001, 1024);
CONSOLE_BOOL_VAL("render", swrt_shadow_use_checkerboard, true);

CONSOLE_INT_VAL("render", rerender_hmap, 0, 0, 16384);
CONSOLE_BOOL_VAL("render", save_rerendered_hmap, true);
CONSOLE_BOOL_VAL("render", rerender_lmesh, true);
CONSOLE_FLOAT_VAL_MINMAX("render", rerender_lmesh_offset, 0, 0, 1024);

CONSOLE_BOOL_VAL("render", stop_camera, false);

CONSOLE_INT_VAL("render", gf_frames, 128, 1, 512);
CONSOLE_BOOL_VAL("render", gi_reset, false);
CONSOLE_BOOL_VAL("render", ssao_blur, true);
CONSOLE_BOOL_VAL("render", dynamic_lights, true);
CONSOLE_BOOL_VAL("render", invalidate_dynamic_shadows, false);
CONSOLE_BOOL_VAL("render", taa, false);
CONSOLE_BOOL_VAL("render", taa_halton, true);
CONSOLE_BOOL_VAL("render", rasterize_sdf_prims, true);
CONSOLE_BOOL_VAL("render", rasterize_sdf_level, true);
CONSOLE_FLOAT_VAL_MINMAX("hshd", hmapShadowsDist, 1024, 128, 32768);
// how far the sun ray is traced through the media scene for canopy shadows, 0 turns them off
CONSOLE_FLOAT_VAL_MINMAX("render", media_shadow_dist, 64, 0, 512);
CONSOLE_FLOAT_VAL_MINMAX("hshd", hmapShadowsScale, 4, 2, 16);
CONSOLE_FLOAT_VAL_MINMAX("hshd", hmapShadowsSize, 384, 128, 2048);
CONSOLE_FLOAT_VAL_MINMAX("hshd", hmapShadowsCascades, 4, 1, 4);
CONSOLE_FLOAT_VAL_MINMAX("hshd", hmapShadowsSteps, 2, 0, 64);

CONSOLE_FLOAT_VAL_MINMAX("render", depthAboveAroundDist, 2048, 128, 8192);

CONSOLE_FLOAT_VAL_MINMAX("render", taa_mip_scale, -1.2, -5, 4);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_newframe_falloff, 0.15f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_newframe_dynamic_falloff, 0.14f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_motion_difference_max, 2.0f, 0, 100.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_motion_difference_max_weight, 0.5f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_frame_weight_min, 0.1f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_frame_weight_dynamic_min, 0.15f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_sharpening, 0.2f, 0, 1.f);
CONSOLE_FLOAT_VAL_MINMAX("render", taa_clamping_gamma, 1.2f, 0.1, 3.f);
CONSOLE_INT_VAL("render", taa_subsamples, 4, 1, 16);

CONSOLE_FLOAT_VAL_MINMAX("render", bvh_radius, 0, 0, 10000); // 0 = match world SDF far distance
CONSOLE_INT_VAL("render", hmap_max_grid_cells, 256, 32, 1024);
CONSOLE_FLOAT_VAL_MINMAX("render", hmap_max_grid_cell_size, 8, 1, 64);

CONSOLE_BOOL_VAL("rtss", freeze, false);
CONSOLE_BOOL_VAL("rtss", terrain, true); // off: no heightmap raster and no max grid, for the rtss and the GI trace
CONSOLE_BOOL_VAL("rtss", reference, false);

ConVarI sleep_msec_val("sleep_msec", 0, 0, 1000, NULL);

class RenderDynamicCube
{
  eastl::unique_ptr<DeferredRenderTarget> target;
  UniqueTex shadedTarget;
  PostFxRenderer copy;

public:
  RenderDynamicCube() {}
  ~RenderDynamicCube() { close(); }
  void close()
  {
    shadedTarget.close();
    target.reset();
    copy.clear();
  }
  void init(int cube_size)
  {
    shadedTarget = dag::create_tex(NULL, cube_size, cube_size, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1, "dynamic_cube_tex_target");
    copy.init("copy_tex");
    // return;
    target = eastl::make_unique<DeferredRenderTarget>("deferred_simple", "cube", cube_size, cube_size,
      DeferredRT::StereoMode::MonoOrMultipass, 0, gbuf_rt, gbuf_fmts, TEXFMT_DEPTH24);
  }
  template <typename Opaque, typename Envi>
  void update(const ManagedTex *cubeTarget, const Point3 &pos, Opaque opaque, Envi envi)
  {
    ShaderGlobal::set_float4(world_view_posVarId, pos.x, pos.y, pos.z, 1);
    float prev_pre_exposure = ShaderGlobal::get_float(get_shader_variable_id("pre_exposure", true));
    ShaderGlobal::set_float(get_shader_variable_id("pre_exposure"), 1);
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    float zn = 0.05, zf = 100;
    ShaderGlobal::set_float4(zn_zfarVarId, zn, zf, 0, 0);
    static int light_probe_posVarId = get_shader_variable_id("light_probe_pos", true);
    ShaderGlobal::set_float4(light_probe_posVarId, pos.x, pos.y, pos.z, 1);
    for (int i = 0; i < 6; ++i)
    {
      target->setRt();
      const Driver3dPerspective persp(1, 1, zn, zf, 0, 0);
      TMatrix4 projTm;
      d3d::calcproj(persp, projTm);
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
      cameraMatrix.setcol(3, pos);
      TMatrix viewTm = orthonormalized_inverse(cameraMatrix);
      const RenderView view(viewTm, projTm, pos);

      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);

      opaque(view);
      view.bind();
      target->resolve(shadedTarget.getTex2D(), viewTm, projTm);
      d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::SampledRO, {{shadedTarget.getTex2D(), 0, 0}});

      // d3d::clearview(CLEAR_ZBUFFER|CLEAR_STENCIL, 0, 0, 0);
      view.bind();
      envi(view, persp);
      // save_rt_image_as_tga(shadedTarget.getTex2D(), String(128, "cube%s.tga", i));

      d3d::set_render_target({}, DepthAccess::RW, {{cubeTarget->getCubeTex(), 0, static_cast<uint32_t>(i)}});
      d3d::settex(2, shadedTarget.getTex2D());
      copy.render();
    }
    ShaderGlobal::set_float(get_shader_variable_id("pre_exposure"), prev_pre_exposure);
  }
};

CONSOLE_BOOL_VAL("render", rasterize_debug_collision, false);
CONSOLE_BOOL_VAL("render", rasterize_gbuf_collision, true);
CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_from_collision_rasterize_below, 3, -1, 16);
CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_rasterize_supersample, 1, 1, 4);

static bool screenshotRequested = false;
static String screenshotFilename;
static bool verifyCaptureRequested = false; // app.save_verify_capture: giVerifier replay input
static String verifyCaptureDir;

namespace bvh
{
void add_mesh(ContextId context_id, uint64_t mesh_id, const MeshInfo &info)
{
  ObjectInfo obj;
  obj.isAnimated = false;
  obj.meshes.push_back(info);
  add_object(context_id, mesh_id, obj);
}
} // namespace bvh

static constexpr RaytracedToroidalStaticShadowSettings RTSS_DEFAULTS{};

class DemoGameScene final : public DagorGameScene, public ICascadeShadowsClient
{
public:
  RenderSWRT swrt;
  eastl::unique_ptr<RaytracedToroidalStaticShadow> rtss;
  BBox3 rtssWorldBox;
  double rtssLastTime = DBL_MAX; // constraint: real_time comes from a wrapping int of microseconds, so it can step back
  Point3 rtssLastCamPos = Point3(0, 0, 0), cameraMoveVel = Point3(0, 0, 0);
  uint32_t rtssLruRevision = 0, rtssHwMask = 0;
  int rtssFramesSinceJob = 0;
  CascadeShadows *csm = nullptr;
  static DemoGameScene *the_scene;
  struct ViewLodSelect
  {
    const LandMeshCullingData *lmesh;
    const dag::Vector<uint64_t> *collision;
    ViewLodSelect() : lmesh(nullptr), collision(nullptr) {}
  };
  struct Geometry
  {
    StrmSceneHolder *level;
    DynamicRenderableSceneInstance *trees;
    int treeCount;
    LRUCollision *collision;
    DynamicShaderHelper *spheres;
    float sphereTime;
    bool lmesh;
    bool terrain;
    BBox2 hmapGridWindow;
  };
  Geometry geometry()
  {
    return {land_panel.level ? binScene : nullptr, land_panel.trees && trees.get() ? test : nullptr, testCount,
      land_panel.collision && rasterize_gbuf_collision ? &lruColl : nullptr, land_panel.spheres ? &drawSpheres : nullptr,
      current_sphere_time, land_panel.lmesh, ::terrain, hmapGridWindow};
  }
  eastl::optional<eastl::pair<Point3, mat44f>> frozenView; // position and culling of render.stop_camera
  Point4 getCascadeShadowAnchor(int cascade_no) override
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    return Point4::xyz0(-itm.getcol(3));
  }
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &)
  {
    G_ASSERTF(false, "Use renderShadowsCascadesCb with explicit view data");
  }

  virtual void getCascadeShadowSparseUpdateParams(int cascade_no, const Frustum &, float &out_min_sparse_dist,
    int &out_min_sparse_frame)
  {
    out_min_sparse_dist = 100000;
    out_min_sparse_frame = -1000;
  }

  Point3 dir_to_sun;
  float sunAnimDt = 0;
  Point3 sliderDirToSun() const
  {
    const float zenith = DegToRad(sun_panel.sun_zenith), azimuth = DegToRad(sun_panel.sun_azimuth);
    return Point3(sinf(zenith) * cosf(azimuth), cosf(zenith), sinf(zenith) * sinf(azimuth));
  }
  float dayRate() const { return 2.f * PI / (sun_panel.day_minutes * 60.f); }
  void animateSun(float dt)
  {
    const float skipMinutes = sun_panel.time_skip ? 30.f : 0.f;
    sun_panel.time_skip = false;
    sunAnimDt = sun_panel.animate && skipMinutes == 0 ? dt : 0.f;
    if (sun_panel.astronomical)
    {
      if (sun_panel.animate)
        sun_panel.time += dt * 24.f / (sun_panel.day_minutes * 60.f);
      // constraint: julian_day is linear in a day past the month end and in a time below 0, but its day is unsigned
      const int days = max(int(floorf((sun_panel.time + skipMinutes / 60.f) / 24.f)), 1 - sun_panel.day);
      sun_panel.day += days;
      sun_panel.time += skipMinutes / 60.f - days * 24.f;
      return;
    }
    if (!sun_panel.animate && skipMinutes == 0)
      return;
    const Point3 axis(0, cosf(DegToRad(sun_panel.axis_tilt)), sinf(DegToRad(sun_panel.axis_tilt)));
    const float angle = (sun_panel.animate ? dayRate() * dt : 0.f) + skipMinutes * (2.f * PI / (24.f * 60.f));
    const Point3 dir = Quat(axis, angle) * sliderDirToSun();
    sun_panel.sun_zenith = RadToDeg(acosf(clamp(dir.y, -1.f, 1.f)));
    sun_panel.sun_azimuth = fmodf(RadToDeg(atan2f(dir.z, dir.x)) + 360.f, 360.f);
  }
  void setDirToSun()
  {
    float gmtTime = sun_panel.time;
    if (sun_panel.local_time)
      gmtTime -= sun_panel.longitude * (12. / 180);
    double jd = julian_day(sun_panel.year, sun_panel.month, sun_panel.day, gmtTime);

    daSkies.setStarsJulianDay(jd);
    daSkies.setStarsLatLon(sun_panel.latitude, sun_panel.longitude);
    if (sun_panel.astronomical)
    {
      daSkies.setAstronomicalSunMoon();
      dir_to_sun = daSkies.getSunDir();
    }
    else
    {
      dir_to_sun = sliderDirToSun();
      daSkies.setMoonDir(-dir_to_sun);
      daSkies.setMoonAge(sun_panel.moon_age);
    }
    ShaderGlobal::set_float4(from_sun_directionVarId, -dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
  }
  eastl::unique_ptr<ClusteredLights> clusteredLights;
  eastl::unique_ptr<DeferredRenderTarget> target;
  carray<UniqueTex, 2> farDownsampledDepth, downsampledNormals, downsampled_close_depth_tex;
  UniqueTex downsampled_checkerboard_depth_tex;
  eastl::unique_ptr<UpscaleSamplingTex> upscale_tex;
  int currentDownsampledDepth;

  UniqueTexWithShaderVar frame, prevFrame;
  UniqueTex taaHistory[2];
  PostFxRenderer postfx;
  eastl::unique_ptr<ScreenSpaceReflections> ssr;
  eastl::unique_ptr<SSAORenderer> ssao;
  eastl::unique_ptr<GTAORenderer> gtao;
  DebugTexOverlay debugTexOverlay;

  UniqueTex superResolutionOutput;

  ///*
  void downsampleDepth(bool downsample_normals)
  {
    TIME_D3D_PROFILE(downsample_depth);
    currentDownsampledDepth = 1 - currentDownsampledDepth;
    UniqueTex null_tex;
    downsample_depth::downsample(target->getDepthAll(), target->getWidth(), target->getHeight(),
      farDownsampledDepth[currentDownsampledDepth], downsampled_close_depth_tex[currentDownsampledDepth],
      downsample_normals ? downsampledNormals[currentDownsampledDepth] : null_tex, null_tex, null_tex,
      downsampled_checkerboard_depth_tex);


    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, farDownsampledDepth[currentDownsampledDepth]);
    ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, farDownsampledDepth[1 - currentDownsampledDepth]);

    ShaderGlobal::set_texture(downsampled_close_depth_texVarId, downsampled_close_depth_tex[currentDownsampledDepth]);
    ShaderGlobal::set_texture(prev_downsampled_close_depth_texVarId, downsampled_close_depth_tex[1 - currentDownsampledDepth]);
    ShaderGlobal::set_texture(downsampled_normalsVarId, downsampledNormals[currentDownsampledDepth]);
    ShaderGlobal::set_texture(prev_downsampled_normalsVarId, downsampledNormals[1 - currentDownsampledDepth]);
    ShaderGlobal::set_texture(upscale_half_res_depth_texVarId, downsampled_close_depth_tex[currentDownsampledDepth]);
    if (upscale_tex)
      upscale_tex->render();
  }

  UniqueTexWithShaderVar combined_shadows;
  PostFxRenderer combine_shadows;

  MultiFramePGF preIntegratedGF;
  void initGF()
  {
    SCOPE_RENDER_TARGET_NAME(_initGF);
    preIntegratedGF.init(PREINTEGRATE_SPECULAR_DIFFUSE_QUALITY_MAX, gf_frames);
  }

  UniqueTex swrt_shadow_target;
  UniqueBuf swrt_shadow_mask;
  void swRTShadows(const Point3 &dir_to_sun, float sun_size)
  {
    if (!swrt.hasData())
    {
      swrt_shadow_target.close();
      ShaderGlobal::set_int4(swrt_shadow_target_sizeVarId, 0, 0, 0, 0);
      return;
    }
    static int shadow_frame = 0;
    ++shadow_frame;
    TextureInfo ti;
    if (swrt_shadow_target)
      swrt_shadow_target.getTex2D()->getinfo(ti, 0);

    // src is the half-res depth width; the checkerboard target is ceil(src/2) so its last column
    // still maps to the last source column (shader maps dtId.x -> 2*dtId.x + parity).
    const bool checkerboard = swrt_shadow_use_checkerboard && !reference;
    const int srcW = target->getWidth() >> 1;
    const int halfW = checkerboard ? (srcW + 1) / 2 : srcW, halfH = target->getHeight() >> 1;
    if (halfW != ti.w || halfH != ti.h)
    {
      swrt_shadow_target.close();
      swrt_shadow_target = dag::create_tex(NULL, halfW, halfH, TEXFMT_R8 | TEXCF_RTARGET | TEXCF_UNORDERED, 1, "swrt_shadow_target");
    }
    if (bvhCtx != bvh::InvalidContextId)
      bvh::bind_resources(bvhCtx, target->getWidth()); // swrt hw rt mode traces the context's main TLAS (render meshes, RTSM only)
    // constraint: the SWRT early-out copies combined_shadows, which holds the rtss term that view 5 checks
    ShaderGlobal::set_int(rtss_referenceVarId, reference ? 1 : 0);
    if (reference)
      ShaderGlobal::set_texture(combined_shadows.getVarId(), get_white_on_demand());
    swrt.renderShadows(dir_to_sun, reference ? 0.f : sun_size, shadow_frame, checkerboard ? shadow_frame : 0, srcW,
      swrt_shadow_mask.getBuf(), swrt_shadow_target.getTex2D(), true);
    if (reference)
      combined_shadows.setVar();
  }
  void combineShadows()
  {
    TIME_D3D_PROFILE(combineShadows);
    SCOPE_RENDER_TARGET_NAME(_combineShadows);
    d3d::set_render_target({}, DepthAccess::RW, {{combined_shadows.getTex2D(), 0, 0}});
    combine_shadows.render();
    combined_shadows.setVar();
  }

  // a device reset loses every gpu resource: the gi history, and the baked media bricks the
  // volumes keep across an ordinary invalidation
  void giAfterDeviceReset()
  {
    if (daGI2)
      daGI2->afterReset();
    mediaVolumes.afterReset();
    rtss->afterDeviceReset();
    giHmapMaxGrid.close(); // constraint: the grid owner closes it after a device reset (heightmapMaxGrid.h)
  }

  void invalidateGI()
  {
    reloadCube(true);
    if (daGI2)
    {
      d3d::GpuAutoLock gpu_al;
      daGI2->afterReset(); // the baked media bricks survive this: only giAfterDeviceReset rebakes
    }
    giHmapMaxGrid.close(); // the per frame init/update recreates and refills
    validAmbientHistory = false;
    // daGI2.reset(create_dagi());
  }

  BBox3 sceneBox, levelBox;
  BBox3 loadedLevelBox; // bin scene bounds only (levelBox also merges collision bounds)

  int giUpdatePosFrameCounter = 1;
  enum
  {
    MAX_GI_FRAMES_TO_INVALIDATE_POS = 24
  };
  void updateSSGIPos(const Point3 &pos_)
  {
    float giUpdateDist = min(64.f, 0.5f * min(sceneBox.width().x, min(sceneBox.width().y, sceneBox.width().z)));
    Point3 pos = pos_;
    if (!rtssWorldBox.isempty())
      pos = min(max(pos, rtssWorldBox[0] + Point3(giUpdateDist, giUpdateDist, giUpdateDist)),
        rtssWorldBox[1] - Point3(giUpdateDist, giUpdateDist, giUpdateDist));
    const bool waitForShadows = ++giUpdatePosFrameCounter <= MAX_GI_FRAMES_TO_INVALIDATE_POS;
    if (waitForShadows && !rtss->isReady(BBox3(pos, giUpdateDist)))
      return;
    giUpdatePosFrameCounter = 0;
  }

  static constexpr int HYBRID_HW_RASTER_LEVELS = 2;
  // constraint: the collision TLAS streams only within its radius, so the coarse levels trace SWRT
  uint32_t rtssHwLevelMask() const
  {
    const bool hwRecipe = rtss_panel.recipe == RTSS_HWRT || rtss_panel.recipe == RTSS_HYBRID_HW;
    const uint32_t traced = rtss_panel.recipe == RTSS_HYBRID_HW ? ~((1u << HYBRID_HW_RASTER_LEVELS) - 1) : ~0u;
    return hwRecipe && hwRtAvailable() ? ((1u << eastl::clamp(rtss_panel.hwLevels, 0, RTSS_MAX_LEVELS)) - 1) & traced : 0;
  }
  RaytracedToroidalStaticShadowSettings rtssSettings(uint32_t hw) const
  {
    RaytracedToroidalStaticShadowSettings settings = rtss_panel;
    // constraint: atan2 sees a wrapped travel as a short step, so a frame whose day travel can pass 60 deg gives no rate
    const float dayStep = sunAnimDt * dayRate();
    settings.sunJumpDeg = dayStep <= PI / 3 ? RadToDeg(1.5f * dayStep) : 0.f;
    for (int k = 0; k < settings.levels; ++k)
    {
      RtssRaster raster = RtssRaster::None;
      if (rtss_panel.recipe == RTSS_RASTER)
        raster = RtssRaster::All;
      else if (rtss_panel.recipe == RTSS_HYBRID_SW)
        raster = settings.texel0 * float(1 << k) <= rtss_panel.rasterMaxTexel * 1.001f ? RtssRaster::All : RtssRaster::Augment;
      else if (rtss_panel.recipe == RTSS_HYBRID_HW)
        raster = k < HYBRID_HW_RASTER_LEVELS ? RtssRaster::All : RtssRaster::Augment;
      if (raster != RtssRaster::All && !(hw & (1u << k)) && !swrt.hasData())
        raster = RtssRaster::All;
      const float cost = raster == RtssRaster::All       ? rtss_panel.rasterCost
                         : raster == RtssRaster::Augment ? rtss_panel.augmentCost
                                                         : 1.f;
      settings.recipes[k] = {raster, cost};
    }
    return settings;
  }
  // constraint: the collision TLAS carries no terrain, so the gi trace and the rtss complete it with this window
  BBox2 hmapGridWindow; // the terrain the trace holds, empty while the grid is closed
  void updateHmapMaxGrid(const Point3 &pos)
  {
    const int cells = get_bigger_pow2(hmap_max_grid_cells);
    const float cell = hmap_max_grid_cell_size;
    const bool ready = terrain && binScene && land_panel.level && giHmapMaxGrid.init(cells, cell);
    const bool wasReady = !hmapGridWindow.isempty();
    hmapGridWindow.setempty();
    if (ready)
    {
      giHmapMaxGrid.update(pos);
      const Point2 lt = Point2(floorf(pos.x / cell) - cells / 2, floorf(pos.z / cell) - cells / 2) * cell; // as HeightmapMaxGrid
      hmapGridWindow = BBox2(lt, lt + Point2(cells, cells) * cell);
    }
    else
      giHmapMaxGrid.close(); // constraint: a level turned off keeps no terrain in the grid
    if ((ready != wasReady) | hmap_max_grid_cells.pullValueChange() | hmap_max_grid_cell_size.pullValueChange())
      rtss->invalidate(); // constraint: the pages hold the terrain of the grid they were built with
  }
  int rtssCasters = -1;
  void updateRtss(const Point3 &cam_pos, double real_time)
  {
    ShaderGlobal::set_int(rtss_screen_filter_modeVarId, rtss_panel.screen_filter);
    ShaderGlobal::set_float(rtss_screen_radiusVarId, rtss_panel.screen_radius);
    const float dt = real_time > rtssLastTime ? float(real_time - rtssLastTime) : 0.f;
    const Point3 camVel = dt > 0 ? (cam_pos - rtssLastCamPos) / dt : Point3(0, 0, 0);
    rtssLastTime = real_time;
    rtssLastCamPos = cam_pos;
    ++rtssFramesSinceJob;
    if (freeze)
      return;
    const uint32_t hw = applyRtssSettings();
    if (hw)
    {
      const bvh::LruCollisionStats lru = bvh::get_lru_collision_stats(bvhCtx);
      if (lru.settled && lru.revision != rtssLruRevision)
      {
        rtssLruRevision = lru.revision;
        const BBox3 box(cam_pos, 2 * (bvhLruRadius + bvhBorder(bvhLruRadius)));
        rtss->invalidateBoxes(&box, 1, hw);
      }
    }
    const int casters = (land_panel.trees && trees.get() && test) | land_panel.lmesh << 1 |
                        (land_panel.collision && rasterize_gbuf_collision) << 2 | land_panel.spheres << 3;
    if (const int old = eastl::exchange(rtssCasters, casters); old >= 0 && old != casters)
      rtss->invalidate(); // constraint: the raster pages draw the casters of these toggles
    rtss->setSun(dir_to_sun);
    buildRtss(cam_pos, camVel, dt);
  }
  uint32_t applyRtssSettings(bool hw_allowed = true)
  {
    const uint32_t hw = hw_allowed ? rtssHwLevelMask() : 0;
    rtss->setSettings(rtssSettings(hw));
    if (hw != rtssHwMask)
      rtss->invalidate(); // constraint: the library does not see the tracers
    rtssHwMask = hw;
    return hw;
  }
  void buildRtss(const Point3 &pos, const Point3 &vel, float dt)
  {
    rtss->update(dpoint3(pos), vel, dt);
    const Geometry geom = geometry();
    precullRtssRegions(*rtss, geom, pos, rtssCulls);
    RtssRasterCallback cb(*rtss, geom, pos, rtssCulls);
    RtssTracers tracers(rtssSwTracer, rtssCollisionTracer, rtssHwMask);
    rtss->render(&cb, &tracers);
    RaytracedToroidalStaticShadow::Stats st;
    rtss->getStats(st);
    if (st.rasterJobs || st.rays) // a raster job with an empty depth range builds without the callback
      rtssFramesSinceJob = 0;
  }

  void renderVoxelsMediaGeom(const BBox3 &scene_box, const Point3 &voxel_size, int mode)
  {
    if (!binScene || !land_panel.level)
      return;
    DA_PROFILE_UNIQUE;
    // debug("invalid render %@ %@", scene_box[0], scene_box[1]);
    static int scene_mode_varId = get_shader_variable_id("scene_mode");
    ShaderGlobal::set_int(scene_mode_varId, mode);
    const IPoint3 res = IPoint3(ceil(div(scene_box.width(), voxel_size)));
    SCOPE_RENDER_TARGET;
    int maxRes = max(max(res.x, res.y), res.z);
    set_voxelization_target_and_override(maxRes, maxRes);
    Point3 voxelize_box0 = scene_box[0];
    Point3 voxelize_box1 = Point3(1. / scene_box.width().x, 1. / scene_box.width().y, 1. / scene_box.width().z);
    ShaderGlobal::set_float4(get_shader_variable_id("voxelize_box0"), P3D(voxelize_box0), maxRes);
    ShaderGlobal::set_float4(get_shader_variable_id("voxelize_box1"), P3D(voxelize_box1), 0.f);
    Point3 voxelize_aspect_ratio = min(point3(res) / maxRes, Point3(1, 1, 1));
    // Point3 voxelize_aspect_ratio = min((point3(res) + Point3(1,1,1)) * 2. / maxRes, Point3(1,1,1));
    // voxelize_aspect_ratio = max(voxelize_aspect_ratio, Point3(0.5,0.5,0.5));
    Point3 mult = 2. * mul(voxelize_box1, voxelize_aspect_ratio);
    Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
    ShaderGlobal::set_float4(get_shader_variable_id("voxelize_world_to_rasterize_space_mul"), P3D(mult), 0);
    ShaderGlobal::set_float4(get_shader_variable_id("voxelize_world_to_rasterize_space_add"), P3D(add), 0);

    Point3 camera_pos(scene_box.center().x, scene_box[1].y, scene_box.center().z);
    TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, CUBEFACE_NEGY);
    cameraMatrix.setcol(3, camera_pos);
    cameraMatrix = orthonormalized_inverse(cameraMatrix);
    const BBox3 box = cameraMatrix * scene_box;
    const RenderView view(cameraMatrix, matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z),
      camera_pos);

    for (int i = 0; i < 3; ++i)
    {
      ShaderGlobal::set_int(get_shader_variable_id("voxelize_axis", true), i);
      binScene->render(view, 0, 0xFFFFFFFF);
    }
    reset_voxelization_override();
    ShaderGlobal::set_int(scene_mode_varId, SCENE_MODE_NORMAL);
  }

  bool useShaderAsserts = false;

  UniqueTex ambient[2];
  UniqueTex ambient_age[2];
  UniqueTex screen_specular[2];
  int cAmbientFrame = 0;
  PostFxRenderer ambientRenderer;

  LRUCollision lruColl;

  eastl::unique_ptr<DaGI> daGI2;
  HeightmapMaxGrid giHmapMaxGrid;
  DynamicShaderHelper debugHmapTraceShader;
  // the traced heightfield into the gbuffer: correct tracing lands exactly on
  // the rasterized landmesh
  void renderDebugHmapTrace()
  {
    if (!debug_hmap_trace.get())
      return;
    if (!debugHmapTraceShader.shader)
      debugHmapTraceShader.init("testgi_hmap_trace_debug", NULL, 0, "testgi_hmap_trace_debug", true);
    if (!debugHmapTraceShader.shader)
      return;
    TIME_D3D_PROFILE(debug_hmap_trace);
    debugHmapTraceShader.shader->setStates(0, true);
    d3d::setvsrc(0, nullptr, 0);
    d3d::draw(PRIM_TRILIST, 0, 1);
  }
  void ensurePrevFrame()
  {
    TextureInfo frameInfo;
    frame.getTex2D()->getinfo(frameInfo);
    TextureInfo prevFrameInfo;
    if (prevFrame)
      prevFrame.getTex2D()->getinfo(prevFrameInfo);
    if (prevFrameInfo.w != frameInfo.w / 2 || prevFrameInfo.h != frameInfo.h / 2)
    {
      prevFrame.close();
      prevFrame = dag::create_tex(NULL, frameInfo.w / 2, frameInfo.h / 2, frameInfo.cflg, 1, "prev_frame_tex");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(get_shader_variable_id("prev_frame_tex_samplerstate"), d3d::request_sampler(smpInfo));
    }
  }

  void initScreenGI(int w, int h)
  {
    const bool supportRGBE_RT = d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_RTARGET);
    gi_fmt = supportRGBE_RT ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
    if (gi_panel.gi_mode == SCREEN_PROBES)
    {
      if (ShaderGlobal::is_var_assumed(dagi_sp_has_exposure_assumeVarId) &&
          ShaderGlobal::get_interval_assumed_value(dagi_sp_has_exposure_assumeVarId) == 0)
        gi_fmt = supportRGBE_RT ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
      else
        gi_fmt = TEXFMT_R32UI;
    }

    if (ambient[0])
    {
      TextureInfo ti;
      ambient[0].getTex2D()->getinfo(ti);
      if (ti.w != w || ti.h != h || ((ti.cflg & TEXFMT_MASK) != gi_fmt))
        ambient[0].close();
    }
    if (ambient[0])
      return;
    ambient[0] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_ambient0");
    screen_specular[0].close();
    screen_specular[0] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_specular0");
  }

  void closeGIReprojection()
  {
    if (!ambient[1])
      return;
    screen_specular[1].close();
    ambient[1].close();
    ambient_age[0].close();
    ambient_age[1].close();
    validAmbientHistory = false;
  }

  void initGIReprojection(int w, int h)
  {
    if (!screen_probes.reproject)
    {
      closeGIReprojection();
      return;
    }
    if (ambient[1] && screen_probes.reproject)
    {
      TextureInfo ti;
      ambient[1].getTex2D()->getinfo(ti);
      if (ti.w == w && ti.h == h)
        return;
    }
    closeGIReprojection();
    ambient[1] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_ambient1");

    screen_specular[1] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_specular1");

    ambient_age[0] = dag::create_tex(NULL, w, h, TEXFMT_R8UI | TEXCF_RTARGET, 1, "screen_ambient_age0");
    ambient_age[1] = dag::create_tex(NULL, w, h, TEXFMT_R8UI | TEXCF_RTARGET, 1, "screen_ambient_age1");
    validAmbientHistory = false;
  }

  ResizableTex otherDepth;

  int oldReflections = REFLECTIONS_OFF;
  void initReflections(int w, int h)
  {
    const int rq = min(gi_panel.reflections, int(gi_panel.gi_mode <= ONLY_AO ? REFLECTIONS_SSR : REFLECTIONS_ALL));
    if (oldReflections == rq)
      return;
    oldReflections = rq;
    ssr.reset();
    if (rq == REFLECTIONS_OFF)
      return;
    ssr.reset(new ScreenSpaceReflections(w / 2, h / 2, 1, TEXFMT_A16B16G16R16F, SSRQuality::Compute));
    ShaderGlobal::set_int(get_shader_variable_id("ssr_alternate_reflections", true), rq == REFLECTIONS_ALL ? 1 : 0);
    ShaderGlobal::set_int(get_shader_variable_id("ssr_quality", true), rq == REFLECTIONS_ALL ? 3 : 0);
  }

  void initRes(int w, int h)
  {
    d3d::GpuAutoLock gpu_al;
    otherDepth.close();
    target.reset();
    target = eastl::make_unique<DeferredRenderTarget>("deferred_shadow_to_buffer", "main", w, h,
      DeferredRT::StereoMode::MonoOrMultipass, 0, gbuf_rt, gbuf_fmts, TEXFMT_DEPTH32);
    target->setVar();
    {
      TextureInfo ti;
      target->getDepth()->getinfo(ti);
      otherDepth = dag::create_tex(NULL, ti.w, ti.h, ti.cflg, 1, "other_target_depth");
    }

    const int halfW = w / 2, halfH = h / 2;
    swrt_shadow_mask.close();
    swrt_shadow_mask = dag::create_sbuffer(sizeof(uint32_t), RenderSWRT::getTileBufferSizeDwords(halfW, halfH),
      SBCF_BIND_SHADER_RES | SBCF_MISC_ALLOW_RAW | SBCF_BIND_UNORDERED, 0, "swrt_shadow_mask");

    combined_shadows.close();
    combined_shadows = dag::create_tex(NULL, w, h, TEXFMT_R8 | TEXCF_RTARGET, 1, "combined_shadows");
    ShaderGlobal::set_sampler(get_shader_variable_id("combined_shadows_samplerstate"), d3d::request_sampler({}));
    debugTexOverlay.setTargetSize(Point2(w, h));

    ShaderGlobal::set_float4(::get_shader_variable_id("lowres_rt_params", true), halfW, halfH, 0, 0);

    ShaderGlobal::set_float4(get_shader_variable_id("rendering_res"), w, h, 1.0f / w, 1.0f / h);
    ShaderGlobal::set_float4(::get_shader_variable_id("taa_display_resolution", true), w, h, 0, 0);
    initReflections(w, h);
    ssao.reset();
    ssao = eastl::make_unique<SSAORenderer>(w / 2, h / 2, 1, SSAO_USE_CONTACT_SHADOWS);

    gtao.reset();
    gtao = eastl::make_unique<GTAORenderer>(w / 2, h / 2, 1, SSAO_USE_CONTACT_SHADOWS);

    initScreenGI(w, h);

    if (ssao || gtao) //||ssr||dynamicLighting
    {
      int numMips = min(get_log2w(w / 2), get_log2w(h / 2)) - 1;
      numMips = max(numMips, 8);
      debug("numMips = %d", numMips);
      int rfmt = 0;
      if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_RTARGET))
        rfmt = TEXFMT_R32F;
      else if ((d3d::get_texformat_usage(TEXFMT_R16F) & d3d::USAGE_RTARGET))
        rfmt = TEXFMT_R16F;
      else
        rfmt = TEXFMT_R16F; // fallback to any supported format?
      for (int i = 0; i < farDownsampledDepth.size(); ++i)
      {
        String name(128, "far_downsampled_depth%d", i);
        farDownsampledDepth[i] = dag::create_tex(NULL, w / 2, h / 2, rfmt | TEXCF_RTARGET, numMips, name);
        downsampled_close_depth_tex[i].close();
        name.printf(128, "close_downsampled_depth%d", i);
        downsampled_close_depth_tex[i] = dag::create_tex(NULL, w / 2, h / 2, rfmt | TEXCF_RTARGET, numMips, name);

        name.printf(128, "close_downsampled_normals%d", i);
        downsampledNormals[i].close();
        downsampledNormals[i] = dag::create_tex(NULL, w / 2, h / 2, TEXCF_RTARGET, 1, name); // TEXFMT_A2B10G10R10 |
      }
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Border;
        smpInfo.border_color = d3d::BorderColor::Color::TransparentBlack;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        smpInfo.mip_map_mode = d3d::MipMapMode::Point;
        d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
        ShaderGlobal::set_sampler(downsampled_far_depth_tex_samplerstateVarId, smp);
        ShaderGlobal::set_sampler(prev_downsampled_far_depth_tex_samplerstateVarId, smp);
      }
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        smpInfo.mip_map_mode = d3d::MipMapMode::Point;
        d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
        ShaderGlobal::set_sampler(downsampled_close_depth_tex_samplerstateVarId, smp);
        ShaderGlobal::set_sampler(prev_downsampled_close_depth_tex_samplerstateVarId, smp);
        ShaderGlobal::set_sampler(upscale_half_res_depth_tex_samplerstateVarId, smp);
      }
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        d3d::SamplerHandle smp = d3d::request_sampler(smpInfo);
        ShaderGlobal::set_sampler(downsampled_normals_samplerstateVarId, smp);
        ShaderGlobal::set_sampler(prev_downsampled_normals_samplerstateVarId, smp);
      }
      upscale_tex.reset();
      upscale_tex.reset(new UpscaleSamplingTex(w, h, "close_"));
      ShaderGlobal::set_float4(lowres_tex_sizeVarId, w / 2, h / 2, 1.f / (w / 2), 1.f / (h / 2));

      downsampled_checkerboard_depth_tex.close();
      downsampled_checkerboard_depth_tex =
        dag::create_tex(NULL, w / 2, h / 2, rfmt | TEXCF_RTARGET, 1, "downsampled_checkerboard_depth_tex");
      ShaderGlobal::set_texture(downsampled_checkerboard_depth_texVarId, downsampled_checkerboard_depth_tex.getTexId());
      ShaderGlobal::set_sampler(downsampled_checkerboard_depth_tex_samplerstateVarId, d3d::request_sampler({}));
    }
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;
    if (!prevFrame)
    {
      prevFrame = dag::create_tex(NULL, w / 2, h / 2, rtFmt | TEXCF_RTARGET, 1, "prev_frame_tex");
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      ShaderGlobal::set_sampler(get_shader_variable_id("prev_frame_tex_samplerstate"), d3d::request_sampler(smpInfo));
    }
    frame.close();
    frame = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "frame_tex");
    taaHistory[0].close();
    taaHistory[1].close();
    taaHistory[0] = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "taa_history_tex");
    taaHistory[1] = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "taa_history_tex2");
    ShaderGlobal::set_sampler(get_shader_variable_id("taa_history_tex_samplerstate"), d3d::request_sampler({}));
    ShaderGlobal::set_sampler(get_shader_variable_id("frame_tex_samplerstate"), d3d::request_sampler({}));
    if (use_snapdragon_super_resolution)
    {
      int fw, fh;
      d3d::get_render_target_size(fw, fh, nullptr);
      superResolutionOutput.close();
      superResolutionOutput = dag::create_tex(NULL, fw, fh, rtFmt | TEXCF_RTARGET, 1, "SuperResolutionOutput");
      snapdragonSuperResolutionRender.SetViewport(w, h);
    }
    clusteredLights->changeResolution(w, h);

    initRtsmDenoiser(w, h);
  }

  // rtsm/denoiser state does not survive a device reset: this is the recovery too
  void initRtsmDenoiser(int w, int h)
  {
    if (!isRtsmEnabled() || !w || !h)
      return;
    rtsm::teardown();
    denoiser::teardown();

    denoiser::initialize(w, h, false, false);
    rtsm::initialize(rtsm::RenderMode::Hard, false);

    denoiser::TexInfoMap textures;

    ::denoiser::get_required_persistent_texture_descriptors(textures, false, false, false);
    ::rtsm::get_required_persistent_texture_descriptors(textures);
    ::rtsm::get_required_transient_texture_descriptors(textures);

    for (auto &tex : textures)
      rt_textures[tex.first] =
        dag::create_tex(nullptr, tex.second.w, tex.second.h, tex.second.cflg | TEXCF_CLEAR_ON_CREATE, tex.second.mipLevels, tex.first);
    rtsmResW = w;
    rtsmResH = h;
  }
  int rtsmResW = 0, rtsmResH = 0;

  DepthAround depthAround;
  HeightmapShadows hmapShadows;
  dag::Vector<int> modelIndex;
  DemoGameScene()
  {
    hmapShadows.init(hmapShadowsSize, hmapShadowsCascades, hmapShadowsDist, hmapShadowsScale, Point2(0, 1000), false);
    depthAround.init(1024);

    the_scene = this;
    init_voxelization();
    rtss = eastl::make_unique<RaytracedToroidalStaticShadow>(RTSS_DEFAULTS);
    FullFileLoadCB riCollision(dgs_get_settings()->getStr("ri_collisions", "ri_collisions.bin"));
    const LevelCollision &level = lruColl.level;
    const int64_t loadRef = ref_time_ticks();
    if (!riCollision.fileHandle)
      debug("missing ri_collisions.bin");
    else if (lruColl.level.load(riCollision))
      lruColl.init();
    const int loadUsec = get_time_usec(loadRef);
    if (level.resourceCount())
    {
      const int trees = buildSwrt();
      const int allUsec = get_time_usec(loadRef);
      debug("level SWRT: %d BLASes, %d with a tree, built in %dus; the load and the SWRT in %dus", swrt.getBLASCount(), trees,
        allUsec - loadUsec, allUsec);
    }
    daGI2.reset(create_dagi());

    setDirToSun();
    loading_job_mgr_id = cpujobs::create_virtual_job_manager(1, 2 << 20);
    visualConsoleDriver = NULL;
    unloaded_splash = false;

    if (!(d3d::get_texformat_usage(gbuf_fmts[2]) & d3d::USAGE_RTARGET))
      gbuf_fmts[2] = TEXFMT_DEFAULT;

    int w, h;
    d3d::get_render_target_size(w, h, nullptr);
    screen_probes.tileSize = clamp<int>((int(h * 8. / 1080.) + 1) & ~1, 8, 32);
    if (use_snapdragon_super_resolution)
    {
      w = (int)((float)w * snapdragon_super_resolution_scale);
      h = (int)((float)h * snapdragon_super_resolution_scale);
    }


    postfx.init("postfx");
    combine_shadows.init("combine_shadows");
    downsample_depth::init("downsample_depth2x");
    ambientRenderer.init("render_ambient");
    {
      clusteredLights.reset(new ClusteredLights);
      clusteredLights->setResolution(w, h);
      clusteredLights->init(8, 6, false);
      clusteredLights->setMaxClusteredDist(128);
      clusteredLights->setMaxShadowDist(128);
      Point4 bias(-0.00006f, -0.1f, 0.01f, 0.015f);
      clusteredLights->setShadowBias(bias.x, bias.y, bias.z, bias.w);
    }
    {
      OmniLight omni(Point3(9.96434, 2.16349, 2.9432), 2 * Color3(1, 1, 0.5), 5, 0.5);
      auto omniId = clusteredLights->addOmniLight(omni);
      clusteredLights->addShadowToLight(omniId, true, false, 16, 8, 2, DynamicShadowRenderGPUObjects::NO);

      SpotLight spot(Point3(-8.70424, 8.19734, 0.452339), 50 * Color3(0.5, 1, 0.5), 25, 0.5, Point3(-0.655051, -0.755133, -0.0261049),
        0.2, true, true);
      auto spotId = clusteredLights->addSpotLight(spot, SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT);
      clusteredLights->addShadowToLight(spotId, true, false, 16, 8, 2, DynamicShadowRenderGPUObjects::NO);
      clusteredLights->setLight(spotId, spot, SpotLightMaskType::SPOT_LIGHT_MASK_DEFAULT, true);
    }
    initRes(w, h);

    d3d::GpuAutoLock gpu_al;
    initGF();


    currentDownsampledDepth = 0;
    CascadeShadows::Settings csmSettings;
    csmSettings.cascadeWidth = 512;
    csmSettings.splitsW = csmSettings.splitsH = 2;
    csmSettings.shadowDepthBias = 0.034f;
    csmSettings.shadowConstDepthBias = 5e-5f;
    csmSettings.shadowDepthSlopeBias = 0.34f;
    csm = CascadeShadows::make(this, csmSettings);


    taaRender.init("taa");
    if (global_cls_drv_pnt)
      global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, w, h);
    localProbe = NULL;


    daSkies.initSky();
    main_pov_data = eastl::unique_ptr<SkiesData, SkiesDataDeleter>(daSkies.createSkiesData("main"), SkiesDataDeleter{&daSkies});
    cube_pov_data = eastl::unique_ptr<SkiesData, SkiesDataDeleter>(daSkies.createSkiesData("cube"), SkiesDataDeleter{&daSkies});

    initBvh();
    if ((useShaderAsserts = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableShaderAsserts", false)))
      shader_assert::init();
  }
  PostFxRenderer taaRender;
  SnapdragonSuperResolution snapdragonSuperResolutionRender;
  enum
  {
    CLOUD_VOLUME_W = 32,
    CLOUD_VOLUME_H = 16,
    CLOUD_VOLUME_D = 32
  };

  struct SkiesDataDeleter
  {
    DaSkies *owner = nullptr;
    void operator()(SkiesData *data)
    {
      if (owner)
        owner->destroy_skies_data(data);
      else
        G_ASSERT(!data);
    }
  };

  eastl::unique_ptr<SkiesData, SkiesDataDeleter> refl_pov_data, main_pov_data, cube_pov_data;

  eastl::unordered_map<const char *, UniqueTex> rt_textures;

  ~DemoGameScene()
  {
    del_it(occlusion_map);
    // joins the collision gather worker before the level data it culls goes
    teardownBvh(false);
    the_scene = nullptr;
    close_voxelization();

    ssr.reset();
    del_it(csm);
    for (int i = 0; i < farDownsampledDepth.size(); ++i)
    {
      farDownsampledDepth[i].close();
      downsampledNormals[i].close();
      downsampled_close_depth_tex[i].close();
    }
    downsampled_checkerboard_depth_tex.close();
    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(prev_downsampled_close_depth_texVarId, BAD_TEXTUREID);
    downsample_depth::close();

    del_it(test);
    del_it(binScene);
    del_it(visualConsoleDriver);
    light_probe::destroy(localProbe);

    cpujobs::destroy_virtual_job_manager(loading_job_mgr_id, true);
    de3_imgui_term();
    ShaderGlobal::reset_textures();
    del_it(freeCam);
    delayed_binary_dumps_unload();
    if (useShaderAsserts)
      shader_assert::close();
    close_draw_cached_debug();
  }

  virtual bool rayTrace(const Point3 &p, const Point3 &norm_dir, float trace_dist, float &max_dist) { return false; }

  virtual void actScene()
  {
    cpujobs::release_done_jobs();
    samplebenchmark::quitIfBenchmarkHasEnded();
    webui::update();

    static String lastExportName;
    if (file_panel.export_min || file_panel.export_max)
    {
      String fileName = de3_imgui_file_save_dlg("Export sky settings", "Blk files|*.blk", "blk", "./", lastExportName);
      if (!fileName.empty())
      {
        DataBlock blk;
        dblk::load(blk, fileName, dblk::ReadFlag::ROBUST);
        skies_utils::save_daSkies(daSkies, blk, file_panel.export_min, *blk.addBlock("level"));
        if (!blk.saveToTextFile(fileName))
        {
          G_ASSERTF(0, "Can't save file");
        }
        lastExportName = fileName;
      }
      file_panel.export_min = false;
      file_panel.export_max = false;
    }
    if (file_panel.import_minmax)
    {
      String fileName = de3_imgui_file_open_dlg("Import sky settings", "Blk files|*.blk", "blk", "./", lastExportName);
      DataBlock blk;
      if (!fileName.empty())
      {
        if (!blk.load(fileName))
        {
          G_ASSERTF(0, "Can't load file");
        }
        else
        {
          skies_utils::load_daSkies(daSkies, blk, grnd(), *blk.getBlockByNameEx("level"));

          ((SkyAtmosphereParams &)sky_panel) = daSkies.getSkyParams();
          sky_colors.mie_scattering_color = sky_panel.mie_scattering_color;
          sky_colors.moon_color = sky_panel.moon_color;
          sky_colors.mie_absorption_color = sky_panel.mie_absorption_color;
          sky_colors.rayleigh_color = sky_panel.rayleigh_color;
          layered_fog.mie2_thickness = sky_panel.mie2_thickness;
          layered_fog.mie2_altitude = sky_panel.mie2_altitude;
          layered_fog.mie2_scale = sky_panel.mie2_scale;
          //((DaSkies::CloudsGen&)clouds_gen) = daSkies.getCloudsGen();
          //((DaSkies::CloudsPosition&)clouds_panel) = daSkies.getCloudsPosition();
          clouds_rendering2 = daSkies.getCloudsRendering();
          clouds_weather_gen2 = daSkies.getWeatherGen();
          clouds_game_params = daSkies.getCloudsGameSettingsParams();
          clouds_form = daSkies.getCloudsForm();
          strata_clouds = daSkies.getStrataClouds();
          lastExportName = fileName;
        }
      }
      file_panel.import_minmax = false;
    }


    static String lastLoadName;
    if (file_panel.save)
    {
      file_panel.save = false;
      String fileName = de3_imgui_file_save_dlg("Save sky settings", "Blk files|*.blk", "blk", "./", lastLoadName);
      DataBlock blk;
      dblk::load(blk, fileName, dblk::ReadFlag::ROBUST);
      de3_imgui_save_values(*blk.addBlock("gui"));
      if (!fileName.empty() && blk.saveToTextFile(fileName))
        lastLoadName = fileName;
    }
    if (file_panel.load)
    {
      String fileName = de3_imgui_file_open_dlg("Load sky settings", "Blk files|*.blk", "blk", "./", lastLoadName);
      DataBlock blk;
      if (!fileName.empty() && blk.load(fileName))
      {
        de3_imgui_load_values(*blk.addBlock("gui"));
        lastLoadName = fileName;
      }
      file_panel.load = false;
    }

    if (!exit_button_pressed && !de3_imgui_act(::dagor_game_act_time))
      curCamera->act();
    const bool panorama = render_panel.render_type == PANORAMA;
    de3_imgui_enable_obj(&render_panel.sky_quality, !panorama);
    de3_imgui_enable_obj(&render_panel.direct_quality, !panorama);
    de3_imgui_enable_obj(&render_panel.infinite_skies, !panorama);
    de3_imgui_enable_obj(&render_panel.panorama_blending, panorama);
    de3_imgui_enable_obj(&render_panel.panoramaResolution, panorama);
    if (exit_button_pressed)
    {
      quit_game(1);
      return;
    }
  }

  ProfileUniqueEventsGUI allEvents;
  void drawProfile() { allEvents.drawProfiled(); }
  using FrameStr = eastl::basic_string<char, framemem_allocator>;
  FrameStr rtssPagesLine(const RaytracedToroidalStaticShadow::Stats &s) const
  {
    return FrameStr(FrameStr::CtorSprintf(),
      "rtss: pages required %.2f dirty %.2f sweep %.2f pretrace %.2f fill %.2f edge %.2f, rays %u, sun %.3f deg/s%s%s",
      s.pages[RTSS_REQUIRED], s.pages[RTSS_DIRTY], s.pages[RTSS_SWEEP], s.pages[RTSS_PRETRACE], s.pages[RTSS_FILL], s.pagesEdge,
      s.rays, s.sunRateDegPerSec, s.sunBelowHorizon ? ", below horizon" : "", freeze ? ", frozen: of the last update" : "");
  }
  void drawRtssStats()
  {
    if (!rtss_panel.show_stats)
      return;
    RaytracedToroidalStaticShadow::Stats s;
    rtss->getStats(s);
    const FrameStr pages = rtssPagesLine(s);
    FrameStr levels;
    for (auto [i, l] : enumerate(s.levels))
    {
      levels.append_sprintf("%sL%d %.2f/%.2f deg ", i ? " | " : "", int(i), l.sunErrorDeg, l.toleranceDeg);
      if (l.sweeping)
        levels.append_sprintf("%d%%%s", int(l.sweepProgress * 100), l.fastSweep ? " F" : "");
      else
        levels += "idle";
    }
    StdGuiRender::ScopeStarterOptional guiScope;
    StdGuiRender::set_font(0);
    StdGuiRender::set_ablend(true);
    const int fontHt = StdGuiRender::get_def_font_ht(0), y = int(StdGuiRender::screen_height()) - fontHt * 3;
    StdGuiRender::set_draw_str_attr(FFT_SHADOW, 1, 1, E3DCOLOR(0, 0, 0, 255), 16);
    StdGuiRender::set_color(E3DCOLOR(255, 255, 255, 255));
    StdGuiRender::goto_xy(30, y);
    StdGuiRender::draw_str(pages.c_str());
    StdGuiRender::goto_xy(30, y + fontHt * 3 / 2);
    StdGuiRender::draw_str(levels.c_str());
    StdGuiRender::reset_draw_str_attr();
    StdGuiRender::flush_data();
  }

  virtual void drawScene()
  {
    int sw, sh;
    d3d::get_render_target_size(sw, sh, frame.getTex2D());
    G_ASSERT(sw != 0);
    G_ASSERT(sh != 0);
    int w, h;
    d3d::get_render_target_size(w, h, nullptr);
    G_ASSERT(w != 0);
    G_ASSERT(h != 0);

    StdGuiRender::reset_per_frame_dynamic_buffer_pos();

    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    if (land_panel.trees != treesInGI && daGI2) // trees appeared or went away, as a game does on destruction
    {
      treesInGI = land_panel.trees;
      if (test) // no model = empty box, and an empty BBox3 is +/-FLT_MAX
        daGI2->invalidateBox(treesWorldBox());
      else // a toggle while the model reloads has no box: setModel runs the clear when it returns
        treesMediaClearPending = true;
    }
    static int64_t time0 = ref_time_ticks();
    int current_time_usec = get_time_usec(time0);
    double realTime = current_time_usec / 1000000.0;
    set_shader_global_time(realTime);
    static int taaFrame = 0;
    bool renderTaa = taa.get() || enable_taa_override;
    // logwarn("[hmatthew] renderTaa = %s", renderTaa ? "TRUE" : "FALSE");

    Driver3dPerspective p;
    d3d::getpersp(p);
    const TMatrix view = orthonormalized_inverse(itm);

    if (renderTaa)
    {
      TemporalAAParams taaParams;
      taaParams.subsamples = taa_subsamples.get();
      taaParams.sharpening = taa_sharpening.get();
      taaParams.useHalton = taa_halton.get();
      taaParams.newframeFalloffStill = taa_newframe_falloff.get();
      taaParams.frameWeightMinStill = taa_frame_weight_min.get();
      taaParams.motionDifferenceMax = taa_motion_difference_max.get();
      taaParams.motionDifferenceMaxWeight = taa_motion_difference_max_weight.get();
      taaParams.clampingGamma = taa_clamping_gamma.get();
      taaParams.frameWeightMinDynamicStill = taa_frame_weight_dynamic_min.get();
      taaParams.newframeFalloffDynamicStill = taa_newframe_dynamic_falloff.get();
      TMatrix4 noJitterGlobTm;
      TMatrix4 noJitterProj;
      d3d::calcproj(p, noJitterProj);
      d3d::calcglobtm(view, noJitterProj, noJitterGlobTm);
      Point2 jitteredOfs = get_taa_jitter(taaFrame, taaParams);
      p.ox += jitteredOfs.x * (2.0f / w);
      p.oy += -jitteredOfs.y * (2.0f / h);
      static TMatrix4 prevNoJitterGlobTm = noJitterGlobTm;
      static int64_t reft = 0;
      int usecs = get_time_usec(reft);
      reft = ref_time_ticks();

      // @TODO: unified reprojection matrix calculation in gameLibs?
      //        Currently daNetGame does it's thing, skyquake moved to it's internal function,
      //        and the following logic was removed from temporalAA
      TMatrix4D reprojection;
      TMatrix4D noJitterGlobTmInv;

      double det;
      G_VERIFY(inverse44(noJitterGlobTm, noJitterGlobTmInv, det));

      {
        reprojection = noJitterGlobTmInv * prevNoJitterGlobTm;
        TMatrix4D scaleBias1 = {0.5, 0, 0, 0, 0, -0.5, 0, 0, 0, 0, 1, 0, 0.5, 0.5, 0, 1};
        TMatrix4D scaleBias2 = {2.0, 0, 0, 0, 0, -2.0, 0, 0, 0, 0, 1, 0, -1.0, 1.0, 0, 1};
        reprojection = scaleBias2 * reprojection * scaleBias1;
      }

      set_temporal_parameters((TMatrix4)reprojection, jitteredOfs, taaFrame == 0 ? 10.f : usecs / 1e6, w, taaParams);
      prevNoJitterGlobTm = noJitterGlobTm;
    }

    alignas(16) TMatrix4 projTm;
    d3d::calcproj(p, projTm);
    RenderView frameView(view, projTm, itm.getcol(3));
    if (!stop_camera || !frozenView)
      frozenView = eastl::make_pair(frameView.position, frameView.culling);
    RenderView geometryView = frameView;
    geometryView.position = frozenView->first;
    geometryView.culling = frozenView->second;
    const Geometry geom = geometry();
    frameView.bind();
    {
      alignas(16) TMatrix4 viewTm = TMatrix4(view);
      mat44f globtm;
      d3d::calcglobtm((mat44f_cref)viewTm, (mat44f_cref)projTm, globtm);
      SCOPE_VIEW_PROJ_MATRIX;
      // can be done in thread
      clusteredLights->cullFrustumLights(v_ldu_p3(itm[3]), globtm, (mat44f_cref)viewTm, (mat44f_cref)projTm, p.zn, p.zf, nullptr,
        SpotLightMaskType::SPOT_LIGHT_MASK_NONE, OmniLightMaskType::OMNI_LIGHT_MASK_NONE);

      vec4f vpos = v_ldu_p3(itm[3]);
      bbox3f dynBox = {v_sub(vpos, v_splats(5)), v_add(vpos, v_splats(5))};
      if (invalidate_dynamic_shadows)
      {
        invalidate_dynamic_shadows = false;
        clusteredLights->invalidateAllShadows();
      }

      DynLightsOptimizationMode dynLightsMode =
        dynamic_lights.get() ? clusteredLights->getLightsCountInterval() : DynLightsOptimizationMode::NO_LIGHTS;

      ShaderGlobal::set_int(dynamic_lights_countVarId, eastl::to_underlying(dynLightsMode));
      // Grid fill is deferred to render(), after the depth downsample: the clustered
      // occlusion z-slice cull reads downsampled_far_depth_tex and must see this
      // frame's depth, otherwise lights are mis-culled while the camera moves.

      dynamic_shadow_render::FrameVolumeData volumesToRender;
      clusteredLights->framePrepareShadows(volumesToRender, itm.getcol(3), globtm, p.hk, make_span_const(&dynBox, 1), false);

      auto globalFrameId = ShaderGlobal::getBlockId("global_frame");
      ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
      clusteredLights->frameRenderShadows(
        volumesToRender,
        [&](mat44f_cref globTm, mat44f_cref projTm, const TMatrix &viewItm, int, int, DynamicShadowRenderGPUObjects) {
          ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
          RenderView shadow(TMatrix::IDENT, reinterpret_cast<const TMatrix4 &>(globTm), geometryView.position);
          shadow.lodProj = geometryView.proj;
          renderOpaque(geom, shadow, true);
          ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
        },
        [](const TMatrix &view_itm, const mat44f &view_tm, const mat44f &proj_tm, int, int) {
          G_UNUSED(view_itm);
          G_UNUSED(view_tm);
          G_UNUSED(proj_tm);
        });
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    }
    const bool hwRt = hwRtAvailable();
    const bool hwRtGi = hwRt && hw_rt_gi.get();
    ShaderGlobal::set_int(use_hw_rt_giVarId, hwRtGi ? 1 : 0);
    if (hwRtGi || rtssHwLevelMask())
      bvh::bind_resources(bvhCtx, w); // the collision TLAS and its normals pool ride the regular bind

    updateHmapMaxGrid(itm.getcol(3));
    updateRtss(itm.getcol(3), realTime);
    // todo: clamp itm.getcol(3) with shrinked sceneBox - there is nothing interesting outside scene
    updateSSGIPos(itm.getcol(3)); // constraint: after updateRtss, which moves the rtss to the camera
    auto ru = gi_panel.update_radiance && giUpdatePosFrameCounter == 0 ? DaGI::RadianceUpdate::On : DaGI::RadianceUpdate::Off;
    daGI2->beforeRender(sw, sh, w, h, itm, projTm, p.zn, p.zf, ru);
    // media volumes own the tree canopy density: append the instances a box touches, and bake
    // a pending type through the VegetationMediaVoxelizer bake shader (see voxelizedMedia.h)
    auto gatherMediaInstances = [&](const BBox3 &box, Tab<DaGIMediaVolumeInstance> &out) {
      if (!land_panel.trees || treeMediaVolType < 0)
        return;
      foreachTreeInstance([&](const TMatrix &tm, const BBox3 &world) {
        if (box & world)
          out.push_back({tm.getcol(3), Point3(1, 1, 1), Quat(0, 0, 0, 1), (uint32_t)treeMediaVolType});
      });
    };
    auto bakeMediaType = [&](int type) -> bool {
      if (!test || !test->getLodsResource()->lods[0].scene)
        return false; // no mesh in lod0 yet (a stub still streaming): retry later
      // the lib draws the mesh with its own vegetation bake shader, at the model origin:
      // neither the node transforms nor the model's own shaders take part
      bool streamed = true;
      test->getLodsResource()->lods[0].scene->getMeshes(
        [&](const ShaderMesh *mesh, int, float, int) {
          streamed &= mediaVegVoxelizer.voxelize(*mesh) != VegetationMediaVoxelizer::Status::NotStreamed;
        },
        [&](const ShaderSkinnedMesh *, int, int) {}); // a skinned part has no rest pose to bake
      return streamed;
    };
    if (mediaVolumes.bakePending(bakeMediaType)) // the bake wave settled: refill for the new bricks
      daGI2->invalidateInitialMedia();
    {
      bool lightsInsideFrustum = true;
      daGI2->updatePosition(
        [&](int sdf_clip, const BBox3 &box, float voxelSize, uintptr_t &) {
          BBox3 sceneBox = box;
          sceneBox[0] -= Point3(voxelSize, voxelSize, voxelSize); // we increase by maximum written + 1 where we rounding due to
                                                                  // rasterization
          sceneBox[1] += Point3(voxelSize, voxelSize, voxelSize);

          dag::Vector<uint64_t, framemem_allocator> handles;
          {
            TIME_D3D_PROFILE(sdf_from_collision)
            // sceneBox[0] -= box.width()*0.25f;
            // sceneBox[1] += box.width()*0.25f;
            bbox3f vbox;
            vbox.bmin = v_ldu(&sceneBox[0].x);
            vbox.bmax = v_ldu(&sceneBox[1].x);
            // vbox.bmin = v_sub(vbox.bmin, v_splats(voxelSize*2));//we rasterize with up 2 voxels dist
            // vbox.bmax = v_add(vbox.bmax, v_splats(voxelSize*2));//we rasterize with up 2 voxels dist
            lruColl.gatherBox(vbox, handles);
          }
          const bool intersectLevel = loadedLevelBox & box & rasterize_sdf_level;
          if (handles.empty() && !intersectLevel)
            return UpdateGiQualityStatus::NOTHING;

          if (clusteredLights->initialized())
          {
            if (daGI2->sdfClipHasVoxelsLitSceneClip(sdf_clip))
            {
              alignas(16) TMatrix4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
              clusteredLights->cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightMaskType::SPOT_LIGHT_MASK_GI,
                OmniLightMaskType::OMNI_LIGHT_MASK_GI);
              clusteredLights->setOutOfFrustumLightsToShader();
            }
            else
              clusteredLights->setEmptyOutOfFrustumLights();
            lightsInsideFrustum = false;
          }

          Point3 voxelize_box0 = sceneBox[0];
          Point3 voxelize_box_sz = max(Point3(1e-6f, 1e-6f, 1e-6f), sceneBox.width());
          IPoint3 res = ipoint3(floor(sceneBox.width() / voxelSize + Point3(0.5, 0.5, 0.5)));
          // Point3 voxelize_box1 = Point3(1./sceneBox.width().x, 1./sceneBox.width().y, 1./sceneBox.width().z);
          const int maxRes = max(max(res.x, res.y), max(res.z, 1));
          Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / maxRes; // use fixed aspect ratio of 1. for
                                                                                      // oversampling and HW clipping
          Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
          Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
          ShaderGlobal::set_float4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
          ShaderGlobal::set_float4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);
          if (handles.size())
          {
            if (voxelSize <= world_sdf_from_collision_rasterize_below)
            {
              SCOPE_RENDER_TARGET;
              set_voxelization_target_and_override(maxRes * world_sdf_rasterize_supersample, maxRes * world_sdf_rasterize_supersample);
              bool prims = rasterize_sdf_prims.get() && !d3d::get_driver_desc().caps.hasBarycentrics;
              lruColl.voxelize.rasterizeSDF(*lruColl.lruColl, dag::Span<uint64_t>(handles.data(), handles.size()), prims);
              reset_voxelization_override();
            }
            else
            {
              lruColl.voxelize.voxelizeSDFCompute(*lruColl.lruColl, dag::Span<uint64_t>(handles.data(), handles.size()));
            }
          }
          if (intersectLevel)
          {
            SCOPE_VIEW_PROJ_MATRIX;
            SCOPE_RENDER_TARGET;
            renderVoxelsMediaGeom(sceneBox, Point3(voxelSize, voxelSize, voxelSize) / world_sdf_rasterize_supersample,
              SCENE_MODE_VOXELIZE_SDF);
          }
          return UpdateGiQualityStatus::RENDERED;
        },
        [&](const BBox3 &box, float voxelSize, uintptr_t &) {
          const bool intersectLevel = loadedLevelBox & box;
          if (!intersectLevel)
            return UpdateGiQualityStatus::NOTHING;
          if (intersectLevel)
          {
            SCOPE_VIEW_PROJ_MATRIX;
            SCOPE_RENDER_TARGET;
            renderVoxelsMediaGeom(box, Point3(voxelSize, voxelSize, voxelSize), SCENE_MODE_VOXELIZE_ALBEDO);
          }
          return UpdateGiQualityStatus::RENDERED;
        },
        [&](const BBox3 &region, float voxelSize) { mediaVolumes.setRegionInstances(region, voxelSize, gatherMediaInstances); });
      if (!lightsInsideFrustum)
        clusteredLights->setInsideOfFrustumLightsToShader();
    }
    // gather the closest source instances for the media volume debug trace (gi_media_vols_debug)
    mediaVolumes.debugPrepare(itm.getcol(3), gatherMediaInstances);

    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {{frame.getTex2D(), 0, 0}});
    render(geom, frameView, geometryView, itm, p, hwRtGi, w);

    frameView.bind();
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {{frame.getTex2D(), 0, 0}});
    daGI2->afterFrameRendered(DaGI::FrameData(DaGI::FrameHasAll));
    frameView.bind();
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::SampledRO, {{frame.getTex2D(), 0, 0}});
    renderWater(frameView);
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {{frame.getTex2D(), 0, 0}});
    renderTrans(frameView);
    frameView.bind();
    daGI2->debugRenderTrans();
    mediaVolumes.debugRender(); // the debug pass view vars are set by debugRenderTrans above
    ShaderGlobal::set_int(dynamic_lights_countVarId, 0);

    if (::grs_draw_wire)
      d3d::setwire(0);

    if (renderTaa)
    {
      int taaHistoryI = taaFrame & 1;
      d3d::set_render_target({}, DepthAccess::RW, {{taaHistory[1 - taaHistoryI].getTex2D(), 0, 0}});
      ShaderGlobal::set_texture(get_shader_variable_id("taa_history_tex"), taaHistory[taaHistoryI]);
      ShaderGlobal::set_texture(get_shader_variable_id("taa_frame_tex"), frame);
      d3d::SamplerInfo smpInfo;
      smpInfo.filter_mode = d3d::FilterMode::Point;
      ShaderGlobal::set_sampler(get_shader_variable_id("taa_frame_tex_samplerstate"), d3d::request_sampler(smpInfo));
      taaRender.render();
      ShaderGlobal::set_texture(frame.getVarId(), taaHistory[1 - taaHistoryI]);
    }
    else
      frame.setVar();

    if (use_snapdragon_super_resolution)
    {
      snapdragonSuperResolutionRender.render(frame.getTex2D(), superResolutionOutput.getTex2D());
      ShaderGlobal::set_texture(frame.getVarId(), superResolutionOutput);
    }

    if (renderTaa)
      taaFrame++;
    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();

    if (verifyCaptureRequested)
    {
      // end of frame: the gbuffer and shader globals still hold this frame's
      // state, exactly what the giVerifier replay capture must snapshot
      verifyCaptureRequested = false;
      const char *dir = verifyCaptureDir.empty() ? "verify_capture" : verifyCaptureDir.c_str();
      // itm and p are this frame's camera and projection, already in hand - no
      // need to ask the driver for state the frame owns
      // df_get_real_name may return a shared buffer
      const String dumpName(df_get_real_name(dgs_get_settings()->getStr("ri_collisions", "ri_collisions.bin")));
      const String levelName(df_get_real_name(dgs_get_settings()->getStr("level", "")));
      bool ok = gi_verify::save_capture(dir, itm, p, target->getWidth(), target->getHeight(), dir_to_sun, dumpName, levelName,
        &gi_verify::write_env_exr);
      console::print_d("save_verify_capture: %s -> %s", ok ? "saved" : "FAILED", dir);
      verifyCaptureDir.clear();
    }

    if (show_gbuffer == DebugGbufferMode::None)
    {
      ShaderGlobal::set_float(get_shader_variable_id("exposure"), gi_panel.exposure);
      d3d::set_render_target();
      TIME_D3D_PROFILE(postfx)
      postfx.render();
      // d3d::stretch_rect(frame.getTex2D(), d3d::get_backbuffer_tex());
    }
    else
      target->debugRender(nullptr, (int)show_gbuffer);

    {
      daGI2->debugRenderScreenDepth();
      daGI2->debugRenderScreen();
    }
    if (rtss_panel.debug == 6)
      rtss->debugDrawAtlas(rtss_panel.debug_level);
    else if (rtss_panel.debug >= 1 && rtss_panel.debug <= 5)
      rtss->debugRenderScreen(rtss_panel.debug);

    debugTexOverlay.render();
    de3_imgui_render();

    if (console::get_visual_driver())
    {
      console::get_visual_driver()->update();
      console::get_visual_driver()->render();
    }
    // if (::dagor_gui_manager)

    {
      TIME_D3D_PROFILE(gui);
      drawProfile();
      drawRtssStats();
    }
    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
    TMatrix4 globtm;
    d3d::calcglobtm(view, projTm, globtm);
    TMatrix4 globtmTr = globtm.transpose();
    ShaderGlobal::set_float4(prev_globtm_psf_0VarId, Color4(globtmTr[0]));
    ShaderGlobal::set_float4(prev_globtm_psf_1VarId, Color4(globtmTr[1]));
    ShaderGlobal::set_float4(prev_globtm_psf_2VarId, Color4(globtmTr[2]));
    ShaderGlobal::set_float4(prev_globtm_psf_3VarId, Color4(globtmTr[3]));
    if (useShaderAsserts)
      shader_assert::readback();

    if (screenshotRequested)
    {
      screenshotRequested = false;
      ScreenShotSystem::ScreenShot screen;
      if (ScreenShotSystem::makeScreenShot(screen))
      {
        if (!screenshotFilename.empty())
        {
          if (ScreenShotSystem::saveScreenShotTo(screen, screenshotFilename))
            console::print_d("screenshot saved to %s", screenshotFilename.str());
          else
            console::print_d("screenshot: failed to save to %s", screenshotFilename.str());
        }
        else
        {
          if (ScreenShotSystem::saveScreenShot(screen))
            console::print_d("screenshot saved: %s", ScreenShotSystem::lastMessage());
          else
            console::print_d("screenshot: failed to save");
        }
      }
      else
        console::print_d("screenshot: failed to capture screen");
    }
  }

  void giQualitySet()
  {
    ShaderGlobal::set_int(get_shader_variable_id("gi_quality"), (int)gi_panel.gi_mode);
    auto s = daGI2->getNextSettings();
    s.voxelScene.sdfResScale = 0.5;
    s.voxelScene.onlyLuma = false;
    s.albedoScene.clips = albedo_scene_panel.clips;
    s.radianceGrid.clips = radiance_grid_panel.clips;
    s.radianceGrid.w = radiance_grid_panel.texWidth;

    if (gi_panel.gi_mode == ONLY_AO)
    {
      s.skyVisibility.clipW = 64;
      s.albedoScene.clips = 0;
      s.radianceGrid.w = 0;
      s.screenProbes.tileSize = 0;
      s.volumetricGI.tileSize = 0;
      s.voxelScene.onlyLuma = true;
      // s.voxelScene.sdfResScale = 0;
    }
    else if (gi_panel.gi_mode == SIMPLE_COLORED)
    {
      s.skyVisibility.clipW = 0;
      s.radianceGrid.irradianceProbeDetail = 2.f;
      s.radianceGrid.additionalIrradianceClips = 2;
      s.screenProbes.tileSize = 0;
      s.volumetricGI.tileSize = volumetric_gi.tileSize;
      s.volumetricGI.slices = volumetric_gi.slices;
      s.volumetricGI.zLogMul = volumetric_gi.zLogMul;
    }
    else if (gi_panel.gi_mode == SCREEN_PROBES)
    {
      s.skyVisibility.clipW = 0;
      s.radianceGrid.irradianceProbeDetail = 1.f;
      s.radianceGrid.additionalIrradianceClips = 0;
      s.screenProbes.tileSize = screen_probes.tileSize;
      s.screenProbes.temporality = screen_probes.temporality;
      s.screenProbes.radianceOctRes = screen_probes.radianceOctRes;
      s.screenProbes.angleFiltering = screen_probes.angleFiltering;
      s.volumetricGI.tileSize = volumetric_gi.tileSize;
      s.volumetricGI.slices = volumetric_gi.slices;
      s.volumetricGI.zLogMul = volumetric_gi.zLogMul;
    }
    s.volumetricGI.historyBlurTexelOfs = volumetric_gi.blurTexelOfs;
    s.sdf.voxel0Size = sdf_panel.voxel0Size;
    s.sdf.clips = sdf_panel.clips;
    s.sdf.texWidth = sdf_panel.texWidth;
    s.sdf.yResScale = sdf_panel.yResScale;
    ShaderGlobal::set_int(gi_debug_froxelsVarId, (gi_panel.gi_mode != ONLY_AO && volumetric_gi.debugUse) ? 1 : 0);
    ShaderGlobal::set_float(media_shadow_distVarId, media_shadow_dist.get());
    if (gi_panel.gi_mode != ENVI_PROBE)
      daGI2->setSettings(s);
  }
  int buildSwrt()
  {
    const LevelCollision &level = lruColl.level;
    swrt.init();
    // constraint: no SWRT_SHADOW_SKIP_LEAF here, so the glass faces are carved out
    auto opaque = [&](int16_t mat_id) { return !level.isLightTransparent(mat_id); };
    const int trees = build_level_swrt_models(swrt, level, modelIndex, opaque);
    swrt.copyToGPUAndDestroy(swrt.buildBottomLevelStructures());
    swrt.clearBLASSourceData();
    remakeSWRTInstances(Point3(0, 0, 0), 0.f);
    swrt.clearTLASSourceData();
    return trees;
  }

  // a full reset loses the gpu data that the scene uploads once at load
  void sceneAfterDeviceReset()
  {
    daSkies.reset();
    daSkies.skiesDataScatteringVolumeBarriers(main_pov_data.get());
    if (clusteredLights)
      clusteredLights->afterResetDevice();
    if (binScene)
      binScene->afterDeviceReset();
    if (lruColl.lruColl)
      lruColl.lruColl->reset();
    if (swrt.hasData())
      buildSwrt();
    preIntegratedGF.reset();
    delayed_call([this] { redrawSceneGpuData(); }); // it draws, so it runs after the reset
  }

  void remakeSWRTInstances(const Point3 &at, float rel_sz)
  {
    swrt.clearTLASSourceData();
    const LevelCollision &level = lruColl.level;
    vec4f posScale = v_perm_xyzd(v_ldu(&at.x), v_splats(rel_sz * rel_sz));
    for (uint32_t r = 0, e = modelIndex.size(); r < e; ++r)
    {
      if (modelIndex[r] < 0)
        continue;
      for (uint32_t i = level.firstInstance(r), ie = level.firstInstance(r + 1); i < ie; ++i)
      {
        vec4f sph = level.sphere(i);
        if (v_extract_w(sph) < LEVEL_SWRT_MIN_RADIUS)
          continue;

        vec3f distSq = v_distance_xyz_x(sph, posScale);
        if (v_test_vec_x_lt(v_splat_w(v_mul(sph, sph)), v_mul_x(distSq, v_splat_w(posScale))))
          continue;
        swrt.addInstance(modelIndex[r], level.transform(i));
      }
    }
    swrt.copyToGPUAndDestroy(swrt.buildTopLevelStructures());
    rtss->invalidate();
  }
  virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
  {
    extern float hmap_water_level;
    hmap_water_level = water_panel.water_level;
    ShaderGlobal::set_float(water_levelVarId, hmap_water_level);

    if (binScene && rerender_hmap > 15 && binScene->heightmap.getTexture())
    {
      BBox3 lbox;
      if (!binScene->getLmeshBBox3(lbox))
        lbox = binScene->heightmap.getWorldBox();
      {
        static int c = 0;
        String s(32, "hmap_rendered_%d", c++);
        UniqueTex heightmap = UniqueTex(dag::create_tex(NULL, rerender_hmap, rerender_hmap, TEXFMT_L16 | TEXCF_RTARGET, 1, s.c_str()));
        PostFxRenderer copy_hmap_depth("copy_hmap_depth");
        ShaderGlobal::set_texture(get_shader_variable_id("source_hmap"), binScene->heightmap.getTexture()->getTexId());
        ShaderGlobal::set_float4(get_shader_variable_id("dest_dim"), rerender_hmap, rerender_hmap, 0, 0);
        SCOPE_VIEW_PROJ_MATRIX;
        SCOPE_RENDER_TARGET;
        d3d::set_render_target({}, DepthAccess::RW, {{heightmap.getTex2D(), 0, 0}});
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
        copy_hmap_depth.render();
        if (rerender_lmesh)
        {
          lbox[0].y = binScene->heightmap.getHeightMin() + rerender_lmesh_offset;
          lbox[1].y = lbox[0].y + binScene->heightmap.getHeightScale();
          shaders::OverrideState state;
          Point3 wid = lbox.width();
          lbox[0].x += wid.x * 0.5 / rerender_hmap;
          lbox[1].x += wid.x * 0.5 / rerender_hmap;
          lbox[0].z += wid.z * 0.5 / rerender_hmap;
          lbox[1].z += wid.z * 0.5 / rerender_hmap;

          ShaderGlobal::set_int(get_shader_variable_id("landmesh_patch_render"), 1);
          renderLandMeshDepthInternal(lbox, true);
          ShaderGlobal::set_int(get_shader_variable_id("landmesh_patch_render"), 0);
        }
        if (auto lockedTex = lock_texture<const uint16_t>(heightmap.getTex2D(), 0, TEXLOCK_READ))
        {
          Tab<uint16_t> data;
          data.resize(rerender_hmap * rerender_hmap);
          for (int y = 0; y < rerender_hmap; ++y)
            memcpy(data.data() + y * rerender_hmap, &lockedTex.at(0, y), rerender_hmap * 2);
          float cellSize = binScene->heightmap.getHeightmapCellSize();
          cellSize *= float(binScene->heightmap.getHeightmapSizeX()) / rerender_hmap;
          binScene->heightmap.initRaw(data.data(), cellSize, rerender_hmap, rerender_hmap, binScene->heightmap.getHeightMin(),
            binScene->heightmap.getHeightScale(), Point2::xz(binScene->heightmap.getHeightmapOffset()));
          binScene->heightmap.initRender(false, hmap_water_level);
          if (binScene->heightmap.heightmapHeightCulling.get())
            binScene->heightmap.heightmapHeightCulling.get()->init(&binScene->heightmap);
          if (save_rerendered_hmap)
          {
            FullFileSaveCB cb("rerendered.r32", DF_WRITE | DF_CREATE);
            if (cb.fileHandle)
            {
              for (int i = 0; i < rerender_hmap; ++i)
              {
                int y = rerender_hmap - 1 - i;
                for (int x = 0; x < rerender_hmap; ++x)
                {
                  float v = data[y * rerender_hmap + x] * binScene->heightmap.getHeightScaleRaw() + binScene->heightmap.getHeightMin();
                  cb.write(&v, sizeof(v));
                }
              }
            }
          }
        }

        // binScene->heightmap.setTexture((UniqueTex &&) heightmap);
        giHmapMaxGrid.close(); // the content changed in place: refill the window
        rtss->invalidate();
        rerender_hmap = 0;
      }
    }
    giQualitySet();
    if (gi_reset)
    {
      daGI2->afterReset(); // the media bricks stay baked, the refill re-injects them
      giHmapMaxGrid.close();
      gi_reset = false;
    }
    static float prev_pre_exposure = 1.f;
    ShaderGlobal::set_float(get_shader_variable_id("prev_pre_exposure", true), prev_pre_exposure);
    // artificially add randomness, to ensure we correctly decode previous frame
    // in normal circumstances it is easy to miss correct decoding,
    // as all exposure is changing gradually and prev frame is similar to current one
    const float new_pre_exposure = sqrt(gi_panel.exposure) * (gfrnd() * 0.5 + 0.5);
    ShaderGlobal::set_float(get_shader_variable_id("pre_exposure"), prev_pre_exposure = new_pre_exposure);
    TIME_D3D_PROFILE(beforeDraw);
    preIntegratedGF.setFramesCount(gf_frames);
    preIntegratedGF.update();
    static bool scene = false;

    if (scene != land_panel.level)
    {
      scene = land_panel.level;
      onSceneLoaded();
    }
    de3_imgui_enable_obj(&sun_panel.sun_azimuth, !sun_panel.astronomical);
    de3_imgui_enable_obj(&sun_panel.sun_zenith, !sun_panel.astronomical);
    de3_imgui_enable_obj(&sun_panel.moon_age, !sun_panel.astronomical);
    animateSun(realtime_elapsed_usec * 1e-6f);
    setDirToSun();
    if (lengthSq(cameraMoveVel) > 0)
    {
      TMatrix itm;
      curCamera->getInvViewMatrix(itm);
      itm.setcol(3, itm.getcol(3) + cameraMoveVel * (realtime_elapsed_usec * 1e-6f));
      curCamera->setInvViewMatrix(itm);
    }
    current_sphere_time += land_panel.spheres_speed * gametime_elapsed_sec / 100.;
    if (current_sphere_time > 1e5)
      current_sphere_time = 0;


    curCamera->setView();

    float windDirX = cosf(DegToRad(render_panel.windDir)), windDirZ = sinf(DegToRad(render_panel.windDir));
    float dt = gametime_elapsed_sec;
    float windX = render_panel.cloudsSpeed * windDirX * dt, windZ = render_panel.cloudsSpeed * windDirZ * dt;
    // debug("%d %f", realtime_elapsed_usec, gametime_elapsed_sec);
    daSkies.setWindDirection(Point2(windDirX, windDirZ));
    daSkies.setCloudsOrigin(daSkies.getCloudsOrigin().x + windX, daSkies.getCloudsOrigin().y + windZ);
    daSkies.setStrataCloudsOrigin(daSkies.getStrataCloudsOrigin().x + render_panel.strataCloudsSpeed * windX,
      daSkies.getStrataCloudsOrigin().y + render_panel.strataCloudsSpeed * windZ);

    sky_panel.moon_color = sky_colors.moon_color;
    sky_panel.mie_scattering_color = sky_colors.mie_scattering_color;
    sky_panel.mie_absorption_color = sky_colors.mie_absorption_color;
    sky_panel.rayleigh_color = sky_colors.rayleigh_color;
    sky_panel.mie2_thickness = layered_fog.mie2_thickness;
    sky_panel.mie2_altitude = layered_fog.mie2_altitude;
    sky_panel.mie2_scale = layered_fog.mie2_scale;
    daSkies.setSkyParams(sky_panel);
    daSkies.setCloudsGameSettingsParams(clouds_game_params);
    daSkies.setWeatherGen(clouds_weather_gen2);
    daSkies.setCloudsForm(clouds_form);
    daSkies.setCloudsRendering(clouds_rendering2);
    sky_panel.min_ground_offset = 0;
    // daSkies.generateClouds(clouds_gen, clouds_gen.gpu, clouds_gen.need_tracer, clouds_gen.regenerate);
    // clouds_gen.regenerate = false;
    // daSkies.setCloudsPosition(clouds_panel, 2);
    // daSkies.setCloudsRender(clouds_light);
    daSkies.setStrataClouds(strata_clouds);

    if (render_panel.render_type == PANORAMA)
    {
      daSkies.initPanorama(main_pov_data.get(), render_panel.panorama_blending, render_panel.panoramaResolution);
    }
    else
      daSkies.closePanorama();
    // debug("regen = %d force_update = %d", sky_panel.regen, force_update);
    // sky_panel.regen  = false;
    daSkies.prepare(dir_to_sun, gametime_elapsed_sec);
    dir_to_sun = daSkies.getPrimarySunDir();
    ShaderGlobal::set_float4(from_sun_directionVarId, -dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      sun = lerp(moon, sun, daSkies.getCurrentSunEffect());
      amb = lerp(moonamb, amb, daSkies.getCurrentSunEffect());
      ShaderGlobal::set_float4(get_shader_variable_id("sun_light_color"), color4(sun / PI, 0));
      ShaderGlobal::set_float4(get_shader_variable_id("amb_light_color", true), color4(amb, 0));
    }
    else
    {
      G_ASSERT(0);
    }

    int w, h;
    d3d::get_render_target_size(w, h, nullptr);
    if (use_snapdragon_super_resolution)
    {
      w = (int)((float)w * snapdragon_super_resolution_scale);
      h = (int)((float)h * snapdragon_super_resolution_scale);
    }
    if (gi_panel.fake_dynres)
    {
      static int sframe;
      const int pdiv = 1 - (sframe / 8) & 1;
      const int div = 1 - (++sframe / 8) & 1;
      if (pdiv != div)
        initRes(w / (div + 1), h / (div + 1));
    }
    initReflections(w, h);
    initScreenGI(w, h);
    target->swapDepth(otherDepth);
    target->setVar();
    ShaderGlobal::set_texture(prev_gbuffer_depthVarId, otherDepth.getTexId());

    if (gi_panel.gi_mode == SCREEN_PROBES)
    {
      TextureInfo ti;
      ambient[0].getTex2D()->getinfo(ti);
      initGIReprojection(ti.w, ti.h);
    }
    else
      closeGIReprojection();

    Driver3dPerspective p;
    G_VERIFY(d3d::getpersp(p));
    ShaderGlobal::set_float4(zn_zfarVarId, p.zn, p.zf, 0, 0);
    // target.prepare();
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    TMatrix4 projTm;
    d3d::calcproj(p, projTm);
    const RenderView frameView(orthonormalized_inverse(itm), projTm, itm.getcol(3));
    if (swrt_rebuild_around >= 0)
    {
      remakeSWRTInstances(itm.getcol(3), swrt_rebuild_around);
      swrt_rebuild_around = -1;
    }
    depthAround.setDist(depthAboveAroundDist);
    depthAround.render(itm.getcol(3), [&](const TMatrix4 &view, const BBox2 &box) {
      if (sceneBox.isempty())
      {
        ShaderGlobal::set_float4(get_shader_variable_id("depth_above_heights", true), -100000, 0, 0, 0);
        return;
      }
      TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[0].y, box[1].y, sceneBox[0].y, sceneBox[1].y);
      ShaderGlobal::set_float4(get_shader_variable_id("depth_above_heights", true), sceneBox[0].y, sceneBox[1].y - sceneBox[0].y, 0,
        0);
      renderOpaque(geometry(), RenderView(tmatrix(view), proj, itm.getcol(3)), true, StrmSceneHolder::Hmap::Off);
    });

    if (hmapShadows.cascadesCount() != hmapShadowsCascades || hmapShadows.cascadeWidth() != hmapShadowsSize)
    {
      hmapShadows.init(hmapShadowsSize, hmapShadowsCascades, hmapShadowsDist, hmapShadowsScale, hmapShadows.getHtMinMax(), false);
    }
    else
    {
      hmapShadows.setDist(hmapShadowsDist);
      hmapShadows.setScale(hmapShadowsScale);
    }

    if (test)
      test->setLod(test->chooseLod(itm.getcol(3)));
    ShaderGlobal::set_float4(world_view_posVarId, itm.getcol(3).x, itm.getcol(3).y, itm.getcol(3).z, 1);
    CascadeShadows::ModeSettings mode;
    mode.powWeight = 0.8;
    mode.maxDist = 50.f;
    mode.shadowStart = p.zn;
    mode.numCascades = 4;
    csm->prepareShadowCascades(mode, dir_to_sun, frameView.view, itm.getcol(3), projTm, Frustum(frameView.culling), Point2(p.zn, p.zf),
      p.zn);
    de3_imgui_before_render();

    buildBvh(itm);

    if (hmapShadowsSteps == 0)
      hmapShadows.invalidate();
    hmapShadows.render(
      itm.getcol(3), -dir_to_sun,
      [&](const BBox2 &needed_full, const BBox2 &needed_now, uint32_t ci, bool is_same) {
        TIME_D3D_PROFILE(lmesh_for_hmap);
        BBox3 lbox;
        if (!lmesh_depth_current || !binScene || !binScene->getLmeshBBox3(lbox))
        {
          setLandMeshDepthVars(lmesh_depth_world.getTexId(), lbox, true);
          return;
        }
        if (is_same)
          return;
        TextureInfo ti, tiW;
        lmesh_depth_current.getTex2D()->getinfo(ti, 0);
        if (lmesh_depth_world)
          lmesh_depth_world.getTex2D()->getinfo(tiW, 0);
        BBox3 box(Point3(needed_full[0].x, lbox[0].y, needed_full[0].y), Point3(needed_full[1].x, lbox[1].y, needed_full[1].y));
        // add alignment
        Point2 texelSize = needed_full.width() / (ti.w - 1);
        if (lmesh_depth_world && (ci == hmapShadows.cascadesCount() - 1 ||
                                   max(texelSize.x, texelSize.y) > min(lbox.width().x / tiW.w, lbox.width().z / tiW.w)))
        {
          setLandMeshDepthVars(lmesh_depth_world.getTexId(), lbox, true);
          return;
        }
        // this rendering can be replaced with toroidal update of texture array.
        // This would use just a bit of additional memory (+1mb VRAM) and also writing a bit of code
        // debug("%d: texel %f, box0.x %f", ci, texelSize.x, floorf(box[0].x/texelSize.x));
        box[0].x = floorf(box[0].x / texelSize.x) * texelSize.x;
        box[0].z = floorf(box[0].z / texelSize.y) * texelSize.y;
        box[1].x = box[0].x + texelSize.x * ti.w;
        box[1].z = box[0].z + texelSize.y * ti.w;
        renderLandMeshDepth(lmesh_depth_current.getTex2D(), box);
        setLandMeshDepthVars(lmesh_depth_current.getTexId(), box, false);
        // debug("cascade %d needs box %@ %@ texel %@, final box %@ %@", ci, needed_full, needed_now, texelSize, box, box.width());
      },
      hmapShadowsSteps);
  }

  virtual void sceneSelected(DagorGameScene * /*prev_scene*/) { loadScene(); }

  virtual void sceneDeselected(DagorGameScene * /*new_scene*/)
  {
    webui::shutdown();
    console::set_visual_driver(NULL, NULL);
    delete this;
  }
  DynamicShaderHelper drawSpheres;
  float current_sphere_time = 0;
  static void renderDynamicSpheres(const Geometry &geom, const RenderView &view)
  {
    if (!geom.spheres)
      return;
    DynamicShaderHelper &drawSpheres = *geom.spheres;
    DA_PROFILE_GPU;
    if (!drawSpheres.shader)
      drawSpheres.init("draw_debug_dynamic_spheres", NULL, 0, "draw_debug_dynamic_spheres");
    view.bind();
    ShaderGlobal::set_float(sphere_timeVarId, geom.sphereTime);
    drawSpheres.shader->setStates(0, true);
    d3d::setvsrc_ex(0, NULL, 0, 0);
    d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, 6);
  }

  static bool renderTreesCulled(const Geometry &geom, const RenderView &view)
  {
    DynamicRenderableSceneInstance *test = geom.trees;
    if (!test)
      return false;
    TIME_D3D_PROFILE(tree);
    const Frustum frustum(view.culling);
    bool drawn = false;
    test->setLod(test->chooseLod(view.position));
    view.bind();
    foreachTreeInstance(test, geom.treeCount, [&](const TMatrix &tm, const BBox3 &world) {
      if (!frustum.testSphereB(world.center(), length(world.width()) * 0.5f))
        return;
      for (int i : test->getLodsResource()->getNames().node.id)
        test->setNodeWtm(i, tm);
      test->render();
      drawn = true;
    });
    return drawn;
  }
  static bool renderLandmeshPastHmap(const Geometry &geom, const RenderView &view)
  {
    StrmSceneHolder *binScene = geom.level;
    if (!geom.terrain || !geom.lmesh || !binScene || !binScene->lMesh)
      return false;
    const HeightmapHandler *hmap = binScene->getHeightmap();
    float minHt, maxHt;
    if ((hmap && hmap->isMirror()) || !binScene->getMinMax(minHt, maxHt))
      return false;
    // constraint: land casters lie in the height range of the land, a short part of the long page prism
    mat44f inv;
    v_mat44_inverse(inv, view.culling);
    Point3 corner[8];
    for (int c = 0; c < 8; ++c)
    {
      const vec4f p = v_mat44_mul_vec4(inv, v_make_vec4f(c & 1 ? 1 : -1, c & 2 ? 1 : -1, c >> 2, 1));
      v_stu_p3(&corner[c].x, v_div(p, v_splat_w(p)));
    }
    BBox2 land;
    for (int a = 0; a < 8; ++a)
      for (int bit = 1; bit < 8; bit <<= 1)
      {
        if (a & bit)
          continue;
        const Point3 &p = corner[a], &q = corner[a | bit];
        float lo = 0, hi = 1;
        if (fabsf(q.y - p.y) > 1e-3f)
        {
          const float ta = (minHt - p.y) / (q.y - p.y), tb = (maxHt - p.y) / (q.y - p.y);
          lo = max(lo, min(ta, tb));
          hi = min(hi, max(ta, tb));
        }
        else if (p.y < minHt || p.y > maxHt)
          continue;
        if (lo > hi)
          continue;
        land += Point2::xz(lerp(p, q, lo));
        land += Point2::xz(lerp(p, q, hi));
      }
    // constraint: the trace holds terrain only inside both the heightmap and the window of the max grid
    if (land.isempty() || (hmap && hmap->worldBox2 & land[0] && hmap->worldBox2 & land[1] && geom.hmapGridWindow & land[0] &&
                            geom.hmapGridWindow & land[1]))
      return false;
    binScene->renderLmesh(view);
    return true;
  }
  static void renderLevel(const Geometry &geom, const RenderView &view, StrmSceneHolder::Hmap hmap, const ViewLodSelect &v)
  {
    if (!geom.level)
      return;
    TIME_D3D_PROFILE(level);
    geom.level->render(view, hmap, (StrmSceneHolder::Lmesh)(hmap == StrmSceneHolder::Hmap::On && geom.lmesh), v.lmesh);
  }
  void renderPrepass(const Geometry &geom, const RenderView &view)
  {
    if (!land_panel.prepass) //! land_panel.trees ||
      return;
    TIME_D3D_PROFILE(testScenePrepass);
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {});
    renderTreesCulled(geom, view);
    renderLevel(geom, view, StrmSceneHolder::Hmap::On, {});
    renderDynamicSpheres(geom, view);
  }
  static void renderOpaque(const Geometry &geom, const RenderView &view, bool depth,
    StrmSceneHolder::Hmap render_hmap = StrmSceneHolder::Hmap::On, const ViewLodSelect &v = {})
  {
    TIME_D3D_PROFILE(opaque);
    renderTreesCulled(geom, view);
    renderDynamicSpheres(geom, view);
    renderLevel(geom, view, render_hmap, v);

    if (geom.collision)
      rasterizeGbufCollision(*geom.collision, view, depth, v);
  }
  void renderCube(const ManagedTex *target, const Point3 &pos, bool level)
  {
    cube.update(
      target, pos,
      [&](const RenderView &view) {
        daSkies.useFog(pos, cube_pov_data.get(), view.view, view.proj);
        if (level && binScene)
          binScene->render(view, StrmSceneHolder::Hmap::On, (StrmSceneHolder::Lmesh)land_panel.lmesh);
      },
      [&](const RenderView &view, const Driver3dPerspective &persp) {
        TIME_D3D_PROFILE(cubeenvi)
        // cube faces are disjoint views sharing cube_pov_data: restart clouds TAA per face
        const TMatrix itm = orthonormalized_inverse(view.view);
        daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(pos), dpoint3(itm.getcol(2)), 2, UniqueTex{}, UniqueTex{}, nullptr,
          cube_pov_data.get(), view.view, view.proj, persp, UpdateSky::On, false, SKY_PREPARE_THRESHOLD,
          CloudsRenderFlags::Default | CloudsRenderFlags::RestartTAA);
      });
  }

  struct RtssRegionCull
  {
    LandMeshCullingData lmesh;
    dag::Vector<uint64_t> coll;
    ViewLodSelect lod;
  };
  dag::Vector<RtssRegionCull> rtssCulls; // storage only: each render call refills it
  struct RtssRasterCallback : public IStaticShadowsCB
  {
    const RaytracedToroidalStaticShadow &rtss;
    const Geometry &geom;
    const Point3 &position;
    const dag::Vector<RtssRegionCull> &culls;
    RtssRasterCallback(const RaytracedToroidalStaticShadow &r, const Geometry &g, const Point3 &pos,
      const dag::Vector<RtssRegionCull> &c) :
      rtss(r), geom(g), position(pos), culls(c)
    {}
    void startRenderStaticShadow(const TMatrix &, const Point2 &, float, float) override
    {
      d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, reinterpret_cast<void *>(1));
    }
    void endRenderStaticShadow() override { d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, (void *)0); }
    RasterizedResult renderStaticShadowDepth(const mat44f &culling_gtm, const ViewTransformData &transform, int region) override
    {
      RenderView view(TMatrix::IDENT, reinterpret_cast<const TMatrix4 &>(transform.globtm), position);
      view.culling = culling_gtm;
      if (rtss.getSettings().recipes[transform.cascade].raster == RtssRaster::Augment)
      {
        const bool drawn = renderTreesCulled(geom, view) | renderLandmeshPastHmap(geom, view);
        return drawn ? RasterizedResult::SomethingRendered : RasterizedResult::NothingRendered;
      }
      renderOpaque(geom, view, true, geom.terrain ? StrmSceneHolder::Hmap::On : StrmSceneHolder::Hmap::Off, culls[region].lod);
      return RasterizedResult::SomethingRendered;
    }
  };
  static void precullRtssRegions(const RaytracedToroidalStaticShadow &rtss, const Geometry &geom, const Point3 &vp,
    dag::Vector<RtssRegionCull> &culls)
  {
    TIME_PROFILE(rtss_precull);
    const int n = rtss.getRasterRegionCount();
    if ((int)culls.size() < n)
      culls.resize(n);
    const LMesh *lmesh = geom.terrain && geom.lmesh && geom.level ? geom.level->lMesh.get() : nullptr;
    threadpool::parallel_for_inline(0, n, 1, [&](uint32_t begin, uint32_t end, uint32_t) {
      for (uint32_t r = begin; r < end; ++r)
      {
        RtssRegionCull &c = culls[r];
        const ViewTransformData &v = rtss.getRasterRegion(r);
        c.lod = {};
        if (rtss.getSettings().recipes[v.cascade].raster == RtssRaster::Augment)
          continue;
        if (lmesh && cull_lmesh(*lmesh, v.globtm, vp, c.lmesh))
          c.lod.lmesh = &c.lmesh;
        if (geom.collision)
        {
          geom.collision->cull(v.globtm, c.coll);
          c.lod.collision = &c.coll;
        }
      }
    });
  }
  struct RtssTracers final : IRtssTraceCB
  {
    RtssComputeTracer &sw, &collision;
    RtssTracers(RtssComputeTracer &s, RtssComputeTracer &c, uint32_t hw) : sw(s), collision(c)
    {
      sw.levelMask = ~hw;
      collision.levelMask = hw;
    }
    bool traceJobs(const RtssTraceBatch &batch) override { return sw.traceJobs(batch) & collision.traceJobs(batch); }
  };
  RtssComputeTracer rtssSwTracer{"rtss_trace_swrt_cs"}, rtssCollisionTracer{"testgi_rtss_trace_collision_cs"};

  void onSceneLoaded()
  {
    if (binScene)
    {
      binScene->onSceneLoaded();
      BBox3 box;
      for (int i = 0; i < binScene->getRsm().getScenes().size(); ++i)
        box += binScene->getRsm().getScenes()[i]->calcBoundingBox();
      // bin scene bounds ONLY: gates level voxelization (renderVoxelsMediaGeom draws just the
      // bin scene; levelBox below also includes collision bounds and spans the whole map)
      loadedLevelBox = box;
      loadedLevelBox.inflate(1.f);
      if (lruColl.level.instanceCount())
      {
        bbox3f colBox = lruColl.level.worldBox();
        box[0] = min(box[0], (const Point3 &)colBox.bmin);
        box[1] = max(box[1], (const Point3 &)colBox.bmax);
      }
      box.inflate(1.f);
      levelBox = box;
      sceneBox = box;

      extern float hmap_water_level;
      box[0].y = min(hmap_water_level, box[0].y);
      Point2 htMinMax(box[0].y, box[1].y);
      binScene->getMinMax(htMinMax.x, htMinMax.y);
      htMinMax.x = min(hmap_water_level, htMinMax.x);
      debug("htMinMax = %@", htMinMax);
      rtssWorldBox = levelBox;
      rtssWorldBox[0].y = min(rtssWorldBox[0].y, htMinMax.x);
      rtssWorldBox[1].y = max(rtssWorldBox[1].y, htMinMax.y);
      rtss->setWorldBox(rtssWorldBox);

      hmapShadows.setMinMax(htMinMax);
    }
    rtss->invalidate();
    // the first collision window saw a partial level: refill it whole
    bvh::invalidate_lru_collision(bvhCtx);
    redrawSceneGpuData();
    console::process_file("autoexec.txt");
  }

  void redrawSceneGpuData()
  {
    clusteredLights->invalidateAllShadows();
    depthAround.invalidate();
    invalidateGI();
    reloadCube(true);
    renderLmeshDepthAbove();
    hmapShadows.invalidate();
  }

  void setLandMeshDepthVars(TEXTUREID t, const BBox3 &box, bool unbound_sample)
  {
    ShaderGlobal::set_texture(lmesh_height_texVarId, t);
    d3d::SamplerInfo smpInfo;
    smpInfo.address_mode_u = smpInfo.address_mode_v = d3d::AddressMode::Mirror;
    lmesh_height_tex_samplerstateVarId.set_sampler(d3d::request_sampler(smpInfo));
    if (box.isempty())
    {
      ShaderGlobal::set_float4(lmesh_height_encodingVarId, 0, 0, 0, 0);
      ShaderGlobal::set_float4(world_to_lmesh_heightVarId, 0, 0, 0, 0);
    }
    else
    {
      const float bias = -3.5f / 65536.f;
      const Point3 w = box.width();
      ShaderGlobal::set_float4(lmesh_height_encodingVarId, box[0].y + bias * w.y, w.y, unbound_sample ? 3e38f : 1.f, 0);
      ShaderGlobal::set_float4(world_to_lmesh_heightVarId, 1.f / w.x, -1.f / w.z, -box[0].x / w.x, 1.f + box[0].z / w.z);
    }
  }
  void renderLandMeshDepthInternal(const BBox3 &box, bool inv = false)
  {
    TMatrix4 view;
    view.setcol(0, 1, 0, 0, 0);
    view.setcol(1, 0, 0, 1, 0);
    view.setcol(2, 0, 1, 0, 0);
    view.setcol(3, 0, 0, 0, 1);
    TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[uint32_t(inv)].z, box[1 - uint32_t(inv)].z, box[0].y, box[1].y);
    binScene->renderLmesh(RenderView(tmatrix(view), proj, box.center()), box);
  }
  void renderLandMeshDepth(Texture *target, const BBox3 &box)
  {
    SCOPE_VIEW_PROJ_MATRIX;
    SCOPE_RENDER_TARGET;
    d3d::set_render_target({target, 0, 0}, DepthAccess::RW, {});
    d3d::clearview(CLEAR_ZBUFFER, 0, 0, 0);
    renderLandMeshDepthInternal(box);
  }

  void renderLmeshDepthAbove()
  {
    setLandMeshDepthVars(BAD_TEXTUREID, BBox3(), false);
    if (!binScene)
      return;
    BBox3 lbox;
    if (!binScene->getLmeshBBox3(lbox))
      return;
    debug("landmesh box %@", lbox);
    const int wWorld = 768;
    lmesh_depth_world = dag::create_tex(NULL, wWorld, wWorld, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "lmesh_depth_world");
    const int w = 512; // about 384*sqrt(2)
    lmesh_depth_current = dag::create_tex(NULL, w, w, TEXCF_RTARGET | TEXFMT_DEPTH16, 1, "lmesh_depth_current");
    d3d::GpuAutoLock gpu_al;
    renderLandMeshDepth(lmesh_depth_world.getTex2D(), lbox);
    setLandMeshDepthVars(lmesh_depth_world.getTexId(), lbox, true);
  }

  UniqueTex lmesh_depth_world, lmesh_depth_current;
  bool validAmbientHistory = false;
  void renderAmbient(UniqueTex &amb, UniqueTex &spec, UniqueTex &age)
  {
    TIME_D3D_PROFILE(current_ambient)
    target->setVar();
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::SampledRO,
      {{amb.getTex2D(), 0, 0}, {spec.getTex2D(), 0, 0}, {age.getTex2D(), 0, 0}});
    ambientRenderer.render();
    ShaderGlobal::set_texture(screen_ambientVarId, amb.getTexId());
    ShaderGlobal::set_texture(screen_specularVarId, spec.getTexId());
  }
  void renderAmbient(const TMatrix4 &cGlobtm)
  {
    TIME_D3D_PROFILE(ambientFixup)
    SCOPE_RENDER_TARGET;
    static TMatrix4 prevGlobTm;
    const bool hasHistory = ambient[1] && screen_probes.reproject && validAmbientHistory;

    if (hasHistory)
    {
      TMatrix4D globTmInv;

      double det;
      G_VERIFY(inverse44(cGlobtm, globTmInv, det));
      TMatrix4D reprojection = globTmInv * prevGlobTm;
      TMatrix4 repr = TMatrix4(reprojection).transpose();
      ShaderGlobal::set_float4x4(ambient_reproject_tmVarId, repr);
    }
    cAmbientFrame = hasHistory ? 1 - cAmbientFrame : 0;
    ShaderGlobal::set_texture(prev_ambientVarId, ambient[1 - cAmbientFrame].getTexId());
    ShaderGlobal::set_texture(prev_specularVarId, screen_specular[1 - cAmbientFrame].getTexId());
    ShaderGlobal::set_texture(prev_ambient_ageVarId, ambient_age[1 - cAmbientFrame].getTexId());
    ShaderGlobal::set_int(ambient_has_historyVarId, hasHistory && daGI2->getHistoryFrames() > 0);
    renderAmbient(ambient[cAmbientFrame], screen_specular[cAmbientFrame], ambient_age[cAmbientFrame]);

    prevGlobTm = cGlobtm;
    validAmbientHistory = true;
  }

  enum
  {
    RESULT,
    LIGHTING,
    DIFFUSE_LIGHTING,
    DIRECT_LIGHTING,
    INDIRECT_LIGHTING
  };
  static void setMatricesVars(const TMatrix &view, const TMatrix4 &projTm)
  {
    set_inv_globtm_to_shader(view, projTm, false);
    set_viewvecs_to_shader(view, projTm);
    TMatrix4 tr_proj_tm = projTm.transpose();
    ShaderGlobal::set_float4(projtm_psf_0VarId, Color4(tr_proj_tm[0]));
    ShaderGlobal::set_float4(projtm_psf_1VarId, Color4(tr_proj_tm[1]));
    ShaderGlobal::set_float4(projtm_psf_2VarId, Color4(tr_proj_tm[2]));
    ShaderGlobal::set_float4(projtm_psf_3VarId, Color4(tr_proj_tm[3]));

    TMatrix4 gtm;
    d3d::calcglobtm(view, projTm, gtm);
    gtm = gtm.transpose();
    ShaderGlobal::set_float4(globtm_psf_0VarId, Color4(gtm[0]));
    ShaderGlobal::set_float4(globtm_psf_1VarId, Color4(gtm[1]));
    ShaderGlobal::set_float4(globtm_psf_2VarId, Color4(gtm[2]));
    ShaderGlobal::set_float4(globtm_psf_3VarId, Color4(gtm[3]));

    TMatrix viewNoOfs = view;
    viewNoOfs.setcol(3, 0, 0, 0);
    TMatrix4 gtm_no_ofs;
    d3d::calcglobtm(viewNoOfs, projTm, gtm_no_ofs);
    gtm_no_ofs = gtm_no_ofs.transpose();
    ShaderGlobal::set_float4(globtm_no_ofs_psf_0VarId, Color4(gtm_no_ofs[0]));
    ShaderGlobal::set_float4(globtm_no_ofs_psf_1VarId, Color4(gtm_no_ofs[1]));
    ShaderGlobal::set_float4(globtm_no_ofs_psf_2VarId, Color4(gtm_no_ofs[2]));
    ShaderGlobal::set_float4(globtm_no_ofs_psf_3VarId, Color4(gtm_no_ofs[3]));
  }

  void render(const Geometry &geom, const RenderView &frame_view, const RenderView &geometry_view, const TMatrix &itm,
    const Driver3dPerspective &persp, bool hw_rt_gi, int output_width)
  {
    TIME_D3D_PROFILE(render)
    const TMatrix &view = frame_view.view;
    const TMatrix4 &projTm = frame_view.proj;
    const mat44f &globtm = frame_view.culling;

    {
      motion_vector_access::set_motion_vector_type(motion_vector_access::MotionVectorType::StaticUVZ);
      motion_vector_access::CameraParams currentCamera{view, itm, projTm, itm.getcol(3), persp.zn, persp.zf};

      motion_vector_access::set_params(currentCamera, previousCamera ? *previousCamera : currentCamera, {}, {}, eastl::nullopt);
      previousCamera = currentCamera;
    }

    {
      TIME_D3D_PROFILE(changeData)
      daSkies.changeSkiesData(render_panel.sky_quality, render_panel.direct_quality, !render_panel.infinite_skies, target->getWidth(),
        target->getHeight(), main_pov_data.get());
      daSkies.useFog(itm.getcol(3), main_pov_data.get(), view, projTm);
    }
    setMatricesVars(view, projTm);
    frame_view.bind();

    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0); //
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    {
      TIME_D3D_PROFILE(scene)
      renderPrepass(geom, geometry_view);
      renderOpaque(geom, geometry_view, false);
      frame_view.bind();
      renderDebugHmapTrace();
    }

    if (::grs_draw_wire)
      d3d::setwire(0);
    extern bool g_ssao_blur;
    g_ssao_blur = ssao_blur.get();
    csm->renderShadowsCascadesCb([&](int count, bool clear_per_view) {
      for (int cascade = 0; cascade < count; ++cascade)
        csm->renderShadowCascadeDepthCb(cascade, [&] {
          if (clear_per_view)
            d3d::clearview(CLEAR_ZBUFFER, 0, 1.f, 0);
          RenderView shadow(TMatrix::IDENT, csm->getWorldRenderMatrix(cascade), geometry_view.position);
          shadow.culling = (mat44f_cref)csm->getWorldCullingMatrix(cascade);
          shadow.lodProj = geometry_view.proj;
          renderOpaque(geom, shadow, true);
        });
    });
    frame_view.bind();
    csm->setCascadesToShader();
    combineShadows();
    downsampleDepth(true);

    // Fill the clustered light grid now that this frame's far depth exists, so its
    // occlusion z-slice cull uses current-frame depth (see drawScene cull site).
    clusteredLights->fillAndSetInsideOfFrustumLightsBuffers();

    {
      static int shadow_frame;
      ShaderGlobal::set_int(shadow_frameVarId, (shadow_frame++) & ((1 << 23) - 1));
      swRTShadows(dir_to_sun, 0.0093f / 4);
    }
    if (gi_panel.ssao && ssao)
    {
      ssao->render(view, projTm, downsampled_close_depth_tex[currentDownsampledDepth].getBaseTex());
    }

    if (gi_panel.gtao && gtao)
      gtao->render(view, projTm);

    if (!gi_panel.ssao && !gi_panel.gtao)
      ShaderGlobal::set_texture(get_shader_variable_id("ssao_tex"), BAD_TEXTUREID);

    if (ssr)
      ssr->render(view, projTm);
    if (!ssr || gi_panel.reflections == REFLECTIONS_OFF)
    {
      ShaderGlobal::set_texture(get_shader_variable_id("ssr_target"), BAD_TEXTUREID);
    }

    if (rtsmIsOn)
    {
      denoiser::TexMap textures;
      for (auto &[name, tex] : rt_textures)
        textures[name] = tex.getTex2D();

      ShaderGlobal::set_float4(get_shader_variable_id("sun_dir_for_shadows", true), -dir_to_sun);


      ::denoiser::FrameParams denoiserParams;
      denoiserParams.textures = textures;
      denoiserParams.viewPos = itm.getcol(3);
      denoiserParams.prevViewPos = previousCamera.has_value() ? previousCamera->position : denoiserParams.viewPos;
      denoiserParams.viewDir = itm.getcol(2);
      denoiserParams.prevViewDir = previousCamera.has_value() ? previousCamera->viewInverse.getcol(2) : denoiserParams.viewDir;
      denoiserParams.viewItm = itm;
      denoiserParams.projTm = projTm;
      denoiserParams.prevViewItm = previousCamera.has_value() ? previousCamera->viewInverse : denoiserParams.viewItm;
      denoiserParams.prevProjTm = previousCamera.has_value() ? previousCamera->projectionNoJitter : projTm;
      denoiserParams.jitter = Point2(0.0f, 0.0f);
      denoiserParams.prevJitter = Point2(0.0f, 0.0f);
      denoiserParams.motionMultiplier = Point3::ONE;

      int w, h;
      d3d::get_render_target_size(w, h, nullptr);
      denoiserParams.dynRes = ::denoiser::DynamicResolutionParams{{w, h}, {w, h}, {w / 2, h / 2}, {w / 2, h / 2}};

      denoiser::prepare(denoiserParams);
      rtsm::render(bvhCtx, itm.getcol(3), -dir_to_sun, projTm, textures);
    }

    // swrt/rtsm above unbind; the GI relight below traces the collision TLAS
    if (hw_rt_gi && bvhCtx != bvh::InvalidContextId)
      bvh::bind_resources(bvhCtx, output_width);
    daGI2->beforeFrameLit(gi_panel.dynamic_gi_quality);

    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), gi_panel.onscreen_mode);
    {
      renderAmbient((const TMatrix4 &)globtm);
      {
        TIME_D3D_PROFILE(shading)
        target->resolve(frame.getTex2D(), view, projTm);
        d3d::set_render_target({}, DepthAccess::RW, {{frame.getTex2D(), 0, 0}});
      }
    }
    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), RESULT);

    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::SampledRO, {{frame.getTex2D(), 0, 0}});
    clusteredLights->renderOtherLights();
    renderEnvi(frame_view, itm, persp);
    d3d::set_render_target({target->getDepth(), 0, 0}, DepthAccess::RW, {{frame.getTex2D(), 0, 0}});
    ensurePrevFrame();
    d3d::stretch_rect(frame.getTex2D(), prevFrame.getTex2D());

    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
    if (sleep_msec_val.get())
      sleep_msec(sleep_msec_val.get());
  }

  void rasterizeDebugCollision(const RenderView &view)
  {
    view.bind();
    ShaderGlobal::set_int(rasterize_collision_typeVarId, 0);
    lruColl.rasterize(view.culling);
  }
  static void rasterizeGbufCollision(LRUCollision &lruColl, const RenderView &view, bool depth, const ViewLodSelect &v)
  {
    view.bind();
    ShaderGlobal::set_int(rasterize_collision_typeVarId, depth ? 2 : 1);
    if (v.collision)
      lruColl.draw(make_span_const(*v.collision));
    else
      lruColl.rasterize(view.culling);
  }
  DynamicShaderHelper drawWater;
  void renderWater(const RenderView &view)
  {
    if (!water_panel.enabled)
      return;
    if (!drawWater.shader)
      drawWater.init("draw_water", NULL, 0, "draw_water");
    DA_PROFILE_GPU;
    view.bind();
    drawWater.shader->setStates(0, true);
    d3d::setvsrc_ex(0, NULL, 0, 0);
    d3d::draw(PRIM_TRILIST, 0, 2);
  }
  void renderTrans(const RenderView &view)
  {
    // begin_draw_cached_debug_lines();
    // for (auto &b : indirBoxes)
    //   draw_cached_debug_box(b, E3DCOLOR(0xff,0x80, 0xff, 0xff));
    // end_draw_cached_debug_lines();

    if (binScene && land_panel.level)
      binScene->renderTrans(view);
    if (rasterize_debug_collision)
      rasterizeDebugCollision(view);
    if (sw_rt)
    {
      view.bind();
      swrt.drawRT();
    }
    using namespace hmap_debug;
    view.bind();
    set_cached_debug_lines_wtm(TMatrix::IDENT);
    begin_draw_cached_debug_lines(false);
    for (int i = 0; i < debug_water_edges_count; ++i)
      draw_cached_debug_line(debug_water_edges[i * 2 + 0], debug_water_edges[i * 2 + 1], E3DCOLOR(0xFF00FFFF));
    {
      draw_cached_debug_line(debug_quad_points[0], debug_quad_points[1], E3DCOLOR(0xFFFF00FF));
      draw_cached_debug_line(debug_quad_points[1], debug_quad_points[3], E3DCOLOR(0xFFFF00FF));
      draw_cached_debug_line(debug_quad_points[3], debug_quad_points[2], E3DCOLOR(0xFFFF00FF));
      draw_cached_debug_line(debug_quad_points[2], debug_quad_points[0], E3DCOLOR(0xFFFF00FF));
    }
    end_draw_cached_debug_lines();
  }

  void reinitCube(int ew)
  {
    cube.close();
    cube.init(ew);
    light_probe::destroy(localProbe);
    localProbe = light_probe::create("local", ew, TEXFMT_A16B16G16R16F);
    initEnviProbe();
    reloadCube(true);
  }
  UniqueTexWithShaderVar enviProbe;
  UniqueTex enviProbe0;
  void initEnviProbe()
  {
    enviProbe0.close();
    enviProbe0 = dag::create_cubetex(64, TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS | TEXCF_CLEAR_ON_CREATE, 1,
      "envi_probe_specular0");

    enviProbe.close();
    enviProbe = dag::create_cubetex(128, TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS | TEXCF_CLEAR_ON_CREATE, 4,
      "envi_probe_specular");
    renderEnviProbe();
  }

  void renderEnviProbe()
  {
    if (!enviProbe.getCubeTex())
    {
      return initEnviProbe();
    }
    // skies render can use envi probe for ground-skies interaction on some of skies configs,
    // causing RW conflict (SRV vs RT usages) workaround this by removing texture from shader var
    ShaderGlobal::set_texture(envi_probe_specularVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(local_light_probe_texVarId, BAD_TEXTUREID);
    renderCube(&enviProbe0, Point3(100000, 10, 0), false);

    ShaderGlobal::set_texture(envi_probe_specularVarId, enviProbe0.getTexId());
    ShaderGlobal::set_texture(local_light_probe_texVarId, enviProbe0.getTexId());

    renderCube(&enviProbe, Point3(100000, 10, 0), land_panel.level);
    enviProbe.getCubeTex()->generateMips();

    if (light_probe::getManagedTex(localProbe))
      ShaderGlobal::set_texture(local_light_probe_texVarId, *light_probe::getManagedTex(localProbe));
    ShaderGlobal::set_texture(envi_probe_specularVarId, enviProbe);
  }

  void reloadCube(bool first = false)
  {
    d3d::GpuAutoLock gpu_al;
    renderEnviProbe();
    Point3 pos = Point3(0, 10, 0);
    if (!first && curCamera)
    {
      TMatrix itm;
      curCamera->getInvViewMatrix(itm);
      pos = itm.getcol(3);
    }
    // constraint: the probe bake reads the rtss, so it is built around pos first
    updateHmapMaxGrid(pos);
    if (!freeze)
    {
      const uint32_t hw = applyRtssSettings(!first); // constraint: the collision TLAS gathers around the camera, not the fixed pos
      if (hw)
        bvh::bind_resources(bvhCtx, target->getWidth());
      rtss->setSun(dir_to_sun);
      for (int i = 0; i < 8 && (i == 0 || !rtss->isReady(BBox3(pos, 64.f), 0)); ++i) // constraint: the first update moves it to pos
        buildRtss(pos, Point3(0, 0, 0), 0.f);
    }
    if (first)
    {
      ShaderGlobal::set_texture(local_light_probe_texVarId, enviProbe);
      SCOPE_RENDER_TARGET;
      for (int i = 0; i < 6; ++i)
      {
        d3d::set_render_target({}, DepthAccess::RW,
          {{light_probe::getManagedTex(localProbe)->getCubeTex(), 0, static_cast<uint32_t>(i)}});
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      }
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        ShaderGlobal::set_float4(get_shader_variable_id(String(128, "enviSPH%d", i)), 0, 0, 0, 0);
      light_probe::update(localProbe, NULL);
    }
    {
      TIME_D3D_PROFILE(cubeRender)
      renderCube(light_probe::getManagedTex(localProbe), pos, land_panel.level);
    }
    {
      TIME_D3D_PROFILE(lightProbeSpecular)
      light_probe::update(localProbe, NULL);
    }
    {
      TIME_D3D_PROFILE(lightProbeDiffuse)
      if (light_probe::calcDiffuse(localProbe, NULL, 1, 1, true))
      {
        const Color4 *sphHarm = light_probe::getSphHarm(localProbe);
        for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
          ShaderGlobal::set_float4(get_shader_variable_id(String(128, "enviSPH%d", i)), sphHarm[i]);
      }
      else
      {
        G_ASSERT(0);
      }
    }

    ShaderGlobal::set_texture(local_light_probe_texVarId, *light_probe::getManagedTex(localProbe));
  }
  enum
  {
    ENVI_PROBE = -1,
    ONLY_AO = 0,
    SIMPLE_COLORED = 1,
    SCREEN_PROBES = 2
  };
  IGameCamera *curCamera; // for console made publuc
  IFreeCameraDriver *freeCam;

  eastl::optional<motion_vector_access::CameraParams> previousCamera;

  friend class TestConsole;

protected:
  DynamicRenderableSceneInstance *test = nullptr;
  DaGIMediaVolumes mediaVolumes;
  VegetationMediaVoxelizer mediaVegVoxelizer;
  int treeMediaVolType = -1;
  const DynamicRenderableSceneLodsResource *mediaVolTypeRes = nullptr; // the res treeMediaVolType was baked for
  bool treesInGI = false;
  bool treesMediaClearPending = false; // an uncheck raced a model reload, its media clear is owed

  BBox3 treesWorldBox() const
  {
    BBox3 box;
    foreachTreeInstance([&](const TMatrix &, const BBox3 &world) { box += world; });
    return box;
  }

  // the sample places its trees with set_test_tm: the media gather and the world box walk it
  // here, so a placement change cannot move one and leave the other behind
  template <typename Cb>
  void foreachTreeInstance(Cb cb) const
  {
    foreachTreeInstance(test, testCount, cb);
  }
  template <typename Cb>
  static void foreachTreeInstance(DynamicRenderableSceneInstance *test, int count, Cb cb)
  {
    if (!test)
      return;
    const BBox3 local = test->getLocalBoundingBox();
    TMatrix tm = TMatrix::IDENT; // set_test_tm writes the translation only: the basis is ours
    for (int i = 0; i < count; ++i)
    {
      set_test_tm(i, tm);
      cb(tm, tm * local);
    }
  }
  scene_type_t *binScene = nullptr;

  const int testCount = 100;

  bool unloaded_splash;

  void loadScene()
  {
#if !_TARGET_PC
    freeCam = create_gamepad_free_camera();
#else
    global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(true);
    freeCam = create_mskbd_free_camera();
    freeCam->scaleSensitivity(4, 2);
#endif
    IFreeCameraDriver::zNear = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("zNear", 0.01);
    IFreeCameraDriver::zFar = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("zFar", 50000);
    IFreeCameraDriver::turboScale = 20;

    de3_imgui_init("Test GI", "Sky properties");

    static GuiControlDesc skyGuiDesc[] = {DECLARE_BOOL_CHECKBOX(gi_panel, ssao, false), DECLARE_BOOL_CHECKBOX(gi_panel, gtao, false),
      DECLARE_INT_COMBOBOX(gi_panel, reflections, REFLECTIONS_ALL, REFLECTIONS_OFF, REFLECTIONS_SSR, REFLECTIONS_ALL),
      // DECLARE_BOOL_CHECKBOX(gi_panel, fake_dynres, false),
      DECLARE_INT_COMBOBOX(gi_panel, onscreen_mode, RESULT, RESULT, LIGHTING, DIFFUSE_LIGHTING, DIRECT_LIGHTING, INDIRECT_LIGHTING),
      DECLARE_INT_COMBOBOX(gi_panel, gi_mode, SCREEN_PROBES, ENVI_PROBE, ONLY_AO, SIMPLE_COLORED, SCREEN_PROBES),
      DECLARE_FLOAT_SLIDER(gi_panel, dynamic_gi_quality, 0, 4., 1.0, 0.01), DECLARE_FLOAT_SLIDER(gi_panel, exposure, 0.1, 16, 1, 0.01),
      DECLARE_BOOL_CHECKBOX(gi_panel, update_radiance, true),

      DECLARE_INT_SLIDER(sdf_panel, clips, 3, 8, 5), DECLARE_INT_SLIDER(sdf_panel, texWidth, 64, 256, 128),
      DECLARE_FLOAT_SLIDER(sdf_panel, voxel0Size, 0.1, 1., 0.15, 0.1), DECLARE_FLOAT_SLIDER(sdf_panel, yResScale, 0.5, 1., 0.5, 0.1),
      DECLARE_FLOAT_SLIDER(screen_probes, temporality, 0, 1., 0.5, 0.01), DECLARE_BOOL_CHECKBOX(screen_probes, reproject, true),
      DECLARE_INT_COMBOBOX(screen_probes, tileSize, screen_probes.tileSize, 8, 10, 12, 14, 16, 20, 24, 32),
      DECLARE_INT_COMBOBOX(screen_probes, radianceOctRes, 8, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16),
      DECLARE_BOOL_CHECKBOX(screen_probes, angleFiltering, false),

      DECLARE_INT_SLIDER(radiance_grid_panel, clips, 2, 4, 3), DECLARE_INT_SLIDER(radiance_grid_panel, texWidth, 16, 64, 32),

      DECLARE_INT_SLIDER(albedo_scene_panel, clips, 2, 4, 3),

      DECLARE_INT_SLIDER(volumetric_gi, slices, 8, 64, 16), DECLARE_INT_SLIDER(volumetric_gi, tileSize, 0, 256, 64),
      DECLARE_FLOAT_SLIDER(volumetric_gi, zLogMul, 1., 16, 6, 0.01),
      DECLARE_FLOAT_SLIDER(volumetric_gi, blurTexelOfs, 0., 1., 0.125, 0.01), DECLARE_BOOL_CHECKBOX(volumetric_gi, debugUse, false),

      ////DECLARE_BOOL_BUTTON(gi_panel, calc_ground_truth, false),
      // DECLARE_BOOL_CHECKBOX(gi_panel, show_ground_truth, false),

      DECLARE_BOOL_BUTTON(file_panel, export_min, false), DECLARE_BOOL_BUTTON(file_panel, export_max, false),
      DECLARE_BOOL_BUTTON(file_panel, import_minmax, false), DECLARE_BOOL_BUTTON(file_panel, save, false),
      DECLARE_BOOL_BUTTON(file_panel, load, false), DECLARE_BOOL_CHECKBOX(sun_panel, astronomical, false),
      DECLARE_FLOAT_SLIDER(sun_panel, sun_zenith, 0.1, 180, 30, 0.01), DECLARE_FLOAT_SLIDER(sun_panel, sun_azimuth, 0., 360, 215, 1),
      DECLARE_FLOAT_SLIDER(sun_panel, moon_age, 0., 29.530588853, 10, 0.1),
      DECLARE_FLOAT_SLIDER(sun_panel, latitude, -90, 90, 55, 0.01), DECLARE_FLOAT_SLIDER(sun_panel, longitude, -180, 180, 37, 0.01),
      DECLARE_INT_SLIDER(sun_panel, year, 1941, 2032, 1939), DECLARE_INT_SLIDER(sun_panel, month, 1, 12, 6),
      DECLARE_INT_SLIDER(sun_panel, day, 1, 365, 6), DECLARE_FLOAT_SLIDER(sun_panel, time, 0, 24, 12, 0.001),
      DECLARE_BOOL_CHECKBOX(sun_panel, local_time, true), DECLARE_BOOL_CHECKBOX(sun_panel, animate, false),
      DECLARE_FLOAT_SLIDER(sun_panel, day_minutes, 1, 240, 24, 1), DECLARE_FLOAT_SLIDER(sun_panel, axis_tilt, 0, 80, 40, 1),
      DECLARE_BOOL_BUTTON(sun_panel, time_skip, false),

      DECLARE_FLOAT_SLIDER(sky_panel, average_ground_albedo, 0.02, 0.83, 0.1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ground_color.r, 0, 1, 1, 0.01), DECLARE_FLOAT_SLIDER(sky_panel, ground_color.g, 0, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ground_color.b, 0, 1, 1, 0.01),

      DECLARE_FLOAT_SLIDER(sky_panel, mie_height, 0.1, 6.0, 1.20, 0.1),
      DECLARE_FLOAT_SLIDER(sky_panel, mie_scale, 0.1, 100.0, 1.0, 0.1),
      DECLARE_FLOAT_SLIDER(sky_panel, mie_absorption_scale, 0.0, 100, 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, mie_assymetry, 0.01, 0.999, 0.8, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, mie_back_assymetry, 0.01, 0.999, 0.15, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, mie_forward_weight, 0.01, 0.999, 0.5, 0.01),

      DECLARE_FLOAT_SLIDER(sky_panel, planet_scale, 0.1, 2.0, 1., 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, atmosphere_scale, 0.1, 10.0, 1., 0.01),

      DECLARE_FLOAT_SLIDER(sky_panel, rayleigh_scale, 0.05, 8.0, 1., 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, rayleigh_alt_scale, 0.1, 10.0, 1., 0.01),

      DECLARE_FLOAT_SLIDER(sky_panel, ozone_alt_dist, 0.1, 2., 1., 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ozone_max_alt, 0.1, 1.5, 1., 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ozone_scale, 0.1, 10.0, 1., 0.01),

      DECLARE_FLOAT_SLIDER(sky_panel, multiple_scattering_factor, 0, 2, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, sun_brightness, 0.5, 2.0, 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, moon_brightness, 0.5, 2.0, 1.0, 0.01),

      DECLARE_FLOAT_SLIDER(sky_colors, rayleigh_color.r, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, rayleigh_color.g, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, rayleigh_color.b, 0.01, 1, 1, 0.01),

      DECLARE_FLOAT_SLIDER(sky_colors, mie_scattering_color.r, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, mie_scattering_color.g, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, mie_scattering_color.b, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, mie_absorption_color.r, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, mie_absorption_color.g, 0.01, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, mie_absorption_color.b, 0.01, 1, 1, 0.01),

      DECLARE_FLOAT_SLIDER(sky_colors, moon_color.r, 0.01, 2, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, moon_color.g, 0.01, 2, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, moon_color.b, 0.01, 2, 1, 0.01),

      DECLARE_BOOL_CHECKBOX(land_panel, enabled, false), DECLARE_BOOL_CHECKBOX(land_panel, lmesh, true),
      DECLARE_BOOL_CHECKBOX(land_panel, level, false), DECLARE_BOOL_CHECKBOX(land_panel, collision, true),
      DECLARE_FLOAT_SLIDER(land_panel, heightScale, 1, 10000.0, 2500.0, 1),
      DECLARE_FLOAT_SLIDER(land_panel, heightMin, -1.0, 1.0, -0.5, 0.1), DECLARE_FLOAT_SLIDER(land_panel, cellSize, 1, 250, 25, 0.5),
      DECLARE_BOOL_CHECKBOX(land_panel, trees, false), DECLARE_BOOL_CHECKBOX(land_panel, spheres, true),
      DECLARE_FLOAT_SLIDER(land_panel, spheres_speed, 0, 32, 8, 0.01), DECLARE_BOOL_CHECKBOX(land_panel, prepass, false),

      DECLARE_FLOAT_SLIDER(clouds_rendering2, forward_eccentricity, 0.1, 0.9999, 0.8, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, back_eccentricity, 0.01, 0.9999, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, forward_eccentricity_weight, 0.0, 1.0, 0.6, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, erosion_noise_size, 1.0, 73, 37.8, 0.1),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, erosionWindSpeed, -100, 100, 50, 0.1),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, ambient_desaturation, 0, 1, 0.5, 0.1),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, ms_contribution, 0, 1, 0.7, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, ms_attenuation, 0.02, 1.0, 0.3, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_rendering2, ms_ecc_attenuation, 0.02, 0.99, 0.3, 0.01),

      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, worldSize, 65536, 262144, 65536, 1.0),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, cumulonimbusCoverage, 0.0, 1.0, 0.0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, cumulonimbusSeed, 0.0, 100.0, 0.0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, epicness, 0.0, 1.0, 0., 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[0].coverage, 0.0, 1.0, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[0].freq, 1, 8, 3, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[0].seed, 0, 100, 0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[1].coverage, 0.0, 1.0, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[1].freq, 1, 8, 6, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_weather_gen2, layers[1].seed, 0, 100, 0, 0.01),

      DECLARE_INT_SLIDER(clouds_form, shapeNoiseScale, 2, 16, 9), DECLARE_INT_SLIDER(clouds_form, cumulonimbusShapeScale, 2, 16, 4),
      DECLARE_INT_SLIDER(clouds_form, turbulenceFreq, 1, 6, 1),
      DECLARE_FLOAT_SLIDER(clouds_form, turbulenceStrength, 0., 2.0, 0.25, 0.01),

      DECLARE_FLOAT_SLIDER(clouds_form, extinction, 0.5, 6.0, 0.75, 0.01), // this is 0.06 multiplier clouds real extinction is
                                                                           // within 0.04-0.24, which is
      DECLARE_FLOAT_SLIDER(clouds_form, layers[0].density, 0.5, 2.0, 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[0].startAt, 0.02, 8.0, 0.8, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[0].thickness, 1.0, 10.0, 8.0, 0.05),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[0].clouds_type, 0, 1.0, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[0].clouds_type_variance, 0, 1.0, 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[1].density, 0.5, 2.0, 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[1].startAt, 0.02, 10.0, 6.5, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[1].thickness, 1.0, 10.0, 3.5, 0.05),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[1].clouds_type, 0, 1.0, 0.25, 0.01),
      DECLARE_FLOAT_SLIDER(clouds_form, layers[1].clouds_type_variance, 0, 1.0, 0.5, 0.01),

#if _TARGET_PC
      DECLARE_INT_SLIDER(clouds_game_params, quality, 1, 4, 2),
#else
      DECLARE_INT_SLIDER(clouds_game_params, quality, 1, 4, 1),
#endif
      DECLARE_BOOL_CHECKBOX(clouds_game_params, fastEvolution, false),
      DECLARE_BOOL_CHECKBOX(clouds_game_params, competitive_advantage, true),
      DECLARE_FLOAT_SLIDER(clouds_game_params, maximum_averaging_ratio, 0.0, 1.0, 0.6, 0.01),
      DECLARE_INT_SLIDER(clouds_game_params, target_quality, 1, 4, 2),

      DECLARE_FLOAT_SLIDER(strata_clouds, amount, 0, 1.0, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(strata_clouds, altitude, 4.0, 14.0, 10.0, 0.100),
      DECLARE_INT_COMBOBOX(render_panel, render_type, DIRECT, PANORAMA, DIRECT),
      DECLARE_BOOL_CHECKBOX(render_panel, infinite_skies, false), DECLARE_INT_SLIDER(render_panel, sky_quality, 0, 3, 1),
      DECLARE_INT_SLIDER(render_panel, direct_quality, 0, 3, 1),
      DECLARE_INT_COMBOBOX(render_panel, panoramaResolution, 2048, 1024, 1536, 2048, 3072, 4096),
      DECLARE_BOOL_CHECKBOX(render_panel, panorama_blending, false), DECLARE_FLOAT_SLIDER(render_panel, cloudsSpeed, 0, 60, 20, 0.1),
      DECLARE_FLOAT_SLIDER(render_panel, strataCloudsSpeed, 0, 30, 10, 0.1), DECLARE_FLOAT_SLIDER(render_panel, windDir, 0, 359, 0, 1),
      DECLARE_BOOL_BUTTON(render_panel, trace_ray, false), DECLARE_BOOL_BUTTON(render_panel, compareCpuSunSky, false),
      DECLARE_BOOL_CHECKBOX(water_panel, enabled, false),

      DECLARE_FLOAT_SLIDER(layered_fog, mie2_scale, 0, 200, 0, 0.00001),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_altitude, 1, 2500, 200, 1),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_thickness, 200, 1500, 400, 1),

      DECLARE_INT_SLIDER(rtss_panel, levels, 1, RTSS_MAX_LEVELS, RTSS_DEFAULTS.levels),
      DECLARE_INT_COMBOBOX(rtss_panel, windowPages, RTSS_DEFAULTS.windowPages, 4, 8, 16, 32),
      DECLARE_INT_COMBOBOX(rtss_panel, slotTexels, RTSS_DEFAULTS.slotTexels, 32, 64, 128),
      DECLARE_INT_SLIDER(rtss_panel, borderTexels, 1, 4, RTSS_DEFAULTS.borderTexels),
      DECLARE_FLOAT_SLIDER(rtss_panel, texel0, 0.01, 0.5, RTSS_DEFAULTS.texel0, 0.01),
      DECLARE_FLOAT_SLIDER(rtss_panel, maxAnisotropy, 1, 8, RTSS_DEFAULTS.maxAnisotropy, 1),
      DECLARE_FLOAT_SLIDER(rtss_panel, toleranceDeg0, 0.01, RTSS_DEFAULTS.toleranceDegMax, RTSS_DEFAULTS.toleranceDeg0, 0.01),
      DECLARE_INT_SLIDER(rtss_panel, updatePagesPerFrame, 1, 256, RTSS_DEFAULTS.updatePagesPerFrame),
      DECLARE_INT_SLIDER(rtss_panel, batchFrames, 1, 16, RTSS_DEFAULTS.batchFrames),
      DECLARE_INT_COMBOBOX(rtss_panel, recipe, RTSS_SWRT, RTSS_SWRT, RTSS_HWRT, RTSS_RASTER, RTSS_HYBRID_SW, RTSS_HYBRID_HW),
      DECLARE_INT_SLIDER(rtss_panel, hwLevels, 0, RTSS_MAX_LEVELS, 3),
      DECLARE_FLOAT_SLIDER(rtss_panel, rasterMaxTexel, 0.01, 2, 0.25, 0.01),
      DECLARE_FLOAT_SLIDER(rtss_panel, rasterCost, 0.1, 16, 1, 0.1), DECLARE_FLOAT_SLIDER(rtss_panel, augmentCost, 0.1, 16, 1.5, 0.1),
      DECLARE_INT_SLIDER(rtss_panel, rasterBatchPages, 4, 64, RTSS_DEFAULTS.rasterBatchPages),
      DECLARE_INT_COMBOBOX(rtss_panel, screen_filter, RTSS_DITHERED_HARDEN, RTSS_DITHERED_HARDEN, RTSS_TENT),
      DECLARE_FLOAT_SLIDER(rtss_panel, screen_radius, 0.25, 4, 1.5, 0.05),
      DECLARE_FLOAT_SLIDER(rtss_panel, seamBand, 0, 0.5, RTSS_DEFAULTS.seamBand, 0.01),
      DECLARE_INT_COMBOBOX(rtss_panel, debug, 0, 0, 1, 2, 3, 4, 5, 6),
      DECLARE_INT_SLIDER(rtss_panel, debug_level, 0, RTSS_MAX_LEVELS - 1, 0), DECLARE_BOOL_CHECKBOX(rtss_panel, show_stats, false)};
    de3_imgui_build(skyGuiDesc, sizeof(skyGuiDesc) / sizeof(skyGuiDesc[0]));

    if (const DataBlock *def_envi = dgs_get_settings()->getBlockByName("guiDefVal"))
    {
      DataBlock env_stg;
      de3_imgui_load_values(*def_envi);
      de3_imgui_save_values(env_stg);
    }

    curCamera = freeCam;
    ::set_camera_pos(*curCamera, Point3(0, 0, 0));
    samplebenchmark::setupBenchmark(curCamera, []() {
      quit_game(1);
      return;
    });

    if (!is_managed_textures_streaming_load_on_demand())
      ddsx::tex_pack2_perform_delayed_data_loading(); // or it will crash here
    d3d::GpuAutoLock gpu_al;
    d3d::set_render_target();
#if _TARGET_C1 | _TARGET_C2

#else
    visualConsoleDriver = new VisualConsoleDriver;
#endif
    console::set_visual_driver(visualConsoleDriver, NULL);
    if (dgs_get_settings()->getStr("model", NULL))
      scheduleDynModelLoad(dgs_get_settings()->getStr("model", "test"), dgs_get_settings()->getStr("model_skel", NULL),
        dgs_get_settings()->getPoint3("model_pos", Point3(0, 0, 0)));

    setDirToSun();

    sky_panel.moon_color = sky_colors.moon_color;
    sky_panel.mie_scattering_color = sky_colors.mie_scattering_color;
    sky_panel.mie_absorption_color = sky_colors.mie_absorption_color;
    sky_panel.rayleigh_color = sky_colors.rayleigh_color;
    sky_panel.mie2_thickness = layered_fog.mie2_thickness;
    sky_panel.mie2_altitude = layered_fog.mie2_altitude;
    sky_panel.mie2_scale = layered_fog.mie2_scale;
    daSkies.setSkyParams(sky_panel);
    sky_panel.min_ground_offset = 0;
    // daSkies.generateClouds(clouds_gen, clouds_gen.gpu, clouds_gen.need_tracer, clouds_gen.regenerate);
    // daSkies.setCloudsPosition(clouds_panel, -1);
    daSkies.setStrataClouds(strata_clouds);
    // daSkies.setLayeredFog(layered_fog);
    daSkies.prepare(dir_to_sun, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      ShaderGlobal::set_float4(get_shader_variable_id("sun_light_color"), color4(sun, 0));
      ShaderGlobal::set_float4(get_shader_variable_id("amb_light_color", true), color4(amb, 0));
    }
    else
    {
      G_ASSERT(0);
    }

    reinitCube(128);

    scheduleLevelBinLoad(dgs_get_settings()->getStr("level", NULL), dgs_get_settings()->getStr("heightmap", NULL),
      dgs_get_settings()->getReal("heightmap_cell_size", 0), dgs_get_settings()->getReal("heightmap_hMin", 0),
      dgs_get_settings()->getReal("heightmap_hMax", 1));
    extern float hmap_water_level;
    water_panel.water_level = hmap_water_level = dgs_get_settings()->getReal("water_level", -10000.0f);
    ShaderGlobal::set_float(water_levelVarId, hmap_water_level);
  }


  void renderEnvi(const RenderView &view, const TMatrix &itm, const Driver3dPerspective &persp)
  {
    TIME_D3D_PROFILE(envi)
    view.bind();
    daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 3,
      farDownsampledDepth[currentDownsampledDepth], farDownsampledDepth[1 - currentDownsampledDepth], target->getDepth(),
      main_pov_data.get(), view.view, view.proj, persp);
  }
  struct FilePanel
  {
    bool save, load, export_min, export_max, import_minmax;
  } file_panel;

  SkyAtmosphereParams sky_panel;
  SkyAtmosphereParams sky_colors;

  SkyAtmosphereParams layered_fog;

  DaSkies::StrataClouds strata_clouds;
  DaSkies::CloudsRendering clouds_rendering2;
  DaSkies::CloudsWeatherGen clouds_weather_gen2;
  DaSkies::CloudsSettingsParams clouds_game_params;
  DaSkies::CloudsForm clouds_form;

  struct SunPanel
  {
    float sun_zenith = 70, sun_azimuth = 30;
    float moon_age = 10;
    bool astronomical = false, local_time = true;
    float longitude = 55, latitude = 37, time = 1;
    int year = 1943, month = 6, day = 1;
    bool animate = false, time_skip = false;
    float day_minutes = 24, axis_tilt = 40;
  } sun_panel;
  struct
  {
    bool level, enabled, trees, spheres, prepass, collision, lmesh;
    float heightScale, heightMin, cellSize;
    float spheres_speed;
  } land_panel;

  struct
  {
    bool ssao = true, gtao = true;
    int reflections = REFLECTIONS_ALL;
    int onscreen_mode = RESULT;
    int gi_mode = ENVI_PROBE;
    float dynamic_gi_quality = 1.0;
    float exposure = 1;
    bool fake_dynres = false;
    bool update_radiance = true;
  } gi_panel;
  enum
  {
    RTSS_DITHERED_HARDEN = 0,
    RTSS_TENT = 1
  };
  enum
  {
    RTSS_SWRT,
    RTSS_HWRT,
    RTSS_RASTER,
    RTSS_HYBRID_SW,
    RTSS_HYBRID_HW
  };
  struct : RaytracedToroidalStaticShadowSettings
  {
    int recipe = RTSS_SWRT;
    int hwLevels = 3;
    float rasterMaxTexel = 0.25f;
    float rasterCost = 1, augmentCost = 1.5;
    int screen_filter = RTSS_DITHERED_HARDEN;
    float screen_radius = 1.5;
    int debug = 0, debug_level = 0;
    bool show_stats = false;
  } rtss_panel;
  struct
  {
    int clips = 5, texWidth = 128;
    float voxel0Size = 0.15, yResScale = 0.5;
  } sdf_panel;
  struct
  {
    int clips = 3, texWidth = 32;
  } radiance_grid_panel;
  struct
  {
    int clips = 2;
  } albedo_scene_panel;

  struct
  {
    int tileSize = 16, radianceOctRes = 8;
    float temporality = 0.5;
    bool reproject = true;
    bool angleFiltering;
  } screen_probes;
  struct
  {
    int tileSize = 64, slices = 16;
    float zLogMul = 6, blurTexelOfs = 0.124;
    bool debugUse = true;
  } volumetric_gi;

  enum
  {
    PANORAMA = 0,
    DIRECT = 2
  };
  struct RenderPanel
  {
    int render_type;
    bool infinite_skies, panorama_blending;
    bool trace_ray, compareCpuSunSky;
    int sky_quality;
    int direct_quality;
    int panoramaResolution;
    float cloudsSpeed, strataCloudsSpeed, windDir;

    RenderPanel() :
      render_type(DIRECT),
      infinite_skies(false),
      sky_quality(1),
      direct_quality(1),
      panoramaResolution(2048),
      panorama_blending(false),
      cloudsSpeed(20),
      strataCloudsSpeed(10),
      windDir(0),
      trace_ray(false),
      compareCpuSunSky(false)
    {}
  } render_panel;
  struct WaterPanel
  {
    bool enabled = false;
    float water_level = 0;
  } water_panel;


  DaSkies daSkies;

  RenderDynamicCube cube;
  light_probe::Cube *localProbe;
  console::IVisualConsoleDriver *visualConsoleDriver;

  void setModel(DynamicRenderableSceneInstance *s)
  {
    test = s;
    // a type is baked from the model, so it can only be reused for the same resource, and the
    // lib has no per type eviction: clear the whole set on a swap. this sample gathers one
    // type, so keeping the old ones would only empty bake bricks nothing ever injects
    auto *res = test ? test->getLodsResource() : nullptr;
    if (res != mediaVolTypeRes)
    {
      mediaVolTypeRes = res;
      mediaVolumes.clear();
      treeMediaVolType = res ? mediaVolumes.addType(test->getLocalBoundingBox()) : -1;
    }
    // a latched checkbox refilled these regions empty already, and an uncheck while no model
    // was loaded skipped its clear: both leave the box regions wrong until this refill
    if ((treesInGI || treesMediaClearPending) && treeMediaVolType >= 0)
    {
      daGI2->invalidateBox(treesWorldBox());
      treesMediaClearPending = false;
    }

    if (test && bvhCtx)
    {
      testId = ++bvhIdGen;
      add_dynrend_resource_to_bvh(bvhCtx, test->getLodsResource(), testId);
    }
  }

  void scheduleDynModelLoad(const char *name, const char *skel_name, const Point3 &pos)
  {
    struct LoadJob : cpujobs::IJob
    {
      DemoGameScene *s;
      SimpleString name, skelName;
      Point3 pos;

      LoadJob(DemoGameScene *sc, const char *_name, const char *_skel_name, const Point3 &_pos) :
        s(sc), name(_name), skelName(_skel_name), pos(_pos)
      {}
      const char *getJobName(bool &) const override { return DAPROFILER_STRING("LoadJob_scheduleDynModelLoad"); }
      virtual void doJob()
      {
        DynamicRenderableSceneInstance *val = NULL;

        auto rrl = create_res_restriction_list(name, 2);
        if (!skelName.empty())
          extend_res_restriction_list(rrl, skelName);

        auto *resource = (DynamicRenderableSceneLodsResource *)get_game_resource_ex(name, DynModelGameResClassId, rrl);
        G_ASSERTF(resource, "Cannot load '%s'", name.str());
        if (resource)
        {
          val = new DynamicRenderableSceneInstance(resource);
          release_game_resource_ex(resource, DynModelGameResClassId); // Release original resource.
        }

        TMatrix tm;
        tm.identity();
        tm.setcol(3, pos);
        if (!skelName.empty() && val)
        {
          if (auto *skel = (GeomNodeTree *)get_game_resource_ex(skelName, GeomNodeTreeGameResClassId, rrl))
          {
            skel->setRootTmScalar(tm);
            skel->invalidateWtm();
            skel->calcWtm();

            iterate_names(val->getLodsResource()->getNames().node, [&](int id, const char *nodeName) {
              if (auto node = skel->findNodeIndex(nodeName))
                val->setNodeWtm(id, skel->getNodeWtmRel(node));
              else if (stricmp(nodeName, "@root") == 0)
                val->setNodeWtm(id, skel->getRootWtmRel());
            });
            release_game_resource_ex(skel, GeomNodeTreeGameResClassId); // Release original resource.
          }
          else
            logerr("skeleton %s not found", skelName);
        }
        else if (val)
        {
          for (int i : val->getLodsResource()->getNames().node.id)
            val->setNodeWtm(i, tm);
        }
        free_res_restriction_list(rrl);

        prefetch_managed_textures_by_textag(TEXTAG_DYNMODEL);
        if (!is_managed_textures_streaming_load_on_demand())
          ddsx::tex_pack2_perform_delayed_data_loading();

        delayed_call([scene = s, val] { scene->setModel(val); });
      }
      virtual void releaseJob() { delete this; }
    };
    test = NULL;
    cpujobs::add_job(loading_job_mgr_id, new LoadJob(this, name, skel_name, pos));
  }

  void scheduleLevelBinLoad(const char *level_bindump_fn, const char *hmap_fn = nullptr, float cell = 0, float hMin = 0,
    float hMax = 1)
  {
    if (!level_bindump_fn && !hmap_fn)
      return;
    struct LoadJob : cpujobs::IJob
    {
      SimpleString fn, hmap_fn;
      float cellSize = 0;
      float hMin = 0, hMax = 1;
      DemoGameScene *scene;

      LoadJob(const char *_fn, const char *hmap_fn, DemoGameScene *owner, float cell, float h_min, float h_max) :
        fn(_fn), hmap_fn(hmap_fn), scene(owner), cellSize(cell), hMin(h_min), hMax(h_max)
      {}
      const char *getJobName(bool &) const override { return DAPROFILER_STRING("LoadJob_scheduleLevelBinLoad"); }
      virtual void doJob()
      {
        scene_type_t *scn = new scene_type_t;
        if (!fn.empty())
        {
          textag_mark_begin(TEXTAG_LAND);
          scn->openSingle(fn);
          textag_mark_end();
        }
        if (!hmap_fn.empty())
        {
          if (cellSize > 0)
          {
            if (strstr(hmap_fn, ".r16"))
              scn->loadHeightmapRaw16(hmap_fn, cellSize, hMin, hMax);
            else
              scn->loadHeightmapRaw(hmap_fn, cellSize);
          }
          else
            scn->loadHeightmap(hmap_fn);
        }
        prefetch_managed_textures_by_textag(TEXTAG_LAND);
        // if (!is_managed_textures_streaming_load_on_demand())
        ddsx::tex_pack2_perform_delayed_data_loading();
        delayed_call([owner = scene, scn] {
          owner->binScene = scn;
          ddsx::tex_pack2_perform_delayed_data_loading();
          owner->onSceneLoaded();
          owner->addBvhMeshes(scn);
        });
      }
      virtual void releaseJob() { delete this; }
    };

    cpujobs::add_job(loading_job_mgr_id, new LoadJob(level_bindump_fn, hmap_fn, this, cell, hMin, hMax));
  }

  // one context: the collision TLAS the GI traces, plus the RTSM meshes when on
  bvh::ContextId bvhCtx = bvh::InvalidContextId;
  eastl::vector<uint64_t> bvhMeshes;
  uint64_t bvhIdGen = 0;
  bool bvhLruConnected = false;
  float bvhLruRadius = 0;
  uint32_t bvhBuildCount = 0;
  uint64_t bvhMeshBase = 0;
  uint64_t bvhMeshCount = 0;
  uint64_t testId = 0;

  struct ElemCallback : public RenderScene::ElemCallback
  {
    bvh::ContextId bvhCtx;
    uint64_t *bvhIdGen;

    void on_elem_found(const Data &data) override
    {
      G_ASSERT(data.normalFormat == -1 || data.normalFormat == VSDT_E3DCOLOR);
      G_ASSERT(data.colorFormat == -1 || data.colorFormat == VSDT_E3DCOLOR);

      uint64_t id = ++(*bvhIdGen);

      bvh::MeshInfo meshInfo;
      meshInfo.indices = data.indices;
      meshInfo.indexCount = data.indexCount;
      meshInfo.startIndex = data.startIndex;
      meshInfo.vertices = data.vertices;
      meshInfo.vertexCount = data.vertexCount;
      meshInfo.startVertex = data.startVertex;
      meshInfo.vertexSize = data.vertexStride;
      meshInfo.positionOffset = data.positionOffset;
      meshInfo.positionFormat = data.positionFormat;
      meshInfo.texcoordOffset = data.texcoordOffset;
      meshInfo.texcoordFormat = data.texcoordFormat;
      meshInfo.normalOffset = data.normalOffset;
      meshInfo.colorOffset = bvh::MeshInfo::invalidOffset; // The sponza shader doesn't use vertex colors
      meshInfo.posMul = Point4::ONE;
      meshInfo.posAdd = Point4::ZERO;
      meshInfo.albedoTextureId = data.albedoTextureId;
      if (data.albedoTextureId != BAD_TEXTUREID)
        meshInfo.vertexProcessor = &bvh::ProcessorInstances::getBakeTextureToVerticesProcessor();
      bvh::add_mesh(bvhCtx, id, meshInfo);
    }
  } elemCallback;

  bool isRtEnabled() const
  {
    if (!bvh::is_available())
      return false;
    return dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableBVH", false);
  }
  bool rtsmIsOn = false;
  bool isRtsmEnabled()
  {
    rtsmIsOn = false;
    if (!isRtEnabled())
      return false;
    return rtsmIsOn = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableRTSM", false);
  }

  void initBvh()
  {
    if (!isRtEnabled())
      return;
    bvh::init();
    const bvh::Features features = isRtsmEnabled() ? static_cast<bvh::Features>(bvh::ForGI | bvh::LruCollision) : bvh::LruCollision;
    bvhCtx = bvh::create_context("GI", features, static_cast<bvh::Features>(0));
  }

public:
  // BLAS/TLAS handles die with the device: torn down before a reset, rebuilt after
  void teardownBvh(bool for_device_reset)
  {
    if (bvhCtx == bvh::InvalidContextId)
      return;
    bvh::teardown(bvhCtx); // joins the collision gather worker
    bvh::teardown(for_device_reset, true);
    bvhLruConnected = false;
    bvhLruRadius = 0;
    if (for_device_reset)
      bvhIdGen = bvhMeshBase = bvhMeshCount = testId = 0;
    else
    {
      rt_textures.clear();
      if (rtsmIsOn)
        rtsm::teardown();
      denoiser::teardown();
    }
  }
  void reinitBvhAfterReset()
  {
    initBvh();
    initRtsmDenoiser(rtsmResW, rtsmResH);
    // the collision streamer reconnects lazily; only the RTSM meshes need re-adding
    if (bvhCtx == bvh::InvalidContextId || !rtsmIsOn)
      return;
    if (binScene)
      addBvhMeshes(binScene);
    if (test)
    {
      testId = ++bvhIdGen;
      add_dynrend_resource_to_bvh(bvhCtx, test->getLodsResource(), testId);
    }
  }

protected:
  void addBvhMeshes(scene_type_t *bin_scene)
  {
    // rendering meshes serve RTSM only
    if (bvhCtx && rtsmIsOn)
    {
      d3d::GpuAutoLock gpu_al;
      elemCallback.bvhCtx = bvhCtx;
      elemCallback.bvhIdGen = &bvhIdGen;

      bvhMeshBase = bvhIdGen + 1;

      RenderSceneManager &sm = bin_scene->getRsm();
      for (RenderScene *scene : sm.getScenes())
      {
        if (!scene)
          continue;

        scene->foreachElem(elemCallback);
      }

      bvhMeshCount = bvhIdGen - bvhMeshBase + 1;
    }
  }

  bool hwRtAvailable() const
  {
    // constraint: the HW shader variants exist only in the dx12 dump
#if _TARGET_PC
    const bool inDump = d3d::get_driver_code().is(d3d::dx12);
#else
    constexpr bool inDump = false;
#endif
    // constraint: with no resident collision (still gathering, or no collision dump) a collision trace resolves to sky
    return inDump && bvh::get_lru_collision_stats(bvhCtx).residentInstances > 0;
  }

  float effectiveBvhRadius() const
  {
    if (bvh_radius > 0)
      return bvh_radius;
    // the world SDF far distance: the TLAS replaces it, so it covers the same range
    return sdf_panel.voxel0Size * float(1 << (sdf_panel.clips - 1)) * sdf_panel.texWidth * 0.5f;
  }
  static float bvhBorder(float radius) { return clamp(radius * 0.25f, 16.f, 128.f); }

  void updateCollisionBvhStreaming()
  {
    if (!bvhLruConnected && lruColl.lruColl && lruColl.level.instanceCount())
    {
      bvh::LruCollisionSettings s;
      s.radius = bvhLruRadius = effectiveBvhRadius();
      s.border = bvhBorder(s.radius);
      bvh::connect_lru_collision(
        bvhCtx, lruColl.lruColl.get(),
        [this](bbox3f_cref box, dag::Vector<rendinst::riex_handle_t> &out, dag::Vector<mat43f> &out_tms) {
          // boxCull takes the scene read lock itself; no outer lock, a second
          // acquire deadlocks if the lock ever stops being reentrant
          lruColl.level.boxCull(box, [&](uint64_t h, mat44f_cref node) {
            out.push_back(h);
            // the w components carry pool/flags/bsphere, the transpose drops them
            mat43f tm;
            v_mat44_transpose_to_mat43(tm, node);
            out_tms.push_back(tm);
          });
        },
        s);
      bvhLruConnected = true;
    }
    const float radius = effectiveBvhRadius();
    if (bvhLruConnected && radius != bvhLruRadius)
    {
      bvhLruRadius = radius;
      bvh::set_lru_collision_range(bvhCtx, radius, bvhBorder(radius));
    }
  }

  void buildBvh(const TMatrix &itm)
  {
    if (bvhCtx == bvh::InvalidContextId)
      return;

    const Point3 viewPos = itm.getcol(3);

    bvh::start_frame();
    updateCollisionBvhStreaming();
    bvh::process_meshes(bvhCtx);

    if (rtsmIsOn)
    {
      // RTSM meshes re-emitted per frame; collision proxies must not cast rt shadows
      bvh::update_instances(bvhCtx, Point3::ZERO, Point3::ZERO, Point3(0, -1, 0), TMatrix::IDENT, TMatrix4::IDENT, Frustum(),
        Frustum(), nullptr, nullptr, nullptr, {}, threadpool::PRIO_HIGH);

      mat43f instanceTransform;
      instanceTransform.row0 = v_make_vec4f(1, 0, 0, 0);
      instanceTransform.row1 = v_make_vec4f(0, 1, 0, 0);
      instanceTransform.row2 = v_make_vec4f(0, 0, 1, 0);

      for (int meshIx = 0; meshIx < bvhMeshCount; ++meshIx)
        bvh::add_instance(bvhCtx, bvhMeshBase + meshIx, instanceTransform);

      if (test)
      {
        // re-emitted per frame, not streamed: not bound to the collision window
        constexpr float rtsmDynCullRadius = 10000.f;
        add_dynrend_instance_to_bvh(bvhCtx, test, testId, testCount, viewPos, rtsmDynCullRadius);
      }
    }

    // every frame: the collision streaming window advances inside build
    bvh::build(bvhCtx, itm, TMatrix4::IDENT, viewPos, Point3::ZERO);
    ++bvhBuildCount;
  }

public:
  void printBvhStats()
  {
    const auto s = bvh::get_lru_collision_stats(bvhCtx);
    console::print_d("bvh: %d builds, collision revision %u, %d resident instances, %d built models, cache %dK of %dK, settled %d",
      bvhBuildCount, s.revision, s.residentInstances, s.builtModels, int(s.cachedBytes >> 10), int(s.cacheLimit >> 10),
      int(s.settled));
  }
  void printRtssStats()
  {
    RaytracedToroidalStaticShadow::Stats s;
    rtss->getStats(s);
    const RaytracedToroidalStaticShadowSettings &set = rtss->getSettings();
    console::print_d("%s", rtssPagesLine(s).c_str());
    for (auto [i, l] : enumerate(s.levels))
      console::print_d(
        "rtss: level %d sweeping %d progress %.2f sun error %.3f deg, last sweep %.2f s, slots valid %d dirty %d empty %d, "
        "tolerance %.3f deg, sweep %.2f s, fast %d, sweeps %d, pretrace blocked %d",
        int(i), l.sweeping, l.sweepProgress, l.sunErrorDeg, l.lastSweepSeconds, l.validSlots, l.dirtySlots, l.emptySlots,
        l.toleranceDeg, l.sweepSeconds, l.fastSweep, l.sweeps, l.pretraceBlocked);
    console::print_d("rtss: debug pass lookups %u, no data %u, fallbacks: released slot %u, bit %u, depth %u, reference %u pixels, "
                     "%.3f%% differ",
      s.gpu.lookups, s.gpu.noData, s.gpu.fallbackReleased, s.gpu.fallbackBit, s.gpu.fallbackDepth, s.gpu.referencePixels,
      100. * s.gpu.referenceDiff / max(s.gpu.referencePixels, 1u));
    console::print_d("rtss: recipe %d, HW mask %x, updatePagesPerFrame %d, cost required %.2f other %.2f, raster jobs %d skipped "
                     "%d provisional %d empty %d",
      rtss_panel.recipe, rtssHwMask, set.updatePagesPerFrame, s.cost[RTSS_REQUIRED],
      s.cost[RTSS_DIRTY] + s.cost[RTSS_SWEEP] + s.cost[RTSS_PRETRACE] + s.cost[RTSS_FILL], s.rasterJobs, s.rasterSkipped,
      s.rasterProvisional, s.rasterEmpty);
    for (int k = 0; k < set.levels; ++k)
      console::print_d("rtss: level %d raster %d cost %.1f", k, int(set.recipes[k].raster), set.recipes[k].pageCost);
    const bvh::LruCollisionStats lru = bvh::get_lru_collision_stats(bvhCtx);
    console::print_d("rtss: lru revision %u (invalidated %u), settled %d, resident %u", lru.revision, rtssLruRevision,
      int(lru.settled), lru.residentInstances);
  }

protected:
};

DemoGameScene *DemoGameScene::the_scene = nullptr;

static void testgi_bvh_before_reset(bool full_reset)
{
  if (full_reset && DemoGameScene::the_scene)
    DemoGameScene::the_scene->teardownBvh(true);
}
REGISTER_D3D_BEFORE_RESET_FUNC(testgi_bvh_before_reset);

static void testgi_bvh_after_reset(bool full_reset)
{
  if (full_reset && DemoGameScene::the_scene)
    DemoGameScene::the_scene->reinitBvhAfterReset();
}
REGISTER_D3D_AFTER_RESET_FUNC(testgi_bvh_after_reset);

static void testgi_scene_and_gi_after_reset(bool full_reset)
{
  if (full_reset && DemoGameScene::the_scene)
  {
    DemoGameScene::the_scene->sceneAfterDeviceReset();
    DemoGameScene::the_scene->giAfterDeviceReset();
  }
}
REGISTER_D3D_AFTER_RESET_FUNC(testgi_scene_and_gi_after_reset);
// headers needed for startup only
// #include <fx/dag_fxInterface.h>
// #include <fx/dag_commonFx.h>
class TestConsole : public console::ICommandProcessor
{
  virtual bool processCommand(const char *argv[], int argc);
  void destroy() {}
};

static bool tobool(const char *s)
{
  if (!s || !*s)
    return false;
  else if (!stricmp(s, "true") || !stricmp(s, "on"))
    return true;
  else if (!stricmp(s, "false") || !stricmp(s, "off"))
    return false;
  else
    return atoi(s);
}

static void toggle_or_set_bool_arg(bool &b, int argn, int argc, const char *argv[])
{
  if (argn < argc)
    b = tobool(argv[argn]);
  else
    b = !b;

  console::print(b ? "on" : "off");
}


bool TestConsole::processCommand(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("app", "quit", 1, 1) { quit_game(0); }
  CONSOLE_CHECK_NAME("app", "exit", 1, 1) { quit_game(0); }
  CONSOLE_CHECK_NAME("app", "screenshot", 1, 2)
  {
    screenshotRequested = true;
    screenshotFilename = argc > 1 ? argv[1] : "";
    console::print_d("screenshot: will capture next frame");
  }
  CONSOLE_CHECK_NAME("app", "save_verify_capture", 1, 2)
  {
    verifyCaptureRequested = true;
    verifyCaptureDir = argc > 1 ? argv[1] : "";
    console::print_d("save_verify_capture: will capture this frame");
  }
  CONSOLE_CHECK_NAME("profiler", "events", 1, 2)
  {
    auto &e = ((DemoGameScene *)dagor_get_current_game_scene())->allEvents;
    bool on = argc > 1 ? console::to_bool(argv[1]) : !e.profileAll;
    if (on)
      e.addProfile("*", 1024);
    else
      e.addProfile("-", 1024);
  }
  CONSOLE_CHECK_NAME("render", "reload_cube", 1, 1)
  {
    console::command("shaders.reload");
    ((DemoGameScene *)dagor_get_current_game_scene())->reloadCube();
  }
  CONSOLE_CHECK_NAME("render", "reinit_cube", 1, 2)
  {
    console::command("shaders.reload");
    ((DemoGameScene *)dagor_get_current_game_scene())->reinitCube(atoi(argv[1]));
  }
  CONSOLE_CHECK_NAME("app", "dflush", 1, 1) { debug_flush(false); }
  CONSOLE_CHECK_NAME("app", "reset_device", 1, 1) { dagor_d3d_force_driver_reset = true; }
  static bool profilerFileServerStarted = false;
  CONSOLE_CHECK_NAME("app", "profiler_start", 1, 1)
  {
    if (!profilerFileServerStarted)
      profilerFileServerStarted = da_profiler::start_file_dump_server(".logs");
    da_profiler::add_mode(da_profiler::CONTINUOUS);
    if (profilerFileServerStarted)
      console::print_d("profiler: continuous capture started");
    else
      console::print_d("profiler: file dump server did not start (profiler compiled out?), nothing will be captured");
  }
  CONSOLE_CHECK_NAME("app", "profiler_stop", 1, 1)
  {
    da_profiler::request_dump();
    da_profiler::remove_mode(da_profiler::CONTINUOUS);
    if (profilerFileServerStarted)
      console::print_d("profiler: capture dump saved to .logs");
    else
      console::print_d("profiler: no file dump server, nothing was captured");
  }
  CONSOLE_CHECK_NAME("render", "bvh_stats", 1, 1) { ((DemoGameScene *)dagor_get_current_game_scene())->printBvhStats(); }
  CONSOLE_CHECK_NAME("camera", "save", 1, 2)
  {
    TMatrix camTm;
    ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->getInvViewMatrix(camTm);
    save_camera_position("cameraPositions.blk", argc > 1 ? argv[1] : "slot0", camTm);
  }
  CONSOLE_CHECK_NAME("camera", "move", 1, 4)
  {
    Point3 &v = ((DemoGameScene *)dagor_get_current_game_scene())->cameraMoveVel;
    v = Point3(argc > 1 ? atof(argv[1]) : 0.f, argc > 2 ? atof(argv[2]) : 0.f, argc > 3 ? atof(argv[3]) : 0.f);
  }
  CONSOLE_CHECK_NAME("camera", "restore", 1, 2)
  {
    TMatrix camTm;
    if (load_camera_position("cameraPositions.blk", argc > 1 ? argv[1] : "slot0", camTm))
      ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->setInvViewMatrix(camTm);
  }
  CONSOLE_CHECK_NAME("camera", "pos", 1, 4)
  {
    TMatrix itm;
    ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->getInvViewMatrix(itm);
    static Point3 lastPos = itm.getcol(3);
    Point3 pos = itm.getcol(3);
    if (argc == 1)
      console::print_d("Camera pos %g %g %g, moved %g", pos.x, pos.y, pos.z, length(lastPos - pos));
    lastPos = pos;
    if (argc == 2)
      itm.setcol(3, pos.x, atof(argv[1]), pos.z);
    if (argc == 3)
      itm.setcol(3, atof(argv[1]), pos.y, atof(argv[2]));
    if (argc == 4)
      itm.setcol(3, atof(argv[1]), atof(argv[2]), atof(argv[3]));

    if (argc > 1)
      ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->setInvViewMatrix(itm);
  }
  CONSOLE_CHECK_NAME("render", "show_gbuffer", 1, 2)
  {
    setDebugGbufferMode(argc > 1 ? argv[1] : "");
    console::print("usage: show_gbuffer (%s)", getDebugGbufferUsage().c_str());
  }
  CONSOLE_CHECK_NAME("render", "wireframe", 1, 2)
  {
    ::grs_draw_wire = argc > 1 ? tobool(argv[1]) : !::grs_draw_wire;
    console::print("wireframe is %s", ::grs_draw_wire ? "on" : "off");
  }
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    String str = scene->debugTexOverlay.processConsoleCmd(argv, argc);
    if (!str.empty())
      console::print(str);
  }
  auto rtssPanel = []() -> auto & { return ((DemoGameScene *)dagor_get_current_game_scene())->rtss_panel; };
  CONSOLE_CHECK_NAME("rtss", "stats", 1, 1) { ((DemoGameScene *)dagor_get_current_game_scene())->printRtssStats(); }
  CONSOLE_CHECK_NAME("rtss", "invalidate", 1, 1) { ((DemoGameScene *)dagor_get_current_game_scene())->rtss->invalidate(); }
  CONSOLE_CHECK_NAME("rtss", "invalidate_around", 2, 3)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    TMatrix itm;
    scene->curCamera->getInvViewMatrix(itm);
    const BBox3 box(itm.getcol(3), 2 * atof(argv[1]));
    scene->rtss->invalidateBoxes(&box, 1, argc > 2 ? atoi(argv[2]) : ~0u);
  }
  CONSOLE_CHECK_NAME("rtss", "settle", 1, 1)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    console::print_d("rtss: %d frames since the last frame with a job", scene->rtssFramesSinceJob);
  }
  CONSOLE_CHECK_NAME("rtss", "layout", 3, 3)
  {
    rtssPanel().windowPages = atoi(argv[1]);
    rtssPanel().slotTexels = atoi(argv[2]);
  }
  CONSOLE_CHECK_NAME("sun", "dir", 3, 3)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    scene->sun_panel.sun_zenith = atof(argv[1]);
    scene->sun_panel.sun_azimuth = atof(argv[2]);
  }
  CONSOLE_CHECK_NAME("sun", "animate", 2, 4)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    scene->sun_panel.animate = tobool(argv[1]);
    if (argc > 2)
      scene->sun_panel.day_minutes = clamp(float(atof(argv[2])), 1.f, 240.f);
    if (argc > 3)
      scene->sun_panel.axis_tilt = clamp(float(atof(argv[3])), 0.f, 80.f);
  }
  CONSOLE_CHECK_NAME("rtss", "debug", 2, 3)
  {
    rtssPanel().debug = clamp(atoi(argv[1]), 0, 6);
    if (argc > 2)
      rtssPanel().debug_level = clamp(atoi(argv[2]), 0, RTSS_MAX_LEVELS - 1);
  }
  CONSOLE_CHECK_NAME("rtss", "update_pages_per_frame", 2, 2) { rtssPanel().updatePagesPerFrame = clamp(atoi(argv[1]), 1, 256); }
  CONSOLE_CHECK_NAME("rtss", "batch_frames", 2, 2) { rtssPanel().batchFrames = clamp(atoi(argv[1]), 1, 16); }
  CONSOLE_CHECK_NAME("rtss", "screen_filter", 2, 2) { rtssPanel().screen_filter = clamp(atoi(argv[1]), 0, 1); }
  CONSOLE_CHECK_NAME("rtss", "screen_radius", 2, 2) { rtssPanel().screen_radius = clamp(float(atof(argv[1])), 0.25f, 4.f); }
  CONSOLE_CHECK_NAME("rtss", "max_anisotropy", 2, 2) { rtssPanel().maxAnisotropy = clamp(float(atof(argv[1])), 1.f, 8.f); }
  CONSOLE_CHECK_NAME("rtss", "border_texels", 2, 2) { rtssPanel().borderTexels = clamp(atoi(argv[1]), 1, 4); }
  CONSOLE_CHECK_NAME("rtss", "normal_bias_texels", 2, 2) { rtssPanel().normalBiasTexels = clamp(float(atof(argv[1])), 0.f, 8.f); }
  CONSOLE_CHECK_NAME("rtss", "seam_band", 2, 2) { rtssPanel().seamBand = clamp(float(atof(argv[1])), 0.f, 0.5f); }
  CONSOLE_CHECK_NAME("rtss", "recipe", 2, 2) { rtssPanel().recipe = clamp(atoi(argv[1]), 0, 4); }
  CONSOLE_CHECK_NAME("rtss", "hw_levels", 2, 2) { rtssPanel().hwLevels = clamp(atoi(argv[1]), 0, RTSS_MAX_LEVELS); }
  CONSOLE_CHECK_NAME("rtss", "raster_max_texel", 2, 2) { rtssPanel().rasterMaxTexel = clamp(float(atof(argv[1])), 0.01f, 2.f); }
  CONSOLE_CHECK_NAME("rtss", "raster_cost", 2, 3)
  {
    rtssPanel().rasterCost = clamp(float(atof(argv[1])), 0.1f, 16.f);
    if (argc > 2)
      rtssPanel().augmentCost = clamp(float(atof(argv[2])), 0.1f, 16.f);
  }
  CONSOLE_CHECK_NAME("rtss", "show_stats", 1, 2) { toggle_or_set_bool_arg(rtssPanel().show_stats, 1, argc, argv); }
  /*CONSOLE_CHECK_NAME("render", "objects_sdf_atlas_sz", 4, 4)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    if (scene->objectsSdf)
      scene->objectsSdf->setAtlasSize(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
  }*/
  return found;
}


static TestConsole test_console;
void game_demo_init()
{
  /*char buf1[66], buf2[66];
  decode_static_str_with_defkey(
  "d\0u\0m\0m\0y\0_\0\0\0t\0\x31\0_\0b\0a\0c\0k\0r\0o\0u\0n\0d\0\0\0t\0\x32\x0_\0b\0a\0c\0k\0r\0o\0u\0n\0d\0\0\0",
  buf1);
  file_ptr_t file = df_open("teste.str", DF_CREATE|DF_WRITE);
  df_write(file, buf1, 66);
  df_close(file);
  char bufe[67] =
  {
    0x74, 0x10, 0x65, 0x10, 0x7D, 0x10, 0x7D, 0x10, 0x69, 0x10, 0x4F, 0x10, 0x10, 0x10, 0x64, 0x10,
    0x21, 0x10, 0x4F, 0x10, 0x72, 0x10, 0x71, 0x10, 0x73, 0x10, 0x7B, 0x10, 0x62, 0x10, 0x7F, 0x10,
    0x65, 0x10, 0x7E, 0x10, 0x74, 0x10, 0x10, 0x10, 0x64, 0x10, 0x22, 0x10, 0x4F, 0x10, 0x72, 0x10,
    0x71, 0x10, 0x73, 0x10, 0x7B, 0x10, 0x62, 0x10, 0x7F, 0x10, 0x65, 0x10, 0x7E, 0x10, 0x74, 0x10,
    0x10, 0x10
  };
  decode_static_str_with_defkey(bufe, buf2);
  file = df_open("testd.str", DF_CREATE|DF_WRITE);
  df_write(file, buf2, 66);
  df_close(file);*/

  debug("[DEMO] registering factories");
  const DataBlock &blk = *dgs_get_settings();
  ::enable_tex_mgr_mt(true, blk.getInt("max_tex_count", 2048));
  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  console::init();
  add_con_proc(&test_console);

  enable_taa_override = dgs_get_settings()->getBlockByNameEx("render")->getBool("taa", false);

  if (auto *sgsrBlock = dgs_get_settings()->getBlockByName("SnapdragonSuperResolution"))
  {
    use_snapdragon_super_resolution = sgsrBlock->getBool("enable", false);
    snapdragon_super_resolution_scale = ((float)sgsrBlock->getInt("scale", 75) / 100.f);
  }
  debug("[hmatthew] enable_taa_override=%d use_snapdragon_super_resolution=%d", enable_taa_override, use_snapdragon_super_resolution);

#define LOAD_RES_PATCH(LOC_NAME)                                                                  \
  if (dd_file_exists("content/patch/" LOC_NAME "/grp_hdr.vromfs.bin") &&                          \
      (vrom = load_vromfs_dump("content/patch/" LOC_NAME "/grp_hdr.vromfs.bin", tmpmem)) != NULL) \
  {                                                                                               \
    String mntPoint("content/patch/" LOC_NAME "/");                                               \
    ::add_vromfs(vrom, true, mntPoint);                                                           \
    load_res_packs_patch(mntPoint + "resPacks.blk", LOC_NAME "/grp_hdr.vromfs.bin", true, true);  \
    ::remove_vromfs(vrom);                                                                        \
    tmpmem->free(vrom);                                                                           \
  }

  VirtualRomFsData *vrom = ::load_vromfs_dump("res/grp_hdr.vromfs.bin", tmpmem);
  if (vrom)
    ::add_vromfs(vrom);
  ::load_res_packs_from_list("res/respacks.blk");
  if (vrom)
  {
    ::remove_vromfs(vrom);
    tmpmem->free(vrom);
  }
  LOAD_RES_PATCH("res");

  if (dd_file_exists("content/hq_tex/res/grp_hdr.vromfs.bin"))
  {
    String mnt("content/hq_tex/res/");
    VirtualRomFsData *vrom = ::load_vromfs_dump("content/hq_tex/res/grp_hdr.vromfs.bin", tmpmem);
    if (vrom)
      ::add_vromfs(vrom, true, mnt);
    ::load_res_packs_from_list(mnt + "respacks.blk");
    if (vrom)
    {
      ::remove_vromfs(vrom);
      tmpmem->free(vrom);
    }
  }
  LOAD_RES_PATCH("content/hq_tex/res");
#undef LOAD_RES_PATCH

  ::dgs_limit_fps = blk.getBool("limitFps", false);
  dagor_set_game_act_rate(blk.getInt("actRate", -60));
  init_webui(NULL);
  dagor_select_game_scene(new DemoGameScene);
}

inline uint64_t make_mesh_id(uint32_t res_id, uint32_t lod_id, uint32_t elem_id)
{
  return (uint64_t(res_id) << 32) | lod_id << 28 | elem_id;
}
inline uint64_t make_elem_id(uint32_t mesh_no, uint32_t elem_id) { return mesh_no << 14 | elem_id; }

void add_dynrend_resource_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneLodsResource *resource, uint64_t bvh_id)
{
  d3d::GpuAutoLock gpu_al;

  BSphere3 bounding;
  bounding = resource->getLocalBoundingBox();

  Point4 posMul, posAdd;
  if (resource->isBoundPackUsed())
  {
    posMul = *((Point4 *)resource->bpC255);
    posAdd = *((Point4 *)resource->bpC254);
  }
  else
  {
    posMul = Point4(1.f, 1.f, 1.f, 0.0f);
    posAdd = Point4(0.f, 0.f, 0.f, 0.0f);
  }

  for (auto [lodIx, lod] : enumerate(make_span_const(resource->lods)))
  {
    auto rigidCount = lod.scene->getRigidsConst().size();
    auto lodIxLocal = lodIx;

    lod.scene->getMeshes(
      [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
        G_UNUSED(node_id);
        G_UNUSED(radius);
        auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);
        for (auto [elemIx, elem] : enumerate(elems))
        {
          auto elemId = make_elem_id(rigid_no, elemIx);
          auto meshId = make_mesh_id(bvh_id, lodIxLocal, elemIx);

          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          bvh::ChannelParser parser;
          if (!elem.mat->enum_channels(parser, parser.flags))
            return;

          if (parser.positionFormat == -1)
            return;

          G_ASSERT(parser.normalFormat == -1 || parser.normalFormat == VSDT_E3DCOLOR);
          G_ASSERT(parser.colorFormat == -1 || parser.colorFormat == VSDT_E3DCOLOR);

          bool isTree = strncmp(elem.mat->getShaderClassName(), "tree_", 5) == 0;

          bool hasAlphaTest = isTree;
          TEXTUREID alphaTextureId = BAD_TEXTUREID;

          ShaderMatData shaderData;
          elem.mat->getMatData(shaderData);

          bool isTwoSided = shaderData.matflags & (SHFLG_2SIDED | SHFLG_REAL2SIDED);

          auto ib = hasIndices ? elem.vertexData->getIB() : nullptr;
          auto vb = elem.vertexData->getVB();

          bvh::MeshInfo meshInfo{};
          meshInfo.indices = ib;
          meshInfo.indexCount = elem.numf * 3;
          meshInfo.startIndex = elem.si;
          meshInfo.vertices = vb;
          meshInfo.vertexCount = elem.numv;
          meshInfo.vertexSize = elem.vertexData->getStride();
          meshInfo.baseVertex = elem.baseVertex;
          meshInfo.startVertex = elem.sv;
          meshInfo.positionFormat = parser.positionFormat;
          meshInfo.positionOffset = parser.positionOffset;
          meshInfo.indicesOffset = parser.indicesOffset;
          meshInfo.weightsOffset = parser.weightsOffset;
          meshInfo.normalOffset = parser.normalOffset;
          meshInfo.colorOffset = parser.colorFormat == VSDT_E3DCOLOR ? parser.colorOffset : bvh::MeshInfo::invalidOffset;
          meshInfo.texcoordFormat = parser.texcoordFormat;
          meshInfo.texcoordOffset = parser.texcoordOffset;
          meshInfo.secTexcoordOffset = parser.secTexcoordOffset;
          meshInfo.albedoTextureId = elem.mat->get_texture(0);
          meshInfo.normalTextureId = elem.mat->get_texture(2);
          meshInfo.alphaTextureId = alphaTextureId;
          meshInfo.alphaTest = hasAlphaTest;
          meshInfo.vertexProcessor = nullptr;
          meshInfo.posMul = posMul;
          meshInfo.posAdd = posAdd;
          meshInfo.boundingSphere = bounding;
          meshInfo.isInterior = false;

          bvh::add_mesh(context_id, meshId, meshInfo);
        }
      },
      [&](const ShaderSkinnedMesh *, int, int) { G_ASSERTF(0, "Skinned parts are not yet supported."); });
  }
}

void add_dynrend_instance_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneInstance *resource, uint64_t bvh_id, int count,
  const Point3 &view_pos, float cull_radius)
{
  resource->getCurSceneResource()->getMeshes(
    [&](const ShaderMesh *mesh, int node_id, float radius, int rigid_no) {
      auto elems = mesh->getElems(ShaderMesh::STG_opaque, ShaderMesh::STG_atest);

      if (elems.empty())
        return;

      for (int treeIx = 0; treeIx < count; ++treeIx)
      {
        TMatrix tm = TMatrix::IDENT;
        set_test_tm(treeIx, tm);

        if ((view_pos - tm.getcol(3)).lengthSq() > sqr(cull_radius))
          continue;

        mat44f tm44;
        mat43f tm43;
        v_mat44_make_from_43cu(tm44, tm.array);
        v_mat44_transpose_to_mat43(tm43, tm44);

        for (auto [elemIx, elem] : enumerate(elems))
        {
          bool hasIndices;
          if (!elem.vertexData->isRenderable(hasIndices))
            continue;

          auto meshId = make_mesh_id(bvh_id, resource->getCurrentLodNo(), make_elem_id(rigid_no, elemIx));
          bvh::add_instance(context_id, meshId, tm43);
        }
      }
    },
    [&](const ShaderSkinnedMesh *, int, int) { G_ASSERTF(0, "Skinned parts are not yet supported."); });
}

// pack textures lose their content with the device and nothing in this sample
// would re-read them (the games do the same in their reset handling)
static void testgi_reload_textures_after_reset(bool)
{
  ddsx::reload_active_textures(0);
  if (!is_managed_textures_streaming_load_on_demand())
    ddsx::tex_pack2_perform_delayed_data_loading();
}
REGISTER_D3D_AFTER_RESET_FUNC(testgi_reload_textures_after_reset);

PULL_CONSOLE_PROC(profiler_console_handler)
PULL_CONSOLE_PROC(def_app_console_handler)
