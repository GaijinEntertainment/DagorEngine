// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <vecmath/dag_vecMathDecl.h>
#include <3d/dag_quadIndexBuffer.h>
#include <render/temporalAA.h>
#include <render/clusteredLights.h>
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
#include <3d/dag_texMgrTags.h>

#include "test_main.h"
#include <shaders/dag_computeShaders.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include "de3_freeCam.h"
#include "de3_visibility_finder.h"
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
#include <render/ssao.h>
#include <render/gtao.h>
#include <render/omniLightsManager.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_TMatrix4more.h>
#include <math/dag_cube_matrix.h>
#include <math/dag_viewMatrix.h>
#include <math/dag_frustum.h>
#include <math/dag_math3d.h>
#include <math/random/dag_random.h>
#include <gui/dag_visConsole.h>
#include <perfMon/dag_statDrv.h>
#include <visualConsole/dag_visualConsole.h>
#include <generic/dag_carray.h>
#include <render/lightCube.h>

#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_delayedAction.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_geomTree.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <render/deferredRenderer.h>
#include <render/downsampleDepth.h>
#include <render/screenSpaceReflections.h>
#include <render/tileDeferredLighting.h>
#include <render/preIntegratedGF.h>
#include <render/viewVecs.h>
#include <render/voxelization_target.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daSkiesToBlk.h>
#include "de3_gui.h"
#include "de3_gui_dialogs.h"
#include "de3_hmapTex.h"
#include <render/spheres_consts.hlsli>
#include <render/scopeRenderTarget.h>
#include <shaders/dag_shaderMatData.h>
#include <scene/dag_loadLevel.h>
#include <streaming/dag_streamingBase.h>
#include <render/debugTexOverlay.h>
#include <render/cameraPositions.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/toroidalStaticShadows.h>
#include <render/upscale/upscaleSampling.h>
#include <daEditorE/daEditorE.h>
#include "entEdit.h"
#include <render/debugGbuffer.h>

#include <math/dag_hlsl_floatx.h>
#include <daGI2/daGI2.h>
#include <daGI2/treesAboveDepth.h>
#include <render/voxelization_target.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/functional.h>
#include <EASTL/bit.h>
#include <generic/dag_enumerate.h>

#include <bvh/bvh.h>
#include <bvh/bvh_processors.h>

#include "de3_benchmark.h"
#include "lruCollision.h"
#include <render/toroidal_update.h>
#include <profileEventsGUI/profileEventsGUI.h>

#include <SnapdragonSuperResolution/SnapdragonSuperResolution.h>

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
  VAR(upscale_half_res_depth_tex)                    \
  VAR(upscale_half_res_depth_tex_samplerstate)       \
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
  VAR(prev_downsampled_normals_samplerstate)         \
  VAR(from_sun_direction)                            \
  VAR(rasterize_collision_type)                      \
  VAR(downsample_depth_type)                         \
  VAR(local_light_probe_tex)                         \
  VAR(envi_probe_specular)                           \
  VAR(local_light_probe_tex_samplerstate)            \
  VAR(envi_probe_specular_samplerstate)

#define GLOBAL_VARS_OPT_LIST                           \
  VAR(gbuffer_for_treesabove)                          \
  VAR(downsampled_checkerboard_depth_tex)              \
  VAR(downsampled_checkerboard_depth_tex_samplerstate) \
  VAR(sphere_time)                                     \
  VAR(dagi_sp_has_exposure_assume)                     \
  VAR(prev_globtm_psf_0)                               \
  VAR(prev_globtm_psf_1)                               \
  VAR(prev_globtm_psf_2)                               \
  VAR(prev_globtm_psf_3)

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
    webui::startup(&cfg);
  }
}

void add_dynrend_resource_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneLodsResource *resource, uint64_t bvhId);
void add_dynrend_instance_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneInstance *resource, uint64_t bvh_id, int count,
  const Point3 &view_pos);

static constexpr float bvh_radius = 100.f;

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


CONSOLE_INT_VAL("render", gf_frames, 128, 1, 512);
CONSOLE_BOOL_VAL("render", gi_reset, false);
CONSOLE_BOOL_VAL("render", ssao_blur, true);
CONSOLE_BOOL_VAL("render", bnao_blur, true);
CONSOLE_BOOL_VAL("render", bent_cones, true);
CONSOLE_BOOL_VAL("render", dynamic_lights, true);
CONSOLE_BOOL_VAL("render", invalidate_dynamic_shadows, false);
CONSOLE_BOOL_VAL("render", taa, false);
CONSOLE_BOOL_VAL("render", taa_halton, true);
CONSOLE_BOOL_VAL("render", rasterize_sdf_prims, true);
CONSOLE_BOOL_VAL("render", rasterize_sdf_level, true);
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
CONSOLE_BOOL_VAL("shadows", render_static_shadows_every_frame, false);

ConVarI sleep_msec_val("sleep_msec", 0, 0, 1000, NULL);

struct IRenderDynamicCubeFace2
{
  virtual void renderLightProbeOpaque() = 0;
  virtual void renderLightProbeEnvi() = 0;
};

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
  void update(const ManagedTex *cubeTarget, IRenderDynamicCubeFace2 &cb, const Point3 &pos)
  {
    ShaderGlobal::set_color4(world_view_posVarId, pos.x, pos.y, pos.z, 1);
    float prev_pre_exposure = ShaderGlobal::get_real(get_shader_variable_id("pre_exposure", true));
    ShaderGlobal::set_real(get_shader_variable_id("pre_exposure"), 1);
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    float zn = 0.05, zf = 100;
    ShaderGlobal::set_color4(zn_zfarVarId, zn, zf, 0, 0);
    static int light_probe_posVarId = get_shader_variable_id("light_probe_pos", true);
    ShaderGlobal::set_color4(light_probe_posVarId, pos.x, pos.y, pos.z, 1);
    for (int i = 0; i < 6; ++i)
    {
      target->setRt();
      TMatrix4 projTm;
      d3d::setpersp(Driver3dPerspective(1, 1, zn, zf, 0, 0), &projTm);
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
      cameraMatrix.setcol(3, pos);
      TMatrix viewTm = orthonormalized_inverse(cameraMatrix);
      d3d::settm(TM_VIEW, viewTm);

      d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);

      cb.renderLightProbeOpaque();
      target->resolve(shadedTarget.getTex2D(), viewTm, projTm);
      d3d::set_render_target(shadedTarget.getTex2D(), 0); // because of cube depth
      d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);

      // d3d::clearview(CLEAR_ZBUFFER|CLEAR_STENCIL, 0, 0, 0);
      cb.renderLightProbeEnvi();
      // save_rt_image_as_tga(shadedTarget.getTex2D(), String(128, "cube%s.tga", i));

      d3d::set_render_target(cubeTarget->getCubeTex(), i, 0);
      d3d::settex(2, shadedTarget.getTex2D());
      copy.render();
    }
    d3d::set_render_target(prevRT);
    ShaderGlobal::set_real(get_shader_variable_id("pre_exposure"), prev_pre_exposure);
  }
};


class IntProperty
{
  int mValue = -1;
  int mPrevValue = -1;

public:
  template <typename Type>
  IntProperty(Type value) : mValue((Type)value)
  {}
  bool pullValueChange()
  {
    bool ret = mPrevValue != mValue;
    mPrevValue = mValue;
    return ret;
  }
  template <typename Type>
  operator Type() const
  {
    return (Type)mValue;
  }
  int *operator&() { return &mValue; }
  int get() const { return mValue; }
};

class FloatProperty
{
  float mValue = -1;
  float mPrevValue = -1;

public:
  template <typename Type>
  FloatProperty(Type value) : mValue((Type)value)
  {}
  bool pullValueChange()
  {
    bool ret = mPrevValue != mValue;
    mPrevValue = mValue;
    return ret;
  }
  template <typename Type>
  operator Type() const
  {
    return (Type)mValue;
  }
  float get() const { return mValue; }
  float *operator&() { return &mValue; }
};

CONSOLE_BOOL_VAL("render", rasterize_debug_collision, false);
CONSOLE_BOOL_VAL("render", rasterize_gbuf_collision, true);
CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_from_collision_rasterize_below, 3, -1, 16);
CONSOLE_FLOAT_VAL_MINMAX("render", world_sdf_rasterize_supersample, 1, 1, 4);

class DemoGameScene final : public DagorGameScene, public IRenderDynamicCubeFace2, public ICascadeShadowsClient
{
public:
  CascadeShadows *csm = nullptr;
  static DemoGameScene *the_scene;
  VisibilityFinder visibilityFinder;
  Point4 getCascadeShadowAnchor(int cascade_no) override
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    return Point4::xyz0(-itm.getcol(3));
  }
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &)
  {
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::settm(TM_PROJ, &csm->getWorldRenderMatrix(cascade_no));
    Frustum frustum = visibilityFinder.getFrustum();
    visibilityFinder.setFrustum(csm->getFrustum(cascade_no));
    renderOpaque((mat44f_cref)csm->getWorldCullingMatrix(cascade_no), false, false);
    visibilityFinder.setFrustum(frustum);
  }

  virtual void getCascadeShadowSparseUpdateParams(int cascade_no, const Frustum &, float &out_min_sparse_dist,
    int &out_min_sparse_frame)
  {
    out_min_sparse_dist = 100000;
    out_min_sparse_frame = -1000;
  }

  Point3 dir_to_sun;
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
      dir_to_sun.x = sin(DegToRad(sun_panel.sun_zenith)) * cos(DegToRad(sun_panel.sun_azimuth));
      dir_to_sun.z = sin(DegToRad(sun_panel.sun_zenith)) * sin(DegToRad(sun_panel.sun_azimuth));
      dir_to_sun.y = cos(DegToRad(sun_panel.sun_zenith));
      daSkies.setMoonDir(-dir_to_sun);
      daSkies.setMoonAge(sun_panel.moon_age);
    }
    ShaderGlobal::set_color4(from_sun_directionVarId, -dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
  }
  eastl::unique_ptr<ClusteredLights> clusteredLights;
  eastl::unique_ptr<DeferredRenderTarget> target;
  carray<UniqueTex, 2> farDownsampledDepth, downsampledNormals, downsampled_close_depth_tex;
  UniqueTex downsampled_checkerboard_depth_tex;
  eastl::unique_ptr<UpscaleSamplingTex> upscale_tex;
  int currentDownsampledDepth;

  UniqueTexHolder frame, prevFrame;
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
      downsample_normals ? downsampledNormals[currentDownsampledDepth] : null_tex, null_tex, downsampled_checkerboard_depth_tex);


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

  UniqueTexHolder combined_shadows;
  PostFxRenderer combine_shadows;

  MultiFramePGF preIntegratedGF;
  void initGF()
  {
    SCOPE_RENDER_TARGET_NAME(_initGF);
    preIntegratedGF.init(PREINTEGRATE_SPECULAR_DIFFUSE_QUALITY_MAX, gf_frames);
  }

  void combineShadows()
  {
    TIME_D3D_PROFILE(combineShadows);
    SCOPE_RENDER_TARGET_NAME(_combineShadows);
    d3d::set_render_target(combined_shadows.getTex2D(), 0);
    combine_shadows.render();
    combined_shadows.setVar();
  }

  void invalidateGI()
  {
    reloadCube(true);
    if (daGI2)
    {
      d3d::GpuAutoLock gpu_al;
      daGI2->afterReset();
    }
    validAmbientHistory = false;
    // daGI2.reset(create_dagi());
  }

  BBox3 sceneBox, levelBox;

  int giUpdatePosFrameCounter = 0;
  enum
  {
    MAX_GI_FRAMES_TO_INVALIDATE_POS = 24
  };
  void updateSSGIPos(const Point3 &pos)
  {
    float giUpdateDist = 128;
    if (++giUpdatePosFrameCounter <= MAX_GI_FRAMES_TO_INVALIDATE_POS) // if frames passed < threshold
    {
      if (!staticShadows->isInside(BBox3(pos, giUpdateDist))) // we are not even inside static shadows
        return;
      if (!staticShadows->isValid(BBox3(pos, giUpdateDist))) // static shadows are invalid
        return;
      if (giUpdatePosFrameCounter == 1 && !staticShadows->isCompletelyValid(BBox3(pos, giUpdateDist))) // if static shadows are fully
                                                                                                       // updated, render immediately,
                                                                                                       // otherwise try to wait for one
                                                                                                       // frame
        return;
    }
    giUpdatePosFrameCounter = 0;

    update_visibility_finder(visibilityFinder);
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
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_box0"), scene_box[0].x, scene_box[0].y, scene_box[0].z, maxRes);
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_box1"), 1. / scene_box.width().x, 1 / scene_box.width().y,
      1 / sceneBox.width().z, 0.f);

    Point3 voxelize_box0 = scene_box[0];
    Point3 voxelize_box1 = Point3(1. / scene_box.width().x, 1. / scene_box.width().y, 1. / scene_box.width().z);
    Point3 voxelize_aspect_ratio = min(point3(res) / maxRes, Point3(1, 1, 1));
    // Point3 voxelize_aspect_ratio = min((point3(res) + Point3(1,1,1)) * 2. / maxRes, Point3(1,1,1));
    // voxelize_aspect_ratio = max(voxelize_aspect_ratio, Point3(0.5,0.5,0.5));
    Point3 mult = 2. * mul(voxelize_box1, voxelize_aspect_ratio);
    Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_world_to_rasterize_space_mul"), P3D(mult), 0);
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_world_to_rasterize_space_add"), P3D(add), 0);

    Frustum f;
    Point3 camera_pos(scene_box.center().x, scene_box[1].y, scene_box.center().z);
    {
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, CUBEFACE_NEGY);
      cameraMatrix.setcol(3, camera_pos);
      cameraMatrix = orthonormalized_inverse(cameraMatrix);
      d3d::settm(TM_VIEW, cameraMatrix);

      BBox3 box = cameraMatrix * scene_box;
      TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
      d3d::settm(TM_PROJ, &proj);
      mat44f globtm;
      d3d::calcglobtm(cameraMatrix, proj, (TMatrix4 &)globtm);
      f = Frustum(globtm);
    }
    VisibilityFinder vf;
    vf.set(v_ldu(&camera_pos.x), f, 0.0f, 0.0f, 1.0f, 1.0f, nullptr);

    for (int i = 0; i < 3; ++i)
    {
      ShaderGlobal::set_int(get_shader_variable_id("voxelize_axis", true), i);
      // TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, (i == 0 ? CUBEFACE_NEGX : i == 1 ? CUBEFACE_NEGY : CUBEFACE_NEGZ));
      // cameraMatrix.setcol(3, camera_pos);
      // cameraMatrix = orthonormalized_inverse(cameraMatrix);
      // d3d::settm(TM_VIEW, cameraMatrix);
      // BBox3 box = cameraMatrix*scene_box;
      /*int xAxis = 0, yAxis = 1, zAxis = 2;
      if (i == 0)
      {
        xAxis = 1; yAxis = 2; zAxis = 0;
      } else if (i == 1)
      {
        xAxis = 0; yAxis = 2; zAxis = 1;
      }
      d3d::setview(0,0, res[xAxis], res[yAxis],0,1);
      d3d::settm(TM_VIEW, TMatrix::IDENT);
      TMatrix4 proj = matrix_ortho_off_center_lh(scene_box[0][xAxis], scene_box[1][xAxis],
                                                 scene_box[1][yAxis], scene_box[0][yAxis],
                                                 scene_box[0][zAxis], scene_box[1][zAxis]);
      d3d::settm(TM_PROJ, &proj);
      mat44f globtm;
      d3d::getglobtm(globtm);*/
      // vf.set(v_ldu(&camera_pos.x), Frustum(globtm), 0.0f, 0.0f, 1.0f, 1.0f, false);
      // d3d::setview(0, 0, maxRes, maxRes, 0, 1);
      binScene->render(vf, 0, 0xFFFFFFFF);
    }
    reset_voxelization_override();
    ShaderGlobal::set_int(scene_mode_varId, SCENE_MODE_NORMAL);
  }

  void debugVolmap()
  {
    // fixme: add here
  }

  UniqueTex ambient[2];
  UniqueTex ambient_age[2];
  UniqueTex screen_specular[2];
  int cAmbientFrame = 0;
  PostFxRenderer ambientRenderer;

  LRUCollision lruColl;

  eastl::unique_ptr<DaGI> daGI2;
  TreesAboveDepth treesAbove;
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
      prevFrame->disableSampler();
    }
  }

  void initScreenGI(int w, int h)
  {
    const bool supportRGBE_RT = d3d::check_texformat(TEXFMT_R9G9B9E5 | TEXCF_RTARGET);
    gi_fmt = supportRGBE_RT ? TEXFMT_R9G9B9E5 : TEXFMT_R11G11B10F;
    if (gi_panel.gi_mode == SCREEN_PROBES)
    {
      if (ShaderGlobal::is_var_assumed(dagi_sp_has_exposure_assumeVarId) &&
          ShaderGlobal::get_interval_assumed_value(dagi_sp_has_exposure_assumeVarId))
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
    ambient[0]->texaddr(TEXADDR_CLAMP);
    screen_specular[0].close();
    screen_specular[0] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_specular0");
    screen_specular[0]->texaddr(TEXADDR_CLAMP);
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
    ambient[1]->texaddr(TEXADDR_CLAMP);

    screen_specular[1] = dag::create_tex(NULL, w, h, gi_fmt | TEXCF_RTARGET, 1, "screen_specular1");
    screen_specular[1]->texaddr(TEXADDR_CLAMP);

    ambient_age[0] = dag::create_tex(NULL, w, h, TEXFMT_R8UI | TEXCF_RTARGET, 1, "screen_ambient_age0");
    ambient_age[1] = dag::create_tex(NULL, w, h, TEXFMT_R8UI | TEXCF_RTARGET, 1, "screen_ambient_age1");
    ambient_age[0]->texaddr(TEXADDR_CLAMP);
    ambient_age[1]->texaddr(TEXADDR_CLAMP);
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

    combined_shadows.close();
    combined_shadows = dag::create_tex(NULL, w, h, TEXFMT_L8 | TEXCF_RTARGET, 1, "combined_shadows");
    ShaderGlobal::set_sampler(get_shader_variable_id("combined_shadows_samplerstate"), d3d::request_sampler({}));
    debugTexOverlay.setTargetSize(Point2(w, h));

    int halfW = w / 2, halfH = h / 2;
    ShaderGlobal::set_color4(::get_shader_variable_id("lowres_rt_params", true), halfW, halfH, 0, 0);

    ShaderGlobal::set_color4(get_shader_variable_id("rendering_res"), w, h, 1.0f / w, 1.0f / h);
    ShaderGlobal::set_color4(::get_shader_variable_id("taa_display_resolution", true), w, h, 0, 0);
    initReflections(w, h);
    ssao.reset();
    ssao = eastl::make_unique<SSAORenderer>(w / 2, h / 2, 1);

    gtao.reset();
    gtao = eastl::make_unique<GTAORenderer>(w / 2, h / 2, 1);

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
        farDownsampledDepth[i]->disableSampler();
        downsampled_close_depth_tex[i].close();
        name.printf(128, "close_downsampled_depth%d", i);
        downsampled_close_depth_tex[i] = dag::create_tex(NULL, w / 2, h / 2, rfmt | TEXCF_RTARGET, numMips, name);
        downsampled_close_depth_tex[i]->disableSampler();

        name.printf(128, "close_downsampled_normals%d", i);
        downsampledNormals[i].close();
        downsampledNormals[i] = dag::create_tex(NULL, w / 2, h / 2, TEXCF_RTARGET, 1, name); // TEXFMT_A2B10G10R10 |
        downsampledNormals[i]->disableSampler();
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
      ShaderGlobal::set_color4(lowres_tex_sizeVarId, w / 2, h / 2, 1.f / (w / 2), 1.f / (h / 2));

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
      prevFrame->disableSampler();
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
  }

  DemoGameScene() : main_pov_data(NULL), cube_pov_data(NULL)
  {
    RenderScene::useSRVBuffers = isRtEnabled();
    LRURendinstCollision::useSRVBuffers = isRtEnabled();

    the_scene = this;
    init_voxelization();
    {
      FullFileLoadCB riCollision("ri_collisions.bin");
      if (riCollision.fileHandle)
        lruColl.load(riCollision);
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
      clusteredLights->init(8, 4096, false);
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
      auto spotId = clusteredLights->addSpotLight(spot, SpotLightsManager::GI_LIGHT_MASK);
      clusteredLights->addShadowToLight(spotId, true, false, 16, 8, 2, DynamicShadowRenderGPUObjects::NO);
      clusteredLights->setLight(spotId, spot, SpotLightsManager::GI_LIGHT_MASK, true);
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
    main_pov_data = daSkies.createSkiesData("main"); // required for each point of view (or the only one if panorama, for panorama
                                                     // center)
    cube_pov_data = daSkies.createSkiesData("cube"); // required for each point of view (or the only one if panorama, for panorama
                                                     // center)

    initBvh();
    enforceLruCache();
  }
  PostFxRenderer taaRender;
  SnapdragonSuperResolution snapdragonSuperResolutionRender;
  enum
  {
    CLOUD_VOLUME_W = 32,
    CLOUD_VOLUME_H = 16,
    CLOUD_VOLUME_D = 32
  };
  SkiesData *main_pov_data, *cube_pov_data;

  ~DemoGameScene()
  {
    the_scene = nullptr;
    close_voxelization();

    teardownBvh();

    daSkies.destroy_skies_data(main_pov_data);
    daSkies.destroy_skies_data(cube_pov_data);
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
  }

  virtual bool rayTrace(const Point3 &p, const Point3 &norm_dir, float trace_dist, float &max_dist) { return false; }

  virtual void actScene()
  {
    samplebenchmark::quitIfBenchmarkHasEnded();
    webui::update();

    static String lastExportName;
    if (file_panel.export_min || file_panel.export_max)
    {
      String fileName = de3_imgui_file_save_dlg("Export sky settings", "Blk files|*.blk", "blk", "./", lastExportName);
      if (fileName.empty())
      {
        // show error
      }
      else
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
      else
      {
        // show error
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
      if (fileName.empty() || !blk.saveToTextFile(fileName))
      {
        // show error
      }
      else
      {
        lastLoadName = fileName;
      }
      file_panel.save = false;
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
      else
      {
        // show error
      }
      file_panel.load = false;
    }

    if (!exit_button_pressed && !de3_imgui_act(::dagor_game_act_time))
      curCamera->act();
    // Quit Game
    static int old_type = -1;
    if (render_panel.render_type != old_type)
    {
      switch (render_panel.render_type)
      {
        case PANORAMA:
          GUI_DISABLE_OBJ(render_panel, sky_quality);
          GUI_DISABLE_OBJ(render_panel, direct_quality);
          // GUI_DISABLE_OBJ(render_panel, temporal_upscale);
          GUI_DISABLE_OBJ(render_panel, infinite_skies);
          // GUI_ENABLE_OBJ(render_panel, panoramaTemporalSpeed);
          GUI_ENABLE_OBJ(render_panel, panorama_blending);
          GUI_ENABLE_OBJ(render_panel, panoramaResolution);
          break;
        case DIRECT:
          GUI_ENABLE_OBJ(render_panel, infinite_skies);
          GUI_ENABLE_OBJ(render_panel, sky_quality);
          GUI_ENABLE_OBJ(render_panel, direct_quality);
          // GUI_ENABLE_OBJ(render_panel, temporal_upscale);
          // GUI_DISABLE_OBJ(render_panel, panoramaTemporalSpeed);
          GUI_DISABLE_OBJ(render_panel, panorama_blending);
          GUI_DISABLE_OBJ(render_panel, panoramaResolution);
          break;
      }

      old_type = render_panel.render_type;
    }
    if (exit_button_pressed)
    {
      quit_game(1);
      return;
    }
  }

  ProfileUniqueEventsGUI allEvents;
  void drawProfile() { allEvents.drawProfiled(); }

  virtual void drawScene()
  {
    int sw, sh;
    d3d::get_render_target_size(sw, sh, frame.getTex2D());
    int w, h;
    d3d::get_render_target_size(w, h, nullptr);

    StdGuiRender::reset_per_frame_dynamic_buffer_pos();

    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    // todo: clamp itm.getcol(3) with shrinked sceneBox - there is nothing interesting outside scene
    updateSSGIPos(itm.getcol(3)); // we can frustum cull here as well, to reduce latency
    static int64_t time0 = ref_time_ticks();
    int current_time_usec = get_time_usec(time0);
    double realTime = current_time_usec / 1000000.0;
    static double lastTime = realTime;
    static double gameTime = 0;
    double realDt = lastTime - realTime;
    lastTime = realTime;
    gameTime += realDt * ::dagor_game_time_scale;
    // set_shader_global_time(gameTime);
    set_shader_global_time(realTime);
    static int taaFrame = 0;
    bool renderTaa = taa.get() || enable_taa_override;
    // logwarn("[hmatthew] renderTaa = %s", renderTaa ? "TRUE" : "FALSE");

    Driver3dPerspective p;
    d3d::getpersp(p);

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
      Driver3dPerspective oldp;
      oldp = p;
      TMatrix4 noJitterGlobTm;
      d3d::getglobtm(noJitterGlobTm);
      Point2 jitteredOfs = get_taa_jitter(taaFrame, taaParams);
      int w, h;
      d3d::get_target_size(w, h);
      p.ox += jitteredOfs.x * (2.0f / w);
      p.oy += -jitteredOfs.y * (2.0f / h);
      d3d::setpersp(p);
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

    TMatrix view;
    d3d::gettm(TM_VIEW, view);
    alignas(16) TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    {
      mat44f globtm;
      d3d::getglobtm(globtm);
      SCOPE_VIEW_PROJ_MATRIX;
      // can be done in thread
      Driver3dPerspective p;
      G_VERIFY(d3d::getpersp(p));
      alignas(16) TMatrix4 viewTm = TMatrix4(view);
      clusteredLights->cullFrustumLights(v_ldu_p3(itm[3]), globtm, (mat44f_cref)viewTm, (mat44f_cref)projTm, p.zn, nullptr,
        SpotLightsManager::MASK_ALL, OmniLightsManager::MASK_ALL);

      vec4f vpos = v_ldu_p3(itm[3]);
      bbox3f dynBox = {v_sub(vpos, v_splats(5)), v_add(vpos, v_splats(5))};
      if (invalidate_dynamic_shadows)
      {
        invalidate_dynamic_shadows = false;
        clusteredLights->invalidateAllShadows();
      }

      const int spotsCount = clusteredLights->getVisibleClusteredSpotsCount();
      const int omniCount = clusteredLights->getVisibleClusteredOmniCount();


      DynLightsOptimizationMode dynLightsMode = (dynamic_lights.get() && clusteredLights->hasClusteredLights())
                                                  ? get_lights_count_interval(spotsCount, omniCount)
                                                  : DynLightsOptimizationMode::NO_LIGHTS;

      ShaderGlobal::set_int(dynamic_lights_countVarId, eastl::to_underlying(dynLightsMode));
      clusteredLights->setBuffersToShader();

      dynamic_shadow_render::VolumesVector volumesToRender;
      clusteredLights->framePrepareShadows(volumesToRender, itm.getcol(3), globtm, p.hk, make_span_const(&dynBox, 1), nullptr);

      auto globalFrameId = ShaderGlobal::getBlockId("global_frame");
      ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
      clusteredLights->frameRenderShadows(
        volumesToRender,
        [&](mat44f_cref globTm, const TMatrix &viewItm, int, int, DynamicShadowRenderGPUObjects) {
          d3d::settm(TM_VIEW, TMatrix::IDENT);
          d3d::settm(TM_PROJ, globTm);
          ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
          renderOpaque((mat44f_cref)globTm, false, true);
          ShaderGlobal::setBlock(globalFrameId, ShaderGlobal::LAYER_FRAME);
        },
        [](const TMatrix &view_itm, const mat44f &view_tm, const mat44f &proj_tm) {
          G_UNUSED(view_itm);
          G_UNUSED(view_tm);
          G_UNUSED(proj_tm);
        });
      ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    }
    daGI2->beforeRender(sw, sh, w, h, itm, projTm, p.zn, p.zf);
    treesAbove.init(256.f, 1.f); // fixme
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
          const bool intersectLevel = levelBox & box & rasterize_sdf_level;
          if (handles.empty() && !intersectLevel)
            return UpdateGiQualityStatus::NOTHING;

          if (clusteredLights->initialized())
          {
            if (daGI2->sdfClipHasVoxelsLitSceneClip(sdf_clip))
            {
              alignas(16) TMatrix4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
              clusteredLights->cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightsManager::GI_LIGHT_MASK,
                OmniLightsManager::GI_LIGHT_MASK);
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
          Point3 voxelize_aspect_ratio = point3(max(res, IPoint3(1, 1, 1))) / maxRes; // use fixed aspect ratio of 1. for oversampling
                                                                                      // and HW clipping
          Point3 mult = 2. * div(voxelize_aspect_ratio, voxelize_box_sz);
          Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
          ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_mulVarId, P3D(mult), 0);
          ShaderGlobal::set_color4(voxelize_world_to_rasterize_space_addVarId, P3D(add), 0);
          if (handles.size())
          {
            if (voxelSize <= world_sdf_from_collision_rasterize_below)
            {
              SCOPE_RENDER_TARGET;
              set_voxelization_target_and_override(maxRes * world_sdf_rasterize_supersample, maxRes * world_sdf_rasterize_supersample);
              bool prims = rasterize_sdf_prims.get() && d3d::get_driver_desc().shaderModel < 6.1_sm;
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
          const bool intersectLevel = levelBox & box;
          if (!intersectLevel)
            return UpdateGiQualityStatus::NOTHING;
          if (intersectLevel)
          {
            SCOPE_VIEW_PROJ_MATRIX;
            SCOPE_RENDER_TARGET;
            renderVoxelsMediaGeom(box, Point3(voxelSize, voxelSize, voxelSize), SCENE_MODE_VOXELIZE_ALBEDO);
          }
          return UpdateGiQualityStatus::RENDERED;
        });
      if (!lightsInsideFrustum)
        clusteredLights->setInsideOfFrustumLightsToShader();
    }

    d3d::set_render_target(frame.getTex2D(), 0);
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    render();

    d3d::settm(TM_VIEW, view);
    d3d::settm(TM_PROJ, &projTm);
    d3d::set_render_target(frame.getTex2D(), 0);
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    daGI2->afterFrameRendered(DaGI::FrameData(DaGI::FrameHasAll));
    d3d::settm(TM_VIEW, view);
    d3d::settm(TM_PROJ, &projTm);
    renderTrans();
    daGI2->debugRenderTrans();
    ShaderGlobal::set_int(dynamic_lights_countVarId, 0);

    if (renderTaa)
    {
      int taaHistoryI = taaFrame & 1;
      d3d::set_render_target(taaHistory[1 - taaHistoryI].getTex2D(), 0);
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
      snapdragonSuperResolutionRender.render(frame, superResolutionOutput.getTex2D());
      ShaderGlobal::set_texture(frame.getVarId(), superResolutionOutput);
    }

    if (renderTaa)
      taaFrame++;
    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();

    if (show_gbuffer == DebugGbufferMode::None)
    {
      ShaderGlobal::set_real(get_shader_variable_id("exposure"), gi_panel.exposure);
      d3d::set_render_target();
      TIME_D3D_PROFILE(postfx)
      postfx.render();
      // d3d::stretch_rect(frame.getTex2D(), NULL);
    }
    else
      target->debugRender((int)show_gbuffer);

    {
      daGI2->debugRenderScreenDepth();
      daGI2->debugRenderScreen();
    }

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
    }
    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    TMatrix4 globtmTr = globtm.transpose();
    ShaderGlobal::set_color4(prev_globtm_psf_0VarId, Color4(globtmTr[0]));
    ShaderGlobal::set_color4(prev_globtm_psf_1VarId, Color4(globtmTr[1]));
    ShaderGlobal::set_color4(prev_globtm_psf_2VarId, Color4(globtmTr[2]));
    ShaderGlobal::set_color4(prev_globtm_psf_3VarId, Color4(globtmTr[3]));
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
      s.volumetricGI.tileSize = 64;
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
      s.volumetricGI.tileSize = 0; // autodetect
    }
    s.sdf.voxel0Size = sdf_panel.voxel0Size;
    s.sdf.clips = sdf_panel.clips;
    s.sdf.texWidth = sdf_panel.texWidth;
    s.sdf.yResScale = sdf_panel.yResScale;
    if (gi_panel.gi_mode != ENVI_PROBE)
      daGI2->setSettings(s);
  }
  virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
  {
    giQualitySet();
    if (gi_reset)
    {
      daGI2->afterReset();
      gi_reset = false;
    }
    static float prev_pre_exposure = 1.f;
    ShaderGlobal::set_real(get_shader_variable_id("prev_pre_exposure", true), prev_pre_exposure);
    // artificially add randomness, to ensure we correctly decode previous frame
    // in normal circumstances it is easy to miss correct decoding,
    // as all exposure is changing gradually and prev frame is similar to current one
    const float new_pre_exposure = sqrt(gi_panel.exposure) * (gfrnd() * 0.5 + 0.5);
    ShaderGlobal::set_real(get_shader_variable_id("pre_exposure"), prev_pre_exposure = new_pre_exposure);
    TIME_D3D_PROFILE(beforeDraw);
    preIntegratedGF.setFramesCount(gf_frames);
    preIntegratedGF.update();
    static bool scene = false;

    if (scene != land_panel.level)
    {
      scene = land_panel.level;
      onSceneLoaded();
    }
    static int astronomical = -1;
    if (astronomical != (sun_panel.astronomical ? 1 : 0))
    {
      astronomical = (sun_panel.astronomical ? 1 : 0);
      if (astronomical)
      {
        GUI_DISABLE_OBJ(sun_panel, sun_azimuth);
        GUI_DISABLE_OBJ(sun_panel, sun_zenith);
        GUI_DISABLE_OBJ(sun_panel, moon_age);
      }
      else
      {
        GUI_ENABLE_OBJ(sun_panel, sun_azimuth);
        GUI_ENABLE_OBJ(sun_panel, sun_zenith);
        GUI_ENABLE_OBJ(sun_panel, moon_age);
      }
    }
    setDirToSun();
    current_sphere_time += land_panel.spheres_speed * gametime_elapsed_sec / 100.;
    if (current_sphere_time > 1e5)
      current_sphere_time = 0;


    curCamera->setView();

    update_visibility_finder(visibilityFinder);
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
      daSkies.initPanorama(main_pov_data, render_panel.panorama_blending, render_panel.panoramaResolution);
    }
    else
      daSkies.closePanorama();
    // debug("regen = %d force_update = %d", sky_panel.regen, force_update);
    // sky_panel.regen  = false;
    daSkies.projectUses2DShadows(render_panel.shadows_2d);
    ShaderGlobal::set_int(get_shader_variable_id("skies_use_2d_shadows", true), render_panel.shadows_2d);
    daSkies.prepare(dir_to_sun, false, gametime_elapsed_sec);
    dir_to_sun = daSkies.getPrimarySunDir();
    ShaderGlobal::set_color4(from_sun_directionVarId, -dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      sun = lerp(moon, sun, daSkies.getCurrentSunEffect());
      amb = lerp(moonamb, amb, daSkies.getCurrentSunEffect());
      ShaderGlobal::set_color4(get_shader_variable_id("sun_light_color"), color4(sun / PI, 0));
      ShaderGlobal::set_color4(get_shader_variable_id("amb_light_color", true), color4(amb, 0));
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
    ShaderGlobal::set_color4(zn_zfarVarId, p.zn, p.zf, 0, 0);
    // target.prepare();
    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    /*TMatrix4 globtmTr = globtm.transpose();
    ShaderGlobal::set_color4(globtm_psf_0VarId,Color4(globtmTr[0]));
    ShaderGlobal::set_color4(globtm_psf_1VarId,Color4(globtmTr[1]));
    ShaderGlobal::set_color4(globtm_psf_2VarId,Color4(globtmTr[2]));
    ShaderGlobal::set_color4(globtm_psf_3VarId,Color4(globtmTr[3]));*/
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    if (test)
      test->setLod(test->chooseLod(itm.getcol(3)));
    ShaderGlobal::set_color4(world_view_posVarId, itm.getcol(3).x, itm.getcol(3).y, itm.getcol(3).z, 1);
    CascadeShadows::ModeSettings mode;
    mode.powWeight = 0.8;
    mode.maxDist = min(50.f, staticShadows->getDistance() * 0.55f);
    mode.shadowStart = p.zn;
    mode.numCascades = 4;
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    csm->prepareShadowCascades(mode, dir_to_sun, inverse(itm), itm.getcol(3), projTm, Frustum(globtm), Point2(p.zn, p.zf), p.zn);
    treesAbove.prepare(itm.getcol(3), sceneBox[0].y, sceneBox[1].y, [&](const BBox3 &box, bool depth_min) {
      TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].z, box[0].z, box[depth_min].y, box[!depth_min].y);
      d3d::settm(TM_PROJ, &proj);

      TMatrix4 view;
      view.setcol(0, 1, 0, 0, 0);
      view.setcol(1, 0, 0, 1, 0);
      view.setcol(2, 0, 1, 0, 0);
      view.setcol(3, 0, 0, 0, 1);
      d3d::settm(TM_VIEW, &view);

      TMatrix4 globtm_ = view * proj;
      mat44f globtm;
      v_mat44_make_from_44cu(globtm, globtm_.m[0]);
      if (!depth_min)
        ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 1);
      renderTrees();
      if (!depth_min)
        ShaderGlobal::set_int(gbuffer_for_treesaboveVarId, 0);
    });
    de3_imgui_before_render();

    buildBvh();
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
  void renderDynamicSpheres()
  {
    if (!land_panel.spheres)
      return;
    DA_PROFILE_GPU;
    if (!drawSpheres.shader)
      drawSpheres.init("draw_debug_dynamic_spheres", NULL, 0, "draw_debug_dynamic_spheres");
    ShaderGlobal::set_real(sphere_timeVarId, current_sphere_time);
    drawSpheres.shader->setStates(0, true);
    d3d::setvsrc_ex(0, NULL, 0, 0);
    d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, 6);
  }

  // IRenderWorld interface
  void renderTrees()
  {
    if (!land_panel.trees || !test)
      return;
    TIME_D3D_PROFILE(tree);

    TMatrix tm;
    tm.identity();
    for (int inst = 0; inst < testCount; inst++)
    {
      set_test_tm(inst, tm);
      for (int i : test->getLodsResource()->getNames().node.id)
        test->setNodeWtm(i, tm);
      test->render();
    }
  }
  void renderLevel(bool hmap)
  {
    if (!land_panel.level || !binScene)
      return;
    TIME_D3D_PROFILE(level);
    binScene->render(visibilityFinder, hmap);
  }
  void renderPrepass()
  {
    if (!land_panel.prepass) //! land_panel.trees ||
      return;
    TIME_D3D_PROFILE(testScenePrepass);
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    d3d_err(d3d::set_render_target());
    d3d_err(d3d::set_render_target(0, nullptr, 0));
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    renderTrees();
    renderLevel(true);
    renderDynamicSpheres();
    d3d::set_render_target(prevRT);
  }
  void renderOpaque(mat44f_cref culling, bool render_decals, bool change_frustum)
  {
    TIME_D3D_PROFILE(opaque);
    renderTrees();
    renderDynamicSpheres();
    Frustum frustum = visibilityFinder.getFrustum();
    if (change_frustum)
      visibilityFinder.setFrustum(Frustum(culling));
    renderLevel(render_decals);
    if (change_frustum)
      visibilityFinder.setFrustum(frustum);

    rasterizeGbufCollision(culling, !render_decals);
  }
  void renderLightProbeOpaque()
  {
    // daSkies.prepare(dir_to_sun);
    TMatrix view, itm;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    itm = orthonormalized_inverse(view);
    daSkies.useFog(itm.getcol(3), cube_pov_data, view, projTm);
    if (binScene && land_panel.level)
      binScene->render(visibilityFinder);
  }

  void renderLightProbeEnvi()
  {
    TIME_D3D_PROFILE(cubeenvi)
    TMatrix view, itm;
    d3d::gettm(TM_VIEW, view);
    itm = orthonormalized_inverse(view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    Driver3dPerspective persp;
    d3d::getpersp(persp);
    daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 2, UniqueTex{}, UniqueTex{},
      BAD_TEXTUREID, cube_pov_data, view, projTm, persp);
  }

  eastl::unique_ptr<ToroidalStaticShadows> staticShadows;
  struct StaticShadowCallback : public IStaticShadowsCB
  {
    DemoGameScene *scene;
    shaders::OverrideStateId depthBiasAndClamp;
    StaticShadowCallback(DemoGameScene *s, shaders::OverrideStateId override) : scene(s), depthBiasAndClamp(override) {}
    void startRenderStaticShadow(const TMatrix &, const Point2 &, float, float) override
    {
      shaders::overrides::set(depthBiasAndClamp);
      char *one = 0;
      one++;
      d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, one); //-V566
    }

    void renderStaticShadowDepth(const mat44f &culling_gtm, const ViewTransformData &, int) override
    {
      scene->renderOpaque(culling_gtm, false, true);
    }
    void endRenderStaticShadow() override
    {
      shaders::overrides::reset();
      d3d::driver_command(Drv3dCommand::OVERRIDE_MAX_ANISOTROPY_LEVEL, (void *)0);
    }
  };


  shaders::UniqueOverrideStateId depthBiasAndClamp;

  void updateStaticShadowAround(const Point3 &p, float t)
  {
    initStaticShadow();
    bool invalidVars = staticShadows->setSunDir(dir_to_sun);
    StaticShadowCallback cb(this, depthBiasAndClamp.get());
    if (!render_static_shadows_every_frame)
    {
      if (staticShadows->updateOriginAndRender(p, t, 0.1f, 0.1f, false, cb).varsChanged || invalidVars)
        staticShadows->setShaderVars();
    }
    else
    {
      staticShadows->invalidate(true);

      for (int i = 0, n = staticShadows->cascadesCount(); i < n; i++)
      {
        staticShadows->enableOptimizeBigQuadsRender(i, false);
        staticShadows->enableTransitionCascade(i, false);

        staticShadows->updateOriginAndRender(p, t, 0.1f, 0.1f, false, cb);
      }

      staticShadows->setShaderVars();
    }
  }
  void updateStaticShadowAround()
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    updateStaticShadowAround(itm.getcol(3), 0.05f);
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

  void initStaticShadow()
  {
    if (!staticShadows)
      staticShadows = eastl::make_unique<ToroidalStaticShadows>(2048, 2, 250.f, 100, true);

    if (!depthBiasAndClamp)
      depthBiasAndClamp = shaders::overrides::create(get_static_shadows_override_state());
  }

  void onSceneLoaded()
  {
    staticShadows->invalidate(true);
    if (binScene)
    {
      BBox3 box;
      for (int i = 0; i < binScene->getRsm().getScenes().size(); ++i)
        box += binScene->getRsm().getScenes()[i]->calcBoundingBox();
      levelBox = box;
      if (lruColl.size())
      {
        bbox3f colBox = lruColl.calcBox();
        box[0] = min(box[0], (const Point3 &)colBox.bmin);
        box[1] = max(box[1], (const Point3 &)colBox.bmax);
      }
      box.inflate(1.f);
      levelBox = box;
      if (lruColl.size())
      {
        bbox3f colBox = lruColl.calcBox();
        box[0] = min(box[0], (const Point3 &)colBox.bmin);
        box[1] = max(box[1], (const Point3 &)colBox.bmax);
      }
      sceneBox = box;
      staticShadows->setWorldBox(box);
      const float htRange = min(ceilf((box[1].y - box[0].y) / 8.f) * 8., 1024.); // no more than 1km
      staticShadows->setMaxHtRange(htRange);                                     // that is only for skewed matrix
    }
    clusteredLights->invalidateAllShadows();
    treesAbove.invalidate();
    invalidateGI();
    reloadCube(true);
  }
  bool validAmbientHistory = false;
  void renderAmbient(UniqueTex &amb, UniqueTex &spec, UniqueTex &age)
  {
    TIME_D3D_PROFILE(current_ambient)
    target->setVar();
    d3d::set_render_target(0, amb.getTex2D(), 0);
    d3d::set_render_target(1, spec.getTex2D(), 0);
    d3d::set_render_target(2, age.getTex2D(), 0);
    d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
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
    ShaderGlobal::set_int(ambient_has_historyVarId, hasHistory);
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
  virtual void render()
  {
    TIME_D3D_PROFILE(render)
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    // d3d::clearview(CLEAR_TARGET, E3DCOLOR(0,0,64,0), 0, 0);
    // d3d::set_render_target();
    // d3d::set_render_target(target.gbuf[target.GBUF_ALBEDO_AO].getTex2D(), 0, false);
    // d3d::clearview(CLEAR_TARGET, 0xFF108080, 0, 0);
    // d3d::set_render_target();
    Driver3dPerspective persp;
    G_VERIFY(d3d::getpersp(persp));
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    TMatrix view = orthonormalized_inverse(itm);
    alignas(16) TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    mat44f globtm;
    d3d::calcglobtm(view, projTm, (TMatrix4 &)globtm);

    {
      TIME_D3D_PROFILE(changeData)
      daSkies.changeSkiesData(render_panel.sky_quality, render_panel.direct_quality, !render_panel.infinite_skies, target->getWidth(),
        target->getHeight(), main_pov_data);
      daSkies.useFog(itm.getcol(3), main_pov_data, view, projTm);
    }
    set_inv_globtm_to_shader(view, projTm, false);
    set_viewvecs_to_shader(view, projTm);

    updateStaticShadowAround();
    d3d::settm(TM_VIEW, view);
    d3d::settm(TM_PROJ, &projTm);

    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0); //
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    {
      TIME_D3D_PROFILE(scene)
      renderPrepass();
      renderOpaque(globtm, true, false);
    }

    if (::grs_draw_wire)
      d3d::setwire(0);
    extern bool g_ssao_blur;
    g_ssao_blur = ssao_blur.get();
    downsampleDepth(true);

    if (ssr)
      ssr->render(view, projTm);
    if (!ssr || gi_panel.reflections == REFLECTIONS_OFF)
    {
      ShaderGlobal::set_texture(get_shader_variable_id("ssr_target"), BAD_TEXTUREID);
    }
    if (gi_panel.ssao && ssao)
    {
      ssao->render(view, projTm, downsampled_close_depth_tex[currentDownsampledDepth]);
    }

    if (gi_panel.gtao && gtao)
      gtao->render(view, projTm);

    if (!gi_panel.ssao && !gi_panel.gtao)
      ShaderGlobal::set_texture(get_shader_variable_id("ssao_tex"), BAD_TEXTUREID);

    csm->renderShadowsCascades();
    csm->setCascadesToShader();
    combineShadows();
    daGI2->beforeFrameLit(gi_panel.dynamic_gi_quality);

    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), gi_panel.onscreen_mode);
    {
      renderAmbient((const TMatrix4 &)globtm);
      {
        TIME_D3D_PROFILE(shading)
        target->resolve(frame.getTex2D(), view, projTm);
        d3d::set_render_target(frame.getTex2D(), 0);
      }
    }
    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), RESULT);

    d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
    clusteredLights->renderOtherLights();
    renderEnvi();
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    ensurePrevFrame();
    d3d::stretch_rect(frame.getTex2D(), prevFrame.getTex2D());

    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
    if (sleep_msec_val.get())
      sleep_msec(sleep_msec_val.get());
  }

  void rasterizeDebugCollision()
  {
    mat44f viewproj;
    d3d::getglobtm(viewproj);
    ShaderGlobal::set_int(rasterize_collision_typeVarId, 0);
    lruColl.rasterize(viewproj);
  }
  void rasterizeGbufCollision(mat44f_cref culling, bool depth)
  {
    if (!rasterize_gbuf_collision)
      return;
    ShaderGlobal::set_int(rasterize_collision_typeVarId, depth ? 2 : 1);
    lruColl.rasterize(culling);
  }
  virtual void renderTrans()
  {
    // begin_draw_cached_debug_lines();
    // for (auto &b : indirBoxes)
    //   draw_cached_debug_box(b, E3DCOLOR(0xff,0x80, 0xff, 0xff));
    // end_draw_cached_debug_lines();

    buildBvhRiColl();

    if (binScene && land_panel.level)
      binScene->renderTrans();
    if (rasterize_debug_collision)
      rasterizeDebugCollision();
    debugVolmap();
  }

  virtual void renderIslDecals() {}

  virtual void renderToShadowMap() {}

  virtual void renderWaterReflection() {}

  virtual void renderWaterRefraction() {}

  void reinitCube(int ew)
  {
    cube.close();
    cube.init(ew);
    light_probe::destroy(localProbe);
    localProbe = light_probe::create("local", ew, TEXFMT_A16B16G16R16F);
    initEnviProbe();
    reloadCube(true);
  }
  UniqueTexHolder enviProbe;
  UniqueTex enviProbe0;
  void initEnviProbe()
  {
    local_light_probe_tex_samplerstateVarId.set_sampler(d3d::request_sampler({}));
    envi_probe_specular_samplerstateVarId.set_sampler(d3d::request_sampler({}));
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
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    const bool old = land_panel.level;
    land_panel.level = false;
    cube.update(&enviProbe0, *this, Point3(100000, 10, 0));
    land_panel.level = old;

    ShaderGlobal::set_texture(envi_probe_specularVarId, enviProbe0.getTexId());
    ShaderGlobal::set_texture(local_light_probe_texVarId, enviProbe0.getTexId());

    cube.update(&enviProbe, *this, Point3(100000, 10, 0));
    land_panel.level = old;
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
    updateStaticShadowAround(pos, 1.);
    if (first)
    {
      ShaderGlobal::set_texture(local_light_probe_texVarId, enviProbe);
      SCOPE_RENDER_TARGET;
      for (int i = 0; i < 6; ++i)
      {
        d3d::set_render_target(light_probe::getManagedTex(localProbe)->getCubeTex(), i, 0);
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      }
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        ShaderGlobal::set_color4(get_shader_variable_id(String(128, "enviSPH%d", i)), 0, 0, 0, 0);
      light_probe::update(localProbe, NULL);
    }
    {
      TIME_D3D_PROFILE(cubeRender)
      cube.update(light_probe::getManagedTex(localProbe), *this, pos);
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
          ShaderGlobal::set_color4(get_shader_variable_id(String(128, "enviSPH%d", i)), sphHarm[i]);
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

protected:
  DynamicRenderableSceneInstance *test = nullptr;
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
    IFreeCameraDriver::zNear = 0.01;
    IFreeCameraDriver::zFar = 50000.0;
    IFreeCameraDriver::turboScale = 20;

    de3_imgui_init("Test GI", "Sky properties");

    static GuiControlDesc skyGuiDesc[] = {
      DECLARE_BOOL_CHECKBOX(gi_panel, ssao, false),
      DECLARE_BOOL_CHECKBOX(gi_panel, gtao, false),
      DECLARE_INT_COMBOBOX(gi_panel, reflections, REFLECTIONS_ALL, REFLECTIONS_OFF, REFLECTIONS_SSR, REFLECTIONS_ALL),
      // DECLARE_BOOL_CHECKBOX(gi_panel, fake_dynres, false),
      DECLARE_INT_COMBOBOX(gi_panel, onscreen_mode, RESULT, RESULT, LIGHTING, DIFFUSE_LIGHTING, DIRECT_LIGHTING, INDIRECT_LIGHTING),
      DECLARE_INT_COMBOBOX(gi_panel, gi_mode, SCREEN_PROBES, ENVI_PROBE, ONLY_AO, SIMPLE_COLORED, SCREEN_PROBES),
      DECLARE_FLOAT_SLIDER(gi_panel, dynamic_gi_quality, 0, 4., 1.0, 0.01),
      DECLARE_FLOAT_SLIDER(gi_panel, exposure, 0.1, 16, 1, 0.01),

      DECLARE_INT_SLIDER(sdf_panel, clips, 3, 8, 5),
      DECLARE_INT_SLIDER(sdf_panel, texWidth, 64, 256, 128),
      DECLARE_FLOAT_SLIDER(sdf_panel, voxel0Size, 0.1, 1., 0.15, 0.1),
      DECLARE_FLOAT_SLIDER(sdf_panel, yResScale, 0.5, 1., 0.5, 0.1),
      DECLARE_FLOAT_SLIDER(screen_probes, temporality, 0, 1., 0.5, 0.01),
      DECLARE_BOOL_CHECKBOX(screen_probes, reproject, true),
      DECLARE_INT_COMBOBOX(screen_probes, tileSize, screen_probes.tileSize, 8, 10, 12, 14, 16, 20, 24, 32),
      DECLARE_INT_COMBOBOX(screen_probes, radianceOctRes, 8, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 16),
      DECLARE_BOOL_CHECKBOX(screen_probes, angleFiltering, false),

      DECLARE_INT_SLIDER(radiance_grid_panel, clips, 2, 4, 3),
      DECLARE_INT_SLIDER(radiance_grid_panel, texWidth, 16, 64, 32),

      DECLARE_INT_SLIDER(albedo_scene_panel, clips, 2, 4, 3),

      ////DECLARE_BOOL_BUTTON(gi_panel, calc_ground_truth, false),
      // DECLARE_BOOL_CHECKBOX(gi_panel, show_ground_truth, false),

      DECLARE_BOOL_BUTTON(file_panel, export_min, false),
      DECLARE_BOOL_BUTTON(file_panel, export_max, false),
      DECLARE_BOOL_BUTTON(file_panel, import_minmax, false),
      DECLARE_BOOL_BUTTON(file_panel, save, false),
      DECLARE_BOOL_BUTTON(file_panel, load, false),
      DECLARE_BOOL_CHECKBOX(sun_panel, astronomical, false),
      DECLARE_FLOAT_SLIDER(sun_panel, sun_zenith, 0.1, 120, 30, 0.01),
      DECLARE_FLOAT_SLIDER(sun_panel, sun_azimuth, 0., 360, 215, 1),
      DECLARE_FLOAT_SLIDER(sun_panel, moon_age, 0., 29.530588853, 10, 0.1),
      DECLARE_FLOAT_SLIDER(sun_panel, latitude, -90, 90, 55, 0.01),
      DECLARE_FLOAT_SLIDER(sun_panel, longitude, -180, 180, 37, 0.01),
      DECLARE_INT_SLIDER(sun_panel, year, 1941, 2032, 1939),
      DECLARE_INT_SLIDER(sun_panel, month, 1, 12, 6),
      DECLARE_INT_SLIDER(sun_panel, day, 1, 365, 6),
      DECLARE_FLOAT_SLIDER(sun_panel, time, 0, 24, 12, 0.001),
      DECLARE_BOOL_CHECKBOX(sun_panel, local_time, true),

      DECLARE_FLOAT_SLIDER(sky_panel, average_ground_albedo, 0.02, 0.83, 0.1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ground_color.r, 0, 1, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_panel, ground_color.g, 0, 1, 1, 0.01),
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

      DECLARE_BOOL_CHECKBOX(land_panel, enabled, false),
      DECLARE_BOOL_CHECKBOX(land_panel, level, false),
      DECLARE_FLOAT_SLIDER(land_panel, heightScale, 1, 10000.0, 2500.0, 1),
      DECLARE_FLOAT_SLIDER(land_panel, heightMin, -1.0, 1.0, -0.5, 0.1),
      DECLARE_FLOAT_SLIDER(land_panel, cellSize, 1, 250, 25, 0.5),
      DECLARE_BOOL_CHECKBOX(land_panel, trees, false),
      DECLARE_BOOL_CHECKBOX(land_panel, spheres, true),
      DECLARE_FLOAT_SLIDER(land_panel, spheres_speed, 0, 32, 8, 0.01),
      DECLARE_BOOL_CHECKBOX(land_panel, prepass, false),

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

      DECLARE_INT_SLIDER(clouds_form, shapeNoiseScale, 2, 16, 9),
      DECLARE_INT_SLIDER(clouds_form, cumulonimbusShapeScale, 2, 16, 4),
      DECLARE_INT_SLIDER(clouds_form, turbulenceFreq, 1, 6, 1),
      DECLARE_FLOAT_SLIDER(clouds_form, turbulenceStrength, 0., 2.0, 0.25, 0.01),

      DECLARE_FLOAT_SLIDER(clouds_form, extinction, 0.5, 6.0, 0.75, 0.01), // this is 0.06 multiplier clouds real extinction is within
                                                                           // 0.04-0.24, which is
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
      DECLARE_BOOL_CHECKBOX(render_panel, infinite_skies, false),
      DECLARE_BOOL_CHECKBOX(render_panel, shadows_2d, false),
      DECLARE_INT_SLIDER(render_panel, sky_quality, 0, 3, 1),
      DECLARE_INT_SLIDER(render_panel, direct_quality, 0, 3, 1),
      DECLARE_INT_COMBOBOX(render_panel, panoramaResolution, 2048, 1024, 1536, 2048, 3072, 4096),
      DECLARE_BOOL_CHECKBOX(render_panel, panorama_blending, false),
      DECLARE_FLOAT_SLIDER(render_panel, cloudsSpeed, 0, 60, 20, 0.1),
      DECLARE_FLOAT_SLIDER(render_panel, strataCloudsSpeed, 0, 30, 10, 0.1),
      DECLARE_FLOAT_SLIDER(render_panel, windDir, 0, 359, 0, 1),
      DECLARE_BOOL_BUTTON(render_panel, trace_ray, false),
      DECLARE_BOOL_BUTTON(render_panel, compareCpuSunSky, false),

      DECLARE_FLOAT_SLIDER(layered_fog, mie2_scale, 0, 200, 0, 0.00001),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_altitude, 1, 2500, 200, 1),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_thickness, 200, 1500, 400, 1),
    };
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
    daSkies.prepare(dir_to_sun, true, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      ShaderGlobal::set_color4(get_shader_variable_id("sun_light_color"), color4(sun, 0));
      ShaderGlobal::set_color4(get_shader_variable_id("amb_light_color", true), color4(amb, 0));
    }
    else
    {
      G_ASSERT(0);
    }

    reinitCube(128);

    if (const char *level_fn = dgs_get_settings()->getStr("level", NULL))
      scheduleLevelBinLoad(level_fn, dgs_get_settings()->getStr("heightmap", ""));
  }


  void renderEnvi()
  {
    TIME_D3D_PROFILE(envi)
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    TMatrix view;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    Driver3dPerspective persp;
    d3d::getpersp(persp);
    daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 3,
      farDownsampledDepth[currentDownsampledDepth], farDownsampledDepth[1 - currentDownsampledDepth], target->getDepthId(),
      main_pov_data, view, projTm, persp);
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
    float sun_zenith, sun_azimuth;
    float moon_age;
    bool astronomical, local_time;
    float longitude, latitude, time;
    int year, month, day;
    SunPanel() :
      sun_zenith(70),
      sun_azimuth(30),
      astronomical(false),
      longitude(55),
      latitude(37),
      time(1),
      year(1943),
      day(1),
      month(6),
      local_time(true)
    {}
  } sun_panel;
  struct
  {
    bool level, enabled, trees, spheres, prepass;
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
  } gi_panel;
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

  enum
  {
    PANORAMA = 0,
    DIRECT = 2
  };
  struct RenderPanel
  {
    int render_type;
    bool infinite_skies, panorama_blending, shadows_2d = false;
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

  DaSkies daSkies;

  RenderDynamicCube cube;
  light_probe::Cube *localProbe;
  console::IVisualConsoleDriver *visualConsoleDriver;

  void setModel(DynamicRenderableSceneInstance *s)
  {
    test = s;
    treesAbove.invalidate();

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
      virtual void doJob()
      {
        GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
        DynamicRenderableSceneInstance *val = NULL;

        FastNameMap rrl;
        rrl.addNameId(name);
        if (!skelName.empty())
          rrl.addNameId(skelName);
        set_required_res_list_restriction(rrl);

        DynamicRenderableSceneLodsResource *resource =
          (DynamicRenderableSceneLodsResource *)get_game_resource_ex(handle, DynModelGameResClassId);
        G_ASSERTF(resource, "Cannot load '%s'", name.str());
        if (resource)
        {
          val = new DynamicRenderableSceneInstance(resource);
          release_game_resource((GameResource *)resource); // Release original resource.
        }

        TMatrix tm;
        tm.identity();
        tm.setcol(3, pos);
        if (!skelName.empty() && val)
        {
          GeomNodeTree *skel = (GeomNodeTree *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(skelName), GeomNodeTreeGameResClassId);

          if (skel)
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
            release_game_resource((GameResource *)skel); // Release original resource.
          }
          else
            logerr("skeleton %s not found", skelName);
        }
        else if (val)
        {
          for (int i : val->getLodsResource()->getNames().node.id)
            val->setNodeWtm(i, tm);
        }

        reset_required_res_list_restriction();
        prefetch_managed_textures_by_textag(TEXTAG_DYNMODEL);
        ddsx::tex_pack2_perform_delayed_data_loading();

        struct UpdatePtr : public DelayedAction
        {
          DemoGameScene *s;
          DynamicRenderableSceneInstance *val;
          UpdatePtr(DemoGameScene *_s, DynamicRenderableSceneInstance *_val) : s(_s), val(_val) {}
          virtual void performAction()
          {
            s->setModel(val);
            cpujobs::release_done_jobs();
          }
        };
        add_delayed_action(new UpdatePtr(s, val));
      }
      virtual void releaseJob() { delete this; }
    };
    test = NULL;
    cpujobs::add_job(loading_job_mgr_id, new LoadJob(this, name, skel_name, pos));
  }

  void scheduleLevelBinLoad(const char *level_bindump_fn, const char *hmap_fn = "")
  {
    struct LoadJob : cpujobs::IJob
    {
      SimpleString fn, hmap_fn;
      scene_type_t **destScn;

      LoadJob(const char *_fn, const char *hmap_fn, scene_type_t **scn) : fn(_fn), hmap_fn(hmap_fn), destScn(scn) {}
      virtual void doJob()
      {
        scene_type_t *scn = new scene_type_t;
        textag_mark_begin(TEXTAG_LAND);
        scn->openSingle(fn);
        if (strlen(hmap_fn))
          scn->loadHeightmap(hmap_fn);
        textag_mark_end();
        prefetch_managed_textures_by_textag(TEXTAG_LAND);
        // if (!is_managed_textures_streaming_load_on_demand())
        ddsx::tex_pack2_perform_delayed_data_loading();
        struct UpdatePtr : public DelayedAction
        {
          scene_type_t **dest, *val;
          UpdatePtr(scene_type_t **_dest, scene_type_t *_val) : dest(_dest), val(_val) {}
          virtual void performAction()
          {
            *dest = val;
            ddsx::tex_pack2_perform_delayed_data_loading();
            ((DemoGameScene *)dagor_get_current_game_scene())->onSceneLoaded();
            ((DemoGameScene *)dagor_get_current_game_scene())->addBvhMeshes(val);
          }
        };
        add_delayed_action(new UpdatePtr(destScn, scn));
      }
      virtual void releaseJob() { delete this; }
    };

    cpujobs::add_job(loading_job_mgr_id, new LoadJob(level_bindump_fn, hmap_fn, &binScene));
  }

  bvh::ContextId bvhCtx = bvh::InvalidContextId;
  eastl::vector<uint64_t> bvhMeshes;
  uint64_t bvhIdGen = 0;
  uint64_t bvhLruMeshBase = 0;
  uint64_t bvhMeshBase = 0;
  uint64_t bvhMeshCount = 0;
  uint64_t testId = 0;

  struct BvhHeightProvider final : public bvh::HeightProvider
  {
    HeightmapHandler *heightmap = nullptr;

    bool embedNormals() const override final { return false; }
    void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count) const override final
    {
      struct TerrainVertex
      {
        Point3 position;
        uint32_t normal;
      };

      TerrainVertex *scratch = (TerrainVertex *)data;

      auto float_to_uchar = [](float a) { return uint32_t(floorf((a * 255) + 0.5f)); };

      for (int z = 0; z <= cell_count; ++z)
        for (int x = 0; x <= cell_count; ++x)
        {
          Point2 loc(x * cell_size, z * cell_size);

          TerrainVertex &v = scratch[z * (cell_count + 1) + x];
          v.position.x = loc.x;
          v.position.z = loc.y;

          Point3 normal;
          if (heightmap->getHeight(loc + origin, v.position.y, &normal))
            v.normal = (float_to_uchar(normal.x * 0.5f + 0.5f) << 16) | (float_to_uchar(normal.y * 0.5f + 0.5f) << 8) |
                       float_to_uchar(normal.z * 0.5f + 0.5f);
          else
            v.normal = 0;
        }
    }
  } heightProvider;

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

  void initBvh()
  {
    if (isRtEnabled())
    {
      bvh::init();
      bvhCtx = bvh::create_context("GI", bvh::ForGI);
    }
  }
  void teardownBvh()
  {
    if (bvhCtx == bvh::InvalidContextId)
      return;

    bvh::teardown(bvhCtx);
    bvh::teardown();
  }

  void addBvhMeshes(scene_type_t *bin_scene)
  {
    if (bvhCtx)
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

  void enforceLruCache()
  {
    // load all instances to BVH. not optimal, but ok for debug purposes
    // in real game we use streaming
    if (bvhCtx != bvh::InvalidContextId && lruColl.lruColl)
    {
      dag::Vector<rendinst::riex_handle_t, framemem_allocator> ri;
      ri.resize(lruColl.collRes.size());
      for (size_t i = 0, ie = ri.size(); i < ie; ++i)
        ri[i] = uint64_t(i) << 32UL;
      lruColl.lruColl->updateLRU(ri);
    }
  }

  void buildBvhRiColl()
  {
    static int countdown = 100;
    if (bvhCtx != bvh::InvalidContextId && lruColl.lruColl && !lruColl.collRes.empty() && !bvhLruMeshBase)
    {
      if (countdown-- > 0)
        return;

      bvhLruMeshBase = bvhIdGen + 1;

      for (auto [modelIx, instances] : enumerate(lruColl.instances))
      {
        uint64_t meshId = ++bvhIdGen;

        auto data = lruColl.getModelData(modelIx);
        if (!data.has_value())
          continue;

        if (data->vertexCount == 0 || data->indexCount == 0)
          continue;

        bvh::MeshInfo meshInfo;
        meshInfo.indices = data->indices;
        meshInfo.indexCount = data->indexCount;
        meshInfo.startIndex = data->startIndex;
        meshInfo.vertices = data->vertices;
        meshInfo.vertexCount = data->vertexCount;
        meshInfo.baseVertex = data->baseVertex;
        meshInfo.vertexSize = data->vertexStride;
        meshInfo.positionOffset = data->positionOffset;
        meshInfo.positionFormat = data->positionFormat;
        meshInfo.posMul = Point4::ONE;
        meshInfo.posAdd = Point4::ZERO;

        bvh::add_mesh(bvhCtx, meshId, meshInfo);
      }
    }

    if (bvhLruMeshBase)
    {
      bvh::update_instances(bvhCtx, Point3::ZERO, Frustum(), nullptr, nullptr, nullptr);

      auto accept = [](auto) { return LRUCollision::ObjectClass::Accept; };
      auto addInstance = [this](size_t i, mat43f_cref tm, bbox3f_cref, bbox3f_cref) {
        bvh::add_instance(bvhCtx, bvhLruMeshBase + i, tm);
      };

      TMatrix itm;
      curCamera->getInvViewMatrix(itm);
      auto viewPos = itm.getcol(3);

      bbox3f bbox;
      v_bbox3_init_by_bsph(bbox, v_ldu(&viewPos.x), v_make_vec3f(bvh_radius, bvh_radius, bvh_radius));
      lruColl.gatherBox(bbox, addInstance, accept);

      mat43f instanceTransform;
      instanceTransform.row0 = v_make_vec4f(1, 0, 0, 0);
      instanceTransform.row1 = v_make_vec4f(0, 1, 0, 0);
      instanceTransform.row2 = v_make_vec4f(0, 0, 1, 0);

      for (int meshIx = 0; meshIx < bvhMeshCount; ++meshIx)
        bvh::add_instance(bvhCtx, bvhMeshBase + meshIx, instanceTransform);

      if (test)
        add_dynrend_instance_to_bvh(bvhCtx, test, testId, testCount, viewPos);
    }
  }
  void buildBvh()
  {
    if (bvhCtx == bvh::InvalidContextId)
      return;

    bvh::start_frame();

    TMatrix itm;
    curCamera->getInvViewMatrix(itm);

    if (binScene)
    {
      heightProvider.heightmap = &binScene->heightmap;
      bvh::add_terrain(bvhCtx, &heightProvider);
      bvh::update_terrain(bvhCtx, Point2::xz(itm.getcol(3)));
    }

    bvh::build(bvhCtx, itm, TMatrix4::IDENT, itm.getcol(3), Point3::ZERO);
  }
};

DemoGameScene *DemoGameScene::the_scene = nullptr;
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
  CONSOLE_CHECK_NAME("camera", "save", 1, 2)
  {
    TMatrix camTm;
    ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->getInvViewMatrix(camTm);
    save_camera_position("cameraPositions.blk", argc > 1 ? argv[1] : "slot0", camTm);
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
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    String str = scene->debugTexOverlay.processConsoleCmd(argv, argc);
    if (!str.empty())
      console::print(str);
  }
  /*CONSOLE_CHECK_NAME("render", "objects_sdf_atlas_sz", 4, 4)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    if (scene->objectsSdf)
      scene->objectsSdf->setAtlasSize(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
  }*/
  return found;
}

uint32_t lru_collision_get_type(rendinst::riex_handle_t h) { return h >> 32ULL; }
mat43f lru_collision_get_transform(rendinst::riex_handle_t h)
{
  uint32_t typ = h >> 32ULL, in = uint32_t(h);
  return DemoGameScene::the_scene->lruColl.instances[typ][in];
}

const CollisionResource *lru_collision_get_collres(uint32_t typ) { return DemoGameScene::the_scene->lruColl.collRes[typ].get(); }


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
  ::enable_tex_mgr_mt(true, blk.getInt("max_tex_count", 1024));
  ::set_gameres_sys_ver(2);
  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  console::init();
  add_con_proc(&test_console);

  ::set_gameres_sys_ver(2);

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
          meshInfo.enableCulling = !isTwoSided;
          meshInfo.boundingSphere = bounding;
          meshInfo.isInterior = false;

          bvh::add_mesh(context_id, meshId, meshInfo);
        }
      },
      [&](const ShaderSkinnedMesh *, int, int) { G_ASSERTF(0, "Skinned parts are not yet supported."); });
  }
}

void add_dynrend_instance_to_bvh(bvh::ContextId context_id, const DynamicRenderableSceneInstance *resource, uint64_t bvh_id, int count,
  const Point3 &view_pos)
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

        if ((view_pos - tm.getcol(3)).lengthSq() > sqr(bvh_radius))
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
