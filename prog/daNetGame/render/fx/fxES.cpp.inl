// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <math/dag_Point2.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_localConv.h>
#include <osApiWrappers/dag_critSec.h>
#include <osApiWrappers/dag_miscApi.h>
#include <memoryProfiler/dag_memoryStat.h>
#include <startup/dag_globalSettings.h>
#include <perfMon/dag_statDrv.h>
#include <math/dag_mathUtils.h> // check_nan
#include <math/dag_hlsl_floatx.h>
#include <daFx/dafx_gravity_zone.hlsli>
#include <rendInst/rendInstFxDetail.h>

#include "render/fx/fx.h"
#include "render/fx/fxRenderTags.h"
#include <landMesh/lmeshManager.h>
#include "render/fx/effectManager.h"
#include <scene/dag_occlusion.h>
#include <math/dag_color.h>
#include <math/dag_frustum.h>
#include <math/random/dag_random.h>
#include "render/fx/renderTrans.h"
#include <fx/dag_baseFxClasses.h>
#include <dafxSystemDesc.h>
#include <perfMon/dag_cpuFreq.h>
#include <generic/dag_tabExt.h>
#include <generic/dag_staticTab.h>
#include <shaders/dag_shaderBlock.h>
#include <3d/dag_render.h>
#include <render/dag_cur_view.h>
#include <supp/dag_prefetch.h>
#include <util/dag_convar.h>
#include <util/dag_oaHashNameMap.h>
#include <EASTL/vector_set.h>
#include <util/dag_hash.h>
#include <gameRes/dag_gameResHooks.h>
#include <gameRes/dag_stdGameResId.h>
#include <render/world/global_vars.h>

#include <render/wind/ambientWind.h>
#include <render/wind/fxWindHelper.h>
#include <daECS/net/time.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>
#include <ecs/render/resPtr.h>
#include <debug/dag_debug3d.h>
#include <ska_hash_map/flat_hash_map2.hpp>
#include <hash/mum_hash.h>
#include <hudprim/dag_hudPrimitives.h>
#include <render/debugMultiTextOverlay.h>

#include <memory/dag_memStat.h>

#define INSIDE_RENDERER 1
#include <render/world/private_worldRenderer.h> // TODO: refactor DNG occlusion instead
#include <render/world/bvh.h>
#include <drv/3d/dag_texture.h>

#define debug(...) logmessage(_MAKE4C('_FX_'), __VA_ARGS__)

CONSOLE_BOOL_VAL("dafx", show_bboxes, false);
CONSOLE_BOOL_VAL("dafx", disable_culling, false);
CONSOLE_BOOL_VAL("dafx", disable_sorting, false);
CONSOLE_BOOL_VAL("dafx", disable_render, false);
CONSOLE_BOOL_VAL("dafx", disable_simulation, false);
CONSOLE_BOOL_VAL("dafx", disable_occlusion, false);
CONSOLE_BOOL_VAL("fx", debug_fx, false);
CONSOLE_BOOL_VAL("fx", debug_fx_detailed, false);
CONSOLE_BOOL_VAL("fx", debug_fx_one_point_radius, false);
CONSOLE_BOOL_VAL("fx", debug_fx_sub, false);


extern void dafx_flowps2_register_cpu_overrides(dafx::ContextId ctx);
extern void dafx_modfx_register_cpu_overrides(dafx::ContextId ctx);
extern void dafx_sparks_register_cpu_overrides(dafx::ContextId ctx);
extern void dafx_modfx_register_cpu_overrides_adv(dafx::ContextId ctx);

extern void dafx_flowps2_set_context(dafx::ContextId ctx);
extern void dafx_sparksfx_set_context(dafx::ContextId ctx);
extern void dafx_modfx_set_context(dafx::ContextId ctx);
extern void dafx_compound_set_context(dafx::ContextId ctx);

extern void wait_start_fx_job_done(bool finish_async_update = false);

extern void dafx_set_modfx_convert_rtag_cb(int (*convert_rtag_cb)(int));

// Container for values
static dag::RelocatableFixedVector<GravityZoneDescriptor, GRAVITY_ZONE_MAX_COUNT, false> dafx_gravity_zone_container;
// Pointer stored in dafx library
extern GravityZoneDescriptor_buffer dafx_gravity_zone_buffer;


int modfx_convert_rtag_disable_highres(int rtag)
{
  if (rtag == dafx_ex::RTAG_HIGHRES)
    rtag = dafx_ex::RTAG_LOWRES;
  return rtag;
}

static void dafx_modfx_disable_highres_rtag() { dafx_set_modfx_convert_rtag_cb(modfx_convert_rtag_disable_highres); }


namespace acesfx
{
int landCrashFireEffectTime = 60;
int landCrashFireEffectTimeForTanks = 20;
static int render_particles_to_gbuffer_block = -1;
static DataBlock g_fx_blk;
static dafx::ContextId g_dafx_ctx;
static dafx::CullingId g_dafx_cull;
static dafx::CullingId g_dafx_cull_fom;
static dafx::CullingId g_dafx_cull_bvh;
static TMatrix4 g_main_cull_tm = TMatrix4::IDENT;
static TMatrix4 g_fom_cull_tm = TMatrix4::IDENT;
static TMatrix4 g_bvh_cull_tm = TMatrix4::IDENT;
static TMatrix4 g_globtm_prev = TMatrix4::IDENT;
static Point3 prev_world_view_pos = Point3::ZERO;
static Point3 camera_velocity = Point3::ZERO;
static bool g_globtm_prev_valid = false;
static FxRenderTargetOverride g_rt_override = FX_RT_OVERRIDE_DEFAULT;
static int dafx_is_water_proj_var_id = -1;
bool start_allowed = true;
static float g_default_mip_bias = 0.f;

// for disabling unnecessary slot clears on forward rendering
//  1. we have global tex bindings(LUT tex for ex.), that are broken by slot clears
//  2. we can't conflict with RT/DS & slots as we use only backbuffer (clears are pointless)
static bool g_relaxed_tex_slots_clear = false;

// name map must be persistent thought whole game, not just session
// game modules often grab fxId on start and cache it, but ids could
// migrate from session-to session (on-demand fx loading)
static NameMap effectNames;
static eastl::vector<EffectManager *> fx_managers;

StaticTab<int, ERT_TAG_COUNT> particles_block_id;

ECS_REGISTER_EVENT(StartEffectEvent);
ECS_REGISTER_EVENT(StartEffectPosNormEvent);

eastl::unique_ptr<HudPrimitives> fx_labels_immediate_context_debug;

const char *get_fxres_name(const char *name)
{
  const DataBlock *blk = g_fx_blk.getBlockByName(name);
  return blk ? blk->getStr("fx", NULL) : name;
}

bool init_new_manager(int fx_type)
{
  const char *name = effectNames.getName(fx_type);
  G_ASSERT_RETURN(name, false);

  DataBlock *blk = g_fx_blk.getBlockByName(name);
  const char *effect_name = blk ? blk->getStr("fx", NULL) : name;

  if (fx_type >= fx_managers.size())
    fx_managers.resize(fx_type + 1, nullptr);

  fx_managers[fx_type] = new EffectManager();
  fx_managers[fx_type]->init(name, blk, blk ? blk->getBool("ground_effect", true) : true, effect_name);

  return true;
}

void use_relaxed_tex_slot_clears() { g_relaxed_tex_slots_clear = true; }

bool init_dafx(bool gpu_sim)
{
  dafx_modfx_disable_highres_rtag(); // DNG has no combined resolution for modfx, but sparks are always highres

  dafx::Config cfg;
  cfg.low_prio_jobs = true;                                  // To give prio visibility jobs
  cfg.max_async_threads = threadpool::get_num_workers() - 1; // 1 is reserved for PUFD
  cfg.sim_lods_enabled = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("fxSimLodsEnabled", true);
  cfg.gen_sim_lods_for_invisible_instances =
    dgs_get_settings()->getBlockByNameEx("graphics")->getBool("fxGenLodsForInvisibleFx", true);
#if _TARGET_C1 | _TARGET_XBOX
  cfg.max_async_threads *= 2; // Double # of jobs to reduce chances of long-running jobs being scheduled out
#endif
  g_dafx_ctx = dafx::create_context(cfg);
  if (!g_dafx_ctx)
  {
    logerr("dafx create_context - failed");
    return false;
  }

  g_globtm_prev_valid = false;

  if (gpu_sim)
  {
    debug("dafx: GPU simulation mode");
  }
  else
  {
    debug("dafx: CPU simulation mode");
    dafx_flowps2_register_cpu_overrides(g_dafx_ctx);
    dafx_modfx_register_cpu_overrides(g_dafx_ctx);
    // FIXME: android/iOS GPU crash due to random OOB in dafx_emission/simulate_shader_cb
#if _TARGET_ANDROID | _TARGET_IOS
    dafx_sparks_register_cpu_overrides(g_dafx_ctx);
    dafx_modfx_register_cpu_overrides_adv(g_dafx_ctx);
#endif
  }

  dafx_flowps2_set_context(g_dafx_ctx);
  dafx_sparksfx_set_context(g_dafx_ctx);
  dafx_modfx_set_context(g_dafx_ctx);
  dafx_compound_set_context(g_dafx_ctx);

  eastl::vector<dafx::CullingDesc> descs;
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_LOWRES], dafx::SortingType::BACK_TO_FRONT));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_HIGHRES], dafx::SortingType::BACK_TO_FRONT));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_GBUFFER], dafx::SortingType::BY_SHADER));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_HAZE], dafx::SortingType::NONE));
  descs.push_back(dafx::CullingDesc("distortion", dafx::SortingType::NONE));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_VOLMEDIA], dafx::SortingType::NONE));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_VOLFOG_INJECTION], dafx::SortingType::NONE));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_ATEST], dafx::SortingType::BY_SHADER));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_WATER_PROJ], dafx::SortingType::BACK_TO_FRONT));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_THERMAL], dafx::SortingType::BACK_TO_FRONT));
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_UNDERWATER], dafx::SortingType::BACK_TO_FRONT));
  g_dafx_cull = dafx::create_culling_state(g_dafx_ctx, descs);

  descs.clear();
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_FOM], dafx::SortingType::NONE));
  g_dafx_cull_fom = dafx::create_culling_state(g_dafx_ctx, descs);

  descs.clear();
  descs.push_back(dafx::CullingDesc(render_tags[ERT_TAG_BVH], dafx::SortingType::NONE));
  g_dafx_cull_bvh = dafx::create_culling_state(g_dafx_ctx, descs);

  set_rt_override(g_rt_override);

  fxwindhelper::load_fx_wind_curve_params(::dgs_get_game_params());

  return true;
}

dafx::ContextId get_dafx_context() { return g_dafx_ctx; }

dafx::CullingId get_cull_id() { return g_dafx_cull; }

dafx::CullingId get_cull_fom_id() { return g_dafx_cull_fom; }

dafx::CullingId get_cull_bvh_id() { return g_dafx_cull_bvh; }

void set_rt_override(FxRenderTargetOverride v)
{
  if (v != FX_RT_OVERRIDE_DEFAULT)
    debug(v == FX_RT_OVERRIDE_LOWRES ? "fx: forced lowres" : "fx: forced highres");

  g_rt_override = v;
  dafx::remap_culling_state_tag(g_dafx_ctx, g_dafx_cull, "lowres", g_rt_override == FX_RT_OVERRIDE_HIGHRES ? "highres" : "lowres");
  dafx::remap_culling_state_tag(g_dafx_ctx, g_dafx_cull, "highres", g_rt_override == FX_RT_OVERRIDE_LOWRES ? "lowres" : "highres");
}

FxRenderTargetOverride get_rt_override() { return g_rt_override; }

void set_quality_mask(FxQuality fx_quality)
{
  G_ASSERT_RETURN(g_dafx_ctx, );

  dafx::Config cfg = dafx::get_config(g_dafx_ctx);
  cfg.qualityMask = fx_quality;
  dafx::set_config(g_dafx_ctx, cfg);
}

void set_default_mip_bias(float mip_bias)
{
  G_ASSERT_RETURN(g_dafx_ctx, );

  if (mip_bias != g_default_mip_bias)
  {
    dafx::reset_samplers_cache(g_dafx_ctx);
    g_default_mip_bias = mip_bias;
  }
}

FxResolutionSetting getFxResolutionSetting()
{
  eastl::string_view fxTargetCfg = dgs_get_settings()->getBlockByNameEx("graphics")->getStr("fxTarget", "lowres");
  FxResolutionSetting fxRes = FX_LOW_RESOLUTION;
  if (fxTargetCfg == "medres")
    fxRes = FX_MEDIUM_RESOLUTION;
  else if (fxTargetCfg == "highres")
    fxRes = FX_HIGH_RESOLUTION;
  else if (!(fxTargetCfg == "lowres"))
    logwarn("Invalid graphics/fxTarget settings string. Using \"lowres\"");
  return fxRes;
}

FxQuality getFxQualityMask()
{
  FxResolutionSetting fxRes = getFxResolutionSetting();
  FxQuality fxQuality = FX_QUALITY_LOW;
  if (fxRes == FX_MEDIUM_RESOLUTION)
    fxQuality = FX_QUALITY_MEDIUM;
  else if (fxRes == FX_HIGH_RESOLUTION)
    fxQuality = FX_QUALITY_HIGH;
  return fxQuality;
}

void setDepthTex(const BaseTexture *tex)
{
  if (!tex)
    return;

  TextureInfo ti;
  tex->getinfo(ti);
  Point2 size(ti.w, ti.h);
  Point2 sizeRcp(safeinv((float)ti.w), safeinv((float)ti.h));
  IPoint2 offset(0, 0); // In case of VR side-by-side rendering, it should properly set for right eye.
                        // Also for collision, left-eye size of `depth_size_for_collision` should be used.

  dafx::set_global_value(g_dafx_ctx, "depth_size", &size, 8);
  dafx::set_global_value(g_dafx_ctx, "depth_size_rcp", &sizeRcp, 8);
  dafx::set_global_value(g_dafx_ctx, "depth_size_for_collision", &size, 8);
  dafx::set_global_value(g_dafx_ctx, "depth_size_rcp_for_collision", &sizeRcp, 8);
  dafx::set_global_value(g_dafx_ctx, "depth_tci_offset", &offset, 8);

  dafx::flush_global_values(g_dafx_ctx);
}

void setNormalsTex(const BaseTexture *tex)
{
  if (!tex)
    return;

  TextureInfo ti;
  tex->getinfo(ti);
  Point2 size(ti.w, ti.h);
  Point2 sizeRcp(safeinv((float)ti.w), safeinv((float)ti.h));
  dafx::set_global_value(g_dafx_ctx, "normals_size", &size.x, 8);
  dafx::set_global_value(g_dafx_ctx, "normals_size_rcp", &sizeRcp.x, 8);
}

void setSkyParams(const Point3 &sun_dir, const Color3 &sun_color, const Color3 &sky_color)
{
  dafx::set_global_value(g_dafx_ctx, "from_sun_direction", &sun_dir, 12);
  dafx::set_global_value(g_dafx_ctx, "sun_color", &sun_color, 12);
  dafx::set_global_value(g_dafx_ctx, "sky_color", &sky_color, 12);
}

void setWindParams(float wind_strength, Point2 wind_dir)
{
  Point2 windDir = wind_dir;
  float windDirL = length(windDir);
  if (windDirL > FLT_EPSILON)
    windDir /= windDirL;

  const float windScroll = get_sync_time();
  const float windSpeed = AmbientWind::beaufort_to_meter_per_second(fxwindhelper::apply_wind_strength_curve(wind_strength));
  dafx::set_global_value(g_dafx_ctx, "wind_dir", &windDir, 8);
  dafx::set_global_value(g_dafx_ctx, "wind_power", &windSpeed, 4);
  dafx::set_global_value(g_dafx_ctx, "wind_scroll", &windScroll, 4);
}


void push_gravity_zone(GravityZoneBuffer &buffer,
  const TMatrix &transform,
  const Point3 &size,
  uint32_t shape,
  uint32_t type,
  float sq_distance_to_camera,
  bool is_important)
{
  G_ASSERT(buffer.capacity() == GRAVITY_ZONE_MAX_COUNT);
  if (buffer.size() < GRAVITY_ZONE_MAX_COUNT)
  {
    buffer.emplace_back(transform, size, shape, type, sq_distance_to_camera, is_important);
    return;
  }

  GravityZoneWithDistanceToCamera *farGravityZone = buffer.begin();

  for (GravityZoneWithDistanceToCamera &grz : buffer)
  {
    if ( // Select not important one, if we can
      farGravityZone->isImportant && !grz.isImportant
      // if important is the same, select which is more far
      || farGravityZone->isImportant == grz.isImportant && grz.sqDistanceToCamera > farGravityZone->sqDistanceToCamera)
      farGravityZone = &grz;
  }

  if (farGravityZone->isImportant)
  {
    LOGERR_ONCE("Too many important gravity zones. Max count=%d", GRAVITY_ZONE_MAX_COUNT);

    if (!is_important)
      return;
  }

  // Replace farGravityZone if current is more important or closer then farGravityZone
  if (is_important && !farGravityZone->isImportant || farGravityZone->sqDistanceToCamera > sq_distance_to_camera)
    *farGravityZone = GravityZoneWithDistanceToCamera{transform, size, shape, type, sq_distance_to_camera, is_important};
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void create_gravity_zone_buffer_es(const ecs::Event &, UniqueBufHolder &dafx_gravity_zone_buffer_gpu)
{
  dafx_gravity_zone_buffer_gpu =
    dag::buffers::create_persistent_sr_structured(sizeof(GravityZoneDescriptor), GRAVITY_ZONE_MAX_COUNT, "dafx_gravity_zone_buffer");
}


template <typename Callable>
static void update_gravity_zone_buffer_ecs_query(Callable callable);


void set_gravity_zones(GravityZoneBuffer &buffer)
{
  unsigned gravity_zone_count = buffer.size();
  G_ASSERT(gravity_zone_count <= GRAVITY_ZONE_MAX_COUNT);
  dafx::set_global_value(g_dafx_ctx, "gravity_zone_count", &gravity_zone_count, sizeof(gravity_zone_count));

  dafx_gravity_zone_container.clear();

  if (gravity_zone_count == 0)
    return;

  for (const auto &e : buffer)
    dafx_gravity_zone_container.emplace_back(e.transform, e.size, e.shape, e.type);

  dafx_gravity_zone_buffer = dafx_gravity_zone_container.data();

  update_gravity_zone_buffer_ecs_query([](UniqueBufHolder &dafx_gravity_zone_buffer_gpu) {
    dafx_gravity_zone_buffer_gpu->updateData(0, dafx_gravity_zone_container.size() * sizeof(dafx_gravity_zone_container[0]),
      dafx_gravity_zone_container.data(), VBLOCK_WRITEONLY);
  });
}


void init(const DataBlock &blk)
{
  memstat::clear_allocations_by("_clone"); // don't care about effects allocations in early_load_resources_in_one_batch()

  for (EffectManager *mgr : fx_managers)
    delete mgr;
  fx_managers.clear();

  g_fx_blk.reset(); // FIXME: DataBlock might (logically) leaks memory on str values unless reset() called
  g_fx_blk = blk;

  particles_block_id.push_back(ShaderGlobal::getBlockId("dynamic_scene_trans"));
  for (int i = (int)ERT_TAG_NORMAL + 1; i < countof(render_tags); ++i)
  {
    String name(128, "dynamic_scene_trans_%s", render_tags[i]);
    particles_block_id.push_back(ShaderGlobal::getBlockId(name));
  }
  particles_block_id[ERT_TAG_ATEST] = ShaderGlobal::getBlockId("dynamic_scene_trans");
  particles_block_id[ERT_TAG_VOLFOG_INJECTION] = ShaderGlobal::getBlockId("dynamic_scene_trans");
  render_particles_to_gbuffer_block = ShaderGlobal::getBlockId("render_particles_to_gbuffer_block");

  landCrashFireEffectTime = blk.getInt("landCrashFireEffectTime", 60);
  landCrashFireEffectTimeForTanks = blk.getInt("landCrashFireEffectTimeForTanks", 20);
  dafx_is_water_proj_var_id = ::get_shader_variable_id("dafx_is_water_proj");

#if DAGOR_DBGLEVEL > 0
  // extra indices for hud primitives (like polys, etc)
  int hud_extra = blk.getInt("main_hud_extra", 1024);

  // num quads for stdgui primitives (used for texts)
  int imm_std_elems = blk.getInt("imm_gui_quads", 4096 * 2);
  int imm_hud_elems = blk.getInt("imm_hud_quads", 4096 * 2);

  fx_labels_immediate_context_debug = eastl::make_unique<HudPrimitives>();
  bool res = fx_labels_immediate_context_debug->createGuiBuffer(imm_std_elems, hud_extra);
  res &= fx_labels_immediate_context_debug->createHudBuffer(imm_hud_elems, hud_extra);

  G_ASSERT(res);
#endif
}

void shutdown()
{
  debug("fx shutdown");

  fx_labels_immediate_context_debug.reset();

  for (EffectManager *mgr : fx_managers)
    delete mgr;
  fx_managers.clear();

  term_dafx();

  particles_block_id.clear();
}

void term_dafx()
{
  if (!g_dafx_ctx)
    return;

  dafx::release_all_systems(g_dafx_ctx);
  dafx::destroy_culling_state(g_dafx_ctx, g_dafx_cull);
  dafx::destroy_culling_state(g_dafx_ctx, g_dafx_cull_fom);
  dafx::destroy_culling_state(g_dafx_ctx, g_dafx_cull_bvh);
  dafx::release_context(g_dafx_ctx);
  g_dafx_ctx = dafx::ContextId();
  dafx_flowps2_set_context(g_dafx_ctx);
  dafx_sparksfx_set_context(g_dafx_ctx);
  dafx_modfx_set_context(g_dafx_ctx);
  dafx_compound_set_context(g_dafx_ctx);
}

static EffectManager::DebugMode get_effect_manager_debug_mode()
{
  int debugMode = 0;
  if (debug_fx_detailed.get())
    debugMode |= static_cast<int>(EffectManager::DebugMode::DETAILED);
  if (debug_fx_one_point_radius.get())
    debugMode |= static_cast<int>(EffectManager::DebugMode::ONE_POINT_RADIUS);
  if (debug_fx_sub.get())
    debugMode |= static_cast<int>(EffectManager::DebugMode::SUBFX_INFO);
  return static_cast<EffectManager::DebugMode>(debugMode);
}

void draw_debug_opt(const TMatrix4 &glob_tm)
{
  dafx::render_debug_opt(g_dafx_ctx);

  if (debug_fx.get() != 0 || debug_fx_detailed.get() != 0 || debug_fx_one_point_radius.get() || debug_fx_sub.get())
  {
    begin_draw_cached_debug_lines(false, false);

    const auto mode = get_effect_manager_debug_mode();
    for (auto &fx_manager : fx_managers)
      if (fx_manager)
        fx_manager->drawDebug(mode);

    if (mode == EffectManager::DebugMode::DEFAULT && fx_labels_immediate_context_debug)
    {
      fx_labels_immediate_context_debug->resetFrame();
      dag::Vector<Point3, framemem_allocator> positions;
      dag::Vector<eastl::string_view, framemem_allocator> names;
      for (EffectManager *mgr : fx_managers)
        if (mgr)
          mgr->getDebugInfo(positions, names);

      DebugMultiTextOverlay cfg;
      draw_debug_multitext_overlay(positions, names, fx_labels_immediate_context_debug.get(), glob_tm, cfg);
    }

    end_draw_cached_debug_lines();
  }
}

void start_update_prepare(float dt, const Driver3dPerspective &persp, int targetWidth, int targetHeight)
{
  TIME_PROFILE(acesfx_start_update);

  {
    TIME_PROFILE_DEV(fx_managers_update);
    for (int i = 0; i < fx_managers.size(); i++)
      if (fx_managers[i])
        fx_managers[i]->update(dt);
  }

  dafx::flush_command_queue(g_dafx_ctx);

#if DAGOR_DBGLEVEL > 0
  uint32_t dbgf = 0;

  dbgf |= show_bboxes.get() ? dafx::DEBUG_SHOW_BBOXES : 0;
  dbgf |= disable_culling.get() ? dafx::DEBUG_DISABLE_CULLING : 0;
  dbgf |= disable_sorting.get() ? dafx::DEBUG_DISABLE_SORTING : 0;
  dbgf |= disable_render.get() ? dafx::DEBUG_DISABLE_RENDER : 0;
  dbgf |= disable_simulation.get() ? dafx::DEBUG_DISABLE_SIMULATION : 0;
  dbgf |= disable_occlusion.get() ? dafx::DEBUG_DISABLE_OCCLUSION : 0;

  dafx::set_debug_flags(g_dafx_ctx, dbgf);
#endif
  Point4 znZfar = Point4(persp.zn, persp.zf, safediv(1.f, persp.zf), safediv(persp.zf - persp.zn, persp.zn * persp.zf));
  Point2 targetSize(targetWidth, targetHeight);
  Point2 targetSizeRcp(safeinv((float)targetWidth), safeinv((float)targetHeight));

  dafx::set_global_value(g_dafx_ctx, "proj_hk", &persp.hk, 4);
  dafx::set_global_value(g_dafx_ctx, "target_size", &targetSize, 8);
  dafx::set_global_value(g_dafx_ctx, "target_size_rcp", &targetSizeRcp, 8);
  dafx::set_global_value(g_dafx_ctx, "zn_zfar", &znZfar, 16);
  dafx::set_global_value(g_dafx_ctx, "dt", &dt, 4);
}

static void setup_cam(float dt, const TMatrix &view_itm)
{
  if (dt > FLT_EPSILON)
    camera_velocity = (view_itm.getcol(3) - prev_world_view_pos) / dt;

  prev_world_view_pos = view_itm.getcol(3);

  dafx::set_global_value(g_dafx_ctx, "world_view_pos", &view_itm.getcol(3), 12);
  dafx::set_global_value(g_dafx_ctx, "view_dir_x", &view_itm.getcol(0), 12);
  dafx::set_global_value(g_dafx_ctx, "view_dir_y", &view_itm.getcol(1), 12);
  dafx::set_global_value(g_dafx_ctx, "view_dir_z", &view_itm.getcol(2), 12);
  dafx::set_global_value(g_dafx_ctx, "camera_velocity", &camera_velocity, 12);
}

void start_update(float dt)
{
  wait_start_fx_job_done();
  setup_cam(dt, static_cast<const WorldRenderer *>(get_world_renderer())->getCameraViewItm());
  dafx::start_update(g_dafx_ctx, dt);
}

void finish_update(const TMatrix4 &tm)
{
  TIME_PROFILE(acesfx_finish_update);

  bool finished = dafx::update_finished(g_dafx_ctx);

  dafx::set_global_value(g_dafx_ctx, "globtm", &tm, 64);
  dafx::set_global_value(g_dafx_ctx, "globtm_sim", g_globtm_prev_valid ? &g_globtm_prev : &tm, 64);
  g_globtm_prev = tm;
  g_globtm_prev_valid = true;

  dafx::finish_update(g_dafx_ctx);

  if (finished)
    return;

  auto occlusion_get_f = [] {
    auto &wr = *static_cast<WorldRenderer *>(get_world_renderer());
    wr.waitMainVisibility();
    return current_occlusion;
  };

  dafx::update_culling_state(g_dafx_ctx, g_dafx_cull, Frustum(g_main_cull_tm), ::grs_cur_view.pos, occlusion_get_f);
  dafx::update_culling_state(g_dafx_ctx, g_dafx_cull_fom, Frustum(g_fom_cull_tm), ::grs_cur_view.pos);
  if (is_bvh_enabled())
    dafx::update_culling_state(g_dafx_ctx, g_dafx_cull_bvh, Frustum(g_bvh_cull_tm), ::grs_cur_view.pos);
  dafx::before_render(g_dafx_ctx);
}

void prepare_main_culling(const TMatrix4 &tm) { g_main_cull_tm = tm; }

void prepare_fom_culling(const TMatrix4 &tm) { g_fom_cull_tm = tm; }

void prepare_bvh_culling(const TMatrix4 &tm) { g_bvh_cull_tm = tm; }

void before_reset() { dafx::before_reset_device(g_dafx_ctx); }

void after_reset() { dafx::after_reset_device(g_dafx_ctx); }

void clear_tex_slots()
{
  int startSlot = 0;
  // clear only needed slots, when we sure that conflict can't happen
  if (g_relaxed_tex_slots_clear)
    startSlot = dafx::get_config(g_dafx_ctx).texture_start_slot;

  for (int i = startSlot, ie = dafx::get_config(g_dafx_ctx).max_res_slots; i < ie; ++i)
  {
    d3d::set_tex(STAGE_PS, i, nullptr, false);
    d3d::set_tex(STAGE_VS, i, nullptr, false);
  }
}

void renderToGbuffer(int global_frame_id)
{
  clear_tex_slots();

  ShaderGlobal::setBlock(global_frame_id, ShaderGlobal::LAYER_FRAME);
  ShaderGlobal::setBlock(render_particles_to_gbuffer_block, ShaderGlobal::LAYER_SCENE);

  TIME_D3D_PROFILE(acesfx__render_to_gbuffer);
  dafx::render(g_dafx_ctx, g_dafx_cull, render_tags[ERT_TAG_GBUFFER], g_default_mip_bias);

  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
}

static bool renderTransBase(FxRenderGroup group, dafx::CullingId cull_id)
{
  clear_tex_slots();

  const int blockId = particles_block_id[0];
  STATE_GUARD(ShaderGlobal::setBlock(VALUE, ShaderGlobal::LAYER_SCENE), blockId, -1);

  TIME_D3D_PROFILE(acesfx__render_trans);

  bool use_default_cull = cull_id == dafx::CullingId();
  bool r = false;
  if (group == FX_RENDER_GROUP_LOWRES)
  {
    TIME_D3D_PROFILE(lowres);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_LOWRES], g_default_mip_bias);
  }
  else if (group == FX_RENDER_GROUP_HIGHRES)
  {
    TIME_D3D_PROFILE(highres);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_HIGHRES], g_default_mip_bias);
  }
  else if (group == FX_RENDER_GROUP_THERMAL)
  {
    TIME_D3D_PROFILE(thermal);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_THERMAL], g_default_mip_bias);
  }
  else if (group == FX_RENDER_GROUP_UNDERWATER)
  {
    TIME_D3D_PROFILE(underwater);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_UNDERWATER], g_default_mip_bias);
  }

  return r;
}

bool renderTransLowRes(dafx::CullingId cull_id) { return renderTransBase(FX_RENDER_GROUP_LOWRES, cull_id); }

bool renderTransHighRes(dafx::CullingId cull_id) { return renderTransBase(FX_RENDER_GROUP_HIGHRES, cull_id); }

bool renderTransThermal(dafx::CullingId cull_id) { return renderTransBase(FX_RENDER_GROUP_THERMAL, cull_id); }

bool renderUnderwater(dafx::CullingId cull_id) { return renderTransBase(FX_RENDER_GROUP_UNDERWATER, cull_id); }

bool renderTransSpecial(uint8_t render_tag, dafx::CullingId cull_id)
{
  G_ASSERT_RETURN(render_tag != 0, false);

  clear_tex_slots();

  const int blockId = particles_block_id[render_tag < particles_block_id.size() ? render_tag : 0];
  STATE_GUARD(ShaderGlobal::setBlock(VALUE, ShaderGlobal::LAYER_SCENE), blockId, -1);

  TIME_D3D_PROFILE(acesfx__render_trans);

  bool use_default_cull = cull_id == dafx::CullingId();
  bool r = false;
  if (render_tag == ERT_TAG_VOLMEDIA)
  {
    TIME_D3D_PROFILE(volmedia);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_VOLMEDIA], g_default_mip_bias);
  }
  else if (render_tag == ERT_TAG_VOLFOG_INJECTION)
  {
    TIME_D3D_PROFILE(volfog_injection);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_VOLFOG_INJECTION], g_default_mip_bias);
  }
  else if (render_tag == ERT_TAG_FOM)
  {
    TIME_D3D_PROFILE(fom);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull_fom : cull_id, render_tags[ERT_TAG_FOM], g_default_mip_bias);
  }
  else if (render_tag == ERT_TAG_ATEST)
  {
    TIME_D3D_PROFILE(atest);
    r = dafx::render(g_dafx_ctx, use_default_cull ? g_dafx_cull : cull_id, render_tags[ERT_TAG_ATEST], g_default_mip_bias);
  }

  return r;
}

void renderTransWaterProj(const TMatrix4 &view, const TMatrix4 &proj, const Point3 &pos, float mip_bias)
{
  TIME_D3D_PROFILE(renderTransWaterProj);

  // render fx
  TMatrix4 globtm = view * proj;
  clear_tex_slots();
  FRAME_LAYER_GUARD(globalFrameBlockId);
  SCENE_LAYER_GUARD(particles_block_id[0]);
  STATE_GUARD_0(ShaderGlobal::set_int(dafx_is_water_proj_var_id, VALUE), 1);

  TMatrix4 prevGtm;
  Point3 prevPos;
  dafx::get_global_value(g_dafx_ctx, "globtm", &prevGtm, 64);
  dafx::get_global_value(g_dafx_ctx, "world_view_pos", &prevPos, 12);

  dafx::set_global_value(g_dafx_ctx, "globtm", &globtm, 64);
  dafx::set_global_value(g_dafx_ctx, "world_view_pos", &pos, 12);

  dafx::flush_global_values(g_dafx_ctx);
  dafx::render(g_dafx_ctx, g_dafx_cull, render_tags[ERT_TAG_WATER_PROJ], mip_bias);

  dafx::set_global_value(g_dafx_ctx, "globtm", &prevGtm, 64);
  dafx::set_global_value(g_dafx_ctx, "world_view_pos", &prevPos, 12);
  dafx::flush_global_values(g_dafx_ctx);
}

void renderTransHaze()
{
  clear_tex_slots();
  ShaderGlobal::setBlock(particles_block_id[0], ShaderGlobal::LAYER_SCENE);
  dafx::render(g_dafx_ctx, g_dafx_cull, render_tags[ERT_TAG_HAZE], g_default_mip_bias);
  dafx::render(g_dafx_ctx, g_dafx_cull, "distortion", g_default_mip_bias);
  ShaderGlobal::setBlock(-1, ShaderGlobal::LAYER_SCENE);
}

bool hasVisibleHaze()
{
  for (int i = 0; i < fx_managers.size(); i++)
    if (fx_managers[i] && fx_managers[i]->hasHaze() && fx_managers[i]->hasVisibleEffects())
      return true;

  return dafx::fetch_culling_state_visible_count(g_dafx_ctx, g_dafx_cull, "distortion");
}

void killEffectsInSphere(const BSphere3 &bsph)
{
  for (int i = 0; i < fx_managers.size(); i++)
    if (fx_managers[i])
      fx_managers[i]->killEffectsInSphere(bsph);
}

void reset()
{
  for (int i = 0; i < fx_managers.size(); i++)
    if (fx_managers[i])
      fx_managers[i]->reset();
}

/*bool can_start_effect(int type, bool is_player)
{
  G_ASSERT(type >= 0 && type < effect_count());
  return fx_managers[type].canStartEffect(is_player);
}*/

void get_stats_by_fx_name(const char *name, eastl::string &o)
{
  int type = get_type_by_name(name);
  if (type < 0)
  {
    o = "unknown fx type";
    return;
  }

  if (type >= fx_managers.size() || !fx_managers[type])
  {
    o = "fx is not inited yet";
    return;
  }

  fx_managers[type]->getStats(o);
}


void dump_statistics() {}

float get_effect_life_time(int type)
{
  if (type < 0 || type >= fx_managers.size() || !fx_managers[type]) // not inited yet
    if (!init_new_manager(type))
      return 0.0;

  return fx_managers[type]->lifeTime();
}

void async_update_started()
{
  if (!::is_main_thread())
    LOGERR_ONCE("fx: async_update_started: Not main thread!");
  start_allowed = false;
}

void async_update_finished()
{
  if (!::is_main_thread())
    LOGERR_ONCE("fx: async_update_finished: Not main thread!");
  start_allowed = true;
}

AcesEffect *start_effect(
  int type, const TMatrix &emitter_tm, const TMatrix &fx_tm, bool is_player, const SoundDesc *snd_desc, FxErrorType *perr)
{
  if (!::is_main_thread())
    LOGERR_ONCE("fx: start_effect: Not main thread!");
  if (!start_allowed)
    LOGERR_ONCE("start_effect() is called while EffectManager::update() is still running in thread pool");

  if (type < 0)
  {
    if (perr)
      *perr = FX_ERR_INVALID_TYPE;
    return NULL;
  }

  G_ASSERT_RETURN(type < effectNames.nameCount(), nullptr);

  if (!prefetch_effect(type))
    return nullptr;

#if DAGOR_DBGLEVEL > 0
  const Point3 &emtPos = emitter_tm.getcol(3);
  // intentionally don't output position in assertion as it's might throw exception again if it's NaNs
  G_ASSERT(!check_nan(emtPos) && emtPos.lengthSq() < 1e12f);
  if (!(emtPos.lengthSq() > 0.0001f || fx_tm.getcol(3).lengthSq() > 0.0001f))
  {
    logerr("empty matrix in %s for effect '%s'", __FUNCTION__, fx_managers[type]->getName());
    // Note: here resided debug_dump_stack() but it was removed because it's might be source of lags by iself
  }
#endif

  Point3 pos = emitter_tm.getcol(3) + fx_tm.getcol(3);

  if (fx_managers[type]->hasEnoughEffectsAtPoint(is_player, pos))
  {
    if (perr)
      *perr = FX_ERR_HAS_ENOUGH;
    if (snd_desc)
      fx_managers[type]->playSoundOneshot(*snd_desc, pos);
    return NULL;
  }

  AcesEffect *fx = fx_managers[type]->startEffect(pos, is_player, perr);

  if (fx)
  {
    if (emitter_tm != TMatrix::IDENT)
      fx->setEmitterTm(emitter_tm);

    if (fx_tm != TMatrix::IDENT)
      fx->setFxTm(fx_tm);
    else if (!is_equal_float(fx->getDefaultScale(), 1.f))
      fx->setFxScale(fx->getDefaultScale());

    if (snd_desc)
      fx->playSound(*snd_desc);

    g_entity_mgr->broadcastEvent(StartEffectEvent(type, fx, pos));
  }

  return fx;
}

AcesEffect *start_effect_pos(int type, const Point3 &pos, bool is_player, float scale, const SoundDesc *snd_desc)
{
  TMatrix tm = TMatrix::IDENT;
  if (scale != 1.f)
    for (int i = 0; i < 3; ++i)
      tm.setcol(i, TMatrix::IDENT.getcol(i) * scale);
  tm.setcol(3, pos);
  return start_effect(type, scale == 1.f ? tm : TMatrix::IDENT, scale != 1.f ? tm : TMatrix::IDENT, is_player, snd_desc);
}

TMatrix create_matrix_from_pos_norm(const Point3 &pos, const Point3 &norm)
{
  TMatrix tm = TMatrix::IDENT;
  tm.setcol(1, normalize(norm));
  tm.setcol(2, normalize(Point3(1, 0, 0) % norm));
  tm.setcol(0, normalize(norm % tm.getcol(2)));
  tm.setcol(3, pos);
  return tm;
}

AcesEffect *start_effect_fxtm(int type, TMatrix tm, bool is_player, float scale, const SoundDesc *snd_desc)
{
  AcesEffect *fx = start_effect(type, TMatrix::IDENT, tm, is_player, snd_desc);
  if (fx)
    fx->setFxScale(scale);
  return fx;
}

AcesEffect *start_effect_pos_norm(
  int type, const Point3 &pos, const Point3 &norm, bool is_player, float scale, const SoundDesc *snd_desc)
{
  TMatrix emmTm = TMatrix::IDENT;
  emmTm.setcol(1, normalize(norm));
  emmTm.setcol(2, normalize(Point3(1, 0, 0) % norm));
  emmTm.setcol(0, normalize(norm % emmTm.getcol(2)));
  for (int i = 0; i < 3; ++i)
    emmTm.setcol(i, emmTm.getcol(i) * scale);
  emmTm.setcol(3, pos);
  AcesEffect *fx = start_effect(type, emmTm, TMatrix::IDENT, is_player, snd_desc);
  if (fx)
    fx->setFxScale(scale);
  return fx;
}

bool prefetch_effect(int type)
{
  if (type < 0)
    return false;

  if (type >= fx_managers.size() || !fx_managers[type]) // not inited yet
    if (!init_new_manager(type))
      return false;

  return true;
}

void stop_effect(AcesEffect *&fx)
{
  if (fx)
  {
    fx->unsetEmitter();
    fx->unlock();
    fx = NULL;
  }
}

void kill_effect(AcesEffect *&fx)
{
  if (fx)
  {
    fx->unsetEmitter();
    fx->kill();
    fx = NULL;
  }
}

void start_all_effects(const Point3 &pos, float rad, int mul, float rad_step)
{
  // Effect gameres
  dag::Vector<eastl::string> effectList;
  ::iterate_gameres_names_by_class(EffectGameResClassId, [&effectList](const char *name) -> void { effectList.emplace_back(name); });

  // Effects from fx.blk
  for (int i = 0; i < g_fx_blk.blockCount(); i++)
  {
    const DataBlock *fxBlk = g_fx_blk.getBlock(i);
    if (fxBlk && fxBlk->paramExists("fx"))
      effectList.emplace_back(fxBlk->getBlockName());
  }

  for (int j = 0; j < mul; ++j)
  {
    for (int i = 0; i < effectList.size(); i++)
    {
      float ang = i / static_cast<float>(effectList.size()) * 2.f * PI;
      int type = get_type_by_name(effectList[i].c_str());
      acesfx::start_effect_pos(type, pos + (rad + rad_step * j) * Point3(sin(ang), 0.f, cos(ang)), true);
    }
  }
}

int get_type_by_name(const char *name, bool optional)
{
  if (!name || !name[0])
    return -1;
  int fxType = effectNames.getNameId(name);

  // try fx.blk
  if (fxType < 0) // first call
    fxType = g_fx_blk.blockExists(name) ? effectNames.addNameId(name) : -1;

  // try direct resource
  if (fxType < 0)
  {
    int resId = gamereshooks::aux_game_res_handle_to_id(GAMERES_HANDLE_FROM_STRING(name), EffectGameResClassId);
    fxType = resId > 0 ? effectNames.addNameId(name) : -1;
  }

#if DAGOR_DBGLEVEL > 0
  if (fxType < 0 && !optional)
  {
    static eastl::vector_set<uint32_t> reportedFxNames;
    if (reportedFxNames.insert(str_hash_fnv1(name)).second) // if inserted
      logerr("Invalid/unknown FX type name '%s'", name);
  }
#else
  G_UNUSED(optional);
#endif
  return fxType;
}

int get_and_init_type_by_name(const char *name)
{
  int fxType = acesfx::get_type_by_name(name);
  if ((size_t)fxType >= fx_managers.size() || !fx_managers[fxType]) // not inited yet
    init_new_manager(fxType);
  return fxType;
}

const char *get_name_by_type(int type) { return effectNames.getName(type); }

void set_dafx_globaldata(const TMatrix4 &tm, const TMatrix &view, const Point3 &view_pos)
{
  dafx::set_global_value(g_dafx_ctx, "globtm", &tm, sizeof(tm));
  dafx::set_global_value(g_dafx_ctx, "view_dir_x", view[0], 12);
  dafx::set_global_value(g_dafx_ctx, "view_dir_y", view[1], 12);
  dafx::set_global_value(g_dafx_ctx, "view_dir_z", view[2], 12);
  dafx::set_global_value(g_dafx_ctx, "world_view_pos", &view_pos, sizeof(view_pos));
  dafx::flush_global_values(g_dafx_ctx);
}

template <typename Callable>
static ecs::QueryCbResult effect_quality_reset_ecs_query(Callable callable);

FxQuality get_fx_target()
{
  FxQuality fxTarget = FX_QUALITY_LOW;
  effect_quality_reset_ecs_query([&](int global_fx__target) {
    fxTarget = FxQuality(global_fx__target);
    return ecs::QueryCbResult::Stop;
  });

  return fxTarget;
}

template <typename Callable>
inline static void thermal_vision_on_ecs_query(ecs::EntityId eid, Callable c);
bool thermal_vision_on()
{
  bool thermalVisionOn = false;
  thermal_vision_on_ecs_query(g_entity_mgr->getSingletonEntity(ECS_HASH("thermal_vision_renderer")),
    [&](bool thermal_vision__on) { thermalVisionOn = thermal_vision__on; });
  return thermalVisionOn;
}

} // namespace acesfx

namespace rifx_composite = rifx::composite::detail;

int rifx_composite::getTypeByName(const char *name) { return acesfx::get_type_by_name(name); }

void rifx_composite::startEffect(int type, const TMatrix &fx_tm, AcesEffect **locked_fx)
{
  AcesEffect *fx = acesfx::start_effect(type, TMatrix::IDENT, fx_tm, false);
  if (locked_fx)
  {
    *locked_fx = fx;
    if (fx)
      fx->lock();
  }
}

void rifx_composite::stopEffect(AcesEffect *fx_handle) { acesfx::stop_effect(fx_handle); }


ECS_TAG(render)
static inline void start_effect_pos_norm_es(const acesfx::StartEffectPosNormEvent &evt)
{
  acesfx::start_effect_pos_norm(evt.get<0>(), evt.get<1>(), evt.get<2>(), evt.get<3>(), evt.get<4>());
}