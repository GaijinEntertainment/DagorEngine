#include <render/temporalAA.h>
#include <math/dag_TMatrix4D.h>
#include <astronomy/astronomy.h>
#include <dag_noise/dag_uint_noise.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <workCycle/dag_genGuiMgr.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_drv3d_platform.h>
#include <3d/dag_drv3dCmd.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_postFxRenderer.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_texPackMgr2.h>

#include "test_main.h"
#include <shaders/dag_computeShaders.h>
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <humanInput/dag_hiGlobals.h>
#include <humanInput/dag_hiKeybIds.h>
#include <humanInput/dag_hiPointing.h>
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
#include <ioSys/dag_fileIO.h>
#include <osApiWrappers/dag_files.h>
#include <render/deferredRenderer.h>
#include <render/downsampleDepth.h>
#include <render/screenSpaceReflections.h>
#include <render/tileDeferredLighting.h>
#include <render/preIntegratedGF.h>
#include <render/viewVecs.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daSkiesToBlk.h>
#include "de3_gui.h"
#include "de3_gui_dialogs.h"
#include "de3_hmapTex.h"
#include <render/scopeRenderTarget.h>
#include <scene/dag_loadLevel.h>
#include <streaming/dag_streamingBase.h>
#include <render/debugTexOverlay.h>
#include <shaders/dag_DynamicShaderHelper.h>
#include <render/toroidalStaticShadows.h>
#include <daEditorE/daEditorE.h>
#include "entEdit.h"
#include <render/debugGbuffer.h>

#include <math/dag_hlsl_floatx.h>
#include <daGI/daGI.h>
#include <shaders/dag_overrideStates.h>
#include <EASTL/functional.h>

#include "de3_benchmark.h"
#include "lruCollision.h"

typedef BaseStreamingSceneHolder scene_type_t;

#define SKY_GUI  1
#define TEST_GUI 0

#define GLOBAL_VARS_LIST              \
  VAR(water_level)                    \
  VAR(zn_zfar)                        \
  VAR(view_vecLT)                     \
  VAR(view_vecRT)                     \
  VAR(view_vecLB)                     \
  VAR(view_vecRB)                     \
  VAR(prev_world_view_pos)            \
  VAR(world_view_pos)                 \
  VAR(downsampled_far_depth_tex)      \
  VAR(prev_downsampled_far_depth_tex) \
  VAR(from_sun_direction)             \
  VAR(downsample_depth_type)

#define VAR(a) static int a##VarId = -1;
GLOBAL_VARS_LIST
#undef VAR

enum
{
  SCENE_MODE_NORMAL = 0,
  SCENE_MODE_VOXELIZE = 1,
  SCENE_MODE_VOXELIZE_ALBEDO = 4
};

enum
{
  ONLY_AO = dagi::GI3D::ONLY_AO,
  COLORED = dagi::GI3D::COLORED
};

enum
{
  ONE_BOUNCE = dagi::GI3D::ONE_BOUNCE,
  TILL_CONVERGED = dagi::GI3D::TILL_CONVERGED,
  MULTIPLE_BOUNCE = dagi::GI3D::CONTINUOUS
};

enum
{
  USE_OFF = 0,
  USE_AO = 1,
  USE_FULL = 2,
  USE_RAYTRACED = 3
};

extern bool grs_draw_wire;
bool exit_button_pressed = false;
enum
{
  NUM_ROUGHNESS = 10,
  NUM_METALLIC = 10
};

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


static const int gbuf_rt = 3;
static unsigned gbuf_fmts[gbuf_rt] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_R8G8};
enum
{
  GBUF_ALBEDO_AO = 0,
  GBUF_NORMAL_ROUGH_MET = 1,
  GBUF_MATERIAL = 2,
  GBUF_NUM = 4
};

CONSOLE_BOOL_VAL("render", rasterize_collision, true);
CONSOLE_BOOL_VAL("render", ssao_blur, true);
CONSOLE_BOOL_VAL("render", bnao_blur, true);
CONSOLE_BOOL_VAL("render", bent_cones, true);
CONSOLE_BOOL_VAL("render", dynamic_lights, true);
CONSOLE_BOOL_VAL("render", taa, false);
CONSOLE_BOOL_VAL("render", taa_halton, true);
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
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    float zn = 0.05, zf = 100;
    ShaderGlobal::set_color4(zn_zfarVarId, zn, zf, 0, 0);
    static int light_robe_posVarId = get_shader_variable_id("light_probe_pos", true);
    ShaderGlobal::set_color4(light_robe_posVarId, pos.x, pos.y, pos.z, 1);
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

class DemoGameScene : public DagorGameScene, public IRenderDynamicCubeFace2, public ICascadeShadowsClient
{
public:
  CascadeShadows *csm;
  virtual void getCascadeShadowAnchorPoint(float cascade_from, Point3 &out_anchor)
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    out_anchor = -itm.getcol(3);
  }
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &)
  {
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::settm(TM_PROJ, &csm->getWorldRenderMatrix(cascade_no));
    Frustum frustum = visibility_finder->getFrustum();
    visibility_finder->setFrustum(csm->getFrustum(cascade_no));
    renderOpaque(false, false);
    visibility_finder->setFrustum(frustum);
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
  eastl::unique_ptr<DeferredRenderTarget> target;
  carray<UniqueTex, 2> farDownsampledDepth;
  UniqueTexHolder downsampledNormals, downsampled_close_depth_tex;
  int currentDownsampledDepth;

  UniqueTexHolder frame, prevFrame;
  UniqueTex taaHistory[2];
  PostFxRenderer postfx;
  eastl::unique_ptr<ScreenSpaceReflections> ssr;
  eastl::unique_ptr<SSAORenderer> ssao;
  eastl::unique_ptr<GTAORenderer> gtao;
  DebugTexOverlay debugTexOverlay;

  ///*
  void downsampleDepth(bool downsample_normals)
  {
    TIME_D3D_PROFILE(downsample_depth);
    currentDownsampledDepth = 1 - currentDownsampledDepth;
    UniqueTexHolder null_tex;
    downsample_depth::downsample(target->getDepthAll(), target->getWidth(), target->getHeight(),
      farDownsampledDepth[currentDownsampledDepth], downsampled_close_depth_tex, downsample_normals ? downsampledNormals : null_tex,
      null_tex, null_tex);

    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, farDownsampledDepth[currentDownsampledDepth]);
    ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, farDownsampledDepth[1 - currentDownsampledDepth]);
  }

  UniqueTexHolder combined_shadows;
  PostFxRenderer combine_shadows;

  UniqueTexHolder preIntegratedGF;
  void initGF()
  {
    SCOPE_RENDER_TARGET_NAME(_initGF);
    preIntegratedGF = render_preintegrated_fresnel_GGX("preIntegratedGF", PREINTEGRATE_SPECULAR_DIFFUSE_QUALTY_MAX);
  }

  void combineShadows()
  {
    TIME_D3D_PROFILE(combineShadows);
    SCOPE_RENDER_TARGET_NAME(_combineShadows);
    d3d::set_render_target(combined_shadows.getTex2D(), 0);
    combine_shadows.render();
    combined_shadows.setVar();
  }

  enum
  {
    NO_DEBUG = -1,
    VISIBLE = dagi::GI3D::VISIBLE,
    INTERSECTED = dagi::GI3D::INTERSECTED,
    NON_INTERSECTED = dagi::GI3D::NON_INTERSECTED,
    PROBE_AMBIENT,
    PROBE_INTERSECTION,
    PROBE_AGE,
  };

  enum
  {
    VIS_RAYCAST = dagi::GI3D::VIS_RAYCAST,
    EXACT_VOXELS = dagi::GI3D::EXACT_VOXELS,
    LIT_VOXELS = dagi::GI3D::LIT_VOXELS
  };
  dagi::GI3D ssgi;
  void invalidateGI() { ssgi.afterReset(); }

  BBox3 sceneBox;

  void initSSGIVoxels()
  {
    TIME_D3D_PROFILE(gi_init);
    if (ssgi_panel.volmap_res_xz.pullValueChange() || ssgi_panel.volmap_res_y.pullValueChange() ||
        ssgi_panel.quality.pullValueChange() || ssgi_panel.gi25DWidthScale.pullValueChange() || ssgi_panel.giWidth.pullValueChange() ||
        ssgi_panel.cascade_scale.pullValueChange() || ssgi_panel.maxHtAboveFloor.pullValueChange())
    {
      ssgi.setQuality((dagi::GI3D::QualitySettings)ssgi_panel.quality.get(), ssgi_panel.maxHtAboveFloor.get(),
        2. * ssgi_panel.gi25DWidthScale.get() * ssgi_panel.giWidth.get(), 2. * ssgi_panel.giWidth.get(), ssgi_panel.cascade_scale,
        ssgi_panel.volmap_res_xz, ssgi_panel.volmap_res_y);
    }

    ssgi.setBouncingMode((dagi::GI3D::BouncingMode)ssgi_panel.bouncing_mode);
    ssgi.updateVoxelSceneBox(sceneBox);
    ssgi.setMaxVoxelsPerBin(ssgi_panel.convergence_speed_intersected, ssgi_panel.convergence_speed_non_intersected,
      ssgi_panel.convergence_speed_initial);
  }
  int giUpdatePosFrameCounter = 0;
  enum
  {
    MAX_GI_FRAMES_TO_INVALIDATE_POS = 24
  };
  void updateSSGIPos(const Point3 &pos)
  {
    float giUpdateDist = 64;
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
    SCOPE_RENDER_TARGET;
    d3d::set_render_target();
    ssgi.updateOrigin(
      pos,
      [this](const BBox3 &box, const Point3 &voxel_size) {
        Sbuffer *scene = ssgi.getSceneVoxels25d().getBuf();
        STATE_GUARD_NULLPTR(d3d::set_rwbuffer(STAGE_PS, 7, VALUE), scene);
        renderVoxelsMediaGeom(box, voxel_size, SCENE_MODE_VOXELIZE_ALBEDO);
      },
      [](const BBox3 &, const Point3 &) {},
      [this](const BBox3 &box, const Point3 &voxel_size) {
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_PS, 6, VALUE, 0, 0), ssgi.getSceneVoxelsAlpha().getVolTex());
        STATE_GUARD_NULLPTR(d3d::set_rwtex(STAGE_PS, 7, VALUE, 0, 0), ssgi.getSceneVoxels().getVolTex());
        renderVoxelsMediaGeom(box, voxel_size, SCENE_MODE_VOXELIZE);
      });
    update_visibility_finder();
  }
  void updateSSGISceneVoxels() { ssgi.markVoxelsFromRT(); }
  void updateSSGI(const TMatrix &view_tm, const TMatrix4 &proj_tm)
  {
    farDownsampledDepth[currentDownsampledDepth]->texfilter(TEXFILTER_POINT);
    farDownsampledDepth[currentDownsampledDepth]->texaddr(TEXADDR_CLAMP);
    ssgi.render(view_tm, proj_tm);
    farDownsampledDepth[currentDownsampledDepth]->texaddr(TEXADDR_BORDER);
  }

  shaders::UniqueOverrideStateId updateHeightmapOverride;
  void initOverrides()
  {
    {
      shaders::OverrideState state;
      state.set(shaders::OverrideState::CULL_NONE);
      updateHeightmapOverride = shaders::overrides::create(state);
    }
  }
  void renderVoxelsMediaGeom(const BBox3 &sceneBox, const Point3 &voxel_size, int mode)
  {
    if (!binScene || !land_panel.level)
      return;
    debug("invalid render %@ %@", sceneBox[0], sceneBox[1]);
    static int scene_mode_varId = get_shader_variable_id("scene_mode");
    ShaderGlobal::set_int(scene_mode_varId, mode);
    const IPoint3 res = IPoint3(ceil(div(sceneBox.width(), voxel_size)));
    SCOPE_RENDER_TARGET;
    int maxRes = max(max(res.x, res.y), res.z);
    ssgi.setSubsampledTargetAndOverride(maxRes, maxRes);
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_box0"), sceneBox[0].x, sceneBox[0].y, sceneBox[0].z, maxRes);
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_box1"), 1. / sceneBox.width().x, 1 / sceneBox.width().y,
      1 / sceneBox.width().z, 0.f);

    Point3 voxelize_box0 = sceneBox[0];
    Point3 voxelize_box1 = Point3(1. / sceneBox.width().x, 1. / sceneBox.width().y, 1. / sceneBox.width().z);
    Point3 voxelize_aspect_ratio = point3(res) / maxRes;
    Point3 mult = 2. * mul(voxelize_box1, voxelize_aspect_ratio);
    Point3 add = -mul(voxelize_box0, mult) - voxelize_aspect_ratio;
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_world_to_rasterize_space_mul"), P3D(mult), 0);
    ShaderGlobal::set_color4(get_shader_variable_id("voxelize_world_to_rasterize_space_add"), P3D(add), 0);

    Frustum f;
    Point3 camera_pos(sceneBox.center().x, sceneBox[1].y, sceneBox.center().z);
    {
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, CUBEFACE_NEGY);
      cameraMatrix.setcol(3, camera_pos);
      cameraMatrix = orthonormalized_inverse(cameraMatrix);
      d3d::settm(TM_VIEW, cameraMatrix);
      BBox3 box = cameraMatrix * sceneBox;
      TMatrix4 proj = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
      d3d::settm(TM_PROJ, &proj);
      mat44f globtm;
      d3d::getglobtm(globtm);
      f = Frustum(globtm);
    }
    VisibilityFinder vf;
    vf.set(v_ldu(&camera_pos.x), f, 0.0f, 0.0f, 1.0f, 1.0f, false);

    for (int i = 0; i < 3; ++i)
    {
      ShaderGlobal::set_int(get_shader_variable_id("voxelize_axis", true), i);
      // TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, (i == 0 ? CUBEFACE_NEGX : i == 1 ? CUBEFACE_NEGY : CUBEFACE_NEGZ));
      // cameraMatrix.setcol(3, camera_pos);
      // cameraMatrix = orthonormalized_inverse(cameraMatrix);
      // d3d::settm(TM_VIEW, cameraMatrix);
      // BBox3 box = cameraMatrix*sceneBox;
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
      TMatrix4 proj = matrix_ortho_off_center_lh(sceneBox[0][xAxis], sceneBox[1][xAxis],
                                                 sceneBox[1][yAxis], sceneBox[0][yAxis],
                                                 sceneBox[0][zAxis], sceneBox[1][zAxis]);
      d3d::settm(TM_PROJ, &proj);
      mat44f globtm;
      d3d::getglobtm(globtm);*/
      // vf.set(v_ldu(&camera_pos.x), Frustum(globtm), 0.0f, 0.0f, 1.0f, 1.0f, false);
      d3d::setview(0, 0, maxRes, maxRes, 0, 1);
      binScene->render(vf, 0, 0xFFFFFFFF);
    }
    ssgi.resetOverride();
    ShaderGlobal::set_int(scene_mode_varId, SCENE_MODE_NORMAL);
  }

  void debugVolmap()
  {
    if (ssgi_panel.debug_volmap != NO_DEBUG)
    {
      if (ssgi_panel.probeCascade >= 2)
        ssgi.debugVolmap25D(ssgi_panel.debug_volmap == INTERSECTED          ? dagi25d::IrradianceDebugType::INTERSECTED
                            : ssgi_panel.debug_volmap == PROBE_INTERSECTION ? dagi25d::IrradianceDebugType::INTERSECTION
                                                                            : dagi25d::IrradianceDebugType::AMBIENT);
      else if (ssgi_panel.debug_volmap >= PROBE_AMBIENT && ssgi_panel.debug_volmap <= PROBE_AGE)
        ssgi.drawDebugAllProbes(ssgi_panel.probeCascade, (dagi::GI3D::DebugAllVolmapType)(ssgi_panel.debug_volmap - PROBE_AMBIENT));
      else
        ssgi.drawDebug(ssgi_panel.probeCascade, (dagi::GI3D::DebugVolmapType)ssgi_panel.debug_volmap);
    }
    if (ssgi_panel.debug_scene != NO_DEBUG)
    {
      ssgi.debugSceneVoxelsRayCast((dagi::GI3D::DebugSceneType)ssgi_panel.debug_scene, ssgi_panel.debug_scene_cascade);
    }
    if (ssgi_panel.debug_scene25d != NO_DEBUG)
    {
      ssgi.debugScene25DVoxelsRayCast((dagi25d::SceneDebugType)ssgi_panel.debug_scene25d);
    }
  }

  UniqueTexHolder cambient[2], ambient[2];
  int cAmbientFrame = 0;
  PostFxRenderer ambientRenderer, ambientTemporal, ambientBlur;

  UniqueTexHolder heightmap;
  Point2 heightmapOrigin = {-100000, 10000};
  LRUCollision lruColl;

  void updateHeightmap(const Point3 &at)
  {
    const float cellSize = 1.f;
    Point2 alignedOrigin = point2(ipoint2(Point2::xz(at) / cellSize + Point2(0.5f, 0.5f))) * cellSize;
    if (length(heightmapOrigin - alignedOrigin) < cellSize * 4)
      return;
    heightmapOrigin = alignedOrigin;
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    d3d::set_render_target();
    // set Matrix
    d3d_err(d3d::set_render_target(0, (Texture *)NULL, 0));
    d3d::set_depth(heightmap.getTex2D(), DepthAccess::RW);
    heightmap->texfilter(TEXFILTER_POINT);
    heightmap.setVar();

    TMatrix vtm;
    vtm.setcol(0, 1, 0, 0);
    vtm.setcol(1, 0, 0, 1);
    vtm.setcol(2, 0, 1, 0);
    vtm.setcol(3, 0, 0, 0);
    d3d::settm(TM_VIEW, vtm);

    const float htSize = 64;
    float minHt = sceneBox[0].y - 1, maxHt = sceneBox[1].y + 1;
    alignas(16) TMatrix4 proj = matrix_ortho_off_center_lh(heightmapOrigin.x - htSize, heightmapOrigin.x + htSize,
      heightmapOrigin.y + htSize, heightmapOrigin.y - htSize, maxHt, minHt);
    float worldToTcScale = 0.5 / htSize;
    ShaderGlobal::set_color4(get_shader_variable_id("scene_voxels_2d_heightmap_area"), worldToTcScale, 0,
      0.5 - heightmapOrigin.x * worldToTcScale, 0.5 - heightmapOrigin.y * worldToTcScale);
    ShaderGlobal::set_color4(get_shader_variable_id("scene_voxels_2d_heightmap_scale"), maxHt, minHt - maxHt, 0, 0);
    d3d::settm(TM_PROJ, &proj);

    d3d::clearview(CLEAR_ZBUFFER | CLEAR_STENCIL, 0, 0, 0);
    shaders::overrides::set(updateHeightmapOverride);

    // render everything to gbuffer!
    if (binScene && land_panel.level)
    {
      mat44f globtm;
      d3d::getglobtm(globtm);
      visibility_finder->setFrustum(Frustum(globtm));
      binScene->render();
    }

    shaders::overrides::reset();
  }


  DemoGameScene() : main_pov_data(NULL), cube_pov_data(NULL)
  {
    setDirToSun();
    cpujobs::start_job_manager(1, 256 << 10);
    visualConsoleDriver = NULL;
    unloaded_splash = false;
    test = NULL;
    binScene = NULL;

    if (!(d3d::get_texformat_usage(gbuf_fmts[2]) & d3d::USAGE_RTARGET))
      gbuf_fmts[2] = TEXFMT_DEFAULT;

    d3d::GpuAutoLock gpu_al;
    int w, h;
    d3d::get_target_size(w, h);
    postfx.init("postfx");
    target = eastl::make_unique<DeferredRenderTarget>("deferred_shadow_to_buffer", "main", w, h,
      DeferredRT::StereoMode::MonoOrMultipass, 0, gbuf_rt, gbuf_fmts, TEXFMT_DEPTH32);
    heightmap = dag::create_tex(NULL, 512, 512, TEXFMT_DEPTH16 | TEXCF_RTARGET, 1, "scene_voxels_2d_heightmap");

    combined_shadows = dag::create_tex(NULL, w, h, TEXFMT_L8 | TEXCF_RTARGET, 1, "combined_shadows");
    combine_shadows.init("combine_shadows");
    initGF();
    debugTexOverlay.setTargetSize(Point2(w, h));

    int halfW = w / 2, halfH = h / 2;
    ShaderGlobal::set_color4(::get_shader_variable_id("lowres_rt_params", true), halfW, halfH, HALF_TEXEL_OFSF / halfW,
      HALF_TEXEL_OFSF / halfH);

    ShaderGlobal::set_color4(get_shader_variable_id("rendering_res"), w, h, 1.0f / w, 1.0f / h);
    ShaderGlobal::set_color4(::get_shader_variable_id("taa_display_resolution", true), w, h, 0, 0);
    ssr.reset(new ScreenSpaceReflections(w / 2, h / 2));
    ssao = eastl::make_unique<SSAORenderer>(w / 2, h / 2, 1);

    gtao = eastl::make_unique<GTAORenderer>(w / 2, h / 2, 1);

    ambient[0] = dag::create_tex(NULL, w, h, TEXFMT_R11G11B10F | TEXCF_RTARGET, 1, "screen_ambient");
    ambient[1] = dag::create_tex(NULL, w, h, TEXFMT_R11G11B10F | TEXCF_RTARGET, 1, "screen_ambient2");
    cambient[0] = dag::create_tex(NULL, w, h, TEXFMT_R11G11B10F | TEXCF_RTARGET, 1, "current_ambient");
    cambient[1] = dag::create_tex(NULL, w, h, TEXFMT_R11G11B10F | TEXCF_RTARGET, 1, "current_ambient2");
    cambient[0]->texaddr(TEXADDR_CLAMP);
    cambient[1]->texaddr(TEXADDR_CLAMP);
    ambient[0]->texaddr(TEXADDR_CLAMP);
    ambient[1]->texaddr(TEXADDR_CLAMP);
    ambientRenderer.init("render_ambient");
    ambientTemporal.init("taa_ambient");
    ambientBlur.init("blur_ambient");

    currentDownsampledDepth = 0;
    CascadeShadows::Settings csmSettings;
    csmSettings.cascadeWidth = 512;
    csmSettings.splitsW = csmSettings.splitsH = 2;
    csmSettings.shadowDepthBias = 0.034f;
    csmSettings.shadowConstDepthBias = 5e-5f;
    csmSettings.shadowDepthSlopeBias = 0.34f;
    csm = CascadeShadows::make(this, csmSettings);

    if (ssao || gtao) //||ssr||dynamicLighting
    {
      downsample_depth::init("downsample_depth2x");
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
        farDownsampledDepth[i]->texaddr(TEXADDR_BORDER);
        farDownsampledDepth[i]->texbordercolor(0);
        farDownsampledDepth[i]->texfilter(TEXFILTER_POINT);
        farDownsampledDepth[i]->texmipmap(TEXMIPMAP_POINT);
      }
      downsampled_close_depth_tex = dag::create_tex(NULL, w / 2, h / 2, rfmt | TEXCF_RTARGET, numMips, "downsampled_close_depth_tex");
      downsampled_close_depth_tex->texaddr(TEXADDR_CLAMP);
      downsampled_close_depth_tex->texfilter(TEXFILTER_POINT);
      downsampled_close_depth_tex->texmipmap(TEXMIPMAP_POINT);

      String name(128, "downsampled_normals");
      downsampledNormals = dag::create_tex(NULL, w / 2, h / 2, TEXCF_RTARGET, 1, name); // TEXFMT_A2B10G10R10 |
      downsampledNormals->texaddr(TEXADDR_CLAMP);
    }
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;

    prevFrame = dag::create_tex(NULL, w / 2, h / 2, rtFmt | TEXCF_RTARGET, 1, "prev_frame_tex");
    prevFrame->texaddr(TEXADDR_CLAMP);
    // prevFrame.getTex2D()->texfilter(TEXFILTER_POINT);
    frame = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "frame_tex");
    taaHistory[0] = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "taa_history_tex");
    taaHistory[1] = dag::create_tex(NULL, w, h, rtFmt | TEXCF_RTARGET, 1, "taa_history_tex2");
    taaRender.init("taa");
    if (global_cls_drv_pnt)
      global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, w, h);
    localProbe = NULL;

#define VAR(a) a##VarId = get_shader_variable_id(#a);
    GLOBAL_VARS_LIST
#undef VAR

    daSkies.initSky();
    main_pov_data = daSkies.createSkiesData("main"); // required for each point of view (or the only one if panorama, for panorama
                                                     // center)
    cube_pov_data = daSkies.createSkiesData("cube"); // required for each point of view (or the only one if panorama, for panorama
                                                     // center)
  }
  PostFxRenderer taaRender;
  enum
  {
    CLOUD_VOLUME_W = 32,
    CLOUD_VOLUME_H = 16,
    CLOUD_VOLUME_D = 32
  };
  SkiesData *main_pov_data, *cube_pov_data;

  ~DemoGameScene()
  {
    daSkies.destroy_skies_data(main_pov_data);
    daSkies.destroy_skies_data(cube_pov_data);
    ssr.reset();
    del_it(csm);
    for (int i = 0; i < farDownsampledDepth.size(); ++i)
      farDownsampledDepth[i].close();
    downsampledNormals.close();
    downsampled_close_depth_tex.close();
    ShaderGlobal::set_texture(downsampled_far_depth_texVarId, BAD_TEXTUREID);
    ShaderGlobal::set_texture(prev_downsampled_far_depth_texVarId, BAD_TEXTUREID);
    downsample_depth::close();

    del_it(test);
    del_it(binScene);
    del_it(visualConsoleDriver);
    light_probe::destroy(localProbe);

    cpujobs::stop_job_manager(1, true);
    de3_imgui_term();
    ShaderGlobal::reset_textures();
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

  virtual void drawScene()
  {

    StdGuiRender::reset_per_frame_dynamic_buffer_pos();

    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    // todo: clamp itm.getcol(3) with shrinked sceneBox - there is nothing interesting outside scene
    initSSGIVoxels();
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
    bool renderTaa = taa.get();

    Driver3dPerspective p;
    d3d::getpersp(p);

    if (renderTaa || gi_panel.per_pixel_trace)
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

      set_temporal_parameters(noJitterGlobTm, prevNoJitterGlobTm, jitteredOfs, taaFrame == 0 ? 10.f : usecs / 1e6, w, taaParams);
      prevNoJitterGlobTm = noJitterGlobTm;
    }


    d3d::set_render_target(frame.getTex2D(), 0);
    render();
    if (gi_panel.update_ssgi && gi_panel.gi_mode == SSGI) // && gi_panel.onscreen_mode == RESULT
    {
      d3d::set_render_target();
      frame.setVar();

      TMatrix4 projTm;
      d3d::calcproj(p, projTm);
      updateSSGI(orthonormalized_inverse(itm), projTm);
    }

    d3d::set_render_target(frame.getTex2D(), 0);
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    renderTrans();


    if (renderTaa)
    {
      int taaHistoryI = taaFrame & 1;
      d3d::set_render_target(taaHistory[1 - taaHistoryI].getTex2D(), 0);
      frame->texfilter(TEXFILTER_POINT);
      ShaderGlobal::set_texture(get_shader_variable_id("taa_history_tex"), taaHistory[taaHistoryI]);
      ShaderGlobal::set_texture(get_shader_variable_id("taa_frame_tex"), frame);
      taaRender.render();
      ShaderGlobal::set_texture(frame.getVarId(), taaHistory[1 - taaHistoryI]);
      taaFrame++;
    }
    else
      frame.setVar();
    if (::grs_draw_wire)
      d3d::setwire(0);

    d3d::set_render_target();
    d3d::set_srgb_backbuffer_write(true);

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

    debugTexOverlay.render();
    de3_imgui_render();

    if (console::get_visual_driver())
    {
      console::get_visual_driver()->update();
      console::get_visual_driver()->render();
    }
    // if (::dagor_gui_manager)

    bool do_start = !StdGuiRender::is_render_started();
    if (do_start)
      StdGuiRender::start_render();
    StdGuiRender::set_font(0);
    StdGuiRender::set_color(255, 0, 255, 255);
    StdGuiRender::set_ablend(true);

    StdGuiRender::goto_xy(200, 25);

    if (gi_panel.gi_mode == ENVI_PROBE)
      ShaderGlobal::set_int(get_shader_variable_id("use_gi_quality"), -2);

    if (gi_panel.gi_mode == SSGI)
    {
      ssgi.setVars();
    }

    StdGuiRender::flush_data();
    if (do_start)
      StdGuiRender::end_render();

    {
      TIME_D3D_PROFILE(gui);
      static Color3 sun[2], sky[2], moonsun[2], moonsky[2];
      static int show_sun_sky = 0;

      bool do_start = !StdGuiRender::is_render_started();
      if (do_start)
        StdGuiRender::start_render();
      StdGuiRender::set_font(0);
      StdGuiRender::set_color(255, 0, 255, 255);
      StdGuiRender::set_ablend(true);
      StdGuiRender::flush_data();
      if (do_start)
        StdGuiRender::end_render();
    }
    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
  }

  virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
  {
    TIME_D3D_PROFILE(beforeDraw);
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

    curCamera->setView();

    update_visibility_finder();
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
    ShaderGlobal::set_color4(world_view_posVarId, itm.getcol(3).x, itm.getcol(3).y, itm.getcol(3).z, 1);
    CascadeShadows::ModeSettings mode;
    mode.powWeight = 0.8;
    mode.maxDist = min(50.f, staticShadows->getDistance() * 0.55f);
    mode.shadowStart = p.zn;
    mode.numCascades = 4;
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    csm->prepareShadowCascades(mode, dir_to_sun, inverse(itm), itm.getcol(3), projTm, Frustum(globtm), Point2(p.zn, p.zf), p.zn);
    updateHeightmap(itm.getcol(3));
    de3_imgui_before_render();
  }

  virtual void sceneSelected(DagorGameScene * /*prev_scene*/) { loadScene(); }

  virtual void sceneDeselected(DagorGameScene * /*new_scene*/)
  {
    webui::shutdown();
    console::set_visual_driver(NULL, NULL);
    delete this;
  }

  // IRenderWorld interface
  void renderTrees()
  {
    if (!land_panel.trees)
      return;
    TIME_D3D_PROFILE(tree);

    if (test)
      test->render();
  }
  void renderLevel()
  {
    if (!land_panel.level || !binScene)
      return;
    TIME_D3D_PROFILE(level);
    binScene->render();
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
    renderLevel();
    d3d::set_render_target(prevRT);
  }
  void renderOpaque(bool render_decals, bool change_frustum)
  {
    TIME_D3D_PROFILE(opaque);
    renderTrees();
    Frustum frustum = visibility_finder->getFrustum();
    if (change_frustum)
    {
      mat44f globtm;
      d3d::getglobtm(globtm);
      visibility_finder->setFrustum(Frustum(globtm));
    }
    renderLevel();
    if (change_frustum)
      visibility_finder->setFrustum(frustum);
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
      binScene->render();
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
      d3d::driver_command(DRV3D_COMMAND_OVERRIDE_MAX_ANISOTROPY_LEVEL, one, NULL, NULL); //-V566
    }

    void renderStaticShadowDepth(const mat44f &, const ViewTransformData &, int) override { scene->renderOpaque(false, true); }
    void endRenderStaticShadow() override
    {
      shaders::overrides::reset();
      d3d::driver_command(DRV3D_COMMAND_OVERRIDE_MAX_ANISOTROPY_LEVEL, (void *)0, NULL, NULL);
    }
  };


  shaders::UniqueOverrideStateId depthBiasAndClamp;

  void updateStaticShadowAround(const Point3 &p, float t)
  {
    initStaticShadow();
    bool invalidVars = staticShadows->setSunDir(dir_to_sun);
    StaticShadowCallback cb(this, depthBiasAndClamp.get());
    if (staticShadows->updateOriginAndRender(p, t, 0.1f, 0.1f, false, cb).varsChanged || invalidVars)
      staticShadows->setShaderVars();
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
      box.inflate(1.f);
      sceneBox = box;
      heightmapOrigin -= Point2(1000, 10000);
      staticShadows->setWorldBox(box);
    }
    invalidateGI();
  }

  void renderAmbient()
  {
    TIME_D3D_PROFILE(ambientFixup)
    SCOPE_RENDER_TARGET;
    target->setVar();
    if (gi_panel.gi_mode == SSGI)
    {
      ShaderGlobal::set_int(get_shader_variable_id("use_gi_quality"),
        (dagi::GI3D::QualitySettings)ssgi_panel.quality == dagi::GI3D::ONLY_AO
          ? USE_AO
          : (gi_panel.per_pixel_trace ? USE_RAYTRACED : USE_FULL));
      if (gi_panel.per_pixel_trace)
      {
        d3d::set_render_target(cambient[0].getTex2D(), 0);
        d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
        {
          TIME_D3D_PROFILE(frame_ambient);
          ambientRenderer.render();
        }
        {
          TIME_D3D_PROFILE(blur_ambient);
          d3d::set_render_target(cambient[1].getTex2D(), 0);
          ShaderGlobal::set_int(get_shader_variable_id("ambient_blur_x"), 1);
          ShaderGlobal::set_texture(get_shader_variable_id("current_ambient"), cambient[0]);
          ambientBlur.render();

          d3d::set_render_target(cambient[0].getTex2D(), 0);
          ShaderGlobal::set_int(get_shader_variable_id("ambient_blur_x"), 0);
          ShaderGlobal::set_texture(get_shader_variable_id("current_ambient"), cambient[1]);
          ambientBlur.render();
        }
        ShaderGlobal::set_texture(get_shader_variable_id("current_ambient"), cambient[0]);

        cAmbientFrame = 1 - cAmbientFrame;
        d3d::set_render_target(ambient[cAmbientFrame].getTex2D(), 0);
        d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
        ShaderGlobal::set_texture(get_shader_variable_id("prev_ambient"), ambient[1 - cAmbientFrame]);
        cambient[0]->texfilter(TEXFILTER_POINT);
        {
          TIME_D3D_PROFILE(temporal);
          ambientTemporal.render();
          ShaderGlobal::set_texture(ambient[0].getVarId(), ambient[cAmbientFrame]);
        }
        cambient[0]->texfilter(TEXFILTER_SMOOTH);
      }
      else
      {
        cAmbientFrame = 0;
        d3d::set_render_target(ambient[0].getTex2D(), 0);
        d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
        ambient[0].setVar();
        ambientRenderer.render();
      }
    }
    else
    {
      cAmbientFrame = 0;
      d3d::set_render_target(ambient[0].getTex2D(), 0);
      ambient[0].setVar();
      d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
      ambientRenderer.render();
    }
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
    TMatrix itm;
    TMatrix view;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    curCamera->getInvViewMatrix(itm);
    {
      TIME_D3D_PROFILE(changeData)
      daSkies.changeSkiesData(render_panel.sky_quality, render_panel.direct_quality, !render_panel.infinite_skies, target->getWidth(),
        target->getHeight(), main_pov_data);

      daSkies.useFog(itm.getcol(3), main_pov_data, view, projTm);
    }

    updateStaticShadowAround();

    target->setRt();
    d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER | CLEAR_STENCIL, 0x00FFFFFF, 0, 0); //
    set_inv_globtm_to_shader(view, projTm, false);
    set_viewvecs_to_shader(view, projTm);
    ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_FRAME);
    {
      TIME_D3D_PROFILE(scene)
      renderPrepass();
      renderOpaque(true, false);
    }

    if (::grs_draw_wire)
      d3d::setwire(0);
    extern bool g_ssao_blur;
    g_ssao_blur = ssao_blur.get();
    downsampleDepth(ssao || ssr || gtao);

    if (ssr)
      ssr->render(view, projTm);
    if (!gi_panel.ssr)
    {
      ShaderGlobal::set_texture(get_shader_variable_id("ssr_target"), BAD_TEXTUREID);
    }
    if (gi_panel.ssao && ssao)
    {
      farDownsampledDepth[1 - currentDownsampledDepth]->texfilter(TEXFILTER_SMOOTH);
      ssao->render(view, projTm, farDownsampledDepth[currentDownsampledDepth]);
      farDownsampledDepth[1 - currentDownsampledDepth]->texfilter(TEXFILTER_POINT);
    }

    if (gi_panel.gtao && gtao)
      gtao->render(view, projTm);

    if (!gi_panel.ssao && !gi_panel.gtao)
      ShaderGlobal::set_texture(get_shader_variable_id("ssao_tex"), BAD_TEXTUREID);

    renderAmbient();
    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), gi_panel.onscreen_mode);
    {
      csm->renderShadowsCascades();
      csm->setCascadesToShader();
      combineShadows();
      {
        if (gi_panel.update_scene && gi_panel.gi_mode == SSGI)
        {
          set_inv_globtm_to_shader(view, projTm, false);
          set_viewvecs_to_shader(view, projTm);
          target->setVar();
          updateSSGISceneVoxels();
        }

        TIME_D3D_PROFILE(shading)
        target->resolve(frame.getTex2D(), view, projTm);
        d3d::set_render_target(frame.getTex2D(), 0);
      }
    }
    ShaderGlobal::set_int(get_shader_variable_id("deferred_lighting_mode"), RESULT);

    d3d::set_depth(target->getDepth(), DepthAccess::SampledRO);
    renderEnvi();
    d3d::set_depth(target->getDepth(), DepthAccess::RW);
    d3d::stretch_rect(frame.getTex2D(), prevFrame.getTex2D());

    if (::grs_draw_wire)
      d3d::setwire(::grs_draw_wire);
    if (sleep_msec_val.get())
      sleep_msec(sleep_msec_val.get());
  }

  void rasterizeCollision() { lruColl.rasterize(lruColl.instances); }

  virtual void renderTrans()
  {
    if (binScene && land_panel.level)
      binScene->renderTrans();
    if (rasterize_collision)
      rasterizeCollision();
    debugVolmap();
  }

  virtual void renderIslDecals() {}

  virtual void renderToShadowMap() {}

  virtual void renderWaterReflection() {}

  virtual void renderWaterRefraction() {}

  void reinitCube(int ew)
  {
    cube.init(ew);
    light_probe::destroy(localProbe);
    localProbe = light_probe::create("local", ew, TEXFMT_A16B16G16R16F);
    reloadCube(true);
    initEnviProbe();
  }
  UniqueTexHolder enviProbe;
  void initEnviProbe()
  {
    enviProbe.close();
    enviProbe = dag::create_cubetex(128, TEXCF_RTARGET | TEXFMT_A16B16G16R16F | TEXCF_GENERATEMIPS | TEXCF_CLEAR_ON_CREATE, 3,
      "envi_probe_specular");
    renderEnviProbe();
  }

  void renderEnviProbe()
  {
    if (!enviProbe.getCubeTex())
    {
      return initEnviProbe();
    }
    SCOPE_RENDER_TARGET;
    SCOPE_VIEW_PROJ_MATRIX;
    float zn = 0.1, zf = 1000;
    for (int i = 0; i < 6; ++i)
    {
      target->setRt();
      d3d::setpersp(Driver3dPerspective(1, 1, zn, zf, 0, 0));
      TMatrix cameraMatrix = cube_matrix(TMatrix::IDENT, i);
      cameraMatrix.setcol(3, 0, 100, 0);
      d3d::settm(TM_VIEW, orthonormalized_inverse(cameraMatrix));

      d3d::set_render_target(enviProbe.getCubeTex(), i, 0);
      d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      renderLightProbeEnvi();
    }
    enviProbe.getCubeTex()->generateMips();
  }

  void reloadCube(bool first = false)
  {
    d3d::GpuAutoLock gpu_al;
    renderEnviProbe();
    initStaticShadow();
    static int local_light_probe_texVarId = get_shader_variable_id("local_light_probe_tex", true);
    if (first)
    {
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
      TMatrix itm;
      curCamera->getInvViewMatrix(itm);
      cube.update(light_probe::getManagedTex(localProbe), *this, first ? Point3(0, -10, 0) : itm.getcol(3));
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
    ENVI_PROBE,
    SSGI
  };
  IGameCamera *curCamera; // for console made publuc
  IFreeCameraDriver *freeCam;

protected:
  DynamicRenderableSceneInstance *test;
  scene_type_t *binScene;

  bool unloaded_splash;

  void loadScene()
  {
    FullFileLoadCB riCollision("ri_collisions.bin");
    if (riCollision.fileHandle)
      lruColl.load(riCollision);
#if !_TARGET_PC
    freeCam = create_gamepad_free_camera();
#else
    global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(true);
    freeCam = create_mskbd_free_camera();
    freeCam->scaleSensitivity(4, 2);
#endif
    IFreeCameraDriver::zNear = 0.05;
    IFreeCameraDriver::zFar = 1000.0;
    IFreeCameraDriver::turboScale = 20;

    de3_imgui_init("Test GI", "Sky properties");

    static GuiControlDesc skyGuiDesc[] = {
      DECLARE_INT_COMBOBOX(ssgi_panel, debug_volmap, NO_DEBUG, NO_DEBUG, VISIBLE, INTERSECTED, NON_INTERSECTED, PROBE_AMBIENT,
        PROBE_INTERSECTION, PROBE_AGE),
      DECLARE_INT_SLIDER(ssgi_panel, probeCascade, 0, 2, 0),
      DECLARE_INT_COMBOBOX(ssgi_panel, debug_scene, NO_DEBUG, NO_DEBUG, VIS_RAYCAST, EXACT_VOXELS, LIT_VOXELS),
      DECLARE_INT_COMBOBOX(ssgi_panel, debug_scene25d, NO_DEBUG, NO_DEBUG, VIS_RAYCAST, EXACT_VOXELS, LIT_VOXELS),
      DECLARE_INT_SLIDER(ssgi_panel, debug_scene_cascade, 0, 4, 0),
      DECLARE_INT_SLIDER(ssgi_panel, volmap_res_xz, 32, 256, 64),
      DECLARE_INT_SLIDER(ssgi_panel, volmap_res_y, 4, 64, 32),
      DECLARE_FLOAT_SLIDER(ssgi_panel, giWidth, 16., 256., 32., 1),
      DECLARE_FLOAT_SLIDER(ssgi_panel, gi25DWidthScale, 2., 16., 4., 0.1),
      DECLARE_FLOAT_SLIDER(ssgi_panel, maxHtAboveFloor, 12., 64., 18., 1),
      DECLARE_FLOAT_SLIDER(ssgi_panel, cascade_scale, 2, 4, 2., 0.1),
      DECLARE_FLOAT_SLIDER(ssgi_panel, convergence_speed_intersected, 0.125, 1, 0.125, 0.0625),
      DECLARE_FLOAT_SLIDER(ssgi_panel, convergence_speed_non_intersected, 0.125, 1, 0.125, 0.0625),
      DECLARE_FLOAT_SLIDER(ssgi_panel, convergence_speed_initial, 0.125, 1, 1, 0.0625),
      DECLARE_INT_COMBOBOX(ssgi_panel, quality, COLORED, ONLY_AO, COLORED),
      DECLARE_INT_COMBOBOX(ssgi_panel, bouncing_mode, MULTIPLE_BOUNCE, ONE_BOUNCE, TILL_CONVERGED, MULTIPLE_BOUNCE),

      DECLARE_BOOL_CHECKBOX(gi_panel, update_ssgi, true),
      DECLARE_BOOL_CHECKBOX(gi_panel, update_scene, true),
      DECLARE_BOOL_CHECKBOX(gi_panel, ssao, false),
      DECLARE_BOOL_CHECKBOX(gi_panel, gtao, false),
      DECLARE_BOOL_CHECKBOX(gi_panel, ssr, true),
      DECLARE_BOOL_CHECKBOX(gi_panel, per_pixel_trace, false),
      DECLARE_INT_COMBOBOX(gi_panel, onscreen_mode, RESULT, RESULT, LIGHTING, DIFFUSE_LIGHTING, DIRECT_LIGHTING, INDIRECT_LIGHTING),
      DECLARE_INT_COMBOBOX(gi_panel, gi_mode, SSGI, ENVI_PROBE, SSGI),
      DECLARE_INT_SLIDER(gi_panel, lightmap_size, 32, 4096, 256),
      DECLARE_INT_SLIDER(gi_panel, voxel_dimensions_xz, 32, 256, 64),
      DECLARE_INT_SLIDER(gi_panel, voxel_dimensions_y, 4, 64, 16),
      DECLARE_INT_SLIDER(gi_panel, voxel_iterations, 1, 4, 1),

      DECLARE_INT_COMBOBOX(gi_panel, cube_size, 128, 32, 64, 128, 256),
      DECLARE_INT_SLIDER(gi_panel, probes_xz, 2, 32, 8),
      DECLARE_INT_SLIDER(gi_panel, probes_y, 2, 16, 4),
      DECLARE_FLOAT_SLIDER(gi_panel, exposure, 0.1, 16, 1, 0.01),

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
      DECLARE_INT_SLIDER(sun_panel, year, 1941, 2016, 1939),
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
    visualConsoleDriver = new VisualConsoleDriver;
    console::set_visual_driver(visualConsoleDriver, NULL);
    if (dgs_get_settings()->getStr("model", NULL))
      scheduleDynModelLoad(&test, dgs_get_settings()->getStr("model", "test"), dgs_get_settings()->getStr("model_skel", NULL),
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
    initOverrides();

    if (const char *level_fn = dgs_get_settings()->getStr("level", NULL))
      scheduleLevelBinLoad(level_fn);
  }


  void renderEnvi()
  {
    TIME_D3D_PROFILE(envi)
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    target->getDepth()->texfilter(TEXFILTER_POINT);
    farDownsampledDepth[currentDownsampledDepth]->texfilter(TEXFILTER_POINT);
    farDownsampledDepth[1 - currentDownsampledDepth]->texaddr(TEXADDR_CLAMP);
    TMatrix view;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    Driver3dPerspective persp;
    d3d::getpersp(persp);
    daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 3,
      farDownsampledDepth[currentDownsampledDepth], farDownsampledDepth[1 - currentDownsampledDepth], target->getDepthId(),
      main_pov_data, view, projTm, persp);
    farDownsampledDepth[1 - currentDownsampledDepth].getTex2D()->texaddr(TEXADDR_BORDER);
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
    bool level, enabled, trees, prepass;
    float heightScale, heightMin, cellSize;
  } land_panel;

  struct
  {
    bool ssao = true, gtao = true, ssr = true;
    bool update_ssgi, update_scene = true;
    bool per_pixel_trace = false;
    int onscreen_mode = RESULT;
    int gi_mode = ENVI_PROBE;
    int voxel_dimensions_y = 16;
    int voxel_dimensions_xz = 64;
    int voxel_iterations = 1;
    int lightmap_size = 256;
    int cube_size = 64;
    float exposure = 1;
    int probes_xz = 8, probes_y = 4;
  } gi_panel;

  struct
  {
    IntProperty volmap_res_y = 16, volmap_res_xz = 64;
    int debug_volmap = NO_DEBUG, probeCascade = 0, debug_scene = NO_DEBUG, debug_scene_cascade = 0, debug_scene25d = NO_DEBUG;
    FloatProperty cascade_scale = 3, giWidth = 44, gi25DWidthScale = 5, maxHtAboveFloor = 12.;
    float convergence_speed_intersected = 0.125, convergence_speed_non_intersected = 0.125, convergence_speed_initial = 1;
    IntProperty quality = COLORED;
    int bouncing_mode = MULTIPLE_BOUNCE;
  } ssgi_panel;

  struct
  {
    bool fill_voxels, always_fill_voxels, debug_raycast, debug_conetrace, debug_rasterize;
  } voxels_panel;


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


  void scheduleDynModelLoad(DynamicRenderableSceneInstance **dm, const char *name, const char *skel_name, const Point3 &pos)
  {
    struct LoadJob : cpujobs::IJob
    {
      DynamicRenderableSceneInstance **dm;
      SimpleString name, skelName;
      Point3 pos;

      LoadJob(DynamicRenderableSceneInstance **_dm, const char *_name, const char *_skel_name, const Point3 &_pos) :
        dm(_dm), name(_name), skelName(_skel_name), pos(_pos)
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
        if (!skelName.empty() && val)
        {
          GeomNodeTree *skel = (GeomNodeTree *)get_game_resource_ex(GAMERES_HANDLE_FROM_STRING(skelName), GeomNodeTreeGameResClassId);

          if (skel)
          {
            TMatrix tm;
            tm.identity();
            tm.setcol(3, pos);
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

        reset_required_res_list_restriction();
        if (!is_managed_textures_streaming_load_on_demand())
          ddsx::tex_pack2_perform_delayed_data_loading();

        struct UpdatePtr : public DelayedAction
        {
          DynamicRenderableSceneInstance **dest, *val;
          UpdatePtr(DynamicRenderableSceneInstance **_dest, DynamicRenderableSceneInstance *_val) : dest(_dest), val(_val) {}
          virtual void performAction() { *dest = val; }
        };
        add_delayed_action(new UpdatePtr(dm, val));
      }
      virtual void releaseJob() { delete this; }
    };
    *dm = NULL;
    cpujobs::add_job(1, new LoadJob(dm, name, skel_name, pos));
  }
  void scheduleLevelBinLoad(const char *level_bindump_fn)
  {
    struct LoadJob : cpujobs::IJob
    {
      SimpleString fn;
      scene_type_t **destScn;

      LoadJob(const char *_fn, scene_type_t **scn) : fn(_fn), destScn(scn) {}
      virtual void doJob()
      {
        scene_type_t *scn = new scene_type_t;
        scn->openSingle(fn);
        if (!is_managed_textures_streaming_load_on_demand())
          ddsx::tex_pack2_perform_delayed_data_loading();

        struct UpdatePtr : public DelayedAction
        {
          scene_type_t **dest, *val;
          UpdatePtr(scene_type_t **_dest, scene_type_t *_val) : dest(_dest), val(_val) {}
          virtual void performAction()
          {
            *dest = val;
            ((DemoGameScene *)dagor_get_current_game_scene())->onSceneLoaded();
          }
        };
        add_delayed_action(new UpdatePtr(destScn, scn));
      }
      virtual void releaseJob() { delete this; }
    };

    if (!visibility_finder)
      visibility_finder = new VisibilityFinder;
    cpujobs::add_job(1, new LoadJob(level_bindump_fn, &binScene));
  }
};

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
    unsigned int slotNo = 0;
    if (argc == 2)
      slotNo = to_int(argv[1]);

    DataBlock cameraPositions;
    if (dd_file_exist("cameraPositions.blk"))
      cameraPositions.load("cameraPositions.blk");

    TMatrix camTm;
    ((DemoGameScene *)dagor_get_current_game_scene())->curCamera->getInvViewMatrix(camTm);


    DataBlock *slotBlk = cameraPositions.getBlockByName(String(100, "slot%d", slotNo));

    if (!slotBlk)
      slotBlk = cameraPositions.addNewBlock(String(100, "slot%d", slotNo));

    slotBlk->setPoint3("tm0", camTm.getcol(0));
    slotBlk->setPoint3("tm1", camTm.getcol(1));
    slotBlk->setPoint3("tm2", camTm.getcol(2));
    slotBlk->setPoint3("tm3", camTm.getcol(3));

    cameraPositions.saveToTextFile("cameraPositions.blk");
  }
  CONSOLE_CHECK_NAME("camera", "restore", 1, 2)
  {
    unsigned int slotNo = 0;
    if (argc == 2)
      slotNo = to_int(argv[1]);

    DataBlock cameraPositions;
    if (dd_file_exist("cameraPositions.blk"))
      cameraPositions.load("cameraPositions.blk");

    TMatrix camTm;
    const DataBlock *slotBlk = cameraPositions.getBlockByNameEx(String(100, "slot%d", slotNo));

    camTm.setcol(0, slotBlk->getPoint3("tm0", Point3(1.f, 0.f, 0.f)));
    camTm.setcol(1, slotBlk->getPoint3("tm1", Point3(0.f, 1.f, 0.f)));
    camTm.setcol(2, slotBlk->getPoint3("tm2", Point3(0.f, 0.f, 1.f)));
    camTm.setcol(3, slotBlk->getPoint3("tm3", ZERO<Point3>()));

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
  return found;
}

uint32_t lru_collision_get_type(rendinst::riex_handle_t h) { return h >> 32ULL; }
mat43f lru_collision_get_transform(rendinst::riex_handle_t h)
{
  uint32_t typ = h >> 32ULL, in = uint32_t(h);
  return ((DemoGameScene *)dagor_get_current_game_scene())->lruColl.instances[typ][in];
}

const CollisionResource *lru_collision_get_collres(uint32_t typ)
{
  return ((DemoGameScene *)dagor_get_current_game_scene())->lruColl.collRes[typ].get();
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
  d3d::driver_command(DRV3D_COMMAND_ENABLE_MT, NULL, NULL, NULL);
  ::enable_tex_mgr_mt(true, 1024);
  ::set_gameres_sys_ver(2);
  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  console::init();
  add_con_proc(&test_console);

  const DataBlock &blk = *dgs_get_settings();
  ::set_gameres_sys_ver(2);

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
