// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <astronomy/astronomy.h>
#include <perfMon/dag_perfTimer.h>
#include <osApiWrappers/dag_direct.h>
#include <debug/dag_logSys.h>
#include <ioSys/dag_dataBlock.h>
#include <workCycle/dag_gameScene.h>
#include <workCycle/dag_workCycle.h>
#include <workCycle/dag_gameSettings.h>
#include <drv/3d/dag_renderTarget.h>
#include <drv/3d/dag_draw.h>
#include <drv/3d/dag_vertexIndexBuffer.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <drv/3d/dag_driver.h>
#include <drv/3d/dag_platform.h>
#include <drv/3d/dag_lock.h>
#include <drv/3d/dag_resetDevice.h>
#include <shaders/dag_shaderMesh.h>
#include <shaders/dag_shaderBlock.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_postFxRenderer.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameRes.h>
#include <3d/dag_texPackMgr2.h>
#include <3d/dag_picMgr.h>

#include "test_main.h"
#include <startup/dag_inpDevClsDrv.h>
#include <startup/dag_globalSettings.h>
#include <drv/hid/dag_hiGlobals.h>
#include <drv/hid/dag_hiKeybIds.h>
#include <drv/hid/dag_hiPointing.h>
#include "de3_freeCam.h"
#include <gui/dag_stdGuiRender.h>
#include <perfMon/dag_perfMonStat.h>
#include <perfMon/dag_cpuFreq.h>
#include <osApiWrappers/dag_vromfs.h>
#include <time.h>
#include <webui/httpserver.h>
#include <webui/helpers.h>
#include <util/dag_console.h>
#include <render/debugTexOverlay.h>
#include <util/dag_convar.h>
#include <render/sphHarmCalc.h>
#include <render/cameraPositions.h>
#include <osApiWrappers/dag_miscApi.h>
#include <render/cascadeShadows.h>
#include <render/ssao.h>
#include <math/dag_TMatrix4.h>
#include <math/dag_cube_matrix.h>
#include <math/dag_frustum.h>
#include <math/random/dag_random.h>
#include <math/dag_plane3.h>
#include <gui/dag_visConsole.h>
#include <perfMon/dag_statDrv.h>
#include <visualConsole/dag_visualConsole.h>
#include <generic/dag_carray.h>
#include <render/lightCube.h>
#include <render/viewVecs.h>

#include <osApiWrappers/dag_cpuJobs.h>
#include <util/dag_delayedAction.h>
#include <util/dag_oaHashNameMap.h>
#include <math/dag_geomTree.h>
#include <heightmap/heightmapRenderer.h>
#include <heightmap/heightmapCulling.h>
#include <ioSys/dag_fileIo.h>
#include <osApiWrappers/dag_files.h>
#include <math/dag_adjpow2.h>
#include <render/deferredRenderer.h>
#include <render/downsampleDepth.h>
#include <render/screenSpaceReflections.h>
#include <render/tileDeferredLighting.h>
#include <daSkies2/daSkies.h>
#include <daSkies2/daSkiesToBlk.h>
#include "de3_gui_dialogs.h"
#include "de3_hmapTex.h"
#include <render/scopeRenderTarget.h>
#include <fftWater/fftWater.h>

#include <startup/dag_startupTex.h>
#include <image/dag_dds.h>

#include <3d/dag_resPtr.h>

#include <render/daBfg/bfg.h>
#include <render/fx/auroraBorealis.h>
#include <render/noiseTex.h>
#include <render/spheres_consts.hlsli>

#include "de3_benchmark.h"

#define USE_WEBUI_VAR_EDITOR 0

#if USE_WEBUI_VAR_EDITOR == 1
#include "de3_gui_webui_proxy.h"
#else
#include "de3_gui.h"
#endif

extern "C" const size_t PS4_USER_MALLOC_MEM_SIZE = 1ULL << 30; // 1GB

PULL_CONSOLE_PROC(def_app_console_handler)
PULL_CONSOLE_PROC(profiler_console_handler)

#define GLOBAL_VARS_LIST      \
  VAR(water_level)            \
  VAR(world_view_pos)         \
  VAR(from_sun_direction)     \
  VAR(plane)                  \
  VAR(screen_pos_to_texcoord) \
  VAR(screen_size)            \
  VAR(zn_zfar)

namespace shvars
{
#define VAR(a) static ShaderVariableInfo a(#a);
GLOBAL_VARS_LIST
#undef VAR
} // namespace shvars

extern bool grs_draw_wire;
bool exit_button_pressed = false;
enum
{
  NUM_ROUGHNESS = 10,
  NUM_METALLIC = 8
};

static void init_webui(const DataBlock *debug_block)
{
  if (!debug_block)
    debug_block = dgs_get_settings()->getBlockByNameEx("debug");
  if (debug_block->getBool("http_server", true))
  {
    static webui::HttpPlugin test_http_plugins[] = {
      webui::profiler_http_plugin,
      webui::shader_http_plugin,
      webui::color_pipette_http_plugin,
      webui::colorpicker_http_plugin,
      webui::cookie_http_plugin,
#if USE_WEBUI_VAR_EDITOR == 1
      webui::editvar_http_plugin,
#endif
      nullptr
    };
    // webui::plugin_lists[1] = aces_http_plugins;
    webui::plugin_lists[0] = webui::dagor_http_plugins;
    webui::plugin_lists[1] = test_http_plugins;
    webui::Config cfg;
    memset(&cfg, 0, sizeof(cfg));
    webui::startup(&cfg);
  }
}


static const int gbuf_rt = 3;
static unsigned gbuf_fmts[gbuf_rt] = {TEXFMT_A8R8G8B8 | TEXCF_SRGBREAD | TEXCF_SRGBWRITE, TEXFMT_A2B10G10R10, TEXFMT_R8};
enum
{
  GBUF_ALBEDO_AO = 0,
  GBUF_NORMAL_ROUGH_MET = 1,
  GBUF_MATERIAL = 2,
  GBUF_NUM = 4
};

static const char *depth_tex_name = "depth_gbuf";
static const char *normal_tex_name = "normal_gbuf";
static const char *gbuf_tex_names[] = {"albedo_gbuf", normal_tex_name, "material_gbuf"};

CONSOLE_INT_VAL("skies", precompute_orders, 3, 1, 5);
CONSOLE_BOOL_VAL("skies", always_precompute, false);
CONSOLE_BOOL_VAL("render", ssao_blur, true);
CONSOLE_BOOL_VAL("render", bnao_blur, true);
CONSOLE_BOOL_VAL("render", bent_cones, true);
CONSOLE_BOOL_VAL("render", dynamic_lights, true);
CONSOLE_INT_VAL("render", waterAnisotropy, 1, 0, 5);
CONSOLE_BOOL_VAL("gui", render_debug, true);


ConVarI sleep_msec_val("sleep_msec", 0, 0, 1000, nullptr);

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
    shadedTarget = dag::create_tex(nullptr, cube_size, cube_size, TEXFMT_A16B16G16R16F | TEXCF_RTARGET, 1, "dynamic_cube_tex_target");
    copy.init("copy_tex");
    // return;
    target = eastl::make_unique<DeferredRenderTarget>("cube_deferred_shading", "cube", cube_size, cube_size,
      DeferredRT::StereoMode::MonoOrMultipass, 0, gbuf_rt, gbuf_fmts, TEXFMT_DEPTH24);
  }
  void update(const ManagedTex *cubeTarget, IRenderDynamicCubeFace2 &cb, const Point3 &pos)
  {
    shvars::world_view_pos.set_color4(pos.x, pos.y, pos.z, 1);
    Driver3dRenderTarget prevRT;
    d3d::get_render_target(prevRT);
    float zn = 0.5, zf = 100;
    shvars::zn_zfar.set_color4(zn, zf, 0, 0);
    static ShaderVariableInfo light_probe_pos("light_probe_pos", true);
    light_probe_pos.set_color4(pos.x, pos.y, pos.z, 1);
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

template <bool isPersp>
inline TMatrix4 oblique_projection_matrix_forward2(const TMatrix4 &forwardProj, const TMatrix &view_itm, const Point4 &worldClipPlane)
{
  TMatrix4 transposedIVTM = TMatrix4(view_itm).transpose();

  // Calculate the clip-space corner point opposite the clipping plane
  // as (sgn(clipPlane.x), sgn(clipPlane.y), 1, 1) and
  // transform it into camera space by multiplying it
  // by the inverse of the projection matrix
  ///*
  Point4 clipPlane = worldClipPlane * transposedIVTM;
  Point4 qB(get_sign(clipPlane.x), get_sign(clipPlane.y), 1, 1);
  Point4 q;
  if (isPersp)
    // optimized for perspective projection, no need to calc inverse
    q = Point4(qB.x / forwardProj(0, 0), qB.y / forwardProj(1, 1), qB.z, (qB.w - forwardProj(2, 2)) / forwardProj(3, 2));
  else
    q = qB * inverse44(forwardProj);

  Point4 c = clipPlane * (1.0f / (clipPlane * q));

  TMatrix4 waterProj = forwardProj;
  waterProj.setcol(2, c); // Point4(c.x, c.y, c.z, -c.w)
  return waterProj;
}

template <bool isPersp>
inline TMatrix4 oblique_projection_matrix_reverse2(const TMatrix4 &proj, const TMatrix &view_itm, const Point4 &worldClipPlane)
{
  TMatrix4 reverseDepthMatrix;
  reverseDepthMatrix.identity();
  reverseDepthMatrix(2, 2) = -1.0f;
  reverseDepthMatrix(3, 2) = 1.0f;
  TMatrix4 forwardProj = proj * reverseDepthMatrix;
  TMatrix4 waterProj = oblique_projection_matrix_forward2<isPersp>(forwardProj, view_itm, worldClipPlane);
  waterProj = waterProj * reverseDepthMatrix;
  return waterProj;
}

class DemoGameScene;


struct Camera
{
  TMatrix viewItm = TMatrix::IDENT;
  TMatrix viewTm = TMatrix::IDENT;
  TMatrix4_vec4 projTm = TMatrix4::IDENT;
  TMatrix4_vec4 globTm = TMatrix4::IDENT;

  Driver3dPerspective persp = {};
  Frustum frustum;

  Point3 worldPos = {};
  ViewVecs viewVecs = {};
};

const Point4 &get_view_vec_LT(const Camera &cam) { return cam.viewVecs.viewVecLT; }
const Point4 &get_view_vec_RT(const Camera &cam) { return cam.viewVecs.viewVecRT; }
const Point4 &get_view_vec_LB(const Camera &cam) { return cam.viewVecs.viewVecLB; }
const Point4 &get_view_vec_RB(const Camera &cam) { return cam.viewVecs.viewVecRB; }

auto bind_camera(dabfg::Registry registry, const char *name)
{
  return registry.read(name)
    .blob<Camera>()
    .bindAsView<&Camera::viewTm>()
    .bindAsProj<&Camera::projTm>()
    .bindToShaderVar<&Camera::worldPos>("world_view_pos")
    .bindToShaderVar<get_view_vec_LT>("view_vecLT")
    .bindToShaderVar<get_view_vec_RT>("view_vecRT")
    .bindToShaderVar<get_view_vec_LB>("view_vecLB")
    .bindToShaderVar<get_view_vec_RB>("view_vecRB");
}

class DemoGameScene : public DagorGameScene, public IRenderDynamicCubeFace2, public ICascadeShadowsClient
{
public:
  eastl::unique_ptr<DebugTexOverlay> debugTexOverlay;
  eastl::unique_ptr<CascadeShadows> csm;

  enum
  {
    CLOUD_VOLUME_W = 32,
    CLOUD_VOLUME_H = 16,
    CLOUD_VOLUME_D = 32
  };

  virtual Point4 getCascadeShadowAnchor(int cascade_no)
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    return Point4::xyz0(-itm.getcol(3));
  }
  virtual void renderCascadeShadowDepth(int cascade_no, const Point2 &)
  {
    const auto &globtm = csm->getWorldRenderMatrix(cascade_no);
    d3d::settm(TM_VIEW, TMatrix::IDENT);
    d3d::settm(TM_PROJ, &globtm);
    const auto &camPos = csm->getRenderCameraWorldViewPos(cascade_no);
    renderOpaque(Frustum(globtm), camPos);
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
    if (sun_panel.astronomical)
    {
      float gmtTime = sun_panel.time;
      if (sun_panel.local_time)
        gmtTime -= sun_panel.longitude * (12. / 180);
      double jd = julian_day(sun_panel.year, sun_panel.month, sun_panel.day, gmtTime);

      daSkies.setStarsJulianDay(jd);
      daSkies.setStarsLatLon(sun_panel.latitude, sun_panel.longitude);
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
  }
  int width = 0, height = 0;

  TEXTUREID waterFoamTexId, perlinTexId;

  void createMainPovData()
  {
    main_pov_data.reset();

    // required for each point of view (or the only one if panorama, for panorama center)
    PreparedSkiesParams params{render_quality_panel.skies_lut_quality, render_quality_panel.scattering_screen_quality,
      render_quality_panel.scattering_depth_slices, render_quality_panel.colored_transmittance_quality,
      render_quality_panel.scattering_range_scale, render_quality_panel.scattering_min_range, PreparedSkiesParams::Panoramic::OFF,
      PreparedSkiesParams::Reprojection::ON};

    main_pov_data =
      eastl::unique_ptr<SkiesData, SkiesDataDeleter>(daSkies.createSkiesData("main", params), SkiesDataDeleter{&daSkies});
  }

  DemoGameScene()
  {
    d3d::get_render_target_size(width, height, nullptr);

    setDirToSun();
    cpujobs::start_job_manager(1, 128 << 10);
    unloaded_splash = false;
    Point3 min_r, max_r;
    init_and_get_perlin_noise_3d(min_r, max_r);

    dabfg::startup();
    dabfg::set_resolution("main_view", IPoint2{width, height});
    dabfg::set_resolution("display", IPoint2{width, height});

    const int reflW = width / 6, reflH = height / 6;

    daSkies.initSky();
    PreparedSkiesParams refl_params;
    // required for each point of view (or the only one if panorama, for panorama center)
    refl_pov_data =
      eastl::unique_ptr<SkiesData, SkiesDataDeleter>(daSkies.createSkiesData("refl", refl_params), SkiesDataDeleter{&daSkies});
    daSkies.changeSkiesData(1, 1, false, reflW, reflH, refl_pov_data.get());

    int halfW = width / 2, halfH = height / 2;
    static ShaderVariableInfo lowres_rt_params("lowres_rt_params", true);
    lowres_rt_params.set_color4(halfW, halfH, 0, 0);

    static ShaderVariableInfo rendering_res("rendering_res", true);
    rendering_res.set_color4(width, height, 1.0f / width, 1.0f / height);
    aurora.reset(new AuroraBorealis());

    CascadeShadows::Settings csmSettings;
    csmSettings.cascadeWidth = 512;
    csmSettings.splitsW = csmSettings.splitsH = 2;
    csmSettings.resourceAccessStrategy = CascadeShadows::Settings::ResourceAccessStrategy::External;
    csm.reset(CascadeShadows::make(this, csmSettings));

    downsample_depth::init("downsample_depth2x");

    if (global_cls_drv_pnt)
      global_cls_drv_pnt->getDevice(0)->setClipRect(0, 0, width, height);

    createMainPovData();

    PreparedSkiesParams cubeParams;
    cubeParams.panoramic = PreparedSkiesParams::Panoramic::ON;
    cubeParams.reprojection = PreparedSkiesParams::Reprojection::OFF;
    // required for each point of view (or the only one if panorama, for panorama center)
    cube_pov_data =
      eastl::unique_ptr<SkiesData, SkiesDataDeleter>(daSkies.createSkiesData("cube", cubeParams), SkiesDataDeleter{&daSkies});

    fft_water::init();

    {
      const char *foamTexName = "water_surface_foam_tex";
      waterFoamTexId = ::get_tex_gameres(foamTexName); // mem leak
      G_ASSERTF(waterFoamTexId != BAD_TEXTUREID, "water foam texture '%s' not found.", foamTexName);
      static ShaderVariableInfo foam_tex("foam_tex");
      static ShaderVariableInfo foam_tex_samplerstate("foam_tex_samplerstate", true);
      foam_tex.set_texture(waterFoamTexId);
      foam_tex_samplerstate.set_sampler(get_texture_separate_sampler(waterFoamTexId));
    }

    {
      const char *perlinName = "water_perlin";
      perlinTexId = ::get_tex_gameres(perlinName); // mem leak
      G_ASSERTF(perlinTexId != BAD_TEXTUREID, "water perlin noise texture '%s' not found.", perlinName);
      static ShaderVariableInfo perlin_noise("perlin_noise");
      static ShaderVariableInfo perlin_noise_samplerstate("perlin_noise_samplerstate", true);
      perlin_noise.set_texture(perlinTexId);
      perlin_noise_samplerstate.set_sampler(get_texture_separate_sampler(perlinTexId));
    }

    {
      fftWater.reset(fft_water::create_water(fft_water::RENDER_GOOD, water_panel.fft_period));
      fft_water::setWaterCell(fftWater.get(), 1.0f, true);
      fft_water::apply_wave_preset(fftWater.get(), 3.0f, Point2(1.0f, 0.0f), fft_water::Spectrum::UNIFIED_DIRECTIONAL);
      reloadUiWaves();
    }

    heightmapRenderer.init("heightmap", "", "hmap_tess_factor", true, 4);

    frameGraph = makeFrameGraph();
  }

  struct WaterDeleter
  {
    void operator()(FFTWater *water) { fft_water::delete_water(water); }
  };
  eastl::unique_ptr<FFTWater, WaterDeleter> fftWater;

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
  eastl::unique_ptr<SkiesData, SkiesDataDeleter> main_pov_data;
  eastl::unique_ptr<SkiesData, SkiesDataDeleter> cube_pov_data;
  eastl::unique_ptr<SkiesData, SkiesDataDeleter> refl_pov_data;

  ~DemoGameScene()
  {
    ShaderGlobal::reset_from_vars(perlinTexId);
    release_managed_tex(perlinTexId);
    ShaderGlobal::reset_from_vars(waterFoamTexId);
    release_managed_tex(waterFoamTexId);

    downsample_depth::close();

    release_perline_noise_3d();

    heightmap.close();
    heightmapRenderer.close();

    cpujobs::stop_job_manager(1, true);
    fft_water::close();
    de3_imgui_term();
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
        aurora_borealis.write(*blk.addBlock("aurora_borealis"));
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
          if (!file_panel.import_seed)
            file_panel.import_seed = grnd();
          skies_utils::load_daSkies(daSkies, blk, file_panel.import_seed, *blk.getBlockByNameEx("level"));
          Point2 clouds = point2(daSkies.getCloudsOrigin());
          Point2 strataClouds = point2(daSkies.getStrataCloudsOrigin());
          clouds.x = blk.getReal("cloudsOffsetX", clouds.x);
          clouds.y = blk.getReal("cloudsOffsetZ", clouds.y);
          strataClouds.x = blk.getReal("strataCloudsOffsetX", strataClouds.x);
          strataClouds.y = blk.getReal("strataCloudsOffsetZ", strataClouds.y);
          daSkies.setCloudsOrigin(clouds.x, clouds.y);
          daSkies.setStrataCloudsOrigin(strataClouds.x, strataClouds.y);
          if (blk.findParam("cloudsPos") >= 0)
            daSkies.setCloudsHolePosition(blk.getPoint2("cloudsPos", Point2(0, 0)));

          ((SkyAtmosphereParams &)sky_panel) = daSkies.getSkyParams();
          sky_colors.mie_scattering_color = sky_panel.mie_scattering_color;
          sky_colors.moon_color = sky_panel.moon_color;
          sky_colors.mie_absorption_color = sky_panel.mie_absorption_color;
          sky_colors.rayleigh_color = sky_panel.rayleigh_color;
          sky_colors.solar_irradiance_scale = sky_panel.solar_irradiance_scale;
          layered_fog.mie2_thickness = sky_panel.mie2_thickness;
          layered_fog.mie2_altitude = sky_panel.mie2_altitude;
          layered_fog.mie2_scale = sky_panel.mie2_scale;

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
    if (file_panel.export_water)
    {
      String fileName = de3_imgui_file_save_dlg("Export water settings", "Blk files|*.blk", "blk", "./", lastExportName);
      if (fileName.empty())
      {
        // show error
      }
      else
      {
        DataBlock blk;
        fft_water::WavePreset wavePreset;
        fft_water::get_wave_preset(fftWater.get(), wavePreset);
        fft_water::save_wave_preset(&blk, wavePreset);
        if (!blk.saveToTextFile(fileName))
        {
          G_ASSERTF(0, "Can't save file");
        }
        lastExportName = fileName;
      }
      file_panel.export_water = false;
    }
    if (file_panel.import_water)
    {
      String fileName = de3_imgui_file_open_dlg("Import water settings", "Blk files|*.blk", "blk", "./", lastExportName);
      DataBlock blk;
      if (!fileName.empty())
      {
        if (!blk.load(fileName))
        {
          G_ASSERTF(0, "Can't load file");
        }
        else
        {
          fft_water::WavePreset preset;
          fft_water::load_wave_preset(&blk, preset);
          float windDirX = cosf(DegToRad(render_panel.windDir)), windDirZ = sinf(DegToRad(render_panel.windDir));
          fft_water::apply_wave_preset(fftWater.get(), preset, Point2(windDirX, windDirZ));

          reloadUiWaves();
          lastExportName = fileName;
        }
      }
      else
      {
        // show error
      }
      file_panel.import_water = false;
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
        de3_imgui_load_values(*blk.getBlockByNameEx("gui"));
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
    if (exit_button_pressed)
    {
      quit_game(1);
      return;
    }

    if (sleep_msec_val.get())
      sleep_msec(sleep_msec_val.get());
  }

  virtual void drawScene()
  {
    TIME_D3D_PROFILE(drawScene)

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

    extern bool g_ssao_blur;
    g_ssao_blur = ssao_blur.get();

    dabfg::update_external_state(dabfg::ExternalState{::grs_draw_wire, false});
    dabfg::run_nodes();

    static bool firstCubeUpdate = false;
    if (!firstCubeUpdate)
      reloadCube(false);
    firstCubeUpdate = true;

    // framegraph resets bound targets to nullptr upon exiting of LegacyBackbufferPass
    // force set backbuffer to render imgui/ui
    d3d::set_render_target();

    de3_imgui_render();
  }

  void mainRenderTrans() { renderTrans(); }

  void renderGui()
  {
    StdGuiRender::reset_per_frame_dynamic_buffer_pos();
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);

    // if (::dagor_gui_manager)
    {
      TIME_D3D_PROFILE(gui);
      bool do_start = !StdGuiRender::is_render_started();
      if (do_start)
        StdGuiRender::start_render();
      if (render_debug.get())
      {
        StdGuiRender::set_font(0);
        StdGuiRender::set_color(255, 0, 255, 255);
        StdGuiRender::set_ablend(true);

        int base_line = StdGuiRender::get_font_ascent() + 5, font_ht = StdGuiRender::get_def_font_ht(0);
        StdGuiRender::goto_xy(400, base_line + font_ht * 0);
        // StdGuiRender::draw_str(String(128,"shadow=%g", shadow));
        // float densityUp = daSkies.getDensityUp(itm.getcol(3));
        // float maxDensity = daSkies.getMaxDensityUp(Point2::xz(itm.getcol(3)));
        // float density = daSkies.getCloudsDensityFilteredPrecise(itm.getcol(3));
        // float shadow = daSkies.getCloudsShadow(itm.getcol(3), daSkies.getSunDir());
        // StdGuiRender::draw_str(String(128,"cloud density here:%.2f above: %.2f, max above:%.2f (average=%g%s), shadow=%g",
        //   density,densityUp, maxDensity,
        //   daSkies.getCloudiness(), daSkies.hasClouds() ? (daSkies.isThunder() ? " thunder" : "") : " clear", shadow));
        if (render_panel.compareCpuSunSky)
        {
          static Color3 sun[2], sky[2], moonsun[2], moonsky[2];
          sky[0] = daSkies.getCpuSunSky(itm.getcol(3), daSkies.getSunDir());
          sun[0] = daSkies.getCpuSun(itm.getcol(3), daSkies.getSunDir());
          moonsky[0] = daSkies.getCpuMoonSky(itm.getcol(3), daSkies.getMoonDir());
          moonsun[0] = daSkies.getCpuMoon(itm.getcol(3), daSkies.getMoonDir());
          float s, m;
          daSkies.currentGroundSunSkyColor(s, m, sun[1], sky[1], moonsun[1], moonsky[1]);
          StdGuiRender::goto_xy(700, base_line + font_ht * 2);
          StdGuiRender::draw_str(String(128, "camera sun/sky %@|%@ ground sun/sky %@|%@", sun[0], sky[0], sun[1], sky[1]));
        }

        if (render_panel.findHole)
          daSkies.resetCloudsHole(itm.getcol(3));

        daSkies.setUseCloudsHole(render_panel.useCloudsHole);

        StdGuiRender::goto_xy(400, base_line + font_ht * 1);
        Color3 mieC = daSkies.getMieColor(itm.getcol(3), daSkies.getSunDir(), 100000);
        float toneMapScale = 1 / (brightness(mieC) + 1);
        Color3 mieCTonemap = mieC * toneMapScale;
        StdGuiRender::draw_str(String(128, "mie sun inscatter: %@->%@", mieC, mieCTonemap));

        for (int cascadeNo = 0; cascadeNo < 4; ++cascadeNo)
        {
          float period = 0.0f, windowIn = 0.0f, windowOut = 0.0f;
          fft_water::get_cascade_period(fftWater.get(), cascadeNo, period, windowIn, windowOut);
          StdGuiRender::goto_xy(400, base_line + font_ht * (3 + cascadeNo));
          StdGuiRender::draw_str(
            String(128, "fft[%d]: period=%0.1f windowIn=%0.1f windowOut=%0.1f", cascadeNo, period, windowIn, windowOut));
        }

        StdGuiRender::flush_data();
      }

      if (do_start)
        StdGuiRender::end_render();
    }

    if (console::get_visual_driver())
    {
      console::get_visual_driver()->update();
      console::get_visual_driver()->render();
    }
  }

  virtual void beforeDrawScene(int realtime_elapsed_usec, float gametime_elapsed_sec)
  {
    TIME_D3D_PROFILE(beforeDraw);
    static ShaderVariableInfo enable_god_rays_from_land("enable_god_rays_from_land", true);
    enable_god_rays_from_land.set_int(render_panel.enable_god_rays_from_land);

    static float currentTime = 0;
    eastl::vector<Point4> balls;
    balls.emplace_back(5, 5, 5, 1);
    balls.emplace_back(2, 5, 3, 1);
    balls.emplace_back(8.5 - sin(2 * currentTime), 5, 5, 1);
    balls.emplace_back(8.5, 8.5 + 3 * sin(currentTime + 1), 5, 1);
    balls.emplace_back(8.5, 5, 8.5, 1);
    balls.emplace_back(8.5, 8.5, 8.5, 1);
    balls.emplace_back(5, 8.5, 8.5, 1);
    balls.emplace_back(5, 7, 7.5, 1);
    currentTime += gametime_elapsed_sec;
    balls[1].x = 5 + sin(currentTime);
    balls[1].z = 3 + cos(currentTime);
    balls.back().y = 7 + 3 * sin(currentTime);

    setDirToSun();
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
    sky_panel.solar_irradiance_scale = sky_colors.solar_irradiance_scale;
    sky_panel.rayleigh_color = sky_colors.rayleigh_color;
    sky_panel.mie2_thickness = layered_fog.mie2_thickness;
    sky_panel.mie2_altitude = layered_fog.mie2_altitude;
    sky_panel.mie2_scale = layered_fog.mie2_scale;
    daSkies.setSkyParams(sky_panel);
    daSkies.setCloudsGameSettingsParams(clouds_game_params);
    daSkies.setWeatherGen(clouds_weather_gen2);
    daSkies.setCloudsForm(clouds_form);
    daSkies.setCloudsRendering(clouds_rendering2);
    aurora->setParams(aurora_borealis, width, height);
    sky_panel.min_ground_offset = heightmapMin;
    if (water_panel.enabled)
      sky_panel.min_ground_offset = max(sky_panel.min_ground_offset, water_panel.water_level);
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
    daSkies.projectUses2DShadows(render_panel.shadows_2d);

    static ShaderVariableInfo skies_use_2d_shadows("skies_use_2d_shadows", true);
    skies_use_2d_shadows.set_int(render_panel.shadows_2d);

    daSkies.prepare(dir_to_sun, false, gametime_elapsed_sec);
    daSkies.setPanoramaReprojectionWeight(render_panel.panoramaReprojection);
    dir_to_sun = daSkies.getPrimarySunDir();
    shvars::from_sun_direction.set_color4(-dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      sun = lerp(moon, sun, daSkies.getCurrentSunEffect());
      amb = lerp(moonamb, amb, daSkies.getCurrentSunEffect());
      static ShaderVariableInfo sun_light_color("sun_light_color");
      sun_light_color.set_color4(color4(sun / PI, 0));
      static ShaderVariableInfo amb_light_color("amb_light_color", true);
      amb_light_color.set_color4(color4(amb, 0));
      if (fftWater)
      {
        static ShaderVariableInfo sun_color_0("sun_color_0");
        sun_color_0.set_color4(color4(sun / PI, 0));
        static ShaderVariableInfo sky_color("sky_color");
        sky_color.set_color4(color4(amb, 0));
        shvars::from_sun_direction.set_color4(-dir_to_sun.x, -dir_to_sun.y, -dir_to_sun.z, 0);
      }
    }
    else
    {
      G_ASSERT(0);
    }


    if (auto water = fftWater.get(); water && water_panel.enabled)
    {
      fft_water::set_wind_speed(water, water_panel.water_strength, Point2(windDirX, windDirZ));
      fft_water::set_roughness(water, water_panel.roughness, 0.0f);
      fft_water::setAnisotropy(water, waterAnisotropy.get());
      fft_water::set_period(water, water_panel.fft_period);
      fft_water::set_small_wave_fraction(water, water_panel.small_waves);
      fft_water::setWaterCell(water, water_panel.cell_size, true);
      fft_water::set_water_dim(water, water_panel.dim_bits);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_SHORE, water_panel.shore);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_WAKE, water_panel.wake);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_SHADOWS, water_panel.shadows);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_DEEPNESS, water_panel.deepness);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_FOAM, water_panel.foam);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_ENV_REFL, water_panel.env_refl);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_PLANAR_REFL, water_panel.planar_refl);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_SUN_REFL, water_panel.sun_refl);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_SEABED_REFR, water_panel.seabed_refr);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_SSS_REFR, water_panel.sss_refr);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_UNDERWATER_REFR, water_panel.underwater_refr);
      fft_water::enable_graphic_feature(water, fft_water::GRAPHIC_PROJ_EFF, water_panel.proj_eff);

      fft_water::SimulationParams simualation = fft_water::get_simulation_params(water);
#define SET_WATER_CASCADE(no) \
      //{ \
      //  cascadeScales.cascade[no].amplitude = water_panel.amplitude##no; \
      //  cascadeScales.cascade[no].choppy = water_panel.choppy_scale##no; \
      //  cascadeScales.cascade[no].speed = water_panel.speed##no; \
      //  cascadeScales.cascade[no].period = water_panel.period_k##no; \
      //  cascadeScales.cascade[no].windowIn = water_panel.window_in_k##no; \
      //  cascadeScales.cascade[no].windowOut = water_panel.window_out_k##no; \
      //  cascadeScales.cascade[no].windDependency = water_panel.wind_dep##no; \
      //}
      SET_WATER_CASCADE(0);
      SET_WATER_CASCADE(1);
      SET_WATER_CASCADE(2);
      SET_WATER_CASCADE(3);

      fft_water::set_simulation_params(water, simualation);

      fft_water::FoamParams foam;
      foam.dissipation_speed = water_foam_panel.foam_dissipation_speed;
      foam.generation_amount = water_foam_panel.foam_generation_amount;
      foam.generation_threshold = water_foam_panel.foam_generation_threshold;
      foam.falloff_speed = water_foam_panel.foam_falloff_speed;
      foam.hats_mul = water_foam_panel.foam_hats_mul;
      foam.hats_threshold = water_foam_panel.foam_hats_threshold;
      foam.hats_folding = water_foam_panel.foam_hats_folding;

      fft_water::set_foam(water, foam);
    }

    Driver3dPerspective p;
    G_VERIFY(d3d::getpersp(p));
    shvars::zn_zfar.set_color4(p.zn, p.zf, 0, 0);

    TMatrix4 globtm;
    d3d::getglobtm(globtm);
    /*TMatrix4 globtmTr = globtm.transpose();
    ShaderGlobal::set_color4(globtm_psf_0VarId,Color4(globtmTr[0]));
    ShaderGlobal::set_color4(globtm_psf_1VarId,Color4(globtmTr[1]));
    ShaderGlobal::set_color4(globtm_psf_2VarId,Color4(globtmTr[2]));
    ShaderGlobal::set_color4(globtm_psf_3VarId,Color4(globtmTr[3]));*/
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
    DA_PROFILE_TAG_LINE(camera_pos, 0xFF00FF, "%g %g %g", P3D(itm.getcol(3)));

    if (test)
      test->setLod(test->chooseLod(itm.getcol(3)));
    shvars::world_view_pos.set_color4(Color4(itm.getcol(3).x, itm.getcol(3).y, itm.getcol(3).z, 1));
    CascadeShadows::ModeSettings mode;
    mode.powWeight = 0.8;
    mode.maxDist = 100;
    mode.shadowStart = p.zn;
    mode.numCascades = 4;
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    csm->prepareShadowCascades(mode, dir_to_sun, inverse(itm), itm.getcol(3), projTm, Frustum(globtm), Point2(p.zn, p.zf), p.zn);

    de3_imgui_before_render();

    static double water_time = 0;
    water_time += gametime_elapsed_sec;
    if (fftWater && water_panel.enabled)
    {
      TIME_D3D_PROFILE(water_simulate);
      // water_time = 0;
      fft_water::simulate(fftWater.get(), water_time);
      fft_water::before_render(fftWater.get());
    }

    static ShaderVariableInfo foam_time("foam_time");
    foam_time.set_real(water_time);
  }

  virtual void sceneSelected(DagorGameScene * /*prev_scene*/) { loadScene(); }

  virtual void sceneDeselected(DagorGameScene * /*new_scene*/)
  {
    webui::shutdown();
    console::set_visual_driver(nullptr, nullptr);
    delete this;
    dabfg::shutdown();
  }

  // IRenderWorld interface
  void drawPlane()
  {
    if (!land_panel.plane)
      return;
    TIME_D3D_PROFILE(plane);
    shvars::plane.set_int(1);
    sphereElem->setStates(0, true);
    d3d::draw_instanced(PRIM_TRILIST, 0, 6, 1);
    // Point3 vert[4] = {Point3(-1, -2, -1), Point3(-1, -2, +1), Point3(+1, -2, -1), Point3(+1, -2, +1)};
    // d3d::draw_up(PRIM_TRISTRIP, 2, vert, sizeof(Point3));
  }
  void drawSpheres()
  {
    if (!land_panel.spheres)
      return;
    TIME_D3D_PROFILE(spheres);
    shvars::plane.set_int(0);
    sphereElem->setStates(0, true);
    d3d::setvsrc_ex(0, nullptr, 0, 0);
    d3d::draw_instanced(PRIM_TRILIST, 0, SPHERES_INDICES_TO_DRAW, NUM_ROUGHNESS * NUM_METALLIC);
  }
  HeightmapRenderer heightmapRenderer;
  UniqueTexHolder heightmap;
  d3d::SamplerHandle heightmapSampler;

  float heightmapCellSize = 1.f, heightmapScale = 1.f, heightmapMin = 0.f, hmapMaxHt = 1.f;

  void drawHeightmap(const Frustum &frustum, const Point3 &cur_camera_pos)
  {
    if (!land_panel.enabled)
      return;
    heightmapScale = land_panel.heightScale / hmapMaxHt;
    heightmapCellSize = land_panel.cellSize;
    heightmapMin = land_panel.heightMin * land_panel.heightScale;
    if (!heightmap)
      return;

    int lodCount = 8;
    const int lodRad = 1, lastLodRad = 3;
    int lod = 0;
    static ShaderVariableInfo heightmap_scale("heightmap_scale");
    heightmap_scale.set_color4(heightmapScale, heightmapMin, 0, 0);
    static ShaderVariableInfo tex_hmap_inv_sizes("tex_hmap_inv_sizes");
    Color4 inv_width = tex_hmap_inv_sizes.get_color4();
    Point2 origin(land_panel.hmapCenterOfsX + 0.5f, land_panel.hmapCenterOfsZ + 0.5f);
    Point2 invWorldSize = Point2(inv_width.r / heightmapCellSize, inv_width.g / heightmapCellSize);
    static ShaderVariableInfo world_to_hmap_low("world_to_hmap_low");
    world_to_hmap_low.set_color4(invWorldSize.x, invWorldSize.y, origin.x, origin.y);

    LodGrid lodGrid;
    lodGrid.init(lodCount - lod, lodRad, 0, lastLodRad);
    LodGridCullData defaultCullData;
    float scaledCell = heightmapCellSize * (1 << min(11, lod));
    float minHt = heightmapMin;
    float maxHt = heightmapMin + hmapMaxHt * heightmapScale;
    float lod0Size = 0;
    cull_lod_grid(lodGrid, lodGrid.lodsCount, floorf(cur_camera_pos.x + 0.5f), floorf(cur_camera_pos.z + 0.5f), scaledCell, scaledCell,
      -1, -1, minHt, maxHt, &frustum, nullptr, defaultCullData, nullptr, lod0Size, heightmapRenderer.get_hmap_tess_factorVarId(),
      heightmapRenderer.getDim(), true);
    if (!defaultCullData.getCount())
      return;
    heightmap.setVar();
    static ShaderVariableInfo hmap_ldetail("hmap_ldetail", true);
    static ShaderVariableInfo hmap_ldetail_samplerstate("hmap_ldetail_samplerstate", true);
    static ShaderVariableInfo heightmap_samplerstate("heightmap_samplerstate", true);
    hmap_ldetail.set_texture(heightmap.getTexId());
    hmap_ldetail_samplerstate.set_sampler(heightmapSampler);
    heightmap_samplerstate.set_sampler(heightmapSampler);
    heightmapRenderer.render(lodGrid, defaultCullData);
  }

  void renderTrees()
  {
    if (!land_panel.trees)
      return;
    TIME_D3D_PROFILE(testScene);
    if (test)
      test->render();
  }
  void renderOpaque(const Frustum &frustum, const Point3 &camera_pos)
  {
    TIME_D3D_PROFILE(opaque);
    drawPlane();
    drawSpheres();
    renderTrees();
    drawHeightmap(frustum, camera_pos);
  }
  void renderLightProbeOpaque()
  {
    // daSkies.prepare(dir_to_sun);
    TMatrix view, itm;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    itm = orthonormalized_inverse(view);
    daSkies.useFog(itm.getcol(3), cube_pov_data.get(), view, projTm);
    drawPlane();
  }

  void renderLightProbeEnvi()
  {
    TIME_D3D_PROFILE(cubeenvi)
    TMatrix view, itm;
    d3d::gettm(TM_VIEW, view);
    TMatrix4 projTm;
    d3d::gettm(TM_PROJ, &projTm);
    Driver3dPerspective persp;
    d3d::getpersp(persp);
    itm = orthonormalized_inverse(view);
    daSkies.renderEnvi(render_panel.infinite_skies, dpoint3(itm.getcol(3)), dpoint3(itm.getcol(2)), 2, UniqueTex{}, UniqueTex{},
      BAD_TEXTUREID, cube_pov_data.get(), view, projTm, persp);
  }
  static inline void calc_water_reflection_matrix(TMatrix &tm, TMatrix &itm, const TMatrix &ivtm, Point3 &pos, float water_level)
  {
    TMatrix reflection;
    reflection.setcol(0, 1.f, 0.f, 0.f);
    reflection.setcol(1, 0.f, -1.f, 0.f);
    reflection.setcol(2, 0.f, 0.f, 1.f);
    reflection.setcol(3, 0.f, 2.f * water_level, 0.f);

    itm = reflection * ivtm;
    itm.setcol(1, -itm.getcol(1)); // Flip Y to preserve culling.
    tm = inverse(itm);
    pos = itm.getcol(3);
  }

  int calc_resolved_frame_texfmt()
  {
    uint32_t rtFmt = TEXFMT_R11G11B10F;
    if (!(d3d::get_texformat_usage(rtFmt) & d3d::USAGE_RTARGET))
      rtFmt = TEXFMT_A16B16G16R16F;
    return rtFmt;
  }


  struct FrameGraph
  {
    // A backbone is the base set of nodes that represents the core rendering
    // architecture of an application. Everything else that the app wants to do
    // is optional and can be added on top of this backbone.
    // The less this backbone is responsible for, the better.
    // Note that right now, a lot of stuff is hard-wired into the backbone, but
    // it should be decoupled as much as possible.
    struct Backbone
    {
      dabfg::NodeHandle makeCamera;

      struct Opaque
      {
        dabfg::NodeHandle makeGbuf;
        dabfg::NodeHandle deferredShading;
      } opaque;

      struct Lighting
      {
        dabfg::NodeHandle ssao;
        dabfg::NodeHandle ssr;
      } lighting;

      struct Shadows
      {
        dabfg::NodeHandle csm;
      } shadows;

      dabfg::NodeHandle downsampleDepth;

      dabfg::NodeHandle startTransparent;

      dabfg::NodeHandle copyFrame;

      dabfg::NodeHandle renderPostFx;
    } backbone;

    // Miscelaneous stuff that extends the backbone. Preferrably, such
    // nodes should be stored inside the storage of the subsystem that
    // they are related to.
    dag::Vector<dabfg::NodeHandle> opaqueRendering;
    dag::Vector<dabfg::NodeHandle> transparentRendering;

    // These nodes are needed to always display at least something on the
    // screen, specifically the debug GUI, and so they are not part of the
    // backbone. We can also display a "broken FG" message through these in
    // case the programmer messes up the backbone.
    struct FinalPresent
    {
      dabfg::NodeHandle createBackbuffer;
      dabfg::NodeHandle debugTexOverlay;
      dabfg::NodeHandle renderGui;
    } finalPresent;

    struct Envi
    {
      dabfg::NodeHandle renderCloudVolumeForParticles;
      dabfg::NodeHandle updateSkiesData;
      dabfg::NodeHandle prepareSkies;
      dabfg::NodeHandle renderSky;
      dabfg::NodeHandle renderClouds;
    } envi;

    struct Water
    {
      struct Reflections
      {
        dabfg::NodeHandle makeReflCamera;
        dabfg::NodeHandle makeReflGbuf;
        dag::Vector<dabfg::NodeHandle> renderOpaque;
        dabfg::NodeHandle waterReflResolve;
        dabfg::NodeHandle waterReflPrepareEnvi;
        dabfg::NodeHandle waterReflEnvi;
        dabfg::NodeHandle clipWaterRefl;
      } reflections;

      dabfg::NodeHandle render;
    } water;
  } frameGraph;

  [[nodiscard]] FrameGraph::Backbone::Opaque makeOpaqueBackbone()
  {
    FrameGraph::Backbone::Opaque result;

    result.makeGbuf = dabfg::register_node("make_gbuf", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.createTexture2d(depth_tex_name, dabfg::History::No,
        dabfg::Texture2dCreateInfo{TEXFMT_DEPTH32 | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

      for (int i = 0; i < gbuf_rt; ++i)
        registry.createTexture2d(gbuf_tex_names[i], dabfg::History::No,
          dabfg::Texture2dCreateInfo{gbuf_fmts[i] | TEXCF_RTARGET, registry.getResolution<2>("main_view")});

      registry.requestRenderPass()
        .color({gbuf_tex_names[0], gbuf_tex_names[1], gbuf_tex_names[2]})
        .depthRw(depth_tex_name)
        .clear(gbuf_tex_names[0], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear(gbuf_tex_names[1], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear(gbuf_tex_names[2], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear(depth_tex_name, make_clear_value(0.f, 0));
    });

    result.deferredShading = dabfg::register_node("deferred_shading", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      auto depthHandle = registry.readTexture(depth_tex_name).atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("depth_gbuf").handle();

      for (int i = 0; i < gbuf_rt; ++i)
        registry.readTexture(gbuf_tex_names[i]).atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();

      auto mainViewRes = registry.getResolution<2>("main_view");

      const auto rtFmt = calc_resolved_frame_texfmt();
      auto resolvedHandle =
        registry.createTexture2d("resolved_frame", dabfg::History::No, dabfg::Texture2dCreateInfo{rtFmt | TEXCF_RTARGET, mainViewRes})
          .atStage(dabfg::Stage::POST_RASTER)
          .useAs(dabfg::Usage::COLOR_ATTACHMENT)
          .handle();

      registry.read("ssr_target").texture().atStage(dabfg::Stage::PS).bindToShaderVar();
      registry.read("ssr_target_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssr_target_samplerstate");
      registry.read("ssao_tex").texture().atStage(dabfg::Stage::PS).bindToShaderVar();
      registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate");
      registry.read("csm").texture().atStage(dabfg::Stage::PS).bindToShaderVar("shadow_cascade_depth_tex");
      registry.read("csm_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("shadow_cascade_depth_tex_samplerstate");

      auto resShading = eastl::make_unique<ShadingResolver>("deferred_shading");
      return [this, depthHandle, resolvedHandle, mainViewRes, camHndl, resolveShading = eastl::move(resShading)]() {
        auto res = mainViewRes.get();
        shvars::screen_pos_to_texcoord.set_color4(1.f / res.x, 1.f / res.y, 0, 0);
        shvars::screen_size.set_color4(res.x, res.y, 1.0 / res.x, 1.0 / res.y);

        const auto &cam = camHndl.ref();
        set_inv_globtm_to_shader(cam.viewTm, cam.projTm, true);

        csm->setCascadesToShader();

        resolveShading->resolve(resolvedHandle.get(), cam.viewTm, cam.projTm);
      };
    });

    return result;
  }

  [[nodiscard]] FrameGraph::Envi makeEnviNodes()
  {
    FrameGraph::Envi result;

    result.renderCloudVolumeForParticles =
      dabfg::register_node("render_clouds_volume_for_particles", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
        auto camHndl = bind_camera(registry, "main_camera").handle();

        // This texture should be used for faking order independent transparent
        // rendering of particles inside clouds (i.e. explosions). This sample
        // currently does not in fact use it for anything, but we leave it
        // here to get at least a semblence of correct resource-based ordering
        // of FG nodes.
        auto cloudVolumeHndl = registry.create("cloud_volume", dabfg::History::No)
                                 .texture(dabfg::Texture3dCreateInfo{TEXFMT_L8 | TEXCF_RTARGET | TEXCF_UNORDERED,
                                   IPoint3{CLOUD_VOLUME_W, CLOUD_VOLUME_H, CLOUD_VOLUME_D}, 1})
                                 .atStage(dabfg::Stage::PS_OR_CS)
                                 .useAs(dabfg::Usage::SHADER_RESOURCE)
                                 .handle();

        return [this, camHndl, cloudVolumeHndl]() {
          const auto &cam = camHndl.ref();
          TIME_D3D_PROFILE(cloudsVolumeForParticles)
          daSkies.renderCloudVolume(cloudVolumeHndl.get(), 30000, cam.viewTm, cam.projTm);
        };
      });

    result.updateSkiesData = dabfg::register_node("update_skies_data", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      bind_camera(registry, "main_camera");

      auto mainSkiesDataHndl = registry.create("main_cam_skies_data", dabfg::History::No).blob<SkiesData *>().handle();

      return [this, mainSkiesDataHndl]() {
        if (render_quality_panel != old_quality_panel)
        {
          old_quality_panel = render_quality_panel;
          createMainPovData();
        }
        daSkies.changeSkiesData(render_quality_panel.sky_res_divisor, render_quality_panel.clouds_res_divisor,
          !render_panel.infinite_skies, width, height, main_pov_data.get());

        mainSkiesDataHndl.ref() = main_pov_data.get();
      };
    });

    result.prepareSkies = dabfg::register_node("prepare_skies", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      auto mainSkiesDataHndl = registry.modify("main_cam_skies_data").blob<SkiesData *>().handle();

      auto fddHandle = registry.readTexture("far_downsampled_depth")
                         .atStage(dabfg::Stage::PS_OR_CS)
                         .bindToShaderVar("downsampled_far_depth_tex")
                         .handle();
      auto prevFddHandle = registry.readTextureHistory("far_downsampled_depth")
                             .atStage(dabfg::Stage::PS_OR_CS)
                             .useAs(dabfg::Usage::SHADER_RESOURCE)
                             .handle();

      return [this, camHndl, mainSkiesDataHndl, fddHandle, prevFddHandle]() {
        const auto &cam = camHndl.ref();
        daSkies.prepareSkyAndClouds(render_panel.infinite_skies, dpoint3(cam.worldPos), dpoint3(cam.viewItm.getcol(2)), 3,
          fddHandle.view(), prevFddHandle.view(), mainSkiesDataHndl.ref(), cam.viewTm, cam.projTm, true, false, 20.0f, aurora.get());
      };
    });

    result.renderSky = dabfg::register_node("render_sky", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      auto mainSkiesDataHndl = registry.read("main_cam_skies_data").blob<SkiesData *>().handle();

      registry.read("far_downsampled_depth").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");

      auto resolvedFrameHndl =
        registry.modify("resolved_frame").texture().atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::COLOR_ATTACHMENT).handle();
      auto depthHndl =
        registry.read("depth_gbuf").texture().atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::DEPTH_ATTACHMENT).handle();

      return [resolvedFrameHndl, depthHndl, mainSkiesDataHndl, camHndl, this]() {
        // TODO: This is a dirty hack due to the fact taht daSkies is not
        // mobile-friendly yet and breaks render passes all over the place.
        d3d::set_render_target({depthHndl.view().getBaseTex()}, DepthAccess::SampledRO, {{resolvedFrameHndl.get()}});

        const auto &cam = camHndl.ref();

        daSkies.renderSky(mainSkiesDataHndl.ref(), cam.viewTm, cam.projTm, cam.persp, true, aurora.get());
      };
    });

    result.renderClouds = dabfg::register_node("render_clouds", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      auto fddHandle = registry.readTexture("far_downsampled_depth")
                         .atStage(dabfg::Stage::PS_OR_CS)
                         .bindToShaderVar("downsampled_far_depth_tex")
                         .handle();

      auto mainSkiesDataHndl = registry.read("main_cam_skies_data").blob<SkiesData *>().handle();


      auto depthReq = registry.read("depth_for_transparent").texture();
      registry.requestRenderPass().color({"color_for_transparent"}).depthRo(depthReq);
      auto depthHndl =
        eastl::move(depthReq).atStage(dabfg::Stage::POST_RASTER).useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE).handle();

      return [depthHndl, fddHandle, mainSkiesDataHndl, camHndl, this]() {
        const auto &cam = camHndl.ref();

        daSkies.renderClouds(render_panel.infinite_skies, fddHandle.view(), depthHndl.d3dResId(), mainSkiesDataHndl.ref(), cam.viewTm,
          cam.projTm);
      };
    });

    return result;
  }

  [[nodiscard]] FrameGraph::Backbone::Lighting makeLightingBackbone()
  {
    FrameGraph::Backbone::Lighting result;

    result.ssr = dabfg::register_node("SSR", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      bind_camera(registry, "main_camera");

      registry.readTexture("downsampled_normals").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();
      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");

      uint32_t fmt = 0;
      SSRQuality tmpQuality = SSRQuality::Low;
      ScreenSpaceReflections::getRealQualityAndFmt(fmt, tmpQuality);
      fmt |= TEXCF_RTARGET;
      const auto mipCount = static_cast<uint32_t>(ScreenSpaceReflections::getMipCount(SSRQuality::Low));

      auto ssrTargetHndl = registry.create("ssr_target", dabfg::History::ClearZeroOnFirstFrame)
                             .texture({fmt, registry.getResolution<2>("main_view", 0.5f), mipCount})
                             .atStage(dabfg::Stage::PS_OR_CS)
                             .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                             .handle();
      registry.create("ssr_target_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("ssr_target_samplerstate");
      auto ssrTargetHistHndl =
        registry.historyFor("ssr_target").texture().atStage(dabfg::Stage::PS_OR_CS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      auto ssr = eastl::make_unique<ScreenSpaceReflections>(width / 2, height / 2, 1, fmt, SSRQuality::Low, SSRFlag::None);
      return [this, ssrTargetHndl, ssrTargetHistHndl, ssr = eastl::move(ssr)]() {
        TMatrix itm;
        curCamera->getInvViewMatrix(itm);
        const DPoint3 worldPos = DPoint3(itm.getcol(3));

        TMatrix viewTm;
        TMatrix4 projTm;
        d3d::gettm(TM_VIEW, viewTm);
        d3d::gettm(TM_PROJ, &projTm);
        ssr->render(viewTm, projTm, worldPos, SubFrameSample::Single, ssrTargetHndl.view(), ssrTargetHistHndl.view(),
          ssrTargetHndl.view(), ::dagor_frame_no());
      };
    });

    result.ssao = dabfg::register_node("SSAO", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      bind_camera(registry, "main_camera");

      registry.readTexture("downsampled_normals").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();
      registry.read("downsampled_normals_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("downsampled_normals_samplerstate");
      auto fddHandle = registry.readTexture("far_downsampled_depth")
                         .atStage(dabfg::Stage::PS_OR_CS)
                         .bindToShaderVar("downsampled_far_depth_tex")
                         .handle();

      const auto halfMainViewRes = registry.getResolution<2>("main_view", 0.5f);
      const auto fmt = ssao_detail::creation_flags_to_format(SSAO_NONE);

      auto ssaoHndl = registry.create("ssao_tex", dabfg::History::ClearZeroOnFirstFrame)
                        .texture({fmt | TEXCF_RTARGET, halfMainViewRes})
                        .atStage(dabfg::Stage::PS)
                        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                        .handle();

      auto ssaoHistHndl =
        registry.historyFor("ssao_tex").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();

      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.border_color = d3d::BorderColor::Color::OpaqueWhite;
        registry.create("ssao_sampler", dabfg::History::No)
          .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
          .bindToShaderVar("ssao_tex_samplerstate")
          .bindToShaderVar("ssao_prev_tex_samplerstate");
      }

      auto ssaoTmpHndl = registry.create("ssao_tmp_tex", dabfg::History::No)
                           .texture({fmt | TEXCF_RTARGET, halfMainViewRes})
                           .atStage(dabfg::Stage::PS)
                           .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                           .handle();

      auto ssao = eastl::make_unique<SSAORenderer>(width / 2, height / 2, 1, SSAO_SKIP_RANDOM_PATTERN_GENERATION, false);
      return [this, fddHandle, ssaoHndl, ssaoHistHndl, ssaoTmpHndl, ssao = eastl::move(ssao)]() {
        TMatrix itm;
        curCamera->getInvViewMatrix(itm);
        const DPoint3 worldPos = DPoint3(itm.getcol(3));

        TMatrix viewTm;
        TMatrix4 projTm;
        d3d::gettm(TM_VIEW, viewTm);
        d3d::gettm(TM_PROJ, &projTm);
        auto ssaoView = ssaoHndl.view();
        auto prevSsaoView = ssaoHistHndl.view();
        auto tmpTexView = ssaoTmpHndl.view();
        ssao->render(viewTm, projTm, fddHandle.view().getBaseTex(), &ssaoView, &prevSsaoView, &tmpTexView, worldPos,
          SubFrameSample::Single);
      };
    });

    return result;
  }

  [[nodiscard]] FrameGraph::Backbone::Shadows makeShadowsBackbone()
  {
    FrameGraph::Backbone::Shadows result;

    result.csm = dabfg::register_node("CSM", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      const auto info = csm->getShadowCascadeTexInfo();
      auto csmHndl = registry.create("csm", dabfg::History::No)
                       .texture(dabfg::Texture2dCreateInfo{info.cflg, IPoint2{info.w, info.h}, info.mipLevels})
                       .atStage(dabfg::Stage::POST_RASTER)
                       .useAs(dabfg::Usage::DEPTH_ATTACHMENT)
                       .handle();
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        smpInfo.filter_mode = d3d::FilterMode::Compare;
        registry.create("csm_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
      }
      return [this, csmHndl]() {
        auto csmView = csmHndl.view();
        csm->renderShadowsCascadesCb(
          [this, csmView](int num_cascades_to_render, bool clear_per_view) {
            for (int i = 0; i < num_cascades_to_render; ++i)
              csm->renderShadowCascadeDepth(i, clear_per_view, csmView);
          },
          csmView);

        csmView->disableSampler();
      };
    });
    return result;
  }

  [[nodiscard]] FrameGraph::Backbone makeBackbone()
  {
    FrameGraph::Backbone result;

    result.makeCamera = dabfg::register_node("make_camera", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = registry.create("main_camera", dabfg::History::No).blob<Camera>().handle();
      return [this, camHndl]() {
        auto &cam = camHndl.ref();
        curCamera->getInvViewMatrix(cam.viewItm);
        // Disgusting hack, common samples framework needs to be reworked
        curCamera->setView();
        d3d::gettm(TM_VIEW, cam.viewTm);
        d3d::gettm(TM_PROJ, (mat44f &)cam.projTm);
        d3d::getglobtm((mat44f &)cam.globTm);
        d3d::getpersp(cam.persp);
        cam.frustum = Frustum(cam.globTm);
        cam.worldPos = cam.viewItm.getcol(3);
        cam.viewVecs = calc_view_vecs(cam.viewTm, cam.projTm);
      };
    });

    result.opaque = makeOpaqueBackbone();

    result.downsampleDepth = dabfg::register_node("downsample_depth", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto gbufDepthHandle = registry.readTexture(depth_tex_name).atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar().handle();

      registry.readTexture(normal_tex_name).atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();

      auto downsampledNormalsHandle = registry
                                        .createTexture2d("downsampled_normals", dabfg::History::No,
                                          dabfg::Texture2dCreateInfo{TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5)})
                                        .atStage(dabfg::Stage::POST_RASTER)
                                        .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                                        .handle();
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
        registry.create("downsampled_normals_sampler", dabfg::History::No).blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo));
      }

      // downsampled depth
      uint32_t numMips = min(get_log2w(width / 2), get_log2w(height / 2)) - 1;
      numMips = max(numMips, 4u);
      uint32_t rfmt = 0;
      if ((d3d::get_texformat_usage(TEXFMT_R32F) & d3d::USAGE_RTARGET))
        rfmt = TEXFMT_R32F;
      else if ((d3d::get_texformat_usage(TEXFMT_R16F) & d3d::USAGE_RTARGET))
        rfmt = TEXFMT_R16F;
      else
        rfmt = TEXFMT_R16F; // fallback to any supported format?

      auto farDownsampledDepthHandle =
        registry
          .createTexture2d("far_downsampled_depth", dabfg::History::DiscardOnFirstFrame,
            dabfg::Texture2dCreateInfo{rfmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5), numMips})
          .atStage(dabfg::Stage::POST_RASTER)
          .useAs(dabfg::Usage::COLOR_ATTACHMENT)
          .handle();

      return [gbufDepthHandle, farDownsampledDepthHandle, downsampledNormalsHandle, this]() {
        auto farDownsampledDepth = farDownsampledDepthHandle.view();

        TIME_D3D_PROFILE(downsample_depth);

        downsample_depth::downsample(gbufDepthHandle.view(), width, height, farDownsampledDepth, ManagedTexView{} /* closest_depth */,
          downsampledNormalsHandle.view(), ManagedTexView{}, ManagedTexView{}, true);
      };
    });

    result.copyFrame = dabfg::register_node("copy_frame", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto hndl = registry.readTexture("resolved_frame").atStage(dabfg::Stage::TRANSFER).useAs(dabfg::Usage::BLIT).handle();
      const auto rtFmt = calc_resolved_frame_texfmt();
      auto copyHndl = registry.create("prev_frame", dabfg::History::ClearZeroOnFirstFrame)
                        .texture(dabfg::Texture2dCreateInfo{rtFmt | TEXCF_RTARGET, registry.getResolution<2>("main_view", 0.5f)})
                        .atStage(dabfg::Stage::TRANSFER)
                        .useAs(dabfg::Usage::BLIT)
                        .handle();

      return [hndl, copyHndl]() {
        TIME_D3D_PROFILE(copy_frame);
        d3d::stretch_rect(hndl.view().getBaseTex(), copyHndl.get());
      };
    });

    result.startTransparent = dabfg::register_node("start_transparent", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      registry.rename("resolved_frame", "color_for_transparent", dabfg::History::No);
      registry.rename("depth_gbuf", "depth_for_transparent", dabfg::History::No);
    });

    result.lighting = makeLightingBackbone();
    result.shadows = makeShadowsBackbone();

    result.renderPostFx = dabfg::register_node("render_postfx", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.readTexture("color_for_transparent").atStage(dabfg::Stage::PS).bindToShaderVar("frame_tex");
      registry.create("frame_sampler", dabfg::History::No).blob(d3d::request_sampler({})).bindToShaderVar("frame_tex_samplerstate");

      registry.requestRenderPass().color({"backbuffer"});

      return [this, postfx = PostFxRenderer("postfx")]() {
        TIME_D3D_PROFILE(postfx)
        postfx.render();
      };
    });

    return result;
  }

  virtual void renderTrans()
  {
    TMatrix itm;
    curCamera->getInvViewMatrix(itm);
  }

  virtual void renderIslDecals() {}

  virtual void renderToShadowMap() {}

  virtual void renderWaterReflection() {}

  virtual void renderWaterRefraction() {}

  [[nodiscard]] FrameGraph::Water::Reflections makeWaterReflectionNodes()
  {
    FrameGraph::Water::Reflections result;

    auto reflNs = dabfg::root() / "water_refl";

    result.makeReflCamera = reflNs.registerNode("make_refl_camera", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto mainCamHndl = registry.read("main_camera").blob<Camera>().handle();
      auto reflCamHndl = registry.create("refl_camera", dabfg::History::No).blob<Camera>().handle();

      return [this, reflCamHndl, mainCamHndl]() {
        if (!(fftWater && water_panel.enabled))
          return;

        const auto &mainCam = mainCamHndl.ref();

        auto &reflCam = reflCamHndl.ref();

        calc_water_reflection_matrix(reflCam.viewTm, reflCam.viewItm, mainCam.viewItm, reflCam.worldPos, water_panel.water_level);

        Point4 worldClipPlane(0, 1, 0, -water_panel.water_level);

        TMatrix4 waterProj = oblique_projection_matrix_reverse2<true>(mainCam.projTm, reflCam.viewItm, worldClipPlane);
        if (fabs(det4x4(waterProj)) > 1e-15) // near plane is not perpendicular to water plane
          reflCam.projTm = waterProj;

        // TODO: this is a hack, we can calculate globTm and persp without
        // setting the matrices
        d3d::settm(TM_VIEW, reflCam.viewTm);
        d3d::settm(TM_PROJ, (mat44f &)reflCam.projTm);
        d3d::getglobtm((mat44f &)reflCam.globTm);
        d3d::getpersp(reflCam.persp);
        reflCam.frustum = Frustum(reflCam.globTm);
        reflCam.viewVecs = calc_view_vecs(reflCam.viewTm, reflCam.projTm);
      };
    });

    result.makeReflGbuf = reflNs.registerNode("make_refl_gbuf", DABFG_PP_NODE_SRC, [](dabfg::Registry registry) {
      auto resolution = registry.getResolution<2>("main_view", 1.f / 6.f);

      registry.createTexture2d("depth_gbuf_refl", dabfg::History::No,
        dabfg::Texture2dCreateInfo{TEXFMT_DEPTH24 | TEXCF_RTARGET, resolution});

      constexpr char const *const refl_gbuf_names[] = {"albedo_gbuf_refl", "normal_gbuf_refl", "material_gbuf_refl"};
      for (int i = 0; i < gbuf_rt; ++i)
        registry.createTexture2d(refl_gbuf_names[i], dabfg::History::No,
          dabfg::Texture2dCreateInfo{gbuf_fmts[i] | TEXCF_RTARGET, resolution});

      registry.requestRenderPass()
        .color({refl_gbuf_names[0], refl_gbuf_names[1], refl_gbuf_names[2]})
        .depthRw("depth_gbuf_refl")
        .clear(refl_gbuf_names[0], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear(refl_gbuf_names[1], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear(refl_gbuf_names[2], make_clear_value(0.f, 0.f, 0.f, 0.f))
        .clear("depth_gbuf_refl", make_clear_value(0.f, 0));
    });

    result.waterReflResolve = reflNs.registerNode("water_reflections_resolve", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "refl_camera").handle();

      auto resolution = registry.getResolution<2>("main_view", 1.f / 6.f);

      registry.read("albedo_gbuf_refl").texture().atStage(dabfg::Stage::PS).bindToShaderVar("albedo_gbuf");
      registry.read("normal_gbuf_refl").texture().atStage(dabfg::Stage::PS).bindToShaderVar("normal_gbuf");
      registry.read("material_gbuf_refl").texture().atStage(dabfg::Stage::PS).bindToShaderVar("material_gbuf");
      registry.read("depth_gbuf_refl").texture().atStage(dabfg::Stage::PS).bindToShaderVar("depth_gbuf");

      auto reflHandle = registry
                          .createTexture2d("water_reflection_tex_temp", dabfg::History::No,
                            dabfg::Texture2dCreateInfo{TEXFMT_A16B16G16R16F | TEXCF_RTARGET, resolution})
                          .atStage(dabfg::Stage::POST_RASTER)
                          .useAs(dabfg::Usage::COLOR_ATTACHMENT)
                          .handle();

      auto reflResolver = eastl::make_unique<ShadingResolver>("cube_deferred_shading");

      return [this, reflHandle, camHndl, reflResolver = eastl::move(reflResolver)]() {
        reflResolver->resolve(reflHandle.get(), camHndl.ref().viewTm, camHndl.ref().projTm);
      };
    });

    result.waterReflPrepareEnvi = reflNs.registerNode("water_refl_perpare_envi", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "refl_camera").handle();

      auto reflSkiesDataHndl = registry.create("refl_cam_skies_data", dabfg::History::No).blob<SkiesData *>().handle();

      auto depthHndl =
        registry.read("depth_gbuf_refl").texture().atStage(dabfg::Stage::PS).useAs(dabfg::Usage::SHADER_RESOURCE).handle();
      return [this, camHndl, depthHndl, reflSkiesDataHndl]() {
        reflSkiesDataHndl.ref() = refl_pov_data.get();
        const auto &cam = camHndl.ref();
        daSkies.prepareSkyAndClouds(render_panel.infinite_skies, dpoint3(cam.worldPos), dpoint3(cam.viewItm.getcol(2)), 3,
          depthHndl.view(), ManagedTexView{}, reflSkiesDataHndl.ref(), cam.viewTm, cam.projTm, true, false, 20.0f, aurora.get());
      };
    });

    result.waterReflEnvi = reflNs.registerNode("water_refl_envi", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "refl_camera").handle();

      auto reflSkiesDataHndl = registry.read("refl_cam_skies_data").blob<SkiesData *>().handle();

      auto depthReq = registry.read("depth_gbuf_refl").texture();
      registry.requestRenderPass().color({"water_reflection_tex_temp"}).depthRo(depthReq);
      auto depthHndl =
        eastl::move(depthReq).atStage(dabfg::Stage::PS).useAs(dabfg::Usage::DEPTH_ATTACHMENT_AND_SHADER_RESOURCE).handle();
      return [this, camHndl, depthHndl, reflSkiesDataHndl]() {
        const auto &cam = camHndl.ref();

        daSkies.renderSky(reflSkiesDataHndl.ref(), cam.viewTm, cam.projTm, cam.persp, true, aurora.get());
        daSkies.renderClouds(render_panel.infinite_skies, depthHndl.view(), depthHndl.d3dResId(), reflSkiesDataHndl.ref(), cam.viewTm,
          cam.projTm);
      };
    });


    result.clipWaterRefl = reflNs.registerNode("clip_water_reflection", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto resolution = registry.getResolution<2>("main_view", 1.f / 6.f);

      auto reflTexReq = registry.createTexture2d("water_reflection_tex", dabfg::History::No,
        dabfg::Texture2dCreateInfo{TEXFMT_A16B16G16R16F | TEXCF_RTARGET, resolution});

      registry.requestRenderPass().color({reflTexReq});

      registry.readTexture("water_reflection_tex_temp").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();
      registry.create("water_reflection_tex_temp_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("water_reflection_tex_temp_samplerstate");

      return [this, clipWaterReflection = PostFxRenderer("clip_water_reflection")]() { clipWaterReflection.render(); };
    });

    return result;
  }

  [[nodiscard]] FrameGraph::Water makeWaterNodes()
  {
    FrameGraph::Water result;

    result.reflections = makeWaterReflectionNodes();

    result.render = dabfg::register_node("render_water", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      registry.readTexture("far_downsampled_depth").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("depth_tex");
      {
        d3d::SamplerInfo smpInfo;
        smpInfo.filter_mode = d3d::FilterMode::Point;
        registry.create("water_fdd_sampler", dabfg::History::No)
          .blob<d3d::SamplerHandle>(d3d::request_sampler(smpInfo))
          .bindToShaderVar("depth_tex_samplerstate");
      }

      (registry.root() / "water_refl").readTexture("water_reflection_tex").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();
      registry.create("water_reflection_tex_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("water_reflection_tex_samplerstate");

      registry.setPriority(-100);
      registry.requestRenderPass().color({"color_for_transparent"}).depthRo("depth_for_transparent");

      registry.requestState().allowWireframe();

      registry.readTextureHistory("prev_frame").atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar("water_refraction_tex");
      registry.create("water_refraction_tex_sampler", dabfg::History::No)
        .blob(d3d::request_sampler({}))
        .bindToShaderVar("water_refraction_tex_samplerstate");

      return [camHndl, this]() {
        if (!(fftWater && water_panel.enabled))
          return;

        TIME_D3D_PROFILE(water_render);
        shvars::water_level.set_real(water_panel.water_level);
        fft_water::set_level(fftWater.get(), water_panel.water_level);

        float dimensions = 30;
        static ShaderVariableInfo water_heightmap_min_max("water_heightmap_min_max", true);
        water_heightmap_min_max.set_color4(1.f / dimensions, -(water_panel.water_level - 0.5 * dimensions) / dimensions, dimensions,
          water_panel.water_level - 0.5 * dimensions);

        const auto &cam = camHndl.ref();
        fft_water::render(fftWater.get(), cam.worldPos, BAD_TEXTUREID, cam.globTm, cam.persp, fft_water::GEOM_NORMAL);
      };
    });


    return result;
  }

  [[nodiscard]] FrameGraph::FinalPresent makeFinalPresentNodes()
  {
    FrameGraph::FinalPresent result;

    result.createBackbuffer = dabfg::register_node("create_backbuffer", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto bbReq = registry.registerBackBuffer("backbuffer");
      registry.requestRenderPass().color({bbReq});
    });

    result.debugTexOverlay = dabfg::register_node("debug_tex_overlay", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.requestRenderPass().color({registry.rename("backbuffer", "backbuffer_with_debug", dabfg::History::No).texture()});

      return [this]() {
        d3d::set_srgb_backbuffer_write(true);
        if (debugTexOverlay)
          debugTexOverlay->render();
        d3d::set_srgb_backbuffer_write(false);
      };
    });

    result.renderGui = dabfg::register_node("render_gui", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.requestRenderPass().color({registry.rename("backbuffer_with_debug", "final_backbuffer", dabfg::History::No).texture()});
      return [this]() { renderGui(); };
    });

    dabfg::update_externally_consumed_resource_set({"final_backbuffer"});

    return result;
  }

  [[nodiscard]] FrameGraph makeFrameGraph()
  {
    FrameGraph result;

    // Stuff that is 100% required to draw at least an error message
    result.finalPresent = makeFinalPresentNodes();

    // Basic FG skeleton, extended by optional nodes
    result.backbone = makeBackbone();

    ////////////////// optional nodes //////////////////

    result.water = makeWaterNodes();
    result.envi = makeEnviNodes();

    result.opaqueRendering.emplace_back(dabfg::register_node("render_opaque", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      auto camHndl = bind_camera(registry, "main_camera").handle();

      registry.requestState().allowWireframe();

      registry.requestRenderPass().color({gbuf_tex_names[0], gbuf_tex_names[1], gbuf_tex_names[2]}).depthRw(depth_tex_name);

      return [this, camHndl]() {
        const auto &cam = camHndl.ref();
        renderOpaque(cam.frustum, cam.worldPos);
      };
    }));

    result.transparentRendering.emplace_back(dabfg::register_node("render_trans", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
      registry.requestRenderPass().color({"color_for_transparent"}).depthRo("depth_for_transparent");

      // Pretend that we are actually rendering particles here
      registry.read("cloud_volume").texture().atStage(dabfg::Stage::PS_OR_CS).bindToShaderVar();

      return [this]() { mainRenderTrans(); };
    }));

    result.water.reflections.renderOpaque.emplace_back(
      (dabfg::root() / "water_refl").registerNode("render_opaque_refl", DABFG_PP_NODE_SRC, [this](dabfg::Registry registry) {
        auto camHndl = bind_camera(registry, "refl_camera").handle();

        registry.requestRenderPass().color({"albedo_gbuf_refl", "normal_gbuf_refl", "material_gbuf_refl"}).depthRw("depth_gbuf_refl");

        return [this, camHndl]() {
          if (!(fftWater && water_panel.enabled))
            return;

          const auto &cam = camHndl.ref();
          renderOpaque(cam.frustum, cam.worldPos);
        };
      }));

    return result;
  }

  void reinitCube(int ew)
  {
    d3d::GpuAutoLock gpu_al;
    cube.init(ew);
    enviProbe.reset(light_probe::create("envi", ew, TEXFMT_A16B16G16R16F));
    reloadCube(true);
  }

  void reloadCube(bool first = false)
  {
    d3d::GpuAutoLock gpu_al;
    SCOPE_RENDER_TARGET;
    if (first)
    {
      SCOPE_RENDER_TARGET;
      for (int i = 0; i < 6; ++i)
      {
        d3d::set_render_target(light_probe::getManagedTex(enviProbe.get())->getCubeTex(), i, 0);
        d3d::clearview(CLEAR_TARGET, 0, 0, 0);
      }
      for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
        ShaderGlobal::set_color4(get_shader_variable_id(String(128, "enviSPH%d", i)), 0, 0, 0, 0);
      light_probe::update(enviProbe.get(), nullptr);
    }
    {
      TIME_D3D_PROFILE(cubeRender)
      TMatrix itm;
      curCamera->getInvViewMatrix(itm);
      cube.update(light_probe::getManagedTex(enviProbe.get()), *this, itm.getcol(3));
    }
    {
      TIME_D3D_PROFILE(lightProbeSpecular)
      light_probe::update(enviProbe.get(), nullptr);
    }
    {
      TIME_D3D_PROFILE(lightProbeDiffuse)
      if (light_probe::calcDiffuse(enviProbe.get(), nullptr, 1, 1, true))
      {
        const Color4 *sphHarm = light_probe::getSphHarm(enviProbe.get());
        for (int i = 0; i < SphHarmCalc::SPH_COUNT; ++i)
          ShaderGlobal::set_color4(get_shader_variable_id(String(128, "enviSPH%d", i)), sphHarm[i]);

        Point4 normal1(0, 1, 0, 1);

        Point3 intermediate0, intermediate1, intermediate2;

        // sph.w - constant frequency band 0, sph.xyz - linear frequency band 1
        const Point4 *sphHarm4 = (const Point4 *)sphHarm;

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

        debug("sky_up = %@", intermediate0 + intermediate1 + intermediate2);
      }
      else
      {
        G_ASSERT(0);
      }
    }
    static ShaderVariableInfo envi_probe_specular("envi_probe_specular", true);
    static ShaderVariableInfo envi_probe_specular_samplerstate("envi_probe_specular_samplerstate", true);
    envi_probe_specular.set_texture(light_probe::getManagedTex(enviProbe.get())->getTexId());
    envi_probe_specular_samplerstate.set_sampler(d3d::request_sampler({}));
  }

  IGameCamera *curCamera; // for console made publuc
  eastl::unique_ptr<IFreeCameraDriver> freeCam;

protected:
  eastl::unique_ptr<DynamicRenderableSceneInstance> test;

  bool unloaded_splash;

  void loadScene()
  {
    int w, h;
    d3d::get_render_target_size(w, h, nullptr);
#if !(_TARGET_PC | _TARGET_IOS | _TARGET_TVOS | _TARGET_C3)
    freeCam = create_gamepad_free_camera();
#else
    global_cls_drv_pnt->getDevice(0)->setRelativeMovementMode(true);
    freeCam.reset(create_mskbd_free_camera());
    freeCam->scaleSensitivity(4, 2);
#endif
    IFreeCameraDriver::zNear = 0.6;
    IFreeCameraDriver::zFar = 80000.0;
    IFreeCameraDriver::turboScale = 2;
    de3_imgui_init("Skies Sample", "Sky properties");

    static GuiControlDesc skyGuiDesc[] = {
      DECLARE_BOOL_BUTTON(file_panel, export_min, false),
      DECLARE_BOOL_BUTTON(file_panel, export_max, false),
      DECLARE_BOOL_BUTTON(file_panel, import_minmax, false),
      DECLARE_INT_SLIDER(file_panel, import_seed, 0, 131072, 0),
      DECLARE_BOOL_BUTTON(file_panel, save, false),
      DECLARE_BOOL_BUTTON(file_panel, load, false),
      DECLARE_BOOL_CHECKBOX(sun_panel, astronomical, true),
      DECLARE_FLOAT_SLIDER(sun_panel, sun_zenith, 0.1, 120, 30, 0.01),
      DECLARE_FLOAT_SLIDER(sun_panel, sun_azimuth, 0., 360, 90, 1),
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

      DECLARE_FLOAT_SLIDER(sky_colors, solar_irradiance_scale.r, 0.01, 2, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, solar_irradiance_scale.g, 0.01, 2, 1, 0.01),
      DECLARE_FLOAT_SLIDER(sky_colors, solar_irradiance_scale.b, 0.01, 2, 1, 0.01),

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

      DECLARE_BOOL_CHECKBOX(land_panel, enabled, true),
      DECLARE_FLOAT_SLIDER(land_panel, heightScale, 1, 10000.0, 2500.0, 1),
      DECLARE_FLOAT_SLIDER(land_panel, heightMin, -1.0, 1.0, -0.5, 0.01),
      DECLARE_FLOAT_SLIDER(land_panel, hmapCenterOfsX, -0.5f, 0.5f, 0, 0.01),
      DECLARE_FLOAT_SLIDER(land_panel, hmapCenterOfsZ, -0.5f, 0.5f, 0, 0.01),
      DECLARE_FLOAT_SLIDER(land_panel, cellSize, 1, 250, 25, 0.5),
      DECLARE_BOOL_CHECKBOX(land_panel, trees, true),
      DECLARE_BOOL_CHECKBOX(land_panel, spheres, true),
      DECLARE_BOOL_CHECKBOX(land_panel, plane, true),

      DECLARE_BOOL_CHECKBOX(water_panel, enabled, true),
      DECLARE_BOOL_CHECKBOX(water_panel, shore, true),
      DECLARE_BOOL_CHECKBOX(water_panel, wake, true),
      DECLARE_BOOL_CHECKBOX(water_panel, shadows, true),
      DECLARE_BOOL_CHECKBOX(water_panel, deepness, true),
      DECLARE_BOOL_CHECKBOX(water_panel, foam, true),
      DECLARE_BOOL_CHECKBOX(water_panel, env_refl, true),
      DECLARE_BOOL_CHECKBOX(water_panel, planar_refl, true),
      DECLARE_BOOL_CHECKBOX(water_panel, sun_refl, true),
      DECLARE_BOOL_CHECKBOX(water_panel, seabed_refr, true),
      DECLARE_BOOL_CHECKBOX(water_panel, sss_refr, true),
      DECLARE_BOOL_CHECKBOX(water_panel, underwater_refr, true),
      DECLARE_BOOL_CHECKBOX(water_panel, proj_eff, true),
      DECLARE_FLOAT_SLIDER(water_panel, water_strength, 0.1, 10.0, 4.0, 0.1),
      DECLARE_FLOAT_SLIDER(water_panel, water_level, -1000, 1000.0, 0.0, 0.1),
      // DECLARE_FLOAT_SLIDER(water_panel, roughness, -1000, 1000.0, 0.0, 0.1),
      // DECLARE_FLOAT_SLIDER(water_panel, fft_period, 1, 3000.0, ,30),
      // DECLARE_FLOAT_SLIDER(water_panel, small_waves, 0, .02, ,0.0001),
      // DECLARE_FLOAT_SLIDER(water_panel, cell_size, 0.01, 2.0, ,0.01),
      // DECLARE_FLOAT_SLIDER(water_panel, dim_bits,3.0, 7.0, ,1.0),
      DECLARE_BOOL_CHECKBOX(water_panel, cascade0, true),
      DECLARE_BOOL_CHECKBOX(water_panel, cascade1, true),
      DECLARE_BOOL_CHECKBOX(water_panel, cascade2, true),
      DECLARE_BOOL_CHECKBOX(water_panel, cascade3, true),

      // DECLARE_BOOL_BUTTON(clouds_gen, regenerate, false),
      // DECLARE_BOOL_CHECKBOX(clouds_gen, gpu, true),
      // DECLARE_BOOL_CHECKBOX(clouds_gen, need_tracer, true),
      // DECLARE_INT_SLIDER(clouds_gen, seed, 0, 32767, 13534),
      // DECLARE_FLOAT_SLIDER(clouds_gen, humidity, 0, 1.0, 0.4, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_gen, persistence, 0, 1.0, 0.65, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_gen, asymmetry, 0, 1.0, 0, 0.01),
      // DECLARE_BOOL_BUTTON(clouds_panel, relight, false),
      // DECLARE_BOOL_CHECKBOX(clouds_panel, update_on_light_change, true),
      // DECLARE_FLOAT_SLIDER(clouds_panel, start_altitude, 0.02, 8.0, 1.83, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_panel, thickness, 2.0, 8.0, 5.0, 0.05),
      // DECLARE_FLOAT_SLIDER(clouds_panel, light_extinction, 0.02, 4.0, 1.0, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_panel, amb_extinction_mul, 0.02, 2.0, 0.50, 0.01),

      // DECLARE_FLOAT_SLIDER(clouds_light, silver_lining, 0, 1.20, 0.55, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_light, sun_light, 0, 1.20, 0.50, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_light, ambient, 0, 1.20, 0.55, 0.01),
      // DECLARE_FLOAT_SLIDER(clouds_light, silver_lining_eccentricity, 0.01, 0.99, 0.75, 0.01),

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
      DECLARE_FLOAT_SLIDER(clouds_game_params, maximum_averaging_ratio, 0.0, 1.0, 0.75, 0.01),
      DECLARE_INT_SLIDER(clouds_game_params, target_quality, 1, 4, 2),

      DECLARE_FLOAT_SLIDER(strata_clouds, amount, 0, 1.0, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(strata_clouds, altitude, 4.0, 14.0, 10.0, 0.100),
      DECLARE_INT_COMBOBOX(render_panel, render_type, DIRECT, PANORAMA, DIRECT),
      DECLARE_BOOL_CHECKBOX(render_panel, enable_god_rays_from_land, false),
      DECLARE_BOOL_CHECKBOX(render_panel, infinite_skies, false),
      DECLARE_BOOL_CHECKBOX(render_panel, shadows_2d, false),
      DECLARE_INT_COMBOBOX(render_panel, panoramaResolution, 2048, 1024, 1536, 2048, 3072, 4096),
      DECLARE_BOOL_CHECKBOX(render_panel, panorama_blending, false),
      DECLARE_FLOAT_SLIDER(render_panel, panoramaReprojection, 0, 1, 0.5, 0.01),
      DECLARE_FLOAT_SLIDER(render_panel, cloudsSpeed, 0, 60, 20, 0.1),
      DECLARE_FLOAT_SLIDER(render_panel, strataCloudsSpeed, 0, 30, 10, 0.1),
      DECLARE_FLOAT_SLIDER(render_panel, windDir, 0, 359, 0, 1),
      DECLARE_BOOL_BUTTON(render_panel, compareCpuSunSky, false),
      DECLARE_BOOL_CHECKBOX(render_panel, findHole, false),
      DECLARE_BOOL_CHECKBOX(render_panel, useCloudsHole, true),

      DECLARE_INT_SLIDER(render_quality_panel, sky_res_divisor, 0, 3, 1),
      DECLARE_INT_SLIDER(render_quality_panel, clouds_res_divisor, 0, 3, 1),
      DECLARE_INT_SLIDER(render_quality_panel, skies_lut_quality, 1, 8, 2),
      DECLARE_INT_SLIDER(render_quality_panel, scattering_screen_quality, 1, 8, 2),
      DECLARE_INT_SLIDER(render_quality_panel, scattering_depth_slices, 16, 128, 64),
      DECLARE_FLOAT_SLIDER(render_quality_panel, scattering_range_scale, 0.1f, 2.0f, 1.f, 0.05),
      DECLARE_FLOAT_SLIDER(render_quality_panel, scattering_min_range, 100.f, 250000.0f, 60000.f, 100.f),
      DECLARE_INT_SLIDER(render_quality_panel, colored_transmittance_quality, 1, 8, 2),
      // DECLARE_BOOL_BUTTON(render_panel, findHole, false),

      DECLARE_FLOAT_SLIDER(layered_fog, mie2_scale, 0, 200, 0, 0.00001),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_altitude, 1, 2500, 200, 1),
      DECLARE_FLOAT_SLIDER(layered_fog, mie2_thickness, 200, 1500, 400, 1),

      DECLARE_BOOL_CHECKBOX(aurora_borealis, enabled, false),
      DECLARE_FLOAT_SLIDER(aurora_borealis, top_height, 3.0, 30.0, 9.5, 0.5),
      DECLARE_POINT3_EDITBOX(aurora_borealis, top_color, 0.1, 0.1, 1.4),
      DECLARE_FLOAT_SLIDER(aurora_borealis, bottom_height, 3.0, 30.0, 5.0, 1.0),
      DECLARE_POINT3_EDITBOX(aurora_borealis, bottom_color, 0.1, 1.3, 0.1),
      DECLARE_FLOAT_SLIDER(aurora_borealis, speed, 0.004, 0.202, 0.02, 0.002),
      DECLARE_FLOAT_SLIDER(aurora_borealis, brightness, 0.2, 5.1, 2.2, 0.05),
      DECLARE_FLOAT_SLIDER(aurora_borealis, luminance, 0.0, 20.0, 8.0, 0.5),
      DECLARE_FLOAT_SLIDER(aurora_borealis, ripples_scale, 1.0, 50.0, 20.0, 1.0),
      DECLARE_FLOAT_SLIDER(aurora_borealis, ripples_speed, 0.01, 1.0, 0.08, 0.01),
      DECLARE_FLOAT_SLIDER(aurora_borealis, ripples_strength, 0.0, 1.0, 0.8, 0.01),
    };
    de3_imgui_build(skyGuiDesc, sizeof(skyGuiDesc) / sizeof(skyGuiDesc[0]));

    if (const DataBlock *def_envi = dgs_get_settings()->getBlockByName("defEnvi"))
    {
      DataBlock env_stg;
      de3_imgui_load_values(*def_envi);
      de3_imgui_save_values(env_stg);
    }

    curCamera = freeCam.get();
    ::set_camera_pos(*curCamera, dgs_get_settings()->getPoint3("camPos", Point3(0, 0, 0)));

    samplebenchmark::setupBenchmark(curCamera, []() {
      quit_game(1);
      return;
    });

    createSpheres();
    ddsx::tex_pack2_perform_delayed_data_loading(); // or it will crash here
    d3d::GpuAutoLock gpu_al;
    d3d::set_render_target();
    visualConsoleDriver.reset(new VisualConsoleDriver);
    console::set_visual_driver(visualConsoleDriver.get(), nullptr);
    scheduleHeightmapLoad(dgs_get_settings()->getStr("heightmap", nullptr));
    scheduleDynModelLoad(test, dgs_get_settings()->getStr("model", "test"), dgs_get_settings()->getStr("model_skel", nullptr),
      dgs_get_settings()->getPoint3("model_pos", Point3(0, 0, 0)));

    debugTexOverlay.reset(new DebugTexOverlay(Point2(w, h)));

    setDirToSun();
    sky_panel.moon_color = sky_colors.moon_color;
    sky_panel.mie_scattering_color = sky_colors.mie_scattering_color;
    sky_panel.mie_absorption_color = sky_colors.mie_absorption_color;
    sky_panel.solar_irradiance_scale = sky_colors.solar_irradiance_scale;
    sky_panel.rayleigh_color = sky_colors.rayleigh_color;
    sky_panel.mie2_thickness = layered_fog.mie2_thickness;
    sky_panel.mie2_altitude = layered_fog.mie2_altitude;
    sky_panel.mie2_scale = layered_fog.mie2_scale;
    sky_panel.min_ground_offset = heightmapMin;
    if (water_panel.enabled)
      sky_panel.min_ground_offset = max(sky_panel.min_ground_offset, water_panel.water_level);
    daSkies.setSkyParams(sky_panel);
    daSkies.setCloudsGameSettingsParams(clouds_game_params);
    daSkies.setWeatherGen(clouds_weather_gen2);
    daSkies.setCloudsForm(clouds_form);
    daSkies.setCloudsRendering(clouds_rendering2);
    aurora->setParams(aurora_borealis, w, h);
    // daSkies.generateClouds(clouds_gen, clouds_gen.gpu, clouds_gen.need_tracer, clouds_gen.regenerate);
    // daSkies.setCloudsPosition(clouds_panel, -1);
    daSkies.setStrataClouds(strata_clouds);
    // daSkies.setLayeredFog(layered_fog);
    daSkies.prepare(dir_to_sun, true, 0);
    Color3 sun, amb, moon, moonamb;
    float sunCos, moonCos;
    if (daSkies.currentGroundSunSkyColor(sunCos, moonCos, sun, amb, moon, moonamb))
    {
      static ShaderVariableInfo sun_light_color("sun_light_color");
      sun_light_color.set_color4(color4(sun, 0));
      static ShaderVariableInfo amb_light_color("amb_light_color", true);
      amb_light_color.set_color4(color4(amb, 0));
    }
    else
    {
      G_ASSERT(0);
    }

    reinitCube(128);
  }

public:
  struct FilePanel
  {
    bool save, load, export_min, export_max, import_minmax, export_water, import_water;
    int import_seed = 0;

    FilePanel() :
      save(false), load(false), export_min(false), export_max(false), import_minmax(false), export_water(false), import_water(false)
    {}
  } file_panel;

  SkyAtmosphereParams sky_panel;
  SkyAtmosphereParams sky_colors;

  SkyAtmosphereParams layered_fog;

  DaSkies::StrataClouds strata_clouds;
  DaSkies::CloudsRendering clouds_rendering2;
  DaSkies::CloudsWeatherGen clouds_weather_gen2;
  DaSkies::CloudsSettingsParams clouds_game_params;
  DaSkies::CloudsForm clouds_form;
  AuroraBorealisParams aurora_borealis;

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
      time(9),
      year(1943),
      day(1),
      month(6),
      local_time(true),
      moon_age(15)
    {}
  } sun_panel;

  struct LandPanel
  {
    bool enabled, trees, spheres, plane;
    float heightScale, heightMin, cellSize;
    float hmapCenterOfsX = 0, hmapCenterOfsZ = 0;
    LandPanel() : enabled(true), trees(true), spheres(true), plane(true), heightScale(2500.0), heightMin(-0.5), cellSize(25) {}
  } land_panel;

  static const fft_water::WavePreset Waves;

  struct WaterPanel
  {
    const fft_water::WavePreset &W = Waves;
    const fft_water::SimulationParams &S = Waves.simulation;

    bool enabled;
    int cascade_no;
    float water_strength, water_level, fft_period, small_waves, cell_size, dim_bits, roughness;
    float speed0, speed1, speed2, speed3, amplitude0, amplitude1, amplitude2, amplitude3, choppy_scale0, choppy_scale1, choppy_scale2,
      choppy_scale3, period_k0, period_k1, period_k2, period_k3, window_in_k0, window_in_k1, window_in_k2, window_in_k3, window_out_k0,
      window_out_k1, window_out_k2, window_out_k3, wind_dep0, wind_dep1, wind_dep2, wind_dep3;
    bool cascade0, cascade1, cascade2, cascade3;
    bool shore, wake, shadows, deepness, foam, env_refl, planar_refl, sun_refl, seabed_refr, sss_refr, underwater_refr, proj_eff;

    WaterPanel() :
      enabled(true),
      water_strength(W.wind),
      water_level(0.0),
      fft_period(W.period),
      small_waves(0.0f),
      cell_size(1.0f),
      dim_bits(5),
      roughness(W.roughness),
      cascade_no(0), /*speed0(S[0].speed), speed1(S[1].speed), speed2(S[2].speed), speed3(S[3].speed),
amplitude0(S[0].amplitude), amplitude1(S[1].amplitude), amplitude2(S[2].amplitude), amplitude3(S[3].amplitude),
choppy_scale0(S[0].choppy), choppy_scale1(S[1].choppy), choppy_scale2(S[2].choppy), choppy_scale3(S[3].choppy),
period_k0(S[0].period), period_k1(S[1].period), period_k2(S[2].period), period_k3(S[3].period),
window_in_k0(S[0].windowIn), window_in_k1(S[1].windowIn), window_in_k2(S[2].windowIn), window_in_k3(S[3].windowIn),
window_out_k0(S[0].windowOut), window_out_k1(S[1].windowOut), window_out_k2(S[2].windowOut), window_out_k3(S[3].windowOut),
wind_dep0(S[0].windDependency), wind_dep1(S[1].windDependency), wind_dep2(S[2].windDependency), wind_dep3(S[3].windDependency),*/
      cascade0(true),
      cascade1(true),
      cascade2(true),
      cascade3(true),
      shore(true),
      wake(true),
      shadows(true),
      deepness(true),
      foam(true),
      env_refl(true),
      planar_refl(true),
      sun_refl(true),
      seabed_refr(true),
      sss_refr(true),
      underwater_refr(true),
      proj_eff(true)
    {}
  } water_panel;

  struct WaterFoamPanel
  {
    const fft_water::FoamParams &F = Waves.foam;

    float foam_dissipation_speed, foam_generation_amount, foam_generation_threshold, foam_falloff_speed, foam_hats_mul,
      foam_hats_threshold, foam_hats_folding;

    WaterFoamPanel() :
      foam_dissipation_speed(F.dissipation_speed),
      foam_generation_amount(F.generation_amount),
      foam_generation_threshold(F.generation_threshold),
      foam_falloff_speed(F.falloff_speed),
      foam_hats_mul(F.hats_mul),
      foam_hats_threshold(F.hats_threshold),
      foam_hats_folding(F.hats_folding)
    {}
  } water_foam_panel;

  struct RenderQualityPanel
  {
    int sky_res_divisor = 1;
    int clouds_res_divisor = 1;
    int skies_lut_quality = 1, scattering_screen_quality = 1, scattering_depth_slices = 64;
    int colored_transmittance_quality = 1;
    float scattering_range_scale = 1.0f;
    float scattering_min_range = 50000.f;
    bool operator==(const RenderQualityPanel &a) const
    {
      return a.sky_res_divisor == sky_res_divisor && a.clouds_res_divisor == clouds_res_divisor &&
             a.skies_lut_quality == skies_lut_quality && a.colored_transmittance_quality == colored_transmittance_quality &&
             a.scattering_screen_quality == scattering_screen_quality && a.scattering_depth_slices == scattering_depth_slices &&
             a.scattering_min_range == scattering_min_range && a.scattering_range_scale == scattering_range_scale;
    }
    bool operator!=(const RenderQualityPanel &a) const { return !(*this == a); }
  } render_quality_panel, old_quality_panel;

  enum
  {
    PANORAMA = 0,
    DIRECT = 2
  };
  struct RenderPanel
  {
    float fog_distance_mul;
    int render_type;
    bool infinite_skies, panorama_blending, shadows_2d = false;
    bool compareCpuSunSky;
    bool findHole = false;
    bool useCloudsHole = true;
    bool enable_god_rays_from_land = false;
    int panoramaResolution;
    float panoramaTemporalSpeed;
    float cloudsSpeed, strataCloudsSpeed, windDir;
    float panoramaReprojection = 0.5;

    RenderPanel() :
      fog_distance_mul(1),
      render_type(DIRECT),
      infinite_skies(false),
      panoramaResolution(2048),
      panorama_blending(false),
      panoramaTemporalSpeed(0.1),
      cloudsSpeed(20),
      strataCloudsSpeed(10),
      windDir(0),
      compareCpuSunSky(false)
    {}
  } render_panel;

  DaSkies daSkies;
  eastl::unique_ptr<AuroraBorealis> aurora;

  RenderDynamicCube cube;
  eastl::unique_ptr<light_probe::Cube, void (*)(light_probe::Cube *)> enviProbe{nullptr, &light_probe::destroy};

  void createSpheres()
  {
    sphereMat = new_shader_material_by_name("sphereMaterial");
    sphereElem = sphereMat->make_elem();
  }

  Ptr<ShaderMaterial> sphereMat;
  Ptr<ShaderElement> sphereElem;
  eastl::unique_ptr<console::IVisualConsoleDriver> visualConsoleDriver;

  void scheduleHeightmapLoad(const char *name)
  {
    struct LoadJob : cpujobs::IJob
    {
      const char *name;
      DemoGameScene &s;
      LoadJob(DemoGameScene &_scene, const char *_name) : s(_scene), name(_name) {}
      virtual void doJob()
      {
        float cell_sz = 1.0f, scale = 100.0f, h0 = 0.0f;
        TexPtr tex = strcmp(dd_get_fname_ext(name), ".mtw") == 0 ? create_tex_from_mtw(name, &cell_sz, &s.hmapMaxHt, &scale, &h0)
                                                                 : create_tex_from_raw_hmap_file(name);
        static ShaderVariableInfo tex_hmap_inv_sizes("tex_hmap_inv_sizes");
        Color4 invSz = tex_hmap_inv_sizes.get_color4();
        int tex_w = safeinv(invSz.r), tex_h = safeinv(invSz.g);

        s.land_panel.cellSize = s.heightmapCellSize = dgs_get_settings()->getReal("heightmapCell", cell_sz);
        s.land_panel.heightScale = s.heightmapScale = dgs_get_settings()->getReal("heightmapScale", scale) * s.hmapMaxHt;
        Point3 hmapSz(tex_w * s.heightmapCellSize, s.heightmapScale, tex_w * s.heightmapCellSize);
        Point3 hmapOfs = dgs_get_settings()->getPoint3("hmapCenterRelOfs",
          div(dgs_get_settings()->getPoint3("hmapCenterOfs", ZERO<Point3>()), hmapSz));

        s.heightmapMin = dgs_get_settings()->getReal("heightmapMin", h0) + hmapOfs.y * hmapSz.y;
        s.land_panel.heightMin = s.heightmapMin / s.land_panel.heightScale;
        hmapOfs.y = s.land_panel.heightMin;

        s.land_panel.hmapCenterOfsX = hmapOfs.x;
        s.land_panel.hmapCenterOfsZ = hmapOfs.z;
        debug("HMAP loaded %s: %.0fx%.0f size(m)=%@ cell=%gm hScale=%g hOfs=%gm hDelta=%gm hmapRelOfs=%@", name, tex_w, tex_h, hmapSz,
          s.heightmapCellSize, s.heightmapScale, s.heightmapMin, s.heightmapScale, hmapOfs);

        struct InitHeightmap : public DelayedAction
        {
          UniqueTexHolder &tex;
          TexPtr ht;
          InitHeightmap(UniqueTexHolder &_tex, TexPtr &&_ht) : ht(eastl::move(_ht)), tex(_tex) {}
          virtual void performAction()
          {
            tex = UniqueTexHolder(eastl::move(ht), "heightmap");
            cpujobs::release_done_jobs();
          }
        };
        add_delayed_action(new InitHeightmap(s.heightmap, eastl::move(tex)));
      };
      virtual void releaseJob() { delete this; }
    };
    {
      d3d::SamplerInfo smpInfo;
      smpInfo.address_mode_u = smpInfo.address_mode_v = smpInfo.address_mode_w = d3d::AddressMode::Clamp;
      heightmapSampler = d3d::request_sampler(smpInfo);
    }
    heightmap.close();
    water_panel.water_level = dgs_get_settings()->getReal("heightmapWaterLevel", water_panel.water_level);

    const DataBlock *hmapTex = dgs_get_settings()->getBlockByNameEx("heightmapTex");
    if (name)
      cpujobs::add_job(1, new LoadJob(*this, name));
  }

  void scheduleDynModelLoad(eastl::unique_ptr<DynamicRenderableSceneInstance> &dm, const char *name, const char *skel_name,
    const Point3 &pos)
  {
    struct LoadJob : cpujobs::IJob
    {
      eastl::unique_ptr<DynamicRenderableSceneInstance> &dm;
      SimpleString name, skelName;
      Point3 pos;

      LoadJob(eastl::unique_ptr<DynamicRenderableSceneInstance> &_dm, const char *_name, const char *_skel_name, const Point3 &_pos) :
        dm(_dm), name(_name), skelName(_skel_name), pos(_pos)
      {}
      virtual void doJob()
      {
        GameResHandle handle = GAMERES_HANDLE_FROM_STRING(name);
        DynamicRenderableSceneInstance *val = nullptr;

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
        ddsx::tex_pack2_perform_delayed_data_loading();

        struct UpdatePtr : public DelayedAction
        {
          eastl::unique_ptr<DynamicRenderableSceneInstance> &dest;
          DynamicRenderableSceneInstance *val;
          UpdatePtr(eastl::unique_ptr<DynamicRenderableSceneInstance> &_dest, DynamicRenderableSceneInstance *_val) :
            dest(_dest), val(_val)
          {}
          virtual void performAction()
          {
            dest.reset(val);
            cpujobs::release_done_jobs();
          }
        };
        add_delayed_action(new UpdatePtr(dm, val));
      }
      virtual void releaseJob() { delete this; }
    };
    dm.reset();
    cpujobs::add_job(1, new LoadJob(dm, name, skel_name, pos));
  }

  void reloadUiWaves()
  {
    fft_water::WavePreset preset;
    fft_water::get_wave_preset(fftWater.get(), preset);
    water_panel.water_strength = preset.wind;
    water_panel.fft_period = preset.period;
    water_panel.roughness = preset.roughness;

#define GET_WATER_CASCADE(no) /*                                                           \
                              {                                                            \
                              water_panel.amplitude##no = preset.scales[no].amplitude;     \
                              water_panel.choppy_scale##no = preset.scales[no].choppy;     \
                              water_panel.speed##no = preset.scales[no].speed;             \
                              water_panel.period_k##no = preset.scales[no].period;         \
                              water_panel.window_in_k##no = preset.scales[no].windowIn;    \
                              water_panel.window_out_k##no = preset.scales[no].windowOut;  \
                              water_panel.wind_dep##no = preset.scales[no].windDependency; \
                              }*/
    GET_WATER_CASCADE(0);
    GET_WATER_CASCADE(1);
    GET_WATER_CASCADE(2);
    GET_WATER_CASCADE(3);

    water_foam_panel.foam_dissipation_speed = preset.foam.dissipation_speed;
    water_foam_panel.foam_generation_amount = preset.foam.generation_amount;
    water_foam_panel.foam_generation_threshold = preset.foam.generation_threshold;
    water_foam_panel.foam_falloff_speed = preset.foam.falloff_speed;
    water_foam_panel.foam_hats_mul = preset.foam.hats_mul;
    water_foam_panel.foam_hats_threshold = preset.foam.hats_threshold;
    water_foam_panel.foam_hats_folding = preset.foam.hats_folding;
  }

public:
  WaterPanel &getWaterPanel() { return water_panel; }
  RenderPanel &getRenderPanel() { return render_panel; }
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

  console::print_d(b ? "on" : "off");
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
  CONSOLE_CHECK_NAME("water", "debug_cs", 1, 2)
  {
    extern bool use_cs_water;
    toggle_or_set_bool_arg(use_cs_water, 1, argc, argv);
  }
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    String output = scene->debugTexOverlay->processConsoleCmd(argv, argc);
    if (!output.empty())
      console::print(output);
  }
  /*CONSOLE_CHECK_NAME("clouds", "cloudsOffset", 2, 2)
  {
    ((DemoGameScene*)dagor_get_current_game_scene())->skies.setCloudsHolePosition(Point2(atof(argv[1]), atof(argv[2])));
  }
  CONSOLE_CHECK_NAME("clouds", "cloudsHole", 2, 2)
  {
    ((DemoGameScene*)dagor_get_current_game_scene())->skies.setCloudsHolePosition(Point2(atof(argv[1]), atof(argv[2])));
  }*/
  CONSOLE_CHECK_NAME("time", "speed", 1, 2)
  {
    if (argc < 2)
      console::print_d("%f realtimeTriggers=%s", (float)::dagor_game_time_scale, "no");
    else
    {
      float timespeed = atof(argv[1]);
      ::dagor_game_time_scale = timespeed;
    }
  }
  CONSOLE_CHECK_NAME("water", "wind", 2, 4)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    auto water = scene->fftWater.get();
    if (!water)
      return true;

    float windDirX = cosf(DegToRad(scene->getRenderPanel().windDir)), windDirZ = sinf(DegToRad(scene->getRenderPanel().windDir));
    Point2 wind_dir = Point2(windDirX, windDirZ);
    if (argc > 3)
      wind_dir = normalize(Point2(atof(argv[2]), atof(argv[3])));
    fft_water::apply_wave_preset(water, atof(argv[1]), wind_dir, fft_water::Spectrum::UNIFIED_DIRECTIONAL);

    fft_water::set_fft_resolution(water, 7);
    fft_water::setWaterCell(water, 1.0f, true);
    fft_water::set_water_dim(water, 5);

    console::print_d("water wind %g beaufort %g %g dir", atof(argv[1]), wind_dir.x, wind_dir.y);
  }
  CONSOLE_CHECK_NAME("water", "tesselation", 2, 2)
  {
    DemoGameScene *scene = (DemoGameScene *)dagor_get_current_game_scene();
    /*fft_water::setWaterCell(scene->water,
      argc > 1 && atoi(argv[1]) == 2 ? 0.25f :
      (argc > 1 && atoi(argv[1]) == 1 ? 0.5f : 1.0f), true);*/
    fft_water::set_water_dim(scene->fftWater.get(), argc > 1 && atoi(argv[1]) == 2 ? 7 : (argc > 1 && atoi(argv[1]) == 1 ? 6 : 5));
  }
  return found;
}


static class MyTestPlugin : public da_profiler::ProfilerPlugin
{
  bool enabled = false;

public:
  bool setEnabled(bool e) override
  {
    enabled = e;
    console::print_d("Profiler Plugin %s", e ? "on" : "off");
    return e;
  }
  bool isEnabled() const override { return enabled; }
} test_plugin;

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
  if (!cpujobs::is_inited())
    cpujobs::init();
  ::set_gameres_sys_ver(2);
  ::register_dynmodel_gameres_factory();
  ::register_geom_node_tree_gameres_factory();
  ::register_tga_tex_load_factory();
  ::register_loadable_tex_create_factory();

  da_profiler::register_plugin("My Test Plugin", &test_plugin);
  console::init();
  add_con_proc(&test_console);

  ::set_gameres_sys_ver(2);

  const char *res_vrom = "res/grp_hdr.vromfs.bin";
  const char *res_list = "res/respacks.blk";
  VirtualRomFsData *vrom = res_vrom ? ::load_vromfs_dump(res_vrom, tmpmem) : nullptr;
  if (vrom)
    ::add_vromfs(vrom);
  ::load_res_packs_from_list(res_list);
  if (vrom)
  {
    ::remove_vromfs(vrom);
    tmpmem->free(vrom);
  }

  ::dgs_limit_fps = blk.getBool("limitFps", false);
  dagor_set_game_act_rate(blk.getInt("actRate", -60));
  init_webui(nullptr);

  PictureManager::init();

  dagor_select_game_scene(new DemoGameScene);
}


void game_demo_close()
{
  dagor_select_game_scene(nullptr);
  PictureManager::release();
}

const fft_water::WavePreset DemoGameScene::Waves;
