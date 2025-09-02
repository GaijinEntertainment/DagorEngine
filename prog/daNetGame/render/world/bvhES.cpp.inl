// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/bvh.h>
#include <bvh/bvh.h>
#include <render/denoiser.h>
#include <render/rtsm.h>
#include <render/rtr.h>
#include <render/rtao.h>
#include <render/ptgi.h>
#include <bvh/bvh_connection.h>

#include <render/daFrameGraph/daFG.h>
#include "EASTL/internal/type_properties.h"
#include "drv/3d/dag_renderTarget.h"
#include "frameGraphHelpers.h"
#include <landMesh/lmeshManager.h>
#include <render/viewVecs.h>
#include <render/world/cameraParams.h>
#include <render/world/wrDispatcher.h>
#include <render/world/global_vars.h>
#include <ecs/render/updateStageRender.h>
#include <render/daFrameGraph/ecs/frameGraphNode.h>
#include <rendInst/visibility.h>
#include <render/skies.h>
#include <render/fx/fxRenderTags.h>
#include <render/cables.h>
#include <shaders/dag_shaderBlock.h>
#include "frameGraphNodes/frameGraphNodes.h"
#include <drv/3d/dag_resetDevice.h>

#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <render/renderEvent.h>
#include <util/dag_convar.h>
#include <drv/3d/dag_rwResource.h>
#include <render/clusteredLights.h>
#include <render/renderEvent.h>
#include <render/resolution.h>
#include <render/renderer.h>
#include <render/renderSettings.h>
#include <render/world/antiAliasingMode.h>

#include <animChar/dag_animCharacter2.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <render/world/animCharRenderUtil.h>
#include <shaders/dag_dynSceneRes.h>
#include <render/world/dynModelRenderer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/world/dynModelRenderPass.h>
#include <util/dag_console.h>
#include <util/dag_threadPool.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtraAccess.h>
#include <camera/sceneCam.h>
#include <rendInst/constants.h>
#include <shaders/dag_shaderResUnitedData.h>

// To reduce shader variants these got assumed in raytracing.blk
// CONSOLE_BOOL_VAL("raytracing", rtr_shadow, false, "Also controls debug view!");
// CONSOLE_BOOL_VAL("raytracing", rtr_use_csm, true, "Also controls debug view!");
constexpr bool rtr_shadow = false;
constexpr bool rtr_use_csm = true;
CONSOLE_INT_VAL("raytracing", ptgi_quality, 0, 0, 3);
CONSOLE_FLOAT_VAL("raytracing", additional_impostor_mip_bias, -2);
CONSOLE_FLOAT_VAL("raytracing", bbox_width, 4000);
CONSOLE_INT_VAL("raytracing",
  rtsm_quality,
  1,
  0,
  2,
  "0 = full res "
  "1 = checkerboard + tracing for holes in penumbra "
  "2 = checkerboard + holes filled with random neighbour");
CONSOLE_FLOAT_VAL("raytracing", dynamic_light_radius, 0.1, "When DLSS RR is enabled!");
CONSOLE_BOOL_VAL("raytracing", draw_debug_dynmodels, false);
CONSOLE_INT_VAL("raytracing", max_ahs_calls, 10, 1, 1000);
CONSOLE_BOOL_VAL("raytracing", prepare_ri_extra_job_on_previous_frame, true);
CONSOLE_BOOL_VAL("raytracing", delay_PUFD, true);
CONSOLE_FLOAT_VAL("raytracing", animate_all_animchars_range, 30);

// TODO move these to entity
static bvh::ContextId bvhRenderingContextId;
static BVHConnection *fxBvhConnection = nullptr;
static bool cablesChanged = false;
static dynmodel_renderer::DynModelRenderingState *bvhDynmodelState = nullptr;
static const dynmodel_renderer::DynamicBufferHolder *bvhDynmodelInstanceBuffer = nullptr;
static void bvh_iterate_over_animchars(
  dynrend::BVHIterateOneInstanceCallback iterate_one_instance, const Point3 &view_position, void *user_data);
static bool rigen_cull_dist_was_increased = false;
static int bvhBuildingCountdown = -1;
static constexpr int MAX_BVH_BUILDING_COUNTDOWN = 20;

struct ResolvedRTSettings
{
  bool isBVHEnabled = false;
  bool isDenoiserEnabled = false;
  bool isRTSMEnabled = false;
  bool isRTSMDynamicEnabled = false;
  bool isRTREnabled = false;
  bool isRTTREnabled = false;
  bool isRTAOEnabled = false;
  bool isPTGIEnabled = false;
  bool isRTWaterEnabled = false;
  bool isRTGIEnabled = false;
  bool isRayReconstructionEnabled = false;
};
ECS_DECLARE_RELOCATABLE_TYPE(ResolvedRTSettings);
ECS_REGISTER_RELOCATABLE_TYPE(ResolvedRTSettings, nullptr);

template <typename Callable>
static void get_resolved_rt_settings_ecs_query(Callable c);
template <typename Callable>
static void set_resolved_rt_settings_ecs_query(Callable c);

static ResolvedRTSettings get_resolved_rt_settings()
{
  ResolvedRTSettings settings;
  get_resolved_rt_settings_ecs_query([&settings](const ResolvedRTSettings &resolved_rt_settings) { settings = resolved_rt_settings; });
  return settings;
}

struct RTFeatureChanged
{
  bool isBVHChanged = false;
  bool isDenoiserChanged = false;
  bool isRTSMChanged = false;
  bool isRTSMDynamicChanged = false;
  bool isRTRChanged = false;
  bool isRTTRChanged = false;
  bool isRTAOChanged = false;
  bool isPTGIChanged = false;
  bool isRTWaterChanged = false;
  bool isRTGIChanged = false;
  bool isRayReconstructionChanged = false;

  RTFeatureChanged() = default;
  RTFeatureChanged(const ResolvedRTSettings &old, const ResolvedRTSettings &current)
  {
    isBVHChanged = old.isBVHEnabled != current.isBVHEnabled;
    isRayReconstructionChanged = old.isRayReconstructionEnabled != current.isRayReconstructionEnabled;
    isDenoiserChanged = old.isDenoiserEnabled != current.isDenoiserEnabled || isRayReconstructionChanged;
    isRTSMChanged = old.isRTSMEnabled != current.isRTSMEnabled || isRayReconstructionChanged;
    isRTSMDynamicChanged = old.isRTSMDynamicEnabled != current.isRTSMDynamicEnabled;
    isRTRChanged = old.isRTREnabled != current.isRTREnabled || isRayReconstructionChanged;
    isRTTRChanged = old.isRTTREnabled != current.isRTTREnabled;
    isRTAOChanged = old.isRTAOEnabled != current.isRTAOEnabled || isRayReconstructionChanged;
    isPTGIChanged = old.isPTGIEnabled != current.isPTGIEnabled || isRayReconstructionChanged;
    isRTWaterChanged = old.isRTWaterEnabled != current.isRTWaterEnabled;
    isRTGIChanged = old.isRTGIEnabled != current.isRTGIEnabled;
  }
  static RTFeatureChanged All()
  {
    RTFeatureChanged result;
    result.isBVHChanged = true;
    result.isDenoiserChanged = true;
    result.isRayReconstructionChanged = true;
    result.isRTSMChanged = true;
    result.isRTSMDynamicChanged = true;
    result.isRTRChanged = true;
    result.isRTTRChanged = true;
    result.isRTAOChanged = true;
    result.isPTGIChanged = true;
    result.isRTWaterChanged = true;
    result.isRTGIChanged = true;
    return result;
  }
};


struct BvhHeightProvider final : public bvh::HeightProvider
{
  LandMeshManager *lmeshMgr = nullptr;
  bool embedNormals() const override final { return false; }
  void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count, bool &has_hole) const override final
  {
    using TerrainVertex = Point3;

    Point3 *scratch = (Point3 *)data;

    has_hole = false;
    if (auto holesMgr = lmeshMgr->getHolesManager())
    {
      BBox2 boxToCheck(origin, origin + Point2(cell_size * cell_count, cell_size * cell_count));
      has_hole = holesMgr->approximateCheckBBox(boxToCheck);
    }

    for (int z = 0; z <= cell_count; ++z)
      for (int x = 0; x <= cell_count; ++x)
      {
        Point2 loc(x * cell_size, z * cell_size);

        TerrainVertex &v = scratch[z * (cell_count + 1) + x];
        v.x = loc.x;
        v.z = loc.y;

        if (!lmeshMgr->getHeight(loc + origin, v.y, nullptr))
          v.y = 0;
      }
  }
};
ECS_DECLARE_BOXED_TYPE(BvhHeightProvider);
ECS_REGISTER_BOXED_TYPE(BvhHeightProvider, nullptr);

struct RiGenVisibilityECS
{
  RiGenVisibility *visibility = nullptr;
};
ECS_DECLARE_BOXED_TYPE(RiGenVisibilityECS);
ECS_REGISTER_BOXED_TYPE(RiGenVisibilityECS, nullptr);

using RWHandle = dafg::VirtualResourceHandle<BaseTexture, true, false>;
using ROHandle = dafg::VirtualResourceHandle<const BaseTexture, true, false>;

struct RTPersistentTexturesECS
{
  enum class Type
  {
    DENOISER,
    RTSM,
    RTR,
    RTAO,
    PTGI
  };
  RTPersistentTexturesECS() = default;
  RTPersistentTexturesECS(const RTPersistentTexturesECS &) = delete;
  RTPersistentTexturesECS &operator=(const RTPersistentTexturesECS &) = delete;
  eastl::vector_map<const char *, UniqueTex> textureMap;

  void allocate(const denoiser::TexInfoMap &textures, Type type)
  {
    textureMap.reserve(32);
    for (auto tex : textures)
      textureMap[tex.first] = UniqueTex{dag::create_tex(nullptr, tex.second.w, tex.second.h, tex.second.cflg | TEXCF_CLEAR_ON_CREATE,
        tex.second.mipLevels, tex.first)};

    auto saveNames = [&textures](dag::Vector<const char *> &names) {
      G_ASSERT(names.empty());
      for (auto pair : textures)
        names.push_back(pair.first);
    };
    switch (type)
    {
      case Type::DENOISER: saveNames(denoiserNames); break;
      case Type::RTSM: saveNames(rtsmNames); break;
      case Type::RTR: saveNames(rtrNames); break;
      case Type::RTAO: saveNames(rtaoNames); break;
      case Type::PTGI: saveNames(ptgiNames); break;
    }
  }

  void clear(Type type)
  {
    auto removeTextures = [this](dag::Vector<const char *> &names) {
      for (auto name : names)
        textureMap.erase(name);
      names.clear();
    };
    switch (type)
    {
      case Type::DENOISER: removeTextures(denoiserNames); break;
      case Type::RTSM: removeTextures(rtsmNames); break;
      case Type::RTR: removeTextures(rtrNames); break;
      case Type::RTAO: removeTextures(rtaoNames); break;
      case Type::PTGI: removeTextures(ptgiNames); break;
    }
  }

  eastl::vector_map<const char *, RWHandle> registerToFg(dafg::Registry registry, const denoiser::TexInfoMap &textures) const
  {
    eastl::vector_map<const char *, RWHandle> result;
    result.reserve(32);
    for (auto tex : textures)
    {
      ManagedTexView view = textureMap.at_key(tex.first);
      result.emplace(eastl::pair{tex.first, registry.registerTexture(tex.first, [view](auto) { return view; })
                                              .atStage(dafg::Stage::CS)
                                              .useAs(dafg::Usage::SHADER_RESOURCE)
                                              .handle()});
    }
    return result;
  }

private:
  dag::Vector<const char *> denoiserNames;
  dag::Vector<const char *> rtsmNames;
  dag::Vector<const char *> rtrNames;
  dag::Vector<const char *> rtaoNames;
  dag::Vector<const char *> ptgiNames;
};
ECS_DECLARE_BOXED_TYPE(RTPersistentTexturesECS);
ECS_REGISTER_BOXED_TYPE(RTPersistentTexturesECS, nullptr);

namespace bvh
{
static void process_elem(const ShaderMesh::RElem &elem,
  MeshInfo &mesh_info,
  const ChannelParser &parser,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures);
}

static void initBVH()
{
  if (is_bvh_enabled())
  {
    bvh::init(bvh::process_elem);
    bvhRenderingContextId = bvh::create_context("Rendering",
      static_cast<bvh::Features>(bvh::Features::Terrain | bvh::Features::RIFull | bvh::Features::Fx | bvh::Features::Cable |
                                 bvh::Features::DynrendRigidFull | bvh::Features::DynrendSkinnedFull));
    bvh::connect_fx(bvhRenderingContextId, [](BVHConnection *connection) { fxBvhConnection = connection; });
    bvhBuildingCountdown = MAX_BVH_BUILDING_COUNTDOWN;
    debug("initBVH: bvhBuildingCountdown reset to %d frames", bvhBuildingCountdown);
  }
}

static void initRTFeatures(int w, int h, bool use_ray_reconstruction)
{
  if (is_bvh_enabled())
  {
    G_ASSERTF(bvhRenderingContextId, "Call initBVH first!");
    if (is_denoiser_enabled())
      denoiser::initialize(w, h, use_ray_reconstruction);
    if (is_rtsm_enabled())
      rtsm::initialize(rtsm::RenderMode::Denoised, false);
    if (is_rtr_enabled())
    {
      rtr::initialize(denoiser::ReflectionMethod::Reblur, !use_ray_reconstruction, !use_ray_reconstruction);
      rtr::set_ray_limit_params(100000, -2, false);
      rtr::set_performance_mode(false);
      rtr::set_use_anti_firefly(true);
    }
    if (is_rtao_enabled())
      rtao::initialize(!use_ray_reconstruction);
    if (is_ptgi_enabled())
      ptgi::initialize(!use_ray_reconstruction);
  }
}

static void closeRTFeatures()
{
  ptgi::teardown();
  rtao::teardown();
  rtr::teardown();
  rtsm::turn_off();
  rtsm::teardown();
  denoiser::teardown();
}

static void closeBVH()
{
  if (bvhRenderingContextId)
  {
    bvh::teardown(bvhRenderingContextId);
    bvh::teardown();
    bvhRenderingContextId = {};
  }
}

static void closeBVHScene()
{
  if (bvhRenderingContextId)
  {
    bvh::finalize_async_atmosphere_update(bvhRenderingContextId);
    bvh::remove_terrain(bvhRenderingContextId);
    bvh::on_before_unload_scene(bvhRenderingContextId);
    bvh::on_unload_scene(bvhRenderingContextId);
  }
}

template <typename Callable>
static void reset_bvh_entity_ecs_query(Callable c);

static void bvh_before_reset(bool full_reset)
{
  if (full_reset)
  {
    closeBVHScene();
    reset_bvh_entity_ecs_query([](bool &bvh__initialized) { bvh__initialized = false; });
    closeRTFeatures();
    closeBVH();
  }
}
REGISTER_D3D_BEFORE_RESET_FUNC(bvh_before_reset);
static void bvh_after_reset(bool full_reset)
{
  if (full_reset)
  {
    initBVH();
    if (get_world_renderer()) // Can't query resolution on login screen
    {
      int w, h;
      get_max_possible_rendering_resolution(w, h);
      initRTFeatures(w, h, is_rr_enabled());
    }
  }
}
REGISTER_D3D_AFTER_RESET_FUNC(bvh_after_reset);

static ShaderVariableInfo bvh_terrain_has_holesVarId("bvh_terrain_has_holes", true);

ECS_TAG(render)
ECS_NO_ORDER
ECS_REQUIRE(eastl::false_type bvh__initialized)
static void setup_bvh_scene_es(
  const UpdateStageInfoBeforeRender &, BvhHeightProvider &bvh__heightProvider, bool &bvh__initialized, int &bvh__frame_counter)
{
  if (is_bvh_enabled())
  {
    bvh__initialized = true;
    bvh__frame_counter = 0;
    if (bvhRenderingContextId)
    {
      auto lmeshMgr = WRDispatcher::getLandMeshManager();
      if (lmeshMgr)
      {
        bvh__heightProvider.lmeshMgr = lmeshMgr;
        ShaderGlobal::set_int(bvh_terrain_has_holesVarId, lmeshMgr->getHolesManager() ? 1 : 0);
        bvh::add_terrain(bvhRenderingContextId, &bvh__heightProvider);
      }
      else
        ShaderGlobal::set_int(bvh_terrain_has_holesVarId, 0);
    }
  }
}

ECS_TAG(render)
ECS_ON_EVENT(UnloadLevel)
ECS_REQUIRE(BvhHeightProvider bvh__heightProvider)
static void close_bvh_scene_es(const ecs::Event &) { closeBVHScene(); }

void bvh_release_bindlessly_held_textures()
{
  if (bvhRenderingContextId)
    bvh::on_before_unload_scene(bvhRenderingContextId);
}

static void wait_bvh_worker_threads();

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void bvh_destroy_es(
  const ecs::Event &, RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility)
{
  wait_bvh_worker_threads();
  closeBVHScene();
  rendinst::destroyRIGenVisibility(bvh__rendinst_visibility.visibility);
  bvh__rendinst_visibility.visibility = nullptr;
  rendinst::destroyRIGenVisibility(bvh__rendinst_oof_visibility.visibility);
  bvh__rendinst_oof_visibility.visibility = nullptr;
  closeRTFeatures();
  closeBVH();
}

template <typename Callable>
static void get_bvh_rigen_visibility_ecs_query(Callable c);
static RiGenVisibility *get_bvh_rigen_visibility()
{
  RiGenVisibility *visibility;
  get_bvh_rigen_visibility_ecs_query(
    [&](RiGenVisibilityECS bvh__rendinst_visibility) { visibility = bvh__rendinst_visibility.visibility; });
  return visibility;
}

template <typename Callable>
static void get_bvh_rigen_oof_visibility_ecs_query(Callable c);
static RiGenVisibility *get_bvh_rigen_oof_visibility()
{
  RiGenVisibility *visibility;
  get_bvh_rigen_oof_visibility_ecs_query(
    [&](RiGenVisibilityECS bvh__rendinst_oof_visibility) { visibility = bvh__rendinst_oof_visibility.visibility; });
  return visibility;
}

static TMatrix4 get_bvh_culling_matrix(const Point3 &cameraPos)
{
  TMatrix viewItm;
  viewItm.setcol(0, {1, 0, 0});
  viewItm.setcol(1, {0, 0, 1});
  viewItm.setcol(2, {0, 1, 0});
  viewItm.setcol(3, Point3::x0z(cameraPos));
  TMatrix4 cullView = inverse(viewItm);
  float minHt, maxHt;
  WRDispatcher::getMinMaxZ(minHt, maxHt);
  TMatrix4 cullProj = matrix_ortho_lh_forward(bbox_width, bbox_width, minHt, maxHt + max(0.0f, cameraPos.y));
  TMatrix4 cullingViewProj = cullView * cullProj;
  return cullingViewProj;
}

void prepareFXForBVH(const Point3 &cameraPos) { acesfx::prepare_bvh_culling(get_bvh_culling_matrix(cameraPos)); }

bool is_bvh_enabled() { return get_resolved_rt_settings().isBVHEnabled; }
bool is_rtsm_enabled() { return get_resolved_rt_settings().isRTSMEnabled; }
bool is_rtsm_dynamic_enabled() { return get_resolved_rt_settings().isRTSMDynamicEnabled; }
bool is_rtr_enabled() { return get_resolved_rt_settings().isRTREnabled; }
bool is_rttr_enabled() { return get_resolved_rt_settings().isRTTREnabled; }
bool is_rtao_enabled() { return get_resolved_rt_settings().isRTAOEnabled; }
bool is_ptgi_enabled() { return get_resolved_rt_settings().isPTGIEnabled; }
bool is_rt_water_enabled() { return get_resolved_rt_settings().isRTWaterEnabled; }
bool is_rtgi_enabled() { return get_resolved_rt_settings().isRTGIEnabled; }
bool is_denoiser_enabled() { return get_resolved_rt_settings().isDenoiserEnabled; }
bool is_rr_enabled() { return get_resolved_rt_settings().isRayReconstructionEnabled; }

void draw_rtr_validation() { rtr::render_validation_layer(); }
bool delay_PUFD_after_bvh() { return is_bvh_enabled() && delay_PUFD; }
void bvh_cables_changed() { cablesChanged = true; }

bool is_rt_supported() { return bvh::is_available(); }

struct BVHRICullingJob final : public cpujobs::IJob
{
  RiGenVisibility *visibility;
  Frustum frustum;
  Point3 cameraPos = {0, 0, 0};

  void start(RiGenVisibility *visibility_, const Frustum &frustum_, const Point3 &cameraPos_, threadpool::JobPriority prio)
  {
    visibility = visibility_;
    frustum = frustum_;
    cameraPos = cameraPos_;
    threadpool::add(this, prio, false); // Note: would be waken up by `bvh_start_before_render_jobs_es`
  }
  const char *getJobName(bool &) const override { return "BVHRICullingJob"; }
  void doJob() override { rendinst::prepareRIGenVisibility(frustum, cameraPos, visibility, false, nullptr); }
} bvh_ri_culling_job;

struct BVHRIOOFCullingJob final : public cpujobs::IJob
{
  RiGenVisibility *visibility;
  Frustum bvhFrustum;
  Point3 cameraPos = {0, 0, 0};

  void start(RiGenVisibility *visibility_, const Frustum &bvh_frustum, const Point3 &cameraPos_, threadpool::JobPriority prio)
  {
    visibility = visibility_;
    bvhFrustum = bvh_frustum;
    cameraPos = cameraPos_;
    threadpool::add(this, prio, false); // Note: would be waken up by `bvh_start_before_render_jobs_es`
  }
  const char *getJobName(bool &) const override { return "BVHRIOOFCullingJob"; }
  void doJob() override
  {
    G_ASSERTF(rigen_cull_dist_was_increased, "BVHRIOOFCullingJob should not be started if RI lod isn't increased");
    rendinst::setRIGenVisibilityMinLod(visibility, rendinst::MAX_LOD_COUNT, 1);
    rendinst::prepareRIGenVisibility(bvhFrustum, cameraPos, visibility, false, nullptr, false);
  }
} bvh_ri_oof_culling_job;

struct BVHPrepareRiExtraInstancesJob final : public cpujobs::IJob
{
  void start(threadpool::JobPriority prio)
  {
    // Note: would be waken up by `bvh_start_before_render_jobs_es` when prepare_ri_extra_job_on_previous_frame is false
    threadpool::add(this, prio, prepare_ri_extra_job_on_previous_frame);
  }
  const char *getJobName(bool &) const override { return "BVHPrepareRiExtraInstancesJob"; }
  void doJob() override { bvh::prepare_ri_extra_instances(); }
} bvh_prepare_ri_extra_instances_job;

static void wait_bvh_worker_threads()
{
  threadpool::wait(&bvh_ri_culling_job);
  threadpool::wait(&bvh_ri_oof_culling_job);
  threadpool::wait(&bvh_prepare_ri_extra_instances_job);
}

static ShaderVariableInfo bvh_usableVarId = ShaderVariableInfo("bvh_usable", true);

ECS_TAG(render)
ECS_BEFORE(animchar_before_render_es)
ECS_AFTER(start_occlusion_and_sw_raster_es) // Note: as it adds quite a bit of jobs which be waken up here
ECS_REQUIRE(dafg::NodeHandle bvh__update_node)
static void bvh_start_before_render_jobs_es(
  const UpdateStageInfoBeforeRender &stg, int &bvh__frame_counter, Point3 &bvh__prev_pos, float &bvh__accumulated_speed)
{
  if (!is_bvh_enabled())
    return;

  bvh::finalize_async_atmosphere_update(bvhRenderingContextId);
  if (rigen_cull_dist_was_increased)
  {
    bvh_ri_culling_job.start(get_bvh_rigen_visibility(), stg.mainCullingFrustum, stg.camPos, threadpool::JobPriority::PRIO_LOW);
    bvh_ri_oof_culling_job.start(get_bvh_rigen_oof_visibility(), get_bvh_culling_matrix(stg.camPos), stg.camPos,
      threadpool::JobPriority::PRIO_LOW);
  }
  else
    bvh_ri_culling_job.start(get_bvh_rigen_visibility(), get_bvh_culling_matrix(stg.camPos), stg.camPos,
      threadpool::JobPriority::PRIO_LOW);
  if (!prepare_ri_extra_job_on_previous_frame)
    bvh_prepare_ri_extra_instances_job.start(threadpool::JobPriority::PRIO_LOW);
  threadpool::wake_up_all(); // Wake all added above and occulision/raster jobs

  constexpr float PREV_FRAME_WEIGHT = 0.95;
  constexpr int INITIAL_FAST_BUILDING_FRAME_COUNT = 500;
  constexpr float MAX_SPEED_FOR_MEDIUM_BUDGET = 5.5; // Slightly above running and free cam speed
  float currectSpeed = safediv(length(stg.camPos - bvh__prev_pos), stg.realDt);
  bvh__prev_pos = stg.camPos;
  bvh__accumulated_speed = bvh__accumulated_speed * PREV_FRAME_WEIGHT + currectSpeed * (1.0f - PREV_FRAME_WEIGHT);
  bvh::BuildBudget budget =
    bvh__frame_counter++ < INITIAL_FAST_BUILDING_FRAME_COUNT
      ? bvh::BuildBudget::High
      : (bvh__accumulated_speed < MAX_SPEED_FOR_MEDIUM_BUDGET ? bvh::BuildBudget::Medium : bvh::BuildBudget::Low);

  bvh::process_meshes(bvhRenderingContextId, budget);

  if (bvhBuildingCountdown >= 0)
  {
    if (bvh::is_building(bvhRenderingContextId))
      bvhBuildingCountdown = MAX_BVH_BUILDING_COUNTDOWN;
    else
    {
      if (bvhBuildingCountdown-- == 0)
        debug("bvhBuildingCountdown reached 0, BVH is now usable");
    }
  }
  ShaderGlobal::set_int(bvh_usableVarId, bvhBuildingCountdown < 0 ? 1 : 0);
}

ECS_TAG(render)
ECS_NO_ORDER
static void bvh_out_of_scope_riex_dist_es(const ecs::UpdateStageInfoAct &, bool bvh__initialized)
{
  if (!is_bvh_enabled() || !bvh__initialized)
    return;
  float riDistMul = rendinst::getDefaultDistAddMul();
  rendinst::setDistMul(riDistMul, 0.0f);
  float cullDistSqMulOOC = rendinst::getCullDistSqMul();
  bvh::override_out_of_camera_ri_dist_mul(cullDistSqMulOOC);
}

static ShaderBlockIdHolder dynamic_scene_trans_block_id{"dynamic_scene_trans"};
static void update_fx_for_bvh()
{
  if (!fxBvhConnection)
    return;
  TIME_D3D_PROFILE(bvh_fx)
  FRAME_LAYER_GUARD(globalFrameBlockId);
  SCENE_LAYER_GUARD(dynamic_scene_trans_block_id);
  d3d::set_render_target(Driver3dRenderTarget{});
  fxBvhConnection->prepare();
  dafx::render(acesfx::get_dafx_context(), acesfx::get_cull_bvh_id(), render_tags[ERT_TAG_BVH], 0,
    [](TEXTUREID id) { fxBvhConnection->textureUsed(id); });
  fxBvhConnection->done();
}

static void upload_dynmodel_instance_data()
{
  TIME_PROFILE(bvh::upload_dynmodel_instance_data)
  bvhDynmodelInstanceBuffer = bvhDynmodelState->requestBuffer(dynmodel_renderer::BufferType::BVH);
  if (bvhDynmodelInstanceBuffer)
    bvhDynmodelState->setVars(bvhDynmodelInstanceBuffer->buffer.getBufId());
}

static void reset_dynmodel_instance_data() { d3d::set_immediate_const(STAGE_CS, nullptr, 0); }

ECS_TAG(render)
static void set_animate_out_of_frustum_trees_es(const RendinstLodRangeIncreasedEvent &evt)
{
  rigen_cull_dist_was_increased = evt.get<0>() || evt.get<1>();
}

void bvh_update_instances(const Point3 &cameraPos, const Frustum &viewFrustum)
{
  if (!is_bvh_enabled())
    return;
  wait_bvh_worker_threads();
  bvhDynmodelState = dynmodel_renderer::create_state("bvh");
  dag::Vector<RiGenVisibility *> visibilities;
  visibilities.push_back(get_bvh_rigen_visibility());
  if (rigen_cull_dist_was_increased)
    visibilities.push_back(get_bvh_rigen_oof_visibility());
  bvh::update_instances(bvhRenderingContextId, cameraPos, get_bvh_culling_matrix(cameraPos), viewFrustum, visibilities,
    bvh_iterate_over_animchars, threadpool::PRIO_LOW);
  rigen_cull_dist_was_increased = false; // Avoid state being stuck when shooter cam is not used next frame
}

static ShaderVariableInfo bvh_additional_impostor_mip_biasVarId = ShaderVariableInfo("bvh_additional_impostor_mip_bias", true);
static ShaderVariableInfo bvh_tan_pixel_angular_radiusVarId = ShaderVariableInfo("bvh_tan_pixel_angular_radius", true);
static ShaderVariableInfo bvh_max_ahs_callsVarId = ShaderVariableInfo("bvh_max_ahs_calls", true);

static dafg::NodeHandle makeBVHUpdateNode()
{
  if (!is_bvh_enabled())
    return {};
  return dafg::register_node("bvh_update", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("prepare_skies_token");
    registry.createBlob<OrderingToken>("bvh_ready_token", dafg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    registry.readBlob<OrderingToken>("acesfx_update_token");
    const auto resolution = registry.getResolution<2>("main_view");
    auto rtWaterHndl = registry.createBlob<int>("water_rt_enabled", dafg::History::No).handle();
    registry.read("fom_shadows_sin").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_sin").optional();
    registry.read("fom_shadows_cos").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    return [cameraHndl, rtWaterHndl, resolution, rt_debug_rt_shadowVarId = get_shader_variable_id("rt_debug_rt_shadow", true),
             rt_debug_use_csmVarId = get_shader_variable_id("rt_debug_use_csm", true)](
             const dafg::multiplexing::Index &multiplexing_index) {
      if (multiplexing_index != dafg::multiplexing::Index{})
        return;

      Point3 cameraPos = Point3::xyz(cameraHndl.ref().cameraWorldPos);
      auto &lights = WRDispatcher::getClusteredLights();
      {
        TIME_PROFILE(cull_lights)
        BBox3 box = BBox3(cameraPos, 100);
        TMatrix4_vec4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
        lights.cullOutOfFrustumLights((mat44f_cref)globtm, SpotLightMaskType::SPOT_LIGHT_MASK_GI,
          OmniLightMaskType::OMNI_LIGHT_MASK_GI);
        lights.setOutOfFrustumLightsToShader();
      }

      ShaderGlobal::set_int(rt_debug_rt_shadowVarId, rtr_shadow ? 1 : 0);
      ShaderGlobal::set_int(rt_debug_use_csmVarId, rtr_use_csm ? 1 : 0);
      float tanPixelAngularRadius = cameraHndl.ref().jitterPersp.hk * tan(0.5f / resolution.get().x);
      ShaderGlobal::set_real(bvh_additional_impostor_mip_biasVarId, additional_impostor_mip_bias);
      ShaderGlobal::set_real(bvh_tan_pixel_angular_radiusVarId, tanPixelAngularRadius);
      ShaderGlobal::set_int(bvh_max_ahs_callsVarId, max_ahs_calls);
      rtWaterHndl.ref() = is_rt_water_enabled();
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      bvh::start_frame();
      bvh::update_terrain(bvhRenderingContextId, Point2::xz(cameraPos));
      upload_dynmodel_instance_data();
      update_fx_for_bvh();
      if (auto cables = get_cables_mgr())
      {
        if (cablesChanged)
        {
          FRAME_LAYER_GUARD(globalFrameBlockId);
          bvh::on_cables_changed(cables, bvhRenderingContextId);
          cablesChanged = false;
        }
      }
      bvh::set_debug_view_min_t(is_free_camera_enabled() ? 0.0f : 0.3f);
      bvh::build(bvhRenderingContextId, cameraHndl.ref().viewItm, cameraHndl.ref().jitterProjTm, cameraPos, Point3::ZERO);
      reset_dynmodel_instance_data();
      auto callback = [](const Point3 &view_pos, const Point3 &view_dir, float d, Color3 &insc, Color3 &loss) {
        get_daskies()->getCpuFogSingle(view_pos, view_dir, d, insc, loss);
      };
      bvh::start_async_atmosphere_update(bvhRenderingContextId, cameraPos, callback);
      if (prepare_ri_extra_job_on_previous_frame)
        bvh_prepare_ri_extra_instances_job.start(threadpool::JobPriority::PRIO_LOW);
      // TODO Refactor this change and restore scheme in BVH nodes, we need to get rid of this global state dependency somehow
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

class RTTextureMaps
{
public:
  RTTextureMaps(dafg::Registry registry,
    RTPersistentTexturesECS &rt_persistent_textures,
    const denoiser::TexInfoMap &persistent_textures,
    const denoiser::TexInfoMap &transient_textures,
    const denoiser::TexInfoMap &read_textures,
    const eastl::vector_map<const char *, const char *> &rename_textures)
  {
    persistentHandles = rt_persistent_textures.registerToFg(registry, persistent_textures);
    transientHandles = createTransient(registry, transient_textures);
    readHandles = readTex(registry, read_textures);
    renameHandles = renameTex(registry, rename_textures);
    readDenoiserCommon(registry);
  }
  void clearTransient()
  {
    TIME_D3D_PROFILE(ClearTransient)
    for (auto h : transientHandles)
      d3d::clear_rt({h.second.get()}, {});
  }
  denoiser::TexMap resolveToPair() const
  {
    denoiser::TexMap pairs;
    pairs.reserve(64);
    resolveTextureHandleToPair(pairs, persistentHandles);
    resolveTextureHandleToPair(pairs, transientHandles);
    resolveTextureHandleToPair(pairs, readHandles);
    resolveTextureHandleToPair(pairs, renameHandles);
    return pairs;
  }

private:
  eastl::vector_map<const char *, RWHandle> persistentHandles;
  eastl::vector_map<const char *, RWHandle> transientHandles;
  eastl::vector_map<const char *, ROHandle> readHandles;
  eastl::vector_map<const char *, RWHandle> renameHandles;

  void readDenoiserCommon(dafg::Registry registry)
  {
    auto motionVecsHndl = read_gbuffer_motion(registry, dafg::Stage::PS_OR_CS).handle();
    auto downsampledMotionVectorsHndl = registry.read("downsampled_motion_vectors_tex")
                                          .texture()
                                          .atStage(dafg::Stage::PS_OR_CS)
                                          .useAs(dafg::Usage::SHADER_RESOURCE)
                                          .handle();
    auto downsampledNormalsHndl =
      registry.readTexture("downsampled_normals").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto closeDepthHndl =
      registry.read("close_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_close_depth_tex").handle();
    readHandles.emplace(eastl::pair{::denoiser::TextureNames::motion_vectors, motionVecsHndl});
    readHandles.emplace(eastl::pair{::denoiser::TextureNames::half_motion_vectors, downsampledMotionVectorsHndl});
    readHandles.emplace(eastl::pair{::denoiser::TextureNames::half_normals, downsampledNormalsHndl});
    readHandles.emplace(eastl::pair{::denoiser::TextureNames::half_depth, closeDepthHndl});
    if (is_rr_enabled())
    {
      auto normalRoughnessHandle =
        registry.readTexture("packed_normal_roughness").atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();
      readHandles.emplace(eastl::pair{::denoiser::TextureNames::denoiser_normal_roughness, normalRoughnessHandle});
    }
  }

  static void resolveTextureHandleToPair(denoiser::TexMap &pairs, const eastl::vector_map<const char *, RWHandle> &handles)
  {
    for (auto h : handles)
      pairs[h.first] = TextureIDPair{h.second.view().getTex2D(), h.second.view().getTexId()};
  }

  static void resolveTextureHandleToPair(denoiser::TexMap &pairs, const eastl::vector_map<const char *, ROHandle> &handles)
  {
    for (auto h : handles)
      pairs[h.first] = TextureIDPair{h.second.view().getTex2D(), h.second.view().getTexId()};
  }

  static eastl::vector_map<const char *, RWHandle> createTransient(dafg::Registry registry, const denoiser::TexInfoMap &textures)
  {
    eastl::vector_map<const char *, RWHandle> result;
    result.reserve(16);
    for (auto tex : textures)
    {
      result.emplace(eastl::pair{tex.first, registry.create(tex.first, dafg::History::No)
                                              .texture(dafg::Texture2dCreateInfo{tex.second.cflg | TEXCF_RTARGET,
                                                IPoint2{tex.second.w, tex.second.h}, tex.second.mipLevels})
                                              .atStage(dafg::Stage::COMPUTE)
                                              .useAs(dafg::Usage::SHADER_RESOURCE)
                                              .handle()});
    }
    return result;
  }

  static eastl::vector_map<const char *, ROHandle> readTex(dafg::Registry registry, const denoiser::TexInfoMap &textures)
  {
    eastl::vector_map<const char *, ROHandle> result;
    result.reserve(16);
    for (auto tex : textures)
    {
      result.emplace(eastl::pair{
        tex.first, registry.read(tex.first).texture().atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle()});
    }
    return result;
  }

  static eastl::vector_map<const char *, RWHandle> renameTex(dafg::Registry registry,
    const eastl::vector_map<const char *, const char *> &textures)
  {
    eastl::vector_map<const char *, RWHandle> result;
    result.reserve(2);
    for (auto tex : textures)
    {
      result.emplace(eastl::pair{tex.first, registry.renameTexture(tex.first, tex.second, dafg::History::No)
                                              .atStage(dafg::Stage::COMPUTE)
                                              .useAs(dafg::Usage::SHADER_RESOURCE)
                                              .handle()});
    }
    return result;
  }
};

static dafg::NodeHandle makeDenoiserPrepareNode(RTPersistentTexturesECS &rt_persistent_textures)
{
  rt_persistent_textures.clear(RTPersistentTexturesECS::Type::DENOISER);
  if (!is_denoiser_enabled())
    return {};

  denoiser::TexInfoMap persistentTextures;
  denoiser::get_required_persistent_texture_descriptors(persistentTextures, true);
  rt_persistent_textures.allocate(persistentTextures, RTPersistentTexturesECS::Type::DENOISER);

  return dafg::register_node("denoiser_prepare", DAFG_PP_NODE_SRC, [&rt_persistent_textures](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);

    denoiser::TexInfoMap persistentTextures;
    denoiser::get_required_persistent_texture_descriptors(persistentTextures, true);
    auto textureMaps = eastl::make_unique<RTTextureMaps>(registry, rt_persistent_textures, persistentTextures, denoiser::TexInfoMap{},
      denoiser::TexInfoMap{}, eastl::vector_map<const char *, const char *>{});

    return [cameraHndl, prevCameraHndl, textureMaps = eastl::move(textureMaps)]() {
      denoiser::FrameParams params;
      params.viewPos = cameraHndl.ref().viewItm.getcol(3);
      params.prevViewPos = prevCameraHndl.ref().viewItm.getcol(3);
      params.viewDir = cameraHndl.ref().viewItm.getcol(2);
      params.prevViewDir = prevCameraHndl.ref().viewItm.getcol(2);
      params.viewItm = cameraHndl.ref().viewItm;
      params.prevViewItm = prevCameraHndl.ref().viewItm;
      params.projTm = cameraHndl.ref().noJitterProjTm;
      params.prevProjTm = prevCameraHndl.ref().noJitterProjTm;
      params.jitter = Point2(cameraHndl.ref().jitterPersp.ox, cameraHndl.ref().jitterPersp.oy);
      params.prevJitter = Point2(prevCameraHndl.ref().jitterPersp.ox, prevCameraHndl.ref().jitterPersp.oy);
      params.motionMultiplier = Point3::ONE;
      params.textures = textureMaps->resolveToPair();
      denoiser::prepare(params);
    };
  });
}

static ShaderVariableInfo sun_dir_for_shadowsVarId = ShaderVariableInfo("sun_dir_for_shadows", true);

static dafg::NodeHandle makeRTSMNode(RTPersistentTexturesECS &rt_persistent_textures)
{
  rt_persistent_textures.clear(RTPersistentTexturesECS::Type::RTSM);
  if (!is_rtsm_enabled())
    return {};

  denoiser::TexInfoMap persistentTextures;
  rtsm::get_required_persistent_texture_descriptors(persistentTextures);
  rt_persistent_textures.allocate(persistentTextures, RTPersistentTexturesECS::Type::RTSM);

  return dafg::register_node("rtsm", DAFG_PP_NODE_SRC, [&rt_persistent_textures](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.createBlob<OrderingToken>("rtsm_token", dafg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto viewPosHndl = registry.readBlob<Point4>("world_view_pos").handle();
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);


    denoiser::TexInfoMap persistentTextures;
    rtsm::get_required_persistent_texture_descriptors(persistentTextures);
    denoiser::TexInfoMap transientTextures;
    rtsm::get_required_transient_texture_descriptors(transientTextures);
    denoiser::TexInfoMap denoiserTextures;
    denoiser::get_required_persistent_texture_descriptors(denoiserTextures, !is_rr_enabled());
    auto textureMaps = eastl::make_unique<RTTextureMaps>(registry, rt_persistent_textures, persistentTextures, transientTextures,
      denoiserTextures, eastl::vector_map<const char *, const char *>{});

    return [cameraHndl, viewPosHndl, textureMaps = eastl::move(textureMaps)]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      Color4 sunDirC = ShaderGlobal::get_color4(from_sun_directionVarId);
      Point3 sunDir = Point3(sunDirC.r, sunDirC.g, sunDirC.b);
      ShaderGlobal::set_color4(sun_dir_for_shadowsVarId, sunDir); // For WT compatibility
      textureMaps->clearTransient();
      denoiser::TexMap textures = textureMaps->resolveToPair();
      rtsm::render(bvhRenderingContextId, Point3::xyz(viewPosHndl.ref()), sunDir, cameraHndl.ref().jitterProjTm, textures, 0, false,
        false, nullptr, d3d::SamplerHandle::Invalid, nullptr, d3d::SamplerHandle::Invalid, is_rr_enabled() ? 0 : rtsm_quality);
    };
  });
}

static dafg::NodeHandle makeRTSMDynamicNode()
{
  if (!is_rtsm_dynamic_enabled())
    return {};

  return dafg::register_node("rtsm_dynamic", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("combine_shadows_node");
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.readBlob<OrderingToken>("rtao_token").optional();
    registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
    registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("upscale_sampling_tex").optional();
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto viewPosHndl = registry.readBlob<Point4>("world_view_pos").handle();
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);

    auto dynamicLightTexHndl =
      registry.create("dynamic_lighting_texture", dafg::History::No)
        .texture(dafg::Texture2dCreateInfo{TEXFMT_R11G11B10F | TEXCF_UNORDERED, registry.getResolution<2>("main_view"), 1})
        .atStage(dafg::Stage::COMPUTE)
        .useAs(dafg::Usage::SHADER_RESOURCE)
        .handle();
    auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

    return [cameraHndl, viewPosHndl, dynamicLightTexHndl, hasAnyDynamicLightsHndl,
             rtsm_dynamic_minimum_output_valueVarId = get_shader_variable_id("rtsm_dynamic_minimum_output_value")]() {
      if (!hasAnyDynamicLightsHndl.ref())
        return;

      auto &lights = WRDispatcher::getClusteredLights();
      lights.setBuffersToShader();

      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      ShaderGlobal::set_real(rtsm_dynamic_minimum_output_valueVarId, 6.1E-5f); // Smallest value on R11G11B10F
      rtsm::render_dynamic_light_shadows(bvhRenderingContextId, Point3::xyz(viewPosHndl.ref()),
        {dynamicLightTexHndl.view().getTex2D(), dynamicLightTexHndl.view().getTexId()}, dynamic_light_radius, is_rr_enabled());
    };
  });
}

static dafg::NodeHandle makeRTRNode(RTPersistentTexturesECS &rt_persistent_textures)
{
  rt_persistent_textures.clear(RTPersistentTexturesECS::Type::RTR);
  if (!is_rtr_enabled())
    return {};

  denoiser::TexInfoMap persistentTextures;
  rtr::get_required_persistent_texture_descriptors(persistentTextures);

#if DAGOR_DBGLEVEL > 0
  // TODO make this created and destroyed dynamically!
  auto &validation = persistentTextures[::denoiser::ReflectionDenoiser::TextureNames::rtr_validation];
  int width, height;
  d3d::get_screen_size(width, height);
  constexpr auto isHalfRes = true;
  validation.w = isHalfRes ? width / 2 : width;
  validation.h = isHalfRes ? height / 2 : height;
  validation.a = 1;
  validation.d = 1;
  validation.cflg = TEXCF_UNORDERED;
  validation.mipLevels = 1;
#endif

  rt_persistent_textures.allocate(persistentTextures, RTPersistentTexturesECS::Type::RTR);

  return dafg::register_node("rtr", DAFG_PP_NODE_SRC, [&rt_persistent_textures](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.createBlob<OrderingToken>("rtr_token", dafg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);

    registry.read("fom_shadows_sin").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_sin").optional();
    registry.read("fom_shadows_cos").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    denoiser::TexInfoMap persistentTextures;
    rtr::get_required_persistent_texture_descriptors(persistentTextures);
#if DAGOR_DBGLEVEL > 0
    persistentTextures[::denoiser::ReflectionDenoiser::TextureNames::rtr_validation] = {};
#endif
    denoiser::TexInfoMap transientTextures;
    rtr::get_required_transient_texture_descriptors(transientTextures);
    denoiser::TexInfoMap denoiserTextures;
    denoiser::get_required_persistent_texture_descriptors(denoiserTextures, true);

    eastl::vector_map<const char *, const char *> renameTextures;
    if (!is_rr_enabled())
    {
      renameTextures = {{"denoiser_view_z", "denoiser_view_z_rtr"}, {"denoiser_half_view_z", "denoiser_half_view_z_rtr"}};
      for (auto &rename : renameTextures)
        G_VERIFY(denoiserTextures.erase(rename.first) > 0);
    }

    auto textureMaps = eastl::make_unique<RTTextureMaps>(registry, rt_persistent_textures, persistentTextures, transientTextures,
      denoiserTextures, renameTextures);

    return [cameraHndl, textureMaps = eastl::move(textureMaps)]() {
      auto &lights = WRDispatcher::getClusteredLights();
      lights.setOutOfFrustumLightsToShader();
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      textureMaps->clearTransient();
      denoiser::TexMap textures = textureMaps->resolveToPair();
      rtr::render(bvhRenderingContextId, cameraHndl.ref().jitterProjTm, rtr_shadow, rtr_use_csm, textures, !is_rr_enabled());
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

static dafg::NodeHandle makeRTTRNode()
{
  if (!is_rttr_enabled())
    return {};
  auto nodeNs = dafg::root() / "transparent" / "close";
  return nodeNs.registerNode("rttr", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

    auto glassGbufferHndl =
      registry.modify("glass_target").texture().atStage(dafg::Stage::CS).bindToShaderVar("translucent_gbuffer").handle();

    registry.read("glass_depth").texture().atStage(dafg::Stage::CS).bindToShaderVar("translucent_gbuffer_depth");

    return
      [glassGbufferHndl, cameraHndl, glass_rt = Ptr(new_compute_shader("rt_glass_reflection")),
        rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true), rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm", true),
        rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI")]() {
        auto &lights = WRDispatcher::getClusteredLights();
        lights.setOutOfFrustumLightsToShader();
        set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
        TextureInfo glassGbufferTi;
        glassGbufferHndl.view().getTex2D()->getinfo(glassGbufferTi);
        ShaderGlobal::set_int4(rtr_resolutionIVarId, glassGbufferTi.w, glassGbufferTi.h, 0, 0);
        rtr::set_rtr_hit_distance_params();
        ShaderGlobal::set_int(rtr_shadowVarId, rtr_shadow ? 1 : 0);
        ShaderGlobal::set_int(rtr_use_csmVarId, rtr_use_csm ? 1 : 0);
        bvh::bind_resources(bvhRenderingContextId, glassGbufferTi.w);

        bvh::bind_tlas_stage(bvhRenderingContextId, STAGE_CS);
        glass_rt->dispatchThreads(glassGbufferTi.w, glassGbufferTi.h, 1);
        bvh::unbind_tlas_stage(STAGE_CS);
        lights.setInsideOfFrustumLightsToShader();
      };
  });
}

static dafg::NodeHandle makeRTAONode(RTPersistentTexturesECS &rt_persistent_textures)
{
  rt_persistent_textures.clear(RTPersistentTexturesECS::Type::RTAO);
  if (!is_rtao_enabled())
    return {};

  denoiser::TexInfoMap persistentTextures;
  rtao::get_required_persistent_texture_descriptors(persistentTextures);
  rt_persistent_textures.allocate(persistentTextures, RTPersistentTexturesECS::Type::RTAO);

  return dafg::register_node("rtao", DAFG_PP_NODE_SRC, [&rt_persistent_textures](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.createBlob<OrderingToken>("rtao_token", dafg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
    // TODO remove this from the interface of rtao::render
    auto closeDepthHndl =
      registry.read("close_depth").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    denoiser::TexInfoMap persistentTextures;
    rtao::get_required_persistent_texture_descriptors(persistentTextures);
    denoiser::TexInfoMap transientTextures;
    rtao::get_required_transient_texture_descriptors(transientTextures);
    denoiser::TexInfoMap denoiserTextures;
    denoiser::get_required_persistent_texture_descriptors(denoiserTextures, true);
    auto textureMaps = eastl::make_unique<RTTextureMaps>(registry, rt_persistent_textures, persistentTextures, transientTextures,
      denoiserTextures, eastl::vector_map<const char *, const char *>{});

    return [cameraHndl, closeDepthHndl, textureMaps = eastl::move(textureMaps), checkerboard = !is_rr_enabled()]() {
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      textureMaps->clearTransient();
      denoiser::TexMap textures = textureMaps->resolveToPair();
      rtao::render(bvhRenderingContextId, cameraHndl.ref().jitterProjTm, true, closeDepthHndl.view().getTexId(), textures,
        checkerboard);
    };
  });
}

static dafg::NodeHandle makePTGINode(RTPersistentTexturesECS &rt_persistent_textures)
{
  rt_persistent_textures.clear(RTPersistentTexturesECS::Type::PTGI);
  if (!is_ptgi_enabled())
    return {};

  denoiser::TexInfoMap persistentTextures;
  ptgi::get_required_persistent_texture_descriptors(persistentTextures);
  rt_persistent_textures.allocate(persistentTextures, RTPersistentTexturesECS::Type::PTGI);

  return dafg::register_node("ptgi", DAFG_PP_NODE_SRC, [&rt_persistent_textures](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    registry.createBlob<OrderingToken>("ptgi_token", dafg::History::No);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
    // TODO remove this from the interface of ptgi::render
    auto closeDepthHndl =
      registry.read("close_depth").texture().atStage(dafg::Stage::PS_OR_CS).useAs(dafg::Usage::SHADER_RESOURCE).handle();

    denoiser::TexInfoMap persistentTextures;
    ptgi::get_required_persistent_texture_descriptors(persistentTextures);
    denoiser::TexInfoMap transientTextures;
    ptgi::get_required_transient_texture_descriptors(transientTextures);
    denoiser::TexInfoMap denoiserTextures;
    denoiser::get_required_persistent_texture_descriptors(denoiserTextures, true);
    auto textureMaps = eastl::make_unique<RTTextureMaps>(registry, rt_persistent_textures, persistentTextures, transientTextures,
      denoiserTextures, eastl::vector_map<const char *, const char *>{});

    return [cameraHndl, closeDepthHndl, textureMaps = eastl::move(textureMaps)]() {
      auto &lights = WRDispatcher::getClusteredLights();
      lights.setOutOfFrustumLightsToShader();
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      textureMaps->clearTransient();
      denoiser::TexMap textures = textureMaps->resolveToPair();
      ptgi::render(bvhRenderingContextId, cameraHndl.ref().jitterProjTm, closeDepthHndl.view().getTexId(), false, textures,
        static_cast<ptgi::Quality>(ptgi_quality.get()), !is_rr_enabled());
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

static ShaderVariableInfo water_rt_frame_indexVarId = ShaderVariableInfo("water_rt_frame_index", true);

const eastl::array<char const *, eastl::to_underlying(WaterRenderMode::COUNT)> WATER_RT_NODE_NAMES = {
  "water_rt_early_before_envi_node", "water_rt_early_after_envi_node", "water_rt_late_node"};

dafg::NodeHandle makeWaterRTNode(WaterRenderMode mode)
{
  if (!is_rt_water_enabled())
    return {};
  const uint32_t modeIdx = eastl::to_underlying(mode);
  return dafg::register_node(WATER_RT_NODE_NAMES[modeIdx], DAFG_PP_NODE_SRC, [mode, modeIdx](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    registry.readBlob<Point4>("world_view_pos").bindToShaderVar("world_view_pos");

    auto waterModeHndl = registry.readBlob<WaterRenderMode>("water_render_mode").handle();

    registry.read(WATER_NORMAL_DIR_TEX[modeIdx]).texture().atStage(dafg::Stage::CS).bindToShaderVar("water_normal_dir");
    registry.readBlob<OrderingToken>(WATER_SSR_COLOR_TOKEN[modeIdx]);
    auto colorTexHndl = registry.modify(WATER_SSR_COLOR_TEX[modeIdx + 1])
                          .texture()
                          .atStage(dafg::Stage::CS)
                          .bindToShaderVar("water_reflection_tex_uav")
                          .handle();
    registry.modify(WATER_SSR_STRENGTH_TEX[modeIdx + 1])
      .texture()
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("water_reflection_strength_tex_uav");

    registry.historyFor(WATER_SSR_COLOR_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
      .texture()
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("water_reflection_tex");
    registry.historyFor(WATER_SSR_STRENGTH_TEX[eastl::to_underlying(WaterRenderMode::COUNT)])
      .texture()
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("water_reflection_strength_tex");
    registry.read("water_ssr_linear_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("water_reflection_tex_samplerstate");

    registry.read(WATER_SSR_DEPTH_TEX[modeIdx + 1]).texture().atStage(dafg::Stage::CS).bindToShaderVar("downsampled_depth");

    registry.read("fom_shadows_sin").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_sin").optional();
    registry.read("fom_shadows_cos").texture().atStage(dafg::Stage::CS).bindToShaderVar("fom_shadows_cos").optional();
    registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("fom_shadows_cos_samplerstate").optional();

    registry.readTexture("water_planar_reflection_clouds").atStage(dafg::Stage::CS).bindToShaderVar().optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto resHndl = registry.getResolution<2>("main_view", 0.5f);

    return [mode, waterModeHndl, resHndl, cameraHndl, colorTexHndl, water_rt = Ptr(new_compute_shader("raytraced_water_reflections")),
             rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true),
             rtr_use_csmVarId = get_shader_variable_id("rtr_use_csm", true)]() {
      if (waterModeHndl.ref() != mode)
        return;
      auto &lights = WRDispatcher::getClusteredLights();
      lights.setOutOfFrustumLightsToShader();
      static int frameIdx = 0;
      ShaderGlobal::set_int(water_rt_frame_indexVarId, (frameIdx++) % 32);
      ShaderGlobal::set_color4(sun_dir_for_shadowsVarId, ShaderGlobal::get_color4(from_sun_directionVarId)); // For WT compatibility
      set_viewvecs_to_shader(cameraHndl.ref().viewTm, cameraHndl.ref().jitterProjTm);
      ShaderGlobal::set_int(rtr_shadowVarId, rtr_shadow ? 1 : 0);
      ShaderGlobal::set_int(rtr_use_csmVarId, rtr_use_csm ? 1 : 0);
      IPoint2 res = resHndl.get();
      bvh::bind_resources(bvhRenderingContextId, res.x);
      rtr::set_water_params();
      d3d::clear_rt({colorTexHndl.get()}, {});
      bvh::bind_tlas_stage(bvhRenderingContextId, STAGE_CS);
      d3d::set_cs_constbuffer_size(256);
      water_rt->dispatchThreads(res.x, res.y, 1);
      d3d::set_cs_constbuffer_size(0);
      bvh::unbind_tlas_stage(STAGE_CS);
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

template <typename Callable>
static void recreate_bvh_nodes_ecs_query(Callable c);

static void recreateBVHNodes(const RTFeatureChanged &changed)
{
  recreate_bvh_nodes_ecs_query(
    [&](dafg::NodeHandle &bvh__update_node, dafg::NodeHandle &denoiser_prepare_node, dafg::NodeHandle &rtsm_node,
      dafg::NodeHandle &rtsm_dynamic_node, dafg::NodeHandle &rtr_node, dafg::NodeHandle &rttr_node, dafg::NodeHandle &rtao_node,
      dafg::NodeHandle &ptgi_node, dafg::NodeHandle &water_rt_early_before_envi_node, dafg::NodeHandle &water_rt_early_after_envi_node,
      dafg::NodeHandle &water_rt_late_node, RTPersistentTexturesECS &rt_persistent_textures) {
      if (changed.isBVHChanged)
        bvh__update_node = makeBVHUpdateNode();
      if (changed.isDenoiserChanged)
        denoiser_prepare_node = makeDenoiserPrepareNode(rt_persistent_textures);
      if (changed.isRTSMChanged)
        rtsm_node = makeRTSMNode(rt_persistent_textures);
      if (changed.isRTSMDynamicChanged)
        rtsm_dynamic_node = makeRTSMDynamicNode();
      if (changed.isRTRChanged)
        rtr_node = makeRTRNode(rt_persistent_textures);
      if (changed.isRTTRChanged)
        rttr_node = makeRTTRNode();
      if (changed.isRTAOChanged)
        rtao_node = makeRTAONode(rt_persistent_textures);
      if (changed.isPTGIChanged)
        ptgi_node = makePTGINode(rt_persistent_textures);
      if (changed.isRTWaterChanged)
      {
        water_rt_early_before_envi_node = makeWaterRTNode(WaterRenderMode::EARLY_BEFORE_ENVI);
        water_rt_early_after_envi_node = makeWaterRTNode(WaterRenderMode::EARLY_AFTER_ENVI);
        water_rt_late_node = makeWaterRTNode(WaterRenderMode::LATE);
      }
    });
}

ECS_TAG(render)
ECS_REQUIRE(RTPersistentTexturesECS rt_persistent_textures)
static void rt_set_resolution_es(const SetResolutionEvent &resEvt)
{
  // TODO dynamic resolution support
  if (resEvt.type == SetResolutionEvent::Type::DYNAMIC_RESOLUTION)
    return;

  closeRTFeatures();
  int w = resEvt.renderingResolution.x;
  int h = resEvt.renderingResolution.y;
  initRTFeatures(w, h, is_rr_enabled());
  recreateBVHNodes(RTFeatureChanged::All());
};

void setup_unitedvdata_allocation_limits()
{
  auto prepare_united_vdata_limits = [&](auto &unitedVdata, const char *type_nm) {
    int ibLimitsKb = INT_MAX;
    int vbLimitsKb = INT_MAX;
    if (is_bvh_enabled())
    {
      const DataBlock *streamingBlk = dgs_get_settings()->getBlockByNameEx(type_nm);
      const DataBlock *limitsBlk = nullptr;
      G_UNUSED(streamingBlk);

#if _TARGET_SCARLETT || _TARGET_C2
      switch (get_console_model())
      {
        case ConsoleModel::XBOX_ANACONDA: limitsBlk = streamingBlk->getBlockByNameEx("scarlettXRTLimits"); break;
        case ConsoleModel::XBOX_LOCKHART: limitsBlk = streamingBlk->getBlockByNameEx("scarlettSRTLimits"); break;
        case ConsoleModel::PS5_PRO: limitsBlk = streamingBlk->getBlockByNameEx("ps5proRTLimits"); break;
        default: break;
      }
#elif _TARGET_PC
      limitsBlk = streamingBlk->getBlockByNameEx("pcRTLimits");
#endif

      if (limitsBlk) //-V547
      {
        ibLimitsKb = limitsBlk->getInt("ibLimitsKb", INT_MAX);
        vbLimitsKb = limitsBlk->getInt("vbLimitsKb", INT_MAX);
      }
    }

    debug("Setting %s limits to ib: %d kb, vb: %d kb", type_nm, ibLimitsKb, vbLimitsKb);

    unitedVdata.setAllocationLimits(ibLimitsKb, vbLimitsKb);
  };

  prepare_united_vdata_limits(unitedvdata::dmUnitedVdata, "unitedVdata.dynModel");
  prepare_united_vdata_limits(unitedvdata::riUnitedVdata, "unitedVdata.rendInst");
}


template <typename Callable>
static void bvh_destroy_ri_visibility_ecs_query(Callable c);
template <typename Callable>
static void bvh_create_ri_visibility_ecs_query(Callable c);

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady)
ECS_TRACK(render_settings__enableBVH,
  render_settings__enableRTSM,
  render_settings__enableRTR,
  render_settings__enableRTTR,
  render_settings__enableRTAO,
  render_settings__enablePTGI,
  render_settings__RTRWater,
  render_settings__enableRTGI,
  render_settings__bare_minimum,
  render_settings__antiAliasingMode,
  render_settings__rayReconstruction)
static void bvh_render_settings_changed_es(const ecs::Event &,
  bool render_settings__enableBVH,
  const ecs::string &render_settings__enableRTSM,
  bool render_settings__enableRTR,
  bool render_settings__enableRTTR,
  bool render_settings__enableRTAO,
  bool render_settings__enablePTGI,
  bool render_settings__RTRWater,
  bool render_settings__enableRTGI,
  bool render_settings__bare_minimum,
  int render_settings__antiAliasingMode,
  bool render_settings__rayReconstruction)
{
  ResolvedRTSettings oldSettings = get_resolved_rt_settings();

  ResolvedRTSettings settings;
  const bool isBVHAvailable = bvh::is_available() && !render_settings__bare_minimum;
  settings.isRTSMEnabled =
    isBVHAvailable && (render_settings__enableRTSM == "sun_and_dynamic" || render_settings__enableRTSM == "sun");
  settings.isRTSMDynamicEnabled = isBVHAvailable && render_settings__enableRTSM == "sun_and_dynamic";
  settings.isRTREnabled = isBVHAvailable && render_settings__enableRTR;
  settings.isRTTREnabled = isBVHAvailable && render_settings__enableRTTR;
  settings.isRTAOEnabled = isBVHAvailable && render_settings__enableRTAO;
  settings.isRTGIEnabled = isBVHAvailable && render_settings__enableRTGI;
  settings.isPTGIEnabled = isBVHAvailable && render_settings__enablePTGI && !settings.isRTGIEnabled && !settings.isRTAOEnabled;
  settings.isRTWaterEnabled = isBVHAvailable && render_settings__RTRWater;
  settings.isDenoiserEnabled = settings.isRTSMEnabled || settings.isRTREnabled || settings.isRTAOEnabled || settings.isPTGIEnabled;
  settings.isRayReconstructionEnabled = isBVHAvailable &&
                                        render_settings__antiAliasingMode == eastl::to_underlying(AntiAliasingMode::DLSS) &&
                                        render_settings__rayReconstruction;
  settings.isRTREnabled = settings.isRTREnabled || settings.isRayReconstructionEnabled;
  // Allow for debugging in the ImGUI window, even with no render features
  constexpr bool isDebug = DAGOR_DBGLEVEL > 0;
  settings.isBVHEnabled = (isDebug && isBVHAvailable && render_settings__enableBVH) || settings.isRTSMEnabled ||
                          settings.isRTTREnabled || settings.isRTREnabled || settings.isRTAOEnabled || settings.isPTGIEnabled ||
                          settings.isRTWaterEnabled || settings.isRTGIEnabled;
  set_resolved_rt_settings_ecs_query([&settings](ResolvedRTSettings &resolved_rt_settings, bool needs_water_heightmap) {
    resolved_rt_settings = settings;
    needs_water_heightmap = settings.isBVHEnabled;
  });

  RTFeatureChanged changed = RTFeatureChanged(oldSettings, settings);

  closeRTFeatures();
  if (oldSettings.isBVHEnabled && !settings.isBVHEnabled)
  {
    closeBVHScene();
    bvh_destroy_ri_visibility_ecs_query(
      [](bool &bvh__initialized, RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility) {
        bvh__initialized = false;
        wait_bvh_worker_threads();
        rendinst::destroyRIGenVisibility(bvh__rendinst_visibility.visibility);
        bvh__rendinst_visibility.visibility = nullptr;
        rendinst::destroyRIGenVisibility(bvh__rendinst_oof_visibility.visibility);
        bvh__rendinst_oof_visibility.visibility = nullptr;
      });
    closeBVH();
    setup_unitedvdata_allocation_limits();
  }
  else if (!oldSettings.isBVHEnabled && settings.isBVHEnabled)
  {
    initBVH();
    bvh_create_ri_visibility_ecs_query(
      [](RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility) {
        bvh__rendinst_visibility.visibility = rendinst::createRIGenVisibility(midmem);
        bvh__rendinst_oof_visibility.visibility = rendinst::createRIGenVisibility(midmem);
      });
    setup_unitedvdata_allocation_limits();
  }
  if (get_world_renderer()) // Can't query resolution on login screen
  {
    int w, h;
    get_max_possible_rendering_resolution(w, h);
    initRTFeatures(w, h, settings.isRayReconstructionEnabled);
    recreateBVHNodes(changed);

    // Shadow subscribes to render_settings__enableRTSM
    uint32_t featuresToReset = 0;
    if (changed.isRTWaterChanged)
      featuresToReset |= WRDispatcher::WATER;
    if (changed.isRTAOChanged)
      featuresToReset |= WRDispatcher::SSAO;
    if (changed.isPTGIChanged)
      featuresToReset |= WRDispatcher::GI | WRDispatcher::SSAO;
    if (changed.isRTGIChanged)
      featuresToReset |= WRDispatcher::GI;
    if (changed.isRTRChanged)
      featuresToReset |= WRDispatcher::SSR;
    if (changed.isBVHChanged)
      featuresToReset |= WRDispatcher::DYNAMIC_RESOLUTION;
    if (changed.isDenoiserChanged)
      featuresToReset |= WRDispatcher::MOTION_VECTOR;
    WRDispatcher::recreateRayTracingDependentNodes(featuresToReset);
  }
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
ECS_REQUIRE(dafg::NodeHandle bvh__update_node)
static void bvh_update_animchar_es(const UpdateStageInfoBeforeRender &stg)
{
  if (!is_bvh_enabled())
    return;
  TIME_PROFILE(bvh_update_animchar)
  TMatrix4 cullingMatrix = get_bvh_culling_matrix(stg.camPos);
  Frustum frustum = cullingMatrix;
  preprocess_visible_animchars_in_frustum(stg, frustum, v_ldu_p3(&stg.camPos.x), VISFLG_BVH);
}

static void bvh_set_dynmodel_instance_data(int istance_offset)
{
  if (!bvhDynmodelInstanceBuffer)
    return;
  constexpr int immediateConstCount = 2;
  const uint32_t immediateConsts[immediateConstCount] = {(uint32_t)(bvhDynmodelInstanceBuffer->curOffset + istance_offset), 0u};

  d3d::set_immediate_const(STAGE_CS, immediateConsts, immediateConstCount);
}

template <typename Callable>
static void bvh_iterate_over_animchars_ecs_query(Callable c);
template <typename Callable>
static void bvh_query_camo_params_ecs_query(ecs::EntityId eid, Callable c);
static ShaderVariableInfo burnt_tank_camoVarId("burnt_tank_camo", true);

static void bvh_iterate_over_animchars(
  dynrend::BVHIterateOneInstanceCallback iterate_one_instance, const Point3 &view_position, void *user_data)
{
  bvhDynmodelState->mode = draw_debug_dynmodels ? dynmodel_renderer::DynModelRenderingState::FOR_RENDERING
                                                : dynmodel_renderer::DynModelRenderingState::ONLY_PER_INSTANCE_DATA;
  TEXTUREID burnt_tank_camo = burnt_tank_camoVarId.get_texture();
  auto getCamoData = [&](ecs::EntityId eid) {
    dynrend::BVHCamoData bvhCamoData;
    bvh_query_camo_params_ecs_query(eid, [&](float vehicle_camo_condition, float vehicle_camo_scale, float vehicle_camo_rotation) {
      bvhCamoData.condition = vehicle_camo_condition;
      bvhCamoData.scale = vehicle_camo_scale;
      bvhCamoData.rotation = vehicle_camo_rotation;
    });
    bvhCamoData.burntCamo = burnt_tank_camo;
    return bvhCamoData;
  };
  vec3f viewPosition = v_ldu(&view_position.x);
  bvh_iterate_over_animchars_ecs_query([&](ECS_REQUIRE_NOT(const Point3 semi_transparent__placingColor,
                                             ecs::Tag invisibleUpdatableAnimchar) ecs::EntityId eid,
                                         const animchar_visbits_t &animchar_visbits,
                                         const AnimV20::AnimcharRendComponent &animchar_render, const ecs::Point4List *additional_data,
                                         const ecs::UInt8List *animchar_render__nodeVisibleStgFilters, const vec4f &animchar_bsph) {
    if (!(animchar_visbits & VISFLG_BVH))
      return;

    const DynamicRenderableSceneInstance *inst = animchar_render.getSceneInstance();
    G_ASSERT_RETURN(inst != nullptr, );
    const auto res = inst->getCurSceneResource();
    if (!res)
      return;

    const uint8_t *filter = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->begin() : nullptr;
    const uint32_t pathFilterSize = animchar_render__nodeVisibleStgFilters ? animchar_render__nodeVisibleStgFilters->size() : 0;
    G_ASSERT(!animchar_render__nodeVisibleStgFilters || pathFilterSize == inst->getNodeCount());

    animchar_additional_data::AnimcharAdditionalDataView additionalDataView =
      draw_debug_dynmodels ? animchar_additional_data::get_optional_data(additional_data) : animchar_additional_data::get_null_data();

    eastl::vector<int, framemem_allocator> offsets;
    bvhDynmodelState->process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_atest, inst, res, additionalDataView,
      draw_debug_dynmodels, nullptr, filter, pathFilterSize, UpdateStageInfoRender::RENDER_SHADOW | UpdateStageInfoRender::RENDER_MAIN,
      false, dynmodel_renderer::RenderPriority::DEFAULT, nullptr, TexStreamingContext(0), &offsets);

    dynrend::BVHCamoData bvhCamoData = getCamoData(eid);
    bool animate = [viewPosition, animchar_visbits, position = animchar_bsph]() {
      if (static_cast<bool>(animchar_visbits & (VISFLG_MAIN_CAMERA_RENDERED | VISFLG_CSM_SHADOW_RENDERED | VISFLG_COCKPIT_VISIBLE)))
        return true;
      if (v_extract_x(v_length3_sq_x(v_sub(position, viewPosition))) < animate_all_animchars_range * animate_all_animchars_range)
        return true;
      return false;
    }();
    iterate_one_instance(*inst, *res, filter, pathFilterSize,
      UpdateStageInfoRender::RENDER_SHADOW | UpdateStageInfoRender::RENDER_MAIN, offsets, bvh_set_dynmodel_instance_data, animate,
      bvhCamoData, user_data);
  });
  BVHAdditionalAnimcharIterateCallback additonalAnimcharCallback =
    [&](ecs::EntityId eid, DynamicRenderableSceneInstance *inst, DynamicRenderableSceneResource *res,
      animchar_additional_data::AnimcharAdditionalDataView additional_data_view, const animchar_visbits_t &animchar_visbits) {
      if (!(animchar_visbits & VISFLG_BVH))
        return;
      if (!res)
        return;

      animchar_additional_data::AnimcharAdditionalDataView additionalDataView =
        draw_debug_dynmodels ? additional_data_view : animchar_additional_data::get_null_data();

      eastl::vector<int, framemem_allocator> offsets;
      bvhDynmodelState->process_animchar(ShaderMesh::STG_opaque, ShaderMesh::STG_atest, inst, res, additionalDataView,
        draw_debug_dynmodels, nullptr, nullptr, 0, 0, false, dynmodel_renderer::RenderPriority::DEFAULT, nullptr,
        TexStreamingContext(0), &offsets);

      dynrend::BVHCamoData bvhCamoData = getCamoData(eid);
      bool animate =
        static_cast<bool>(animchar_visbits & (VISFLG_MAIN_CAMERA_RENDERED | VISFLG_CSM_SHADOW_RENDERED | VISFLG_COCKPIT_VISIBLE));
      iterate_one_instance(*inst, *res, nullptr, 0, 0, offsets, bvh_set_dynmodel_instance_data, animate, bvhCamoData, user_data);
    };
  g_entity_mgr->broadcastEventImmediate(BVHAdditionalAnimcharIterate(additonalAnimcharCallback));
}

static dafg::NodeHandle makeBvhDrawDebugNode()
{
  return dafg::register_node("bvh_debug_dynmodels_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    auto dynmodelDebugRT = registry.create("bvh_dynmodel_debug", dafg::History::No)
                             .texture({TEXCF_RTARGET | TEXFMT_R11G11B10F, registry.getResolution<2>("main_view")})
                             .clear(make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
    auto dynmodelDebugDepthRT = registry.create("bvh_dynmodel_debug_depth", dafg::History::No)
                                  .texture({TEXCF_RTARGET | TEXFMT_DEPTH32, registry.getResolution<2>("main_view")})
                                  .clear(make_clear_value(0.0f, 0.0f, 0.0f, 0.0f));
    registry.requestRenderPass().color({dynmodelDebugRT}).depthRw(dynmodelDebugDepthRT);
    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<OrderingToken>("bvh_ready_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass"),
             dynamicSceneBlockId = ShaderGlobal::getBlockId("dynamic_scene")]() {
      if (!bvhDynmodelState || !bvhDynmodelInstanceBuffer)
        return;
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      SCOPE_VIEW_PROJ_MATRIX;
      d3d::settm(TM_PROJ, &cameraHndl.ref().jitterProjTm);
      TMatrix vtm = cameraHndl.ref().viewTm;
      vtm.setcol(3, 0, 0, 0);
      d3d::settm(TM_VIEW, vtm);
      bvhDynmodelState->prepareForRender();
      bvhDynmodelState->setVars(bvhDynmodelInstanceBuffer->buffer.getBufId());
      SCENE_LAYER_GUARD(dynamicSceneBlockId);
      bvhDynmodelState->render(bvhDynmodelInstanceBuffer->curOffset);
    };
  });
}

ECS_TAG(render, dev)
static void recreate_draw_dynmodel_debug_node_es(
  const UpdateStageInfoBeforeRender &, dafg::NodeHandle &bvh_debug_dynmodels_node, int &bvh_debug_dynmodels_console_command)
{
  enum ConsoleCommandToRun
  {
    NO_CONSOLE_COMMAD = 0,
    SHOW_TEX,
    HIDE_TEX
  };
  // Delay 1 frame, because FG needs to be recompiled for the console command to find the texture
  if (bvh_debug_dynmodels_console_command == SHOW_TEX)
    console::command("dafg.show_tex bvh_dynmodel_debug");
  else if (bvh_debug_dynmodels_console_command == HIDE_TEX)
    console::command("dafg.show_tex");
  bvh_debug_dynmodels_console_command = NO_CONSOLE_COMMAD;

  bool needNode = is_bvh_enabled() && draw_debug_dynmodels;
  if (needNode && !bvh_debug_dynmodels_node)
  {
    bvh_debug_dynmodels_node = makeBvhDrawDebugNode();
    bvh_debug_dynmodels_console_command = SHOW_TEX;
  }
  else if (!needNode && bvh_debug_dynmodels_node)
  {
    bvh_debug_dynmodels_node = {};
    bvh_debug_dynmodels_console_command = HIDE_TEX;
  }
}

namespace bvh
{
static void process_elem(const ShaderMesh::RElem &elem,
  MeshInfo &mesh_info,
  const ChannelParser &parser,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures)
{
  G_UNUSED(parser);
  static int atestVarId = get_shader_variable_id("atest");
  static int paint_detailsVarId = get_shader_variable_id("paint_details");
  static int palette_indexVarId = get_shader_variable_id("palette_index");
  static int use_paintingVarId = get_shader_variable_id("use_painting");
  static int paint_palette_rowVarId = get_shader_variable_id("paint_palette_row", true);
  static int paint_white_pointVarId = get_shader_variable_id("paint_white_point", true);
  static int paint_black_pointVarId = get_shader_variable_id("paint_black_point", true);
  static int use_alpha_as_maskVarId = get_shader_variable_id("use_alpha_as_mask", true);
  static int paint_multVarId = get_shader_variable_id("paint_mult", true);
  static int detailsTileVarId = get_shader_variable_id("details_tile");
  static int invert_height1VarId = get_shader_variable_id("invert_height1", true);
  static int invert_height2VarId = get_shader_variable_id("invert_height2", true);
  static int paint_pointsVarId = get_shader_variable_id("paint_points");
  static int atlas_tile_uVarId = get_shader_variable_id("atlas_tile_u");
  static int atlas_tile_vVarId = get_shader_variable_id("atlas_tile_v");
  static int atlas_first_tileVarId = get_shader_variable_id("atlas_first_tile");
  static int atlas_last_tileVarId = get_shader_variable_id("atlas_last_tile");
  static int detail0_const_colorVarId = get_shader_variable_id("detail0_const_color");
  static int detail1_const_colorVarId = get_shader_variable_id("detail1_const_color");
  static int detail2_const_colorVarId = get_shader_variable_id("detail2_const_color");
  static int mask_by_normalVarId = get_shader_variable_id("mask_by_normal");
  static int invert_heightsVarId = get_shader_variable_id("invert_heights");
  static int paint_const_colorVarId = get_shader_variable_id("paint_const_color");
  static int is_rendinst_clipmapVarId = get_shader_variable_id("is_rendinst_clipmap");
  static int mask_gammaVarId = get_shader_variable_id("mask_gamma");
  static int emissive_colorVarId = get_shader_variable_id("emissive_color", true);
  static int emission_albedo_multVarId = get_shader_variable_id("emission_albedo_mult", true);
  static int parameters_channelVarId = get_shader_variable_id("parameters_channel", true);
  static int nightlyVarId = get_shader_variable_id("nightly", true);
  static int use_alpha_for_emission_maskVarId = get_shader_variable_id("use_alpha_for_emission_mask", true);

  int isClipmap = 0;
  elem.mat->getIntVariable(is_rendinst_clipmapVarId, isClipmap);
  bool isTree = strncmp(elem.mat->getShaderClassName(), "rendinst_tree", 13) == 0;
  bool isLayered = strncmp(elem.mat->getShaderClassName(), "rendinst_layered", 16) == 0 ||
                   strncmp(elem.mat->getShaderClassName(), "dynamic_layered", 15) == 0;
  bool isMaskLayered = strncmp(elem.mat->getShaderClassName(), "rendinst_mask_layered", 21) == 0;
  bool isPerlinLayered = strncmp(elem.mat->getShaderClassName(), "rendinst_perlin_layered", 23) == 0 ||
                         strncmp(elem.mat->getShaderClassName(), "rendinst_tree_perlin_layered", 28) == 0;
  bool isCamo = strcmp(elem.mat->getShaderClassName(), "dynamic_masked_tank") == 0 ||
                strcmp(elem.mat->getShaderClassName(), "dynamic_masked_weapon") == 0 ||
                strcmp(elem.mat->getShaderClassName(), "dynamic_masked_ship") == 0;
  bool isEye = strncmp(elem.mat->getShaderClassName(), "dynamic_eye", 12) == 0;

  TEXTUREID alphaTextureId = BAD_TEXTUREID;

  int alphaTestValue = 0;
  bool hasAlphaTest = (elem.mat->getIntVariable(atestVarId, alphaTestValue) && alphaTestValue);

  // A bit of a hack, but in short, this shader is barely used, and the models that use it only have tiny holes.
  // For BVHs, this adds useless overhead and we can save performance by treating it as opaque.
  if (strcmp(elem.e->getShaderClassName(), "rendinst_vcolor_layered") == 0)
    hasAlphaTest = false;

  if (hasAlphaTest)
  {
    if (isTree)
      alphaTextureId = elem.mat->get_texture(1);
  }

  if (impostor_textures)
    hasAlphaTest = true;

  int use_paintingValue = 0;
  elem.mat->getIntVariable(use_paintingVarId, use_paintingValue);
  int palette_indexValue = 0;
  elem.mat->getIntVariable(palette_indexVarId, palette_indexValue);

  bool isEmissive = false;
  Color4 emissive_color = Color4(1, 1, 1, 1);
  float emission_albedo_mult = 1;
  int nightly = 0;
  int use_alpha_for_emission_mask = 1;
  if (elem.mat->getColor4Variable(emissive_colorVarId, emissive_color) &&
      elem.mat->getRealVariable(emission_albedo_multVarId, emission_albedo_mult))
  {
    int parameters_channel = -1;
    elem.mat->getIntVariable(parameters_channelVarId, parameters_channel);
    if (parameters_channel < 0) // Not supporting per instance data emissive!
    {
      isEmissive = true;
      emissive_color.r = pow(emissive_color.r, 2.2f);
      emissive_color.g = pow(emissive_color.g, 2.2f);
      emissive_color.b = pow(emissive_color.b, 2.2f);
      emissive_color.a = max(emissive_color.r, max(emissive_color.g, emissive_color.b)) * emissive_color.a;

      elem.mat->getIntVariable(nightlyVarId, nightly);
      elem.mat->getIntVariable(use_alpha_for_emission_maskVarId, use_alpha_for_emission_mask);
    }
  }

  int paint_palette_rowValue = 1;
  float paint_details_strength = 0;
  int use_alpha_as_maskValue = 0;
  Point4 detailsData = Point4(0.0, 0.0, 0.0, 0.0);
  uint32_t packedDetailsData = 0;
  if (elem.mat->getIntVariable(paint_palette_rowVarId, paint_palette_rowValue)) // INIT_SIMPLE_PAINTED
  {
    float paint_detailsValue = 0;
    elem.mat->getRealVariable(paint_detailsVarId, paint_detailsValue);
    paint_details_strength = paint_detailsValue;

    float paint_white_pointValue = 0.5;
    elem.mat->getRealVariable(paint_white_pointVarId, paint_white_pointValue);

    float paint_black_pointValue = 0.1;
    elem.mat->getRealVariable(paint_black_pointVarId, paint_black_pointValue);

    float paint_multValue = 0.0;
    elem.mat->getRealVariable(paint_multVarId, paint_multValue);

    elem.mat->getIntVariable(use_alpha_as_maskVarId, use_alpha_as_maskValue);

    const uint32_t use_alpha_as_maskBit = use_alpha_as_maskValue ? 1 : 0;

    packedDetailsData = (static_cast<int>(clamp(paint_white_pointValue * 1023.0f, 0.0f, 1023.0f)) & 0x3FF) |
                        ((static_cast<int>(clamp(paint_black_pointValue * 1023.0f, 0.0f, 1023.0f)) & 0x3FF) << 10) |
                        ((static_cast<int>(clamp(paint_multValue * 1023.0f, 0.0f, 1023.0f)) & 0x3FF) << 20) |
                        (use_alpha_as_maskBit << 30);
  }
  else
  {
    Color4 paint_detailsValue = Color4(1, 1, 1);
    elem.mat->getColor4Variable(paint_detailsVarId, paint_detailsValue);
    paint_palette_rowValue = paint_detailsValue.a;
    paint_details_strength = paint_detailsValue.r;
  }

  // Only one of these is used at the same time, the other will just fail
  Color4 colorOverride = Color4(0.5, 0.5, 0.5, 0);
  elem.mat->getColor4Variable(detail0_const_colorVarId, colorOverride);
  elem.mat->getColor4Variable(paint_const_colorVarId, colorOverride);

  float atlas_tile_uValue = 1;
  float atlas_tile_vValue = 1;
  int atlas_first_tileValue = 0;
  int atlas_last_tileValue = 0;

  if (isLayered)
  {
    elem.mat->getRealVariable(atlas_tile_uVarId, atlas_tile_uValue);
    elem.mat->getRealVariable(atlas_tile_vVarId, atlas_tile_vValue);
    elem.mat->getIntVariable(atlas_first_tileVarId, atlas_first_tileValue);
    elem.mat->getIntVariable(atlas_last_tileVarId, atlas_last_tileValue);

    G_ASSERT(atlas_first_tileValue >= 0);
    G_ASSERT(atlas_last_tileValue >= atlas_first_tileValue);
  }

  float invert_height1Value = 0;
  float invert_height2Value = 0;
  Color4 paint_pointsValue = Color4(0, 0.00001, 0, 0.00001);

  if (isMaskLayered)
  {
    elem.mat->getRealVariable(invert_height1VarId, invert_height1Value);
    elem.mat->getRealVariable(invert_height2VarId, invert_height2Value);
    elem.mat->getColor4Variable(paint_pointsVarId, paint_pointsValue);
  }

  if (isClipmap)
    mesh_info.isClipmap = true;
  if (isEye)
    mesh_info.isEye = true;

  if (use_paintingValue > 0)
  {
    mesh_info.painted = true;
    mesh_info.paintData =
      Point4(paint_palette_rowValue, palette_indexValue, bitwise_cast<float>(packedDetailsData), paint_details_strength);
  }
  mesh_info.colorOverride = Point4::rgba(colorOverride);

  if (isEmissive)
  {
    mesh_info.isEmissive = true;
    mesh_info.paintData = Point4::rgba(emissive_color);
    mesh_info.colorOverride = Point4(nightly, use_alpha_for_emission_mask, emission_albedo_mult, 0);
  }

  if (isLayered)
  {
    mesh_info.isLayered = true;
    mesh_info.useAtlas = true;
    mesh_info.atlasTileU = atlas_tile_uValue;
    mesh_info.atlasTileV = atlas_tile_vValue;
    mesh_info.atlasFirstTile = atlas_first_tileValue;
    mesh_info.atlasLastTile = atlas_last_tileValue;
  }

  if (isMaskLayered)
  {
    mesh_info.isLayered = true;
    mesh_info.useAtlas = false;
  }

  if (isMaskLayered || isLayered)
  {
    mesh_info.painted = true; // need to pass colorOverride into materialdata2

    mesh_info.alphaTextureId = elem.mat->get_texture(3);
    mesh_info.extraTextureId = elem.mat->get_texture(5);

    Color4 paint_detailsValue = Color4(1, 1, 1);
    elem.mat->getColor4Variable(paint_detailsVarId, paint_detailsValue);

    uint32_t packedData1 = uint32_t(float_to_half(invert_height1Value)) | uint32_t(float_to_half(invert_height2Value)) << 16;
    uint32_t packedData2 = uint32_t(float_to_half(paint_detailsValue.r)) | uint32_t(float_to_half(paint_detailsValue.g)) << 16;
    uint32_t packedData3 = uint32_t(float_to_half(paint_pointsValue.r)) | uint32_t(float_to_half(paint_pointsValue.g)) << 16;
    uint32_t packedData4 = uint32_t(float_to_half(paint_pointsValue.b)) | uint32_t(float_to_half(paint_pointsValue.a)) << 16;

    mesh_info.colorOverride = Point4(bitwise_cast<float>(packedData1), bitwise_cast<float>(packedData2),
      bitwise_cast<float>(packedData3), bitwise_cast<float>(packedData4));
  }

  if (isPerlinLayered)
  {
    mesh_info.isPerlinLayered = true;

    Color4 normalMask;
    Color4 invertHeights;
    Color4 mask_gammaValue = Color4(1, 1, 1, 0.25);

    Color4 colorOverride1 = Color4(0.5, 0.5, 0.5, 0.0);
    Color4 colorOverride2 = Color4(0.5, 0.5, 0.5, 0.0);

    elem.mat->getColor4Variable(mask_gammaVarId, mask_gammaValue);
    elem.mat->getColor4Variable(mask_by_normalVarId, normalMask);
    elem.mat->getColor4Variable(invert_heightsVarId, invertHeights);

    elem.mat->getColor4Variable(detail1_const_colorVarId, colorOverride1);
    elem.mat->getColor4Variable(detail2_const_colorVarId, colorOverride2);

    Color4 paint_detailsValue = Color4(1, 1, 1);
    elem.mat->getColor4Variable(paint_detailsVarId, paint_detailsValue);
    paint_palette_rowValue = paint_detailsValue.a;

    mesh_info.detailsData1 = uint32_t(float_to_half(mask_gammaValue.r)) | uint32_t(float_to_half(mask_gammaValue.g)) << 16;
    mesh_info.detailsData2 = uint32_t(float_to_half(mask_gammaValue.b)) | uint32_t(float_to_half(mask_gammaValue.a)) << 16;
    mesh_info.detailsData3 = uint32_t(float_to_half(invertHeights.r)) | uint32_t(float_to_half(invertHeights.g)) << 16;
    mesh_info.detailsData4 = uint32_t(float_to_half(invertHeights.b)) | uint32_t(float_to_half(invertHeights.a)) << 16;

    uint32_t packedColor1 = (static_cast<int>(clamp(colorOverride.r * 255.0f, 0.0f, 255.0f)) & 0xFF) |
                            ((static_cast<int>(clamp(colorOverride.g * 255.0f, 0.0f, 255.0f)) & 0xFF) << 8) |
                            ((static_cast<int>(clamp(colorOverride.b * 255.0f, 0.0f, 255.0f)) & 0xFF) << 16) |
                            ((static_cast<int>(clamp(colorOverride.a * 255.0f, 0.0f, 255.0f)) & 0xFF) << 24);

    uint32_t packedColor2 = (static_cast<int>(clamp(colorOverride1.r * 255.0f, 0.0f, 255.0f)) & 0xFF) |
                            ((static_cast<int>(clamp(colorOverride1.g * 255.0f, 0.0f, 255.0f)) & 0xFF) << 8) |
                            ((static_cast<int>(clamp(colorOverride1.b * 255.0f, 0.0f, 255.0f)) & 0xFF) << 16) |
                            ((static_cast<int>(clamp(colorOverride1.a * 255.0f, 0.0f, 255.0f)) & 0xFF) << 24);

    uint32_t packedColor3 = (static_cast<int>(clamp(colorOverride2.r * 255.0f, 0.0f, 255.0f)) & 0xFF) |
                            ((static_cast<int>(clamp(colorOverride2.g * 255.0f, 0.0f, 255.0f)) & 0xFF) << 8) |
                            ((static_cast<int>(clamp(colorOverride2.b * 255.0f, 0.0f, 255.0f)) & 0xFF) << 16) |
                            ((static_cast<int>(clamp(colorOverride2.a * 255.0f, 0.0f, 255.0f)) & 0xFF) << 24);

    uint32_t packedNormal1 = uint32_t(float_to_half(normalMask.r)) | uint32_t(float_to_half(normalMask.g)) << 16;
    uint32_t packedNormal2 = uint32_t(float_to_half(normalMask.b)) | uint32_t(float_to_half(normalMask.a)) << 16;

    mesh_info.paintData = Point4(bitwise_cast<float>(packedNormal1), bitwise_cast<float>(packedNormal2),
      bitwise_cast<float>(packedColor1), bitwise_cast<float>(packedColor2));

    uint32_t packedPaintDetails1 = uint32_t(float_to_half(paint_detailsValue.r)) | uint32_t(float_to_half(paint_detailsValue.g)) << 16;
    uint32_t packedPaintDetails2 = uint32_t(float_to_half(paint_detailsValue.b)); // here is 16 free bits
    mesh_info.colorOverride = Point4(bitwise_cast<float>(packedPaintDetails1), bitwise_cast<float>(packedPaintDetails2),
      bitwise_cast<float>(packedColor3), 0.0);

    mesh_info.atlasTileU = paint_palette_rowValue;
    mesh_info.atlasTileV = palette_indexValue;

    mesh_info.alphaTextureId = elem.mat->get_texture(3);
    mesh_info.extraTextureId = elem.mat->get_texture(5);
  }
  Color4 detailsTile(1, 1, 1, 0);
  if (elem.mat->getColor4Variable(detailsTileVarId, detailsTile))
    mesh_info.texcoordScale = detailsTile.r;

  if (impostor_params)
  {
    mesh_info.isImpostor = true;
    mesh_info.impostorHeightOffset = impostor_params->boundingSphere.y;
    mesh_info.impostorSliceTm1 = impostor_params->perSliceParams[2].sliceTm;
    mesh_info.impostorSliceTm2 = impostor_params->perSliceParams[0].sliceTm;
    mesh_info.impostorSliceClippingLines1 = impostor_params->perSliceParams[2].clippingLines;
    mesh_info.impostorSliceClippingLines2 = impostor_params->perSliceParams[0].clippingLines;
    mesh_info.impostorScale = impostor_params->scale;
    memcpy(mesh_info.impostorOffsets, impostor_params->vertexOffsets.begin(), sizeof(mesh_info.impostorOffsets));
  }

  if (!isPerlinLayered && !isMaskLayered && !isLayered)
  {
    mesh_info.alphaTextureId = alphaTextureId;
  }

  mesh_info.albedoTextureId = impostor_textures ? impostor_textures->albedo_alpha : elem.mat->get_texture(0);
  mesh_info.normalTextureId = elem.mat->get_texture(2);
  mesh_info.alphaTest = hasAlphaTest;
  mesh_info.isCamo = isCamo;
}
} // namespace bvh
