// Copyright (C) Gaijin Games KFT.  All rights reserved.

#define INSIDE_RENDERER 1 // fixme: move to jam
// #include "main/console.h"
#include <gamePhys/collision/collisionLib.h>
#include <daECS/core/componentTypes.h>

#include <gui/dag_visConsole.h>
#include <visualConsole/dag_visualConsole.h>
#include <util/dag_console.h>
#include <drv/3d/dag_driver.h>
#include <3d/dag_render.h>
#include <startup/dag_loadSettings.h>
#include <rendInst/rendInstExtra.h>
#include <rendInst/rendInstExtraRender.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstGenRender.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_direct.h>
#include <math/random/dag_random.h>
#include <render/omniLight.h>
#include <daFx/dafx.h>
#include "private_worldRenderer.h"
#include <landMesh/clipMap.h>
#include <landMesh/biomeQuery.h>
#include <render/debugTexOverlay.h>
#include <render/debugGbuffer.h>
#include <render/dag_cur_view.h>

#include <stdlib.h>

#include <ecs/core/entityManager.h>
#include <ecs/anim/anim.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <gameRes/dag_stdGameResId.h>
#include "game/player.h"
#include "render/fx/fx.h"
#include <render/dynamicQuality.h>
#include "camera/sceneCam.h"

#include <ioSys/dag_fileIo.h>
#include <scene/dag_occlusion.h>

#include <gui/dag_visualLog.h>
#include <shaders/dag_shaderDbg.h>

void prerun_fx();
float get_camera_fov();
extern void set_up_show_shadows_entity(int show_shadows);
extern void set_up_show_gbuffer_entity();
extern void set_up_debug_indoor_probes_on_screen_entity(bool render);

class RendererConsole : public console::ICommandProcessor
{
  virtual bool processCommand(const char *argv[], int argc);
  void destroy() {}
};

static RendererConsole renderer_console;

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


bool RendererConsole::processCommand(const char *argv[], int argc)
{
  G_UNREFERENCED(&toggle_or_set_bool_arg);
  const TMatrix &itm = get_cam_itm();

  if (argc < 1)
    return false;
  int found = 0;
  WorldRenderer *wr = ((WorldRenderer *)get_world_renderer());
  CONSOLE_CHECK_NAME("render", "reload_cube", 1, 1)
  {
    console::command("render.reload_shaders");
    ((WorldRenderer *)get_world_renderer())->setEnviProbePos(itm.getcol(3));
    ((WorldRenderer *)get_world_renderer())->reloadCube(false);
  }
  CONSOLE_CHECK_NAME("render", "reinit_cube", 1, 2)
  {
    console::command("render.reload_shaders");
    ((WorldRenderer *)get_world_renderer())->reinitCube(atoi(argv[1]), itm.getcol(3));
  }
  CONSOLE_CHECK_NAME("render", "showFrameTimings", 1, 1)
  {
    bool &isDebugLogEnabled = ((WorldRenderer *)get_world_renderer())->isDebugLogFrameTiming;
    isDebugLogEnabled = !isDebugLogEnabled;
    if (isDebugLogEnabled)
    {
      visuallog::setOffset(IPoint2(30, 40)); // To not overdraw fps
      visuallog::setMaxItems(4);
    }
    else
    {
      visuallog::reset();
    }
  }
  CONSOLE_CHECK_NAME("render", "prepareLastClip", 1, 1) { ((WorldRenderer *)get_world_renderer())->prepareLastClip(); }
  CONSOLE_CHECK_NAME("render", "clipmap_invalidate", 1, 2)
  {
    ((WorldRenderer *)get_world_renderer())->invalidateClipmap(argc > 1 ? to_bool(argv[1]) : false);
  }
  CONSOLE_CHECK_NAME("render", "clipmap_dump", 1, 1) { ((WorldRenderer *)get_world_renderer())->dumpClipmap(); }
  CONSOLE_CHECK_NAME("render", "clipmap_texel", 1, 2)
  {
    if (argc > 1)
      ((WorldRenderer *)get_world_renderer())->clipmap->setStartTexelSize(atof(argv[1]));
    console::print_d("texel size %f", ((WorldRenderer *)get_world_renderer())->clipmap->getStartTexelSize());
  }
  CONSOLE_CHECK_NAME("rendinst", "check_occluders", 1, 2)
  {
    extern bool check_occluders;
    toggle_or_set_bool_arg(check_occluders, 1, argc, argv);
  }
  CONSOLE_CHECK_NAME("rendinst", "use_vertical_impostors", 1, 2)
  {
    static bool val = false;
    toggle_or_set_bool_arg(val, 1, argc, argv);
    rendinst::set_billboards_vertical(val);
  }
  CONSOLE_CHECK_NAME("rendinst", "color", 6, 10)
  {
    E3DCOLOR from(clamp(atoi(argv[2]), 0, 255), clamp(atoi(argv[3]), 0, 255), clamp(atoi(argv[4]), 0, 255),
      clamp(atoi(argv[5]), 0, 255));
    E3DCOLOR to = from;
    if (argc >= 10)
      to = E3DCOLOR(clamp(atoi(argv[6]), 0, 255), clamp(atoi(argv[7]), 0, 255), clamp(atoi(argv[8]), 0, 255),
        clamp(atoi(argv[9]), 0, 255));
    else
      console::print_d("second color not defined - use one color");
    if (!rendinst::render::update_rigen_color(argv[1], from, to))
      console::print_d("can not change rendinst color for <%s>", argv[1]);
    else
      console::print_d("changed");
  }
#if DAGOR_DBGLEVEL > 0
  CONSOLE_CHECK_NAME("rendinst", "debug_load_resource_async", 1, 2)
  {
    struct debugRILoadAsyncJob final : public cpujobs::IJob
    {
      int riToLoad, loaded = 0;
      void doJob() override
      {
        DataBlock riBlk;
        dblk::load(riBlk, "content/common/gamedata/ri_list.blk");
        for (int i = 0, e = riBlk.paramCount(); i < e && riToLoad > 0; ++i)
        {
          const char *riName = riBlk.getParamName(i);
          if (rendinst::getRIGenExtraResIdx(riName) != -1)
            continue; // skip already loaded
          RenderableInstanceLodsResource *riRes =
            (RenderableInstanceLodsResource *)::get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(riName), RendInstGameResClassId);
          release_game_resource((GameResource *)riRes);
          riToLoad--;
          loaded++;
        }
        free_unused_game_resources();
      }
      void releaseJob()
      {
        console::print_d("loaded %d RI", loaded);
        delete this;
      }
    };
    int riToLoad = 100;
    if (argc >= 2)
      riToLoad = console::to_int(argv[1]);
    if (dd_file_exist("content/common/gamedata/ri_list.blk"))
    {
      debugRILoadAsyncJob *job = new debugRILoadAsyncJob;
      job->riToLoad = riToLoad;
      cpujobs::add_job(ecs::get_common_loading_job_mgr(), job);
    }
    else
      console::print_d("ri_list.blk is missing");
  }
#endif
  CONSOLE_CHECK_NAME("light", "omni", 1, 2)
  {
    const int TEST_LIGHTS = 16;
    float ofst = 2;
    float boxSize = 10000;
    if (argc > 1)
      boxSize = atof(argv[1]);
    TMatrix box{boxSize};
    for (int i = 0; i < TEST_LIGHTS; ++i)
    {
      float rad = (1 + gfrnd() * 2.f);
      ofst += rad + 0.5f;
      Point3 pos = itm.getcol(3) + itm.getcol(2) * ofst;
      pos.y += 0.5f;
      box.setcol(3, pos);
      ((WorldRenderer *)get_world_renderer())
        ->addOmniLight({pos, Color3(1.f + gfrnd(), 1.f + gfrnd(), 1.f + gfrnd()) * (3 + 2 * gfrnd()), rad, 0.f, box});
    }

    console::print_d("16 lights added");
  }
  CONSOLE_CHECK_NAME("light", "spotlight", 1, 1)
  {
    const int TEST_LIGHTS = 16;
    float ofst = 2;
    for (int i = 0; i < TEST_LIGHTS; ++i)
    {
      float rad = (1 + gfrnd() * 2.f);
      ofst += rad + 0.5f;
      Point3 pos = itm.getcol(3) + itm.getcol(2) * ofst;
      pos.y += 0.5f;
      for (const auto &dir : {Point3{1, 0, 0}, Point3{-1, 0, 0}, Point3{0, 1, 0}, Point3{0, -1, 0}, Point3{0, 0, 1}, Point3{0, 0, -1}})
      {
        ((WorldRenderer *)get_world_renderer())
          ->addSpotLight(
            {pos, Color3(1.f + gfrnd(), 1.f + gfrnd(), 1.f + gfrnd()) * (3 + 2 * gfrnd()), rad, 0.f, dir, 30 * PI / 180, false, false},
            ~0);
      }
    }

    console::print_d("96 lights added");
  }

  CONSOLE_CHECK_NAME("render", "show_gbuffer", 1, 2)
  {
    setDebugGbufferMode(argc > 1 ? argv[1] : nullptr);
    set_up_show_gbuffer_entity();
    console::print("usage: show_gbuffer (%s)", getDebugGbufferUsage().c_str());
  }
  CONSOLE_CHECK_NAME("render", "show_gbuffer_with_vectors", 1, 4)
  {
    const char *mode = argc > 1 ? argv[1] : nullptr;
    int vectorsCount = argc > 2 ? console::to_int(argv[2]) : 1000;
    float vectorsScale = argc > 3 ? console::to_real(argv[3]) : 0.05;
    setDebugGbufferWithVectorsMode(mode, vectorsCount, vectorsScale);
    set_up_show_gbuffer_entity();
    console::print("usage: show_gbuffer_with_vectors (%s) [vectors count = 1000] [vectors scale = 0.05]",
      getDebugGbufferWithVectorsUsage().c_str());
  }
  CONSOLE_CHECK_NAME("render", "show_shadows", 1, 2)
  {
    const eastl::array<eastl::string, 8> options = {
      "static", "csm", "csm_cascades", "contact", "combine", "clouds", "static_cascades", "ssss"};
    int show_shadows = -1;
    if (argc > 1)
    {
      int fnd = -2, next_found = -2;
      for (int i = 0; i < options.size(); ++i)
        if (stricmp(options[i].c_str(), argv[1]) == 0)
        {
          fnd = i;
          break;
        }
        else if (strstr(options[i].c_str(), argv[1]) == options[i].c_str())
        {
          next_found = next_found == -2 || show_shadows == next_found ? i : next_found;
        }
      if (fnd >= -1)
        show_shadows = fnd;
      else if (next_found >= -1)
        show_shadows = next_found;
    }
    set_up_show_shadows_entity(show_shadows);
    String str;
    for (int i = 0; i < options.size(); ++i)
      str.aprintf(128, " %s", options[i]);
    console::print("usage: show_shadows (%s), %d", str.str(), show_shadows);
  }
  CONSOLE_CHECK_NAME("render", "debug_indoor_probes_on_screen", 1, 2)
  {
    static bool render = false;
    if (argc == 1)
      render = !render;
    else
      render = console::to_bool(argv[1]);
    set_up_debug_indoor_probes_on_screen_entity(render);
  }
  CONSOLE_CHECK_NAME("render", "show_tex", 1, DebugTexOverlay::MAX_CONSOLE_ARGS_CNT)
  {
    String str = ((WorldRenderer *)get_world_renderer())->showTexCommand(argv, argc);
    if (!str.empty())
      console::print_d(str.str());
  }
  CONSOLE_CHECK_NAME("render", "show_tonemap", 1, 5)
  {
    String str = ((WorldRenderer *)get_world_renderer())->showTonemapCommand(argv, argc);
    if (!str.empty())
      console::print_d(str.str());
  }
  CONSOLE_CHECK_NAME("app", "prerun_fx", 1, 1) { prerun_fx(); }
  CONSOLE_CHECK_NAME("render", "invalidate_light_probes", 1, 1) { ((WorldRenderer *)get_world_renderer())->invalidateLightProbes(); }
  CONSOLE_CHECK_NAME("render", "invalidate_gi", 1, 2)
  {
    ((WorldRenderer *)get_world_renderer())->invalidateGI(argc > 1 ? to_bool(argv[1]) : false);
  }
  CONSOLE_CHECK_NAME("render", "invalidate_volumelight", 1, 1) { ((WorldRenderer *)get_world_renderer())->invalidateVolumeLight(); }
  CONSOLE_CHECK_NAME("render", "show_boxes", 1, 4)
  {
    String str = ((WorldRenderer *)get_world_renderer())->showBoxesCommand(argv, argc);
    if (!str.empty())
      console::print_d(str.str());
  }
  CONSOLE_CHECK_NAME("clipmap", "manually_trigger_updates", 1, 2)
  {
    Clipmap *clipmap = ((WorldRenderer *)get_world_renderer())->clipmap;
    if (clipmap)
      clipmap->toggleManualUpdate(argc == 2 ? to_bool(argv[1]) : true);
  }
  CONSOLE_CHECK_NAME("clipmap", "trigger_updates", 1, 2)
  {
    Clipmap *clipmap = ((WorldRenderer *)get_world_renderer())->clipmap;
    if (clipmap)
      clipmap->requestManualUpdates(argc == 2 ? to_int(argv[1]) : 1);
  }
  CONSOLE_CHECK_NAME_EX("fx", "spawn", 2, 4, "Spawns visual effect in front of you",
    "<effect_name> [distance from camera in front = 0.0] [is_player = true]")
  {
    int fxType = acesfx::get_type_by_name(argv[1]);
    if (fxType >= 0)
    {
      const float offset = argc > 2 ? to_real(argv[2]) : 0.0f;
      const bool isPlayer = argc > 3 ? to_bool(argv[3]) : true;
      acesfx::start_effect_pos(fxType, itm.getcol(3) + offset * itm.getcol(2), isPlayer);
    }
    else
    {
      console::print_d("Invalid effect name");
    }
  }
  CONSOLE_CHECK_NAME("fx", "spawn_all", 1, 2)
  {
    int mul = argc > 1 ? to_int(argv[1]) : 1;
    acesfx::start_all_effects(itm.getcol(3), 100.f, mul, 10.f);
  }
  CONSOLE_CHECK_NAME("fx", "spawn_rate", 3, 3)
  {
    dafx::ContextId cid = acesfx::get_dafx_context();
    if (cid)
      dafx::set_instance_emission_rate(cid, (dafx::InstanceId)to_int(argv[1]), to_real(argv[2]));
  }
  CONSOLE_CHECK_NAME("fx", "stats", 1, 2)
  {
    dafx::ContextId cid = acesfx::get_dafx_context();
    eastl::string s;
    if (argc > 1)
      acesfx::get_stats_by_fx_name(argv[1], s);
    else
      dafx::get_stats_as_string(cid, s);

    console::print_d("dafx stats:\n%s", s);
  }
  CONSOLE_CHECK_NAME("fx", "set_param_f", 4, 7)
  {
    dafx::ContextId cid = acesfx::get_dafx_context();
    if (cid)
    {
      float v[4];
      v[0] = argc > 3 ? to_real(argv[3]) : 0;
      v[1] = argc > 4 ? to_real(argv[4]) : 0;
      v[2] = argc > 5 ? to_real(argv[5]) : 0;
      v[3] = argc > 6 ? to_real(argv[6]) : 0;

      dafx::set_instance_value_opt(cid, (dafx::InstanceId)to_int(argv[1]), argv[2], v, (argc - 3) * sizeof(float));
    }
  }
  CONSOLE_CHECK_NAME("render", "taa_upsampling_ratio", 2, 2)
  {
    DataBlock blk;
    blk.addBlock("video")->setReal("temporalUpsamplingRatio", console::to_real(argv[1]));
    ::dgs_apply_config_blk(blk, false, false);
    ((WorldRenderer *)get_world_renderer())->applySettingsChanged();
  }
  CONSOLE_CHECK_NAME("render", "dlss_toggle", 1, 1)
  {
    extern bool dlss_toggle;
    dlss_toggle = !dlss_toggle;
    ((WorldRenderer *)get_world_renderer())->applySettingsChanged();
  }
  CONSOLE_CHECK_NAME("render", "toggle_voxel_reflections", 1, 1)
  {
    DataBlock blk;
    bool wasVoxelReflectionsEnabled = ::dgs_get_settings()->getBlockByNameEx("graphics")->getBool("voxelReflections", false);
    blk.addBlock("graphics")->setBool("voxelReflections", !wasVoxelReflectionsEnabled);
    ::dgs_apply_config_blk(blk, false, false);
    FastNameMap changed;
    changed.addNameId("graphics/voxelReflections");
    get_world_renderer()->onSettingsChanged(changed, false);
    console::print_d("Voxel reflections turned %s", wasVoxelReflectionsEnabled ? "off" : "on");
  }
  CONSOLE_CHECK_NAME("render", "anisotropy", 1, 2)
  {
    if (argc < 2)
    {
      int anisotropy = ::dgs_get_settings()->getBlockByNameEx("graphics")->getInt("anisotropy", 1);
      console::print_d("current anisotropy: %d", anisotropy);
    }
    else
    {
      DataBlock blk;
      blk.addBlock("graphics")->setInt("anisotropy", atoi(argv[1]));
      ::dgs_apply_config_blk(blk, false, false);
      FastNameMap changed;
      changed.addNameId("graphics/anisotropy");
      get_world_renderer()->onSettingsChanged(changed, false); // won't cause videomode change anyway
    }
  }
  CONSOLE_CHECK_NAME("render", "on_setting_change", 1, 2)
  {
    if (argc < 2)
    {
      console::print_d("usage: on_setting_change <setting name>."
                       " Causes setting with specific name to be reloaded (from current settings)");
    }
    else
    {
      FastNameMap changed;
      changed.addNameId(argv[1]);
      get_world_renderer()->onSettingsChanged(changed, false); // won't cause videomode change anyway
    }
  }
  CONSOLE_CHECK_NAME("skies", "switchVolumetricAndPanoramicClouds", 1, 1)
  {
    ((WorldRenderer *)get_world_renderer())->switchVolumetricAndPanoramicClouds();
  }
#if DAGOR_DBGLEVEL > 0
  CONSOLE_CHECK_NAME("render", "dump_occlusion", 1, 1)
  {
    if (current_occlusion)
    {
      const char *name = "occlusion.bin";
      FullFileSaveCB cb(name);
      save_occlusion(cb, *current_occlusion);
      console::print_d("saved to: (%s)", name);
    }
    else
      console::print_d("no occlusion!");
  }
#endif

  CONSOLE_CHECK_NAME("render", "toggleWireFrame", 1, 1) { grs_draw_wire = !grs_draw_wire; }
  CONSOLE_CHECK_NAME("render", "toggleFeatures", 1, 3)
  {
    FeatureRenderFlagMask mask;
    if (argc > 2)
      mask = parse_render_features(argv[1]);
    if (mask.any())
      wr->toggleFeatures(mask, to_bool(argv[2]));

    console::print_d("Current features %s", render_features_to_string(wr->getFeatures()));
  }
#if DAGOR_DBGLEVEL > 0
  CONSOLE_CHECK_NAME("render", "send_event_dynamicQuality", 1, 2)
  {
    if (argc == 2)
    {
      String name(argv[1]);
      g_entity_mgr->broadcastEventImmediate(DynamicQualityChangeEvent(name));
      console::print_d("send DynamicQualityChangeEvent(%s)", argv[1]);
    }
    else
      console::print("usage: send_event_dynamicQuality [event name from blk]");
  }
  CONSOLE_CHECK_NAME("render", "verify_rendinst", 1, 2)
  {
    if (argc == 2)
    {
      wr->setRIVerifyDistance(console::to_real(argv[1]));
    }
    else
    {
      wr->setRIVerifyDistance(-1.f);
      console::print("usage: verify_rendinst [distance to camera]");
    }
  }
  CONSOLE_CHECK_NAME("rendinst", "verify_heavy_shaders_on_far_lods", 1, 2)
  {
    int w, h;
    wr->getRenderingResolution(w, h);
    int pixelsHistogram = argc > 1 ? console::to_int(argv[1]) : w;

    const Point3_vec4 viewPos = ::grs_cur_view.itm.getcol(3);
    vec4f pos_fov = v_make_vec4f(viewPos.x, viewPos.y, viewPos.z, get_camera_fov());
    rendinst::render::validateFarLodsWithHeavyShaders(wr->rendinst_main_visibility, pos_fov, 1.f * w / pixelsHistogram);
  }
  CONSOLE_CHECK_NAME("render", "verify_all_rendinst", 1, 3)
  {
    console::print("see log for info");
    int w, h;
    wr->getRenderingResolution(w, h);
    int pixelsHistogram = argc > 1 ? console::to_int(argv[1]) : w;
    int binCount = argc > 2 ? console::to_int(argv[2]) : 100;
    eastl::vector<eastl::vector_map<eastl::string_view, int>> name_count_bins(binCount);

    const Point3_vec4 viewPos = ::grs_cur_view.itm.getcol(3);
    vec4f pos_fov = v_make_vec4f(viewPos.x, viewPos.y, viewPos.z, get_camera_fov());

    rendinst::render::collectPixelsHistogramRIGenExtra(wr->rendinst_main_visibility, pos_fov, 1.f * w / pixelsHistogram,
      name_count_bins);
    debug("pixel view");
    for (int bin = 0; bin < binCount; bin++)
    {
      int minPix = pixelsHistogram * bin / binCount;
      int maxPix = pixelsHistogram * (bin + 1) / binCount;
      debug("Range : %d - %d", minPix, maxPix);
      eastl::vector_map<int, eastl::string_view> count_name_map; // for sort
      for (const auto &it : name_count_bins[bin])
        count_name_map[-it.second] = it.first; //-it.second from max to min
      for (const auto &it : count_name_map)
        debug("RI name %s %d", it.second.data(), -it.first);
    }
  }
#endif
  CONSOLE_CHECK_NAME_EX("render", "profile_stcode", 1, 1,
    "On the first execution the command starts profiling. On the others it dumps profiling results to a log.", "")
  {
    dump_exec_stcode_time();
  }
  CONSOLE_CHECK_NAME("render", "get_rendering_res", 1, 1)
  {
    int w = 0, h = 0;
    wr->getRenderingResolution(w, h);
    console::print_d("rendering resolution: %dx%d", w, h);
  }
  CONSOLE_CHECK_NAME("render", "get_device_name", 1, 1) { console::print_d(d3d::get_device_name()); }
  CONSOLE_CHECK_NAME("render", "forceStaticScaleResolutionOff", 1, 2)
  {
    if (argc > 1)
    {
      const bool force = console::to_bool(argv[1]);
      wr->forceStaticResolutionScaleOff(force);
    }
  }
  CONSOLE_CHECK_NAME("render", "printResolutionScaleInfo", 1, 1) { wr->printResolutionScaleInfo(); }
  CONSOLE_CHECK_NAME("render", "mobile_terrain_quality", 1, 2)
  {
    if (argc > 1)
    {
      const int quality = console::to_int(argv[1]);
      wr->setMobileTerrainQuality((WorldRenderer::MobileTerrainQuality)quality);
    }
  }
  CONSOLE_CHECK_NAME("render", "get_managed_res_refcount", 2, 2)
  {
    D3DRESID resId = get_managed_res_id(argv[1]);
    int count = get_managed_res_refcount(resId);
    console::print_d("%s with resId = %d has %d references", argv[1], resId, count);
  }
  CONSOLE_CHECK_NAME("render", "printFeatureFlags", 1, 1)
  {
    console::print_d("Feature flags: %s", render_features_to_string(wr->getFeatures()));
  }
  CONSOLE_CHECK_NAME("biome_query", "queryPos", 1, 5) { biome_query::console_query_pos(argv, argc); }
  CONSOLE_CHECK_NAME("biome_query", "queryCameraPos", 1, 2) { biome_query::console_query_camera_pos(argv, argc, ::grs_cur_view.itm); }
  return found;
}

void init_renderer_console() { add_con_proc(&renderer_console); }
void close_renderer_console() { del_con_proc(&renderer_console); }
