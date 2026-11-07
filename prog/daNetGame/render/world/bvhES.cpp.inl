// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <render/world/bvh.h>
#include <bvh/bvh.h>
#include <render/denoiser/denoiser_api.h>
#include <render/rtsm/rtsm_api.h>
#include <render/rtr/rtr_api.h>
#include <render/rtao/rtao_api.h>
#include <render/ptgi/ptgi_api.h>
#include <bvh/bvh_connection.h>

#include <render/daFrameGraph/daFG.h>
#include "EASTL/internal/type_properties.h"
#include "drv/3d/dag_renderTarget.h"
#include "frameGraphHelpers.h"
#include <landMesh/lmeshManager.h>
#include <render/viewVecs.h>
#include <render/cameraParams.h>
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
#include <osApiWrappers/dag_critSec.h>

#include <daECS/core/coreEvents.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/entityManager.h>
#include <render/renderEvent.h>
#include <util/dag_convar.h>
#include <startup/dag_globalSettings.h>
#include <drv/3d/dag_rwResource.h>
#include <render/lights/clusteredLights.h>
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
#include <render/dynmodelRenderer.h>
#include <drv/3d/dag_shaderConstants.h>
#include <drv/3d/dag_matricesAndPerspective.h>
#include <render/world/dynModelRenderPass.h>
#include <util/dag_threadPool.h>
#include <rendInst/rendInstGen.h>
#include <rendInst/rendInstExtraAccess.h>
#include <rendInst/rendInstExtraRender.h>
#include <camera/sceneCam.h>
#include <rendInst/constants.h>
#include <render/rendererFeatures.h>
#include <render/antialiasing.h>
#include <osApiWrappers/dag_miscApi.h>
#include <render/grass/grassRenderer.h>
#include <bvh/bvh_processors.h>
#include <drv/3d/dag_viewScissor.h>
#include <3d/dag_nvFeatures.h>
#include <render/dasModules/bvh.h>
#include <generic/dag_sort.h>
#include <drv/3d/dag_commands.h>
#include <drv/3d/dag_lock.h>
#include <EASTL/optional.h>
#include <EASTL/vector_set.h>
#include <userSystemInfo/systemInfo.h>
#include <folders/folders.h>

// To reduce shader variants these got assumed in raytracing.blk
// CONSOLE_BOOL_VAL("raytracing", rtr_shadow, false, "Also controls debug view!");
constexpr bool rtr_shadow = false;
CONSOLE_INT_VAL("raytracing", ptgi_quality, 0, 0, 2);
CONSOLE_BOOL_VAL("raytracing", ptgi_half_rate, true);
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
CONSOLE_FLOAT_VAL("raytracing",
  dynamic_light_radius,
  0.1,
  "Used for smooth shadows when DLSS RR is enabled, otherwise just a bias to fix lamp self occlusion.");
CONSOLE_BOOL_VAL("raytracing", draw_debug_dynmodels, false);
CONSOLE_INT_VAL("raytracing", max_ahs_calls, 10, 1, 1000);
CONSOLE_BOOL_VAL("raytracing", prepare_ri_extra_job_on_previous_frame, true);
CONSOLE_BOOL_VAL("raytracing", delay_PUFD, true);
CONSOLE_FLOAT_VAL("raytracing", animate_all_animchars_range, 30);
CONSOLE_BOOL_VAL("raytracing", rtr_use_prev_frame_sample, true);
CONSOLE_BOOL_VAL("raytracing", rtr_rr_stochastic_ray, true);
CONSOLE_BOOL_VAL("raytracing", rtr_checkerboard_depth, false);
CONSOLE_BOOL_VAL("raytracing", rtr_path_traced, false); // TODO Add a proper UI integration and remove this
CONSOLE_BOOL_VAL("raytracing", ptgi_use_prev_frame_sample, true);
CONSOLE_BOOL_VAL("raytracing", only_impostors_optimization, true);
CONSOLE_FLOAT_VAL("raytracing", grass_range, 32);
CONSOLE_BOOL_VAL("raytracing", rt_lighting_ao_hacks, false);
CONSOLE_FLOAT_VAL_MINMAX("raytracing", grass_fraction_to_keep, 0.25, 0.01, 1);

CONSOLE_FLOAT_VAL("rt_speed", max_speed_for_tree_meshes, 20);
CONSOLE_FLOAT_VAL("rt_speed", max_speed_for_medium_budget, 5.5, "Slightly above running and free cam speed");
CONSOLE_FLOAT_VAL("rt_speed", max_speed_for_grass, 50);
CONSOLE_FLOAT_VAL("rt_speed", teleport_detection_distance, 50, "Camera jump per frame above this is a teleport, not movement");
CONSOLE_FLOAT_VAL("rt_speed", airplane_detection_speed, 20);
CONSOLE_FLOAT_VAL("rt_speed", airplane_cockpit_radius, 5);

CONSOLE_BOOL_VAL("raytracing", bvh_mem_overlay, false);
CONSOLE_INT_VAL("raytracing", bvh_max_skinned_memory_mb, -1, -1, INT_MAX);
CONSOLE_FLOAT_VAL_MINMAX("raytracing", bvh_animchar_lod_dist_mul, 1, 1, 4, "Only applies to objects failing frustum culling.");

// TODO move these to entity
static bvh::ContextId bvhRenderingContextId;
static BVHConnection *fxBvhConnection = nullptr;
static BVHConnection *smokeTracersBvhConnection = nullptr;
static bool cablesChanged = false;
static dynrend::ContextId bvhDynmodelCtx = dynrend::ContextId::Invalid;
static GlobalVariableStates bvhDebugGvarsState;
static const GlobalVariableStates *get_bvh_debug_gvars_state() { return draw_debug_dynmodels ? &bvhDebugGvarsState : nullptr; }
static bool bvhUnloadDynModels = false;
static eastl::vector_set<const DynamicRenderableSceneLodsResource *> bvhActiveDynResources;
static void bvh_iterate_over_animchars(
  dynrend::BVHIterateOneInstanceCallback iterate_one_instance, const Point3 &view_position, void *user_data);
static bool rigen_cull_dist_was_increased = false;
static int bvhBuildingCountdown = -1;
static constexpr int MAX_BVH_BUILDING_COUNTDOWN = 20;
static BVHInstanceMapper *dagdpInstanceMapper = nullptr;
static bool prev_rtr_validation_enabled = false;
static bool prev_ptgi_validation_enabled = false;
static bool prev_ptgi_second_chance_enabled = false;
static CulledLightsPtr bvhLights;

static void set_bvh_lights_to_shader()
{
  if (bvhLights)
    WRDispatcher::getClusteredLights().setOutOfFrustumLightsToShader(*bvhLights);
}

namespace var
{
static ShaderVariableInfo rtr_use_prev_frame_sample("rtr_use_prev_frame_sample", true);
static ShaderVariableInfo rtr_rr_stochastic_ray("rtr_rr_stochastic_ray", true);
static ShaderVariableInfo rtr_is_binocular("rtr_is_binocular", true);
static ShaderVariableInfo ptgi_use_prev_frame_sample("ptgi_use_prev_frame_sample", true);
static ShaderVariableInfo prev_zn_zfar("prev_zn_zfar", true);
static ShaderVariableInfo bvh_usable("bvh_usable", true);
static ShaderVariableInfo bvh_instance_data_buffer("bvh_instance_data_buffer", true);
static ShaderVariableInfo bvh_instance_data_buffer_cur_offset("bvh_instance_data_buffer_cur_offset", true);
static ShaderVariableInfo bvh_additional_impostor_mip_bias("bvh_additional_impostor_mip_bias", true);
static ShaderVariableInfo bvh_tan_pixel_angular_radius("bvh_tan_pixel_angular_radius", true);
static ShaderVariableInfo bvh_max_ahs_calls("bvh_max_ahs_calls", true);
static ShaderVariableInfo sun_dir_for_shadows("sun_dir_for_shadows", true);
static ShaderVariableInfo water_rt_frame_index("water_rt_frame_index", true);
static ShaderVariableInfo water_rt_output_mode("water_rt_output_mode", true);
static ShaderVariableInfo glass_rtr_shadow("glass_rtr_shadow", true);
static ShaderVariableInfo burnt_tank_camo("burnt_tank_camo", true);
static ShaderVariableInfo rt_lighting_ao_hacks("rt_lighting_ao_hacks", true);
} // namespace var

enum class RTRMode
{
  off,
  full,
  halfCheckerboardDepth,
  halfSmartPattern
};


struct ResolvedRTSettings
{
  RTRMode rtrMode = RTRMode::off;
  bool isBVHEnabled = false;
  bool isDenoiserEnabled = false;
  bool isRTSMEnabled = false;
  bool isRTSMDynamicEnabled = false;
  bool isRTTREnabled = false;
  bool isRTAOEnabled = false;
  bool isPTGIEnabled = false;
  bool isRTWaterEnabled = false;
  bool isRTGIEnabled = false;
  bool isRayReconstructionEnabled = false;
  bool isDagdpEnabled = false;
  bool isBvhDynModelsEnabled = false;
  bool ultraPerformanceOverwrite = false;

  bool isRTREnabled() const { return rtrMode != RTRMode::off; }
  bool isRTRHalfRes() const { return isRTREnabled() && rtrMode != RTRMode::full; }
  bool isRTRRaySkipping() const { return rtrMode == RTRMode::halfSmartPattern; }

  bool operator==(const ResolvedRTSettings &) const = default;
};
ECS_DECLARE_RELOCATABLE_TYPE(ResolvedRTSettings);
ECS_REGISTER_RELOCATABLE_TYPE(ResolvedRTSettings, nullptr);

template <typename Callable>
static void get_resolved_rt_settings_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
static void set_resolved_rt_settings_ecs_query(ecs::EntityManager &manager, Callable c);

static ResolvedRTSettings get_resolved_rt_settings()
{
  ResolvedRTSettings settings;
  get_resolved_rt_settings_ecs_query(*g_entity_mgr,
    [&settings](const ResolvedRTSettings &resolved_rt_settings) { settings = resolved_rt_settings; });
  return settings;
}

static bool is_rtr_probes_enabled(bool use_ray_reconstruction)
{
  static bool rtrPobesEnabled = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableRTRProbe", true);
  return rtrPobesEnabled && !use_ray_reconstruction && !rtr_path_traced.get();
}

static void set_rtr_ray_limit_params(bool use_ray_reconstruction)
{
  rtr::set_ray_limit_params(100000, -2, 2, is_rtr_probes_enabled(use_ray_reconstruction));
}

struct BvhHeightProvider final : public bvh::HeightProvider
{
  LandMeshManager *lmeshMgr = nullptr;
  bool embedNormals() const override final { return false; }

  template <bool has_hole>
  void getHeightWithHole(void *data, const Point2 &origin, int cell_size, int cell_count, LandMeshHolesManager *holesMgr) const
  {
    using TerrainVertex = Point3;

    Point3 *scratch = (Point3 *)data;

    for (int z = 0; z <= cell_count; ++z)
      for (int x = 0; x <= cell_count; ++x)
      {
        Point2 loc(x * cell_size, z * cell_size);

        TerrainVertex &v = scratch[z * (cell_count + 1) + x];
        v.x = loc.x;
        v.z = loc.y;

        Point2 pos = loc + origin;
        if constexpr (has_hole)
        {
          if (holesMgr->check(pos))
          {
            v.y = NAN;
            continue;
          }
        }
        if (!lmeshMgr->getHeight(pos, v.y, nullptr))
          v.y = 0;
      }
  }

  void getHeight(void *data, const Point2 &origin, int cell_size, int cell_count) const override final
  {
    if (auto holesMgr = lmeshMgr->getHolesManager())
    {
      ScopedLockReadTemplate<OSReadWriteLock> holesGuard(holesMgr->rewriteLock()); // main thread rewrites holes
      getHeightWithHole<true>(data, origin, cell_size, cell_count, holesMgr);
    }
    else
      getHeightWithHole<false>(data, origin, cell_size, cell_count, nullptr);
  }

  uint32_t getVersion() const override final
  {
    auto holesMgr = lmeshMgr->getHolesManager();
    return holesMgr ? holesMgr->getRewriteCount() : 0;
  }
};
ECS_DECLARE_BOXED_TYPE(BvhHeightProvider);
ECS_REGISTER_BOXED_TYPE(BvhHeightProvider, nullptr);

struct RiGenVisibilityECS
{
  struct VisibilityDeleter
  {
    void operator()(RiGenVisibility *visibility) { rendinst::destroyRIGenVisibility(visibility); }
  };
  eastl::unique_ptr<RiGenVisibility, VisibilityDeleter> visibility;
  static RiGenVisibilityECS create()
  {
    RiGenVisibilityECS result;
    result.visibility.reset(rendinst::createRIGenVisibility(midmem));
    return result;
  }
};
ECS_DECLARE_BOXED_TYPE(RiGenVisibilityECS);
ECS_REGISTER_BOXED_TYPE(RiGenVisibilityECS, nullptr);

namespace bvh
{
static void process_elem(const ShaderMesh::RElem &elem,
  MeshInfo &mesh_info,
  const ChannelParser &parser,
  const RenderableInstanceLodsResource::ImpostorParams *impostor_params,
  const RenderableInstanceLodsResource::ImpostorTextures *impostor_textures);
}

static void use_bindless_fom_shadows(dafg::Registry registry, dafg::Stage stage)
{
  registry.readBlob<OrderingToken>("bvh_fom_shadows_registered");
  registry.read("fom_shadows_sin").texture().atStage(stage).useAs(dafg::Usage::SHADER_RESOURCE).optional();
  registry.read("fom_shadows_cos").texture().atStage(stage).useAs(dafg::Usage::SHADER_RESOURCE).optional();
}

// Orders the OOM report against closeBVH: an in-flight report holds the mutex, and the driver
// loads the hook pointer before calling, so a late call must re-check it under the mutex.
static WinCritSec bvh_oom_report_mutex;

// Called from the failing allocation, possibly on a thread already inside BVH, hence the try variant.
static void log_bvh_memory_on_gpu_oom()
{
  WinAutoLock lock(bvh_oom_report_mutex);
  if (dgs_report_gpu_out_of_memory)
    bvh::try_log_rt_memory_overhead(bvhRenderingContextId);
}

static void applyVoxelActivitySettings()
{
  const DataBlock *graphicsBlk = ::dgs_get_settings()->getBlockByNameEx("graphics");
  if (!graphicsBlk->getBool("bvhVoxelActivity", false))
    return;

  bvh::VoxelActivitySettings vaSettings;
  vaSettings.voxelSize = graphicsBlk->getReal("bvhVoxelActivityVoxelSize", vaSettings.voxelSize);
  vaSettings.activeValue = graphicsBlk->getInt("bvhVoxelActivityActiveValue", vaSettings.activeValue);
  vaSettings.inactiveKeepFraction = graphicsBlk->getReal("bvhVoxelActivityKeepFraction", vaSettings.inactiveKeepFraction);
  const float range = graphicsBlk->getReal("bvhVoxelActivityRange", bbox_width.get() * 0.5f);
  const float verticalRange = graphicsBlk->getReal("bvhVoxelActivityVerticalRange", 512.f);
  auto toDim = [&](float r) { return max(int(ceilf(r * 2 / vaSettings.voxelSize)), 8); };
  vaSettings.dims = IPoint3(toDim(range), toDim(verticalRange), toDim(range));
  bvh::set_voxel_activity(bvhRenderingContextId, vaSettings);

  static int rtrProbesMarkVarId = get_shader_variable_id("rtr_probes_mark_voxel_activity", true);
  static int ptgiMarkBouncesVarId = get_shader_variable_id("ptgi_mark_voxel_activity_bounces", true);
  ShaderGlobal::set_int(rtrProbesMarkVarId, graphicsBlk->getBool("bvhVoxelActivityMarkRTRProbes", true) ? 1 : 0);
  ShaderGlobal::set_int(ptgiMarkBouncesVarId, graphicsBlk->getInt("bvhVoxelActivityMarkPTGIBounces", 1));
}

static void initBVH()
{
  if (is_bvh_enabled())
  {
#if _TARGET_C2



#endif
    auto features = bvh::Features::Terrain | bvh::Features::RIFull | bvh::Features::Fx | bvh::Features::SmokeTracers |
                    bvh::Features::Cable | bvh::Features::BinScene | bvh::Features::FftWater | bvh::Features::Dagdp |
                    bvh::Features::GPUGrass | bvh::Features::GpuObjects;

    static constexpr uint32_t BVH_FULL_MESH_DYNMODEL_FEATURES =
      bvh::Features::DynrendRigidFull | bvh::Features::DynrendSkinnedFull | bvh::Features::Splinegen;
    const ResolvedRTSettings &rtSettings = get_resolved_rt_settings();
    if (rtSettings.isBvhDynModelsEnabled)
      features |= BVH_FULL_MESH_DYNMODEL_FEATURES;

    bbox_width = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhCullingBboxWidth", bbox_width);
    rtsm_quality = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhRTSMQuality", rtsm_quality);
    animate_all_animchars_range =
      dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhAnimateAllAnimcharsRange", animate_all_animchars_range);
    grass_range = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhGrassRange", grass_range);

    constexpr float ULTRA_PERF_LOD_BIAS = 200.0f;

    bvh::AdditionalSettings additionalSettings;
    additionalSettings.batchedSkinning = true;
    additionalSettings.riLodDistBias = rtSettings.ultraPerformanceOverwrite
                                         ? ULTRA_PERF_LOD_BIAS
                                         : dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhRiLodDistBias", 0.0f);
    additionalSettings.prioritizeCompactions =
      rtSettings.ultraPerformanceOverwrite
        ? true
        : dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhPrioritizeCompaction", false);
    additionalSettings.useFastTlasBuild = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhUseFastTlasBuild", false);
    additionalSettings.discardDestrAssets = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhDiscardDestrAssets", 0);
    additionalSettings.singleLodFilterMaxFaces = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhSingleLodMaxFaces", 0);
    additionalSettings.singleLodFilterMaxRange = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhSingleLodMaxRange", 0);
    additionalSettings.enableCaching = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhEnableCaching", true);
    additionalSettings.unloadDynModels = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhUnloadDynModels", false);
    bvhUnloadDynModels = additionalSettings.unloadDynModels;
    additionalSettings.buildDynOnDemand = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhBuildDynOnDemand", false);
    additionalSettings.dynModelRetentionSec =
      dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhDynModelRetentionSec", additionalSettings.dynModelRetentionSec);
    additionalSettings.enableOmm = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhEnableOmm", false);
    additionalSettings.ommDataArrayBudget = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhOmmDataArrayBudget", 0);
    additionalSettings.retainOmmBakeResults =
      dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhRetainOmmBakeResults", false);
    additionalSettings.ommCacheRetentionFrames = dgs_get_settings()
                                                   ->getBlockByNameEx("graphics")
                                                   ->getInt("bvhOmmCacheRetentionFrames", additionalSettings.ommCacheRetentionFrames);
    additionalSettings.ommCacheIdleBudget =
      dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhOmmCacheIdleBudget", additionalSettings.ommCacheIdleBudget);
    constexpr int SLOW_DISK_OMM_TEXTURE_WAIT_MUL = 4;
    if (!is_rt_supported_on_disk())
      additionalSettings.ommTextureWaitAttempts *= SLOW_DISK_OMM_TEXTURE_WAIT_MUL;
    additionalSettings.ommTextureWaitAttempts =
      dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhOmmTextureWaitAttempts", additionalSettings.ommTextureWaitAttempts);
    additionalSettings.strictAssetChecks = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("bvhStrictAssetChecks", false);
    bvh_max_skinned_memory_mb = dgs_get_settings()->getBlockByNameEx("graphics")->getInt("bvhMaxSkinnedMemoryMB", -1);
    bvh_animchar_lod_dist_mul = dgs_get_settings()->getBlockByNameEx("graphics")->getReal("bvhAnimcharLodDistMul", 1);
    rt_lighting_ao_hacks = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("rt_lighting_ao_hacks", false);
    bvh::init(bvh::process_elem, nullptr, additionalSettings);
    bvhRenderingContextId = bvh::create_context("Rendering", static_cast<bvh::Features>(features),
      static_cast<bvh::Features>(BVH_FULL_MESH_DYNMODEL_FEATURES));
    bvh::connect_fx(bvhRenderingContextId, [](BVHConnection *connection) { fxBvhConnection = connection; });
    bvh::connect_smoke_tracers(bvhRenderingContextId, [](BVHConnection *connection) { smokeTracersBvhConnection = connection; });
    bvh::connect_dagdp(bvhRenderingContextId, [](BVHInstanceMapper *mapper) { dagdpInstanceMapper = mapper; });
    applyVoxelActivitySettings();
    dgs_report_gpu_out_of_memory = log_bvh_memory_on_gpu_oom;
    bvhBuildingCountdown = MAX_BVH_BUILDING_COUNTDOWN;
    debug("initBVH: bvhBuildingCountdown reset to %d frames", bvhBuildingCountdown);
    if (auto cables = get_cables_mgr())
      cables->markAllCablesDirty();
  }
}

static void initRTFeatures()
{
  const ResolvedRTSettings &rtSettings = get_resolved_rt_settings();
  if (rtSettings.isBVHEnabled)
  {
    G_ASSERTF(bvhRenderingContextId, "Call initBVH first!");
    if (rtSettings.isDenoiserEnabled)
      denoiser::initialize(rtSettings.isRayReconstructionEnabled, rtSettings.isRayReconstructionEnabled);
    if (rtSettings.isRTSMEnabled)
      rtsm::initialize();
    if (rtSettings.isRTREnabled())
    {
      rtr::initialize(rtSettings.isRTRHalfRes(), rtSettings.isRTRRaySkipping(), rtr_path_traced.get());
      set_rtr_ray_limit_params(rtSettings.isRayReconstructionEnabled);
      rtr::set_performance_mode(false);
      rtr::set_use_anti_firefly(true);
    }
    if (rtSettings.isRTAOEnabled)
      rtao::initialize(!rtSettings.isRayReconstructionEnabled);
    if (rtSettings.isPTGIEnabled)
      ptgi::initialize(!rtSettings.isRayReconstructionEnabled);
  }
}

static void closeRTFeatures()
{
  ptgi::teardown();
  rtao::teardown();
  rtr::teardown();
  rtsm::teardown();
  denoiser::teardown();
}

static void closeBVH(bool device_reset = false)
{
  if (bvhRenderingContextId)
  {
    {
      WinAutoLock lock(bvh_oom_report_mutex);
      dgs_report_gpu_out_of_memory = nullptr;
    }
    bvh::teardown(bvhRenderingContextId);
    bvh::teardown(device_reset, true);
    g_entity_mgr->broadcastEventImmediate(RemoveSplinegenBVHEvent());
    bvhRenderingContextId = {};
    fxBvhConnection = nullptr;
    smokeTracersBvhConnection = nullptr;
    dagdpInstanceMapper = nullptr;
    bvhLights.reset();
    ShaderGlobal::set_int(var::bvh_usable, 0);
    toggle_rtsm_dynamic(false);
  }
}

static void flush_deferred_resource_frees() { d3d::driver_command(Drv3dCommand::D3D_FLUSH); }

struct ScopedSwapchainPresentBlock
{
  eastl::optional<d3d::GpuAutoLock> blockPresents;
  ScopedSwapchainPresentBlock()
  {
    if (bvhRenderingContextId != bvh::InvalidContextId)
      blockPresents.emplace();
  }
  ~ScopedSwapchainPresentBlock()
  {
    if (blockPresents)
      flush_deferred_resource_frees();
  }
};

static void wait_bvh_worker_threads();

static void closeBVHScene()
{
  if (bvhRenderingContextId)
  {
    wait_bvh_worker_threads();
    bvh::finalize_async_atmosphere_update(bvhRenderingContextId);
    bvh::remove_terrain(bvhRenderingContextId);
    bvh::on_before_unload_scene(bvhRenderingContextId);
    bvh::on_unload_scene(bvhRenderingContextId);
  }
}

template <typename Callable>
static void reset_bvh_entity_ecs_query(ecs::EntityManager &manager, Callable c);

static void bvh_before_reset(bool full_reset)
{
  if (full_reset)
  {
    closeBVHScene();
    reset_bvh_entity_ecs_query(*g_entity_mgr, [](bool &bvh__initialized) { bvh__initialized = false; });
    closeRTFeatures();
    closeBVH(true);
  }
}
REGISTER_D3D_BEFORE_RESET_FUNC(bvh_before_reset);
static void bvh_after_reset(bool full_reset)
{
  if (full_reset)
  {
    initBVH();
    if (get_world_renderer())
      initRTFeatures();
  }
}
REGISTER_D3D_AFTER_RESET_FUNC(bvh_after_reset);

template <typename Callable>
static void get_grass_renderer_for_bvh_ecs_query(ecs::EntityManager &manager, Callable c);

static const GrassRenderer *get_grass_renderer()
{
  const GrassRenderer *renderer = nullptr;
  get_grass_renderer_for_bvh_ecs_query(*g_entity_mgr, [&](const GrassRenderer &grass_render) { renderer = &grass_render; });
  return renderer;
}

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
        bvh::add_terrain(bvhRenderingContextId, &bvh__heightProvider);
      }

      if (auto binScene = WRDispatcher::getBinScene())
        bvh::add_bin_scene(bvhRenderingContextId, *binScene);
      if (auto fftwater = get_world_renderer()->getWater())
        bvh::add_water(bvhRenderingContextId, *fftwater);

      if (auto grass = get_grass_renderer())
        bvh::gpu_grass_make_meta(bvhRenderingContextId, grass->grass.base);
    }
  }
}

ECS_TAG(render)
ECS_ON_EVENT(EventRenderSceneUnload)
ECS_REQUIRE(BvhHeightProvider bvh__heightProvider)
static void close_bvh_scene_es(const ecs::Event &) { closeBVHScene(); }

void bvh_release_bindlessly_held_textures()
{
  if (bvhRenderingContextId)
    bvh::on_before_unload_scene(bvhRenderingContextId);
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void bvh_destroy_es(
  const ecs::Event &, RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility)
{
  ScopedSwapchainPresentBlock presentBlock;
  wait_bvh_worker_threads();
  closeBVHScene();
  bvh__rendinst_visibility = {};
  bvh__rendinst_oof_visibility = {};
  closeRTFeatures();
  closeBVH();
}

template <typename Callable>
static void get_bvh_rigen_visibility_ecs_query(ecs::EntityManager &manager, Callable c);
static RiGenVisibility *get_bvh_rigen_visibility()
{
  RiGenVisibility *visibility;
  get_bvh_rigen_visibility_ecs_query(*g_entity_mgr,
    [&](RiGenVisibilityECS &bvh__rendinst_visibility) { visibility = bvh__rendinst_visibility.visibility.get(); });
  return visibility;
}

template <typename Callable>
static void get_bvh_rigen_oof_visibility_ecs_query(ecs::EntityManager &manager, Callable c);
static RiGenVisibility *get_bvh_rigen_oof_visibility()
{
  RiGenVisibility *visibility;
  get_bvh_rigen_oof_visibility_ecs_query(*g_entity_mgr,
    [&](RiGenVisibilityECS &bvh__rendinst_oof_visibility) { visibility = bvh__rendinst_oof_visibility.visibility.get(); });
  return visibility;
}

TMatrix4 get_bvh_culling_matrix(const Point3 &cameraPos)
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

BVHInstanceMapper *get_bvh_dagdp_instance_mapper() { return dagdpInstanceMapper; }

void prepareFXForBVH(const Point3 &cameraPos) { acesfx::prepare_bvh_culling(get_bvh_culling_matrix(cameraPos)); }

bool is_bvh_enabled() { return get_resolved_rt_settings().isBVHEnabled; }
bool is_bvh_usable() { return bool(bvhRenderingContextId) && bvhBuildingCountdown < 0; }

ECS_NO_ORDER
ECS_TAG(dev, render)
ECS_ON_EVENT(RenderEventDebugGUI)
static void bvh_render_mem_overlay_es(const ecs::Event &)
{
  if (bvh_mem_overlay)
    bvh::render_rt_mem_overlay(bvhRenderingContextId);
}

bool is_rtsm_enabled() { return get_resolved_rt_settings().isRTSMEnabled; }
bool is_rtsm_dynamic_enabled() { return get_resolved_rt_settings().isRTSMDynamicEnabled; }
bool is_rtr_enabled() { return get_resolved_rt_settings().isRTREnabled(); }
bool is_rttr_enabled() { return get_resolved_rt_settings().isRTTREnabled; }
bool is_rtao_enabled() { return get_resolved_rt_settings().isRTAOEnabled; }
bool is_ptgi_enabled() { return get_resolved_rt_settings().isPTGIEnabled; }
bool is_rt_water_enabled() { return get_resolved_rt_settings().isRTWaterEnabled; }
bool is_rtgi_enabled() { return get_resolved_rt_settings().isRTGIEnabled; }
bool is_denoiser_enabled() { return get_resolved_rt_settings().isDenoiserEnabled; }
bool is_rr_enabled() { return get_resolved_rt_settings().isRayReconstructionEnabled; }
bool is_bvh_dagdp_enabled() { return get_resolved_rt_settings().isDagdpEnabled; }
bool is_bvh_dyn_models_enabled() { return get_resolved_rt_settings().isBvhDynModelsEnabled; }

void draw_rtr_validation() { rtr::render_validation_layer(); }
void draw_ptgi_validation() { ptgi::render_validation_layer(); }
bool should_delay_pufd_until_bvh_jobs_done() { return is_bvh_enabled() && delay_PUFD; }

static void (*on_bvh_parallel_jobs_finished_cb)() = nullptr;
void set_bvh_on_parallel_jobs_finished_cb(void (*cb)())
{
  on_bvh_parallel_jobs_finished_cb = cb;
  bvh::set_on_parallel_jobs_finished_cb(cb);
}
void bvh_cables_changed() { cablesChanged = true; }

bool is_rt_supported() { return rt_support_error_code() == eastl::to_underlying(bvh::BvhAvailabilityCode::AVAILABLE); }

bool is_rt_supported_on_disk()
{
#if _TARGET_PC
  String diskPath = folders::get_exe_dir();
  if (diskPath.empty())
  {
    diskPath = folders::get_game_dir();
  }

  systeminfo::DiskType diskType;
  if (!systeminfo::get_disk_type(diskPath.c_str(), diskType))
  {
    LOGWARN_ONCE("BVH was not able to query the players disk type!");
    return true;
  }

  // Not sure who has access to RAM disks at this age, but it should be more than capable
  if (diskType == systeminfo::DiskType::Ssd || diskType == systeminfo::DiskType::RamDisk)
    return true;

  if (is_bvh_enabled())
    LOGWARN_ONCE("RT is enabled even though the player has disk type:%s, which might not be fast enough!",
      systeminfo::to_string(diskType));

  return false;
#endif
  return true;
}

int rt_support_error_code()
{
  if (!denoiser::is_available())
  {
    LOGWARN_ONCE("bvh: RT is not supported, the denoiser is not available");
    return eastl::to_underlying(bvh::BvhAvailabilityCode::NOT_SUPPORTED);
  }
  return eastl::to_underlying(bvh::is_available_verbose());
}

void bvh_bind_resources(int render_width)
{
  if (bvhRenderingContextId)
    bvh::bind_resources(bvhRenderingContextId, render_width);
}
void bvh_unbind_resources()
{
  if (bvhRenderingContextId)
    bvh::unbind_resources();
}

// the main thread writes it only while no cull job runs
static struct BVHRiGenStart
{
  dag::Vector<RiGenVisibility *> visibilities;
  Point3 viewPosition;
  Point3 lightDirection;
  Frustum viewFrustum;
  volatile int cullJobsLeft = 0;
} bvh_ri_gen_start;

static void on_bvh_ri_cull_job_done()
{
  if (interlocked_decrement(bvh_ri_gen_start.cullJobsLeft) == 0)
    bvh::start_ri_gen_instances_update(bvhRenderingContextId, bvh_ri_gen_start.visibilities, bvh_ri_gen_start.viewPosition,
      bvh_ri_gen_start.lightDirection, bvh_ri_gen_start.viewFrustum, threadpool::PRIO_LOW);
}

static bool use_bvh_ri_culling_job = false;
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
    threadpool::add(this, prio, false); // Note: would be waken up by `start_ri_jobs`
  }
  const char *getJobName(bool &) const override { return DAPROFILER_STRING("BVHRICullingJob"); }
  void doJob() override
  {
    G_ASSERT(use_bvh_ri_culling_job);
    rendinst::prepareRIGenVisibility(frustum, cameraPos, visibility, false, nullptr);
    on_bvh_ri_cull_job_done();
  }
} bvh_ri_culling_job;

static bool use_bvh_ri_oof_culling_job = false;
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
    threadpool::add(this, prio, false); // Note: would be waken up by `start_ri_jobs`
  }
  const char *getJobName(bool &) const override { return DAPROFILER_STRING("BVHRIOOFCullingJob"); }
  void doJob() override
  {
    G_ASSERT(use_bvh_ri_oof_culling_job);
    rendinst::setRIGenVisibilityMinLod(visibility, rendinst::MAX_LOD_COUNT, 1);
    rendinst::setRIGenVisibilityForcedLodRangeCull(visibility, true);
    rendinst::prepareRIGenVisibility(bvhFrustum, cameraPos, visibility, false, nullptr, false);
    on_bvh_ri_cull_job_done();
  }
} bvh_ri_oof_culling_job;

struct BVHPrepareRiExtraInstancesJob final : public cpujobs::IJob
{
  void start(threadpool::JobPriority prio)
  {
    // Note: would be waken up by `start_ri_jobs` when prepare_ri_extra_job_on_previous_frame is false
    threadpool::add(this, prio, prepare_ri_extra_job_on_previous_frame);
  }
  const char *getJobName(bool &) const override { return DAPROFILER_STRING("BVHPrepareRiExtraInstancesJob"); }
  void doJob() override { bvh::prepare_ri_extra_instances(); }
} bvh_prepare_ri_extra_instances_job;

static void wait_bvh_worker_threads()
{
  threadpool::wait(&bvh_ri_culling_job);
  threadpool::wait(&bvh_ri_oof_culling_job);
  threadpool::wait(&bvh_prepare_ri_extra_instances_job);
}

static void start_ri_jobs(const Point3 &cam_pos,
  const Frustum &culling_frustum,
  float real_dt,
  int &bvh__frame_counter,
  Point3 &bvh__prev_pos,
  float &bvh__accumulated_speed)
{
  if (!is_bvh_enabled())
    return;

  bvh::finalize_async_atmosphere_update(bvhRenderingContextId);

  constexpr int INITIAL_FAST_BUILDING_FRAME_COUNT = 500;
  constexpr int TELEPORT_FAST_BUILDING_FRAME_COUNT = 100;

  constexpr float PREV_FRAME_WEIGHT = 0.95;
  float frameDist = length(cam_pos - bvh__prev_pos);
  bvh__prev_pos = cam_pos;
  if (frameDist > teleport_detection_distance)
  {
    bvh__accumulated_speed = 0;
    bvh__frame_counter = min(bvh__frame_counter, INITIAL_FAST_BUILDING_FRAME_COUNT - TELEPORT_FAST_BUILDING_FRAME_COUNT);
  }
  else
  {
    float currectSpeed = safediv(frameDist, real_dt);
    bvh__accumulated_speed = bvh__accumulated_speed * PREV_FRAME_WEIGHT + currectSpeed * (1.0f - PREV_FRAME_WEIGHT);
  }

  const Frustum bvhCullingFrustum = get_bvh_culling_matrix(cam_pos);
  use_bvh_ri_culling_job = use_bvh_ri_oof_culling_job = false;
  Frustum frustum;
  if (rigen_cull_dist_was_increased)
  {
    frustum = culling_frustum;
    use_bvh_ri_culling_job = true;
    use_bvh_ri_oof_culling_job = true;
  }
  else
  {
    bool onlyImpostors = only_impostors_optimization && bvh__accumulated_speed > max_speed_for_tree_meshes;
    if (onlyImpostors)
    {
      use_bvh_ri_oof_culling_job = true;
    }
    else
    {
      frustum = bvhCullingFrustum;
      use_bvh_ri_culling_job = true;
    }
  }

  if (!prepare_ri_extra_job_on_previous_frame)
  {
    bvh_prepare_ri_extra_instances_job.start(threadpool::JobPriority::PRIO_LOW);
    threadpool::wake_up_all();
  }

  bvh::BuildBudget budget =
    bvh__frame_counter++ < INITIAL_FAST_BUILDING_FRAME_COUNT
      ? bvh::BuildBudget::High
      : (bvh__accumulated_speed < max_speed_for_medium_budget ? bvh::BuildBudget::Medium : bvh::BuildBudget::Low);

  bvh::set_grass_range(bvhRenderingContextId, bvh__accumulated_speed < max_speed_for_grass ? grass_range : 0);
  bvh::set_grass_fraction_to_keep(bvhRenderingContextId, grass_fraction_to_keep);

  // before the instance jobs: it takes the object lock exclusively, they hold it shared
  bvh::process_meshes(bvhRenderingContextId, budget);

  threadpool::wait(&bvh_prepare_ri_extra_instances_job);
  {
    // before the riExtra jobs: a pending rebuild takes the riExtra write lock, they hold it shared
    TIME_PROFILE(ensureElemsRebuiltRIGenExtra);
    rendinst::render::ensureElemsRebuiltRIGenExtra(false);
  }

  // before this frame's sky update, unlike bvh_update_dyn_instances
  const Point3 lightDirection = get_daskies() ? -get_daskies()->getPrimarySunDir() : Point3(0, -1, 0);
  bvh::update_ri_instances(bvhRenderingContextId, cam_pos, cam_pos, lightDirection, bvhCullingFrustum, culling_frustum,
    threadpool::PRIO_LOW);

  RiGenVisibility *riGenVisibility = use_bvh_ri_culling_job ? get_bvh_rigen_visibility() : nullptr;
  RiGenVisibility *riGenOofVisibility = use_bvh_ri_oof_culling_job ? get_bvh_rigen_oof_visibility() : nullptr;
  bvh_ri_gen_start.visibilities.clear();
  if (use_bvh_ri_culling_job)
    bvh_ri_gen_start.visibilities.push_back(riGenVisibility);
  if (use_bvh_ri_oof_culling_job)
    bvh_ri_gen_start.visibilities.push_back(riGenOofVisibility);
  bvh_ri_gen_start.viewPosition = cam_pos;
  bvh_ri_gen_start.lightDirection = lightDirection;
  bvh_ri_gen_start.viewFrustum = culling_frustum;
  interlocked_release_store(bvh_ri_gen_start.cullJobsLeft, (int)bvh_ri_gen_start.visibilities.size());
  if (use_bvh_ri_culling_job)
  {
    bvh_ri_culling_job.start(riGenVisibility, frustum, cam_pos, threadpool::JobPriority::PRIO_LOW);
  }
  if (use_bvh_ri_oof_culling_job)
  {
    bvh_ri_oof_culling_job.start(riGenOofVisibility, bvhCullingFrustum, cam_pos, threadpool::JobPriority::PRIO_LOW);
  }
  threadpool::wake_up_all();

  if (bvhBuildingCountdown >= 0)
  {
    if (bvh::is_building(bvhRenderingContextId))
      bvhBuildingCountdown = MAX_BVH_BUILDING_COUNTDOWN;
    else
    {
      if (bvhBuildingCountdown-- == 0)
      {
        debug("bvhBuildingCountdown reached 0, BVH is now usable");
        toggle_rtsm_dynamic(is_rtsm_dynamic_enabled());
      }
    }
  }
  ShaderGlobal::set_int(var::bvh_usable, bvhBuildingCountdown < 0 ? 1 : 0);
}

ECS_TAG(render)
ECS_BEFORE(animchar_before_render_es)
ECS_AFTER(start_occlusion_and_sw_raster_es) // Note: as it adds quite a bit of jobs which be waken up here
ECS_REQUIRE(dafg::NodeHandle bvh__update_node)
static void bvh_wake_up_before_render_jobs_es(const UpdateStageInfoBeforeRender &)
{
  if (is_bvh_enabled())
    threadpool::wake_up_all();
}

template <typename Callable>
static void bvh_start_ri_jobs_ecs_query(Callable c);

void bvh_start_ri_jobs(const Point3 &cameraPos, const Frustum &cullingFrustum, float realDt)
{
  TIME_PROFILE(bvh_start_ri_jobs);
  bvh_start_ri_jobs_ecs_query(
    [&](int &bvh__frame_counter, Point3 &bvh__prev_pos, float &bvh__accumulated_speed ECS_REQUIRE(dafg::NodeHandle bvh__update_node)) {
      start_ri_jobs(cameraPos, cullingFrustum, realDt, bvh__frame_counter, bvh__prev_pos, bvh__accumulated_speed);
    });
}

void bvh_wait_ri_jobs()
{
  if (!bvhRenderingContextId)
    return;
  wait_bvh_worker_threads(); // the last cull job starts the riGen jobs
  bvh::wait_ri_instances_jobs(bvhRenderingContextId);
}

ECS_TAG(render)
ECS_NO_ORDER
ECS_BEFORE(camera_update_lods_scaling_es)
static void bvh_out_of_scope_riex_dist_es(const ecs::UpdateStageInfoAct &, bool bvh__initialized)
{
  if (!is_bvh_enabled() || !bvh__initialized)
    return;
  float riDistMul = rendinst::getDefaultDistAddMul();
  rendinst::setDynamicDistMul(riDistMul, 0.0f);
  float cullDistSqMulOOC = rendinst::getRiExtraDistSqScale();
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
  dafx::render(
    acesfx::get_dafx_context(), acesfx::get_cull_bvh_id(), render_tags[ERT_TAG_BVH], 0,
    [](TEXTUREID id) { fxBvhConnection->textureUsed(id); }, 0.5f);
  fxBvhConnection->done();
}

static bool bvhDynmodelDataReady = false;

static void upload_dynmodel_instance_data()
{
  TIME_PROFILE(bvh::upload_dynmodel_instance_data)
  bvh::wait_dynamic_instances_jobs();
  if (bvhUnloadDynModels)
  {
    bvh::tell_active_dynamic_resources(bvhRenderingContextId, bvhActiveDynResources);
    bvhActiveDynResources.clear();
  }
  bvhDynmodelDataReady = dynrend::prepare_render_current(bvhDynmodelCtx);
  if (!bvhDynmodelDataReady)
    return;
  D3DRESID bufId = dynrend::get_context_buffer_id(bvhDynmodelCtx);
  int bufPos = dynrend::get_context_buffer_pos(bvhDynmodelCtx);
  ShaderGlobal::set_buffer(var::bvh_instance_data_buffer, bufId);
  ShaderGlobal::set_int(var::bvh_instance_data_buffer_cur_offset, bufPos);
  static_cast<const bvh::SkinnedVertexProcessorBatched &>(bvh::ProcessorInstances::getSkinnedVertexProcessorBatched())
    .instanceDataBaseOffset = bufPos;
}

static void update_splinegen_for_bvh()
{
  if (bvhRenderingContextId)
    g_entity_mgr->broadcastEventImmediate(GatherSplinegenBVHDataEvent(bvhRenderingContextId));
}

static void update_smoke_tracers_for_bvh()
{
  if (!smokeTracersBvhConnection)
    return;
  g_entity_mgr->broadcastEventImmediate(GatherSmokeTracersBVHDataEvent(smokeTracersBvhConnection));
}

ECS_TAG(render)
static void set_animate_out_of_frustum_trees_es(const RendinstLodRangeIncreasedEvent &evt)
{
  rigen_cull_dist_was_increased = evt.get<0>() || evt.get<1>();
}

template <typename Callable>
static void copy_animchar_visbits_ecs_query(Callable c);

static void copy_animchar_visbits()
{
  copy_animchar_visbits_ecs_query([](const animchar_visbits_t &animchar_visbits, animchar_visbits_t &animchar_visbits_copy_for_bvh) {
    animchar_visbits_copy_for_bvh = animchar_visbits;
  });
}

void bvh_update_dyn_instances(const Point3 &cameraPos,
  const Point3 &lightDirection,
  const TMatrix &itm,
  const TMatrix4 &viewRotTm,
  const TMatrix4 &projTm,
  const TMatrix4 &prevViewRotTm,
  const TMatrix4 &prevProjTm)
{
  if (!is_bvh_enabled())
    return;
  wait_bvh_worker_threads();
  copy_animchar_visbits(); // bvh::update_dyn_instances processes animchars in a thread
  bvhDynmodelCtx = dynrend::get_or_create_context("bvh");
  dynrend::set_instance_data_only(bvhDynmodelCtx, !draw_debug_dynmodels);
  dynrend::set_context_view_proj(bvhDynmodelCtx, viewRotTm, projTm, prevViewRotTm, prevProjTm);
  if (draw_debug_dynmodels)
  {
    ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
    copy_current_global_variables_states(bvhDebugGvarsState);
  }
  update_splinegen_for_bvh();
  bvh::update_dyn_instances(bvhRenderingContextId, cameraPos, lightDirection, itm, projTm, bvh_iterate_over_animchars);
  bvh::set_for_gpu_objects(bvhRenderingContextId);
  rigen_cull_dist_was_increased = false; // Avoid state being stuck when shooter cam is not used next frame
}

static dafg::NodeHandle makeBVHUpdateNode()
{
  if (!is_bvh_enabled())
    return {};
  return dafg::register_node("bvh_update", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("prepare_skies_token");
    registry.createBlob<OrderingToken>("bvh_ready_token");
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");
    registry.readBlob<OrderingToken>("acesfx_update_token");
    registry.readBlob<OrderingToken>("grass_generated_token").optional();
    const auto resolution = registry.getResolution<2>("main_view");
    auto rtWaterHndl = registry.createBlob<int>("water_rt_enabled").handle();
    use_bindless_fom_shadows(registry, dafg::Stage::CS);

    auto dagdpCounterHndl =
      registry.read("dagdp_bvh_counter").buffer().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    auto dagdpIntsancesHndl =
      registry.read("dagdp_bvh_instances").buffer().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();

    auto camera = registry.readBlob<CameraParams>("current_camera");
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();

    return [cameraHndl, rtWaterHndl, resolution, dagdpCounterHndl, dagdpIntsancesHndl,
             rt_debug_rt_shadowVarId = get_shader_variable_id("rt_debug_rt_shadow", true)](
             const dafg::multiplexing::Index &multiplexing_index) {
      if (multiplexing_index != dafg::multiplexing::Index{})
        return;

      Point3 cameraPos = Point3::xyz(cameraHndl.ref().cameraWorldPos);
      auto &lights = WRDispatcher::getClusteredLights();
      {
        TIME_PROFILE(cull_lights)
        BBox3 box = BBox3(cameraPos, 100);
        TMatrix4_vec4 globtm = matrix_ortho_off_center_lh(box[0].x, box[1].x, box[1].y, box[0].y, box[0].z, box[1].z);
        if (!bvhLights)
          bvhLights = ClusteredLights::createCulledLights("bvh");
        lights.cullOutOfFrustumLights(*bvhLights, (mat44f_cref)globtm, SpotLightMaskType::SPOT_LIGHT_MASK_NONE,
          OmniLightMaskType::OMNI_LIGHT_MASK_NONE);
      }

      ShaderGlobal::set_int(var::rt_lighting_ao_hacks, rt_lighting_ao_hacks);
      ShaderGlobal::set_int(rt_debug_rt_shadowVarId, rtr_shadow ? 1 : 0);
      float tanPixelAngularRadius = cameraHndl.ref().jitterPersp.hk * tan(0.5f / resolution.get().x);
      ShaderGlobal::set_float(var::bvh_additional_impostor_mip_bias, additional_impostor_mip_bias);
      ShaderGlobal::set_float(var::bvh_tan_pixel_angular_radius, tanPixelAngularRadius);
      ShaderGlobal::set_int(var::bvh_max_ahs_calls, max_ahs_calls);
      rtWaterHndl.ref() = is_rt_water_enabled();
      bvh::start_frame();
      bvh::update_terrain(bvhRenderingContextId, Point2::xz(cameraPos));
      upload_dynmodel_instance_data();
      bvh::ensure_particle_buffer_capacity(fxBvhConnection ? fxBvhConnection->getMaxCount() : 0,
        smokeTracersBvhConnection ? smokeTracersBvhConnection->getMaxCount() : 0);
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
      if (dagdpInstanceMapper)
        dagdpInstanceMapper->submitBuffers(dagdpCounterHndl.get(), dagdpIntsancesHndl.get());
      bool hasGrass = is_bvh_dagdp_enabled() && get_grass_renderer();
      bvh::generate_gpu_grass_instances(bvhRenderingContextId, hasGrass);
      update_smoke_tracers_for_bvh();
      bvh::build(bvhRenderingContextId, cameraHndl.ref().viewItm, cameraHndl.ref().jitterProjTm, cameraPos, Point3::ZERO);
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

static dafg::NodeHandle pufdFallbackNode;

static dafg::NodeHandle makePUFDFallbackNode()
{
  if (!is_bvh_enabled())
    return {};
  return dafg::register_node("pufd_fallback", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.executionHas(dafg::SideEffects::External);
    registry.readBlob<OrderingToken>("bvh_ready_token");
    return [](const dafg::multiplexing::Index &multiplexing_index) {
      if (multiplexing_index != dafg::multiplexing::Index{})
        return;
      if (!should_delay_pufd_until_bvh_jobs_done())
        return;
      if (on_bvh_parallel_jobs_finished_cb)
        on_bvh_parallel_jobs_finished_cb();
    };
  });
}

static dafg::NodeHandle makeBvhRegisterFomShadowsNode()
{
  if (!is_bvh_enabled())
    return {};

  return dafg::register_node("bvh_register_fom_shadows", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.createBlob<OrderingToken>("bvh_fom_shadows_registered");
    auto fomSinHndl =
      registry.read("fom_shadows_sin").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    auto fomCosHndl =
      registry.read("fom_shadows_cos").texture().atStage(dafg::Stage::CS).useAs(dafg::Usage::SHADER_RESOURCE).optional().handle();
    auto fomSamplerHndl = registry.read("fom_shadows_sampler").blob<d3d::SamplerHandle>().optional().handle();
    return [fomSinHndl, fomCosHndl, fomSamplerHndl]() {
      bvh::bind_fom_textures(bvhRenderingContextId, fomSinHndl.view().getTex2D(), fomCosHndl.view().getTex2D(), fomSamplerHndl.get());
    };
  });
}

static void bvh_read_all_gbuffer_bindlessly(dafg::Registry registry)
{
  registry.read("gbuf_0").texture().atStage(dafg::Stage::PS_OR_CS).bindlessShaderVar("bvh_gbuffer_albedo_index");
  registry.read("gbuf_1").texture().atStage(dafg::Stage::PS_OR_CS).bindlessShaderVar("bvh_gbuffer_normal_index");
  registry.read("gbuf_2").texture().atStage(dafg::Stage::PS_OR_CS).bindlessShaderVar("bvh_gbuffer_material_index");
  registry.read("motion_vecs").texture().atStage(dafg::Stage::PS_OR_CS).bindlessShaderVar("bvh_gbuffer_motion_index");
  registry.read("gbuf_depth").texture().atStage(dafg::Stage::PS_OR_CS).bindlessShaderVar("bvh_gbuffer_depth_index");
}

template <typename Callable>
static void get_bvh_accumulated_speed_ecs_query(ecs::EntityManager &manager, Callable c);

static float calculate_denoiser_airplane_radius()
{
  if (is_free_camera_enabled())
    return 0;
  float speed = 0;
  get_bvh_accumulated_speed_ecs_query(*g_entity_mgr, [&](float bvh__accumulated_speed) { speed = bvh__accumulated_speed; });
  if (speed > airplane_detection_speed)
    return airplane_cockpit_radius;
  return 0;
}

// The cameras a trace node reads in its declaration, for its state hooks
struct RtTraceState
{
  eastl::optional<dafg::VirtualResourceHandle<const CameraParams, false, false>> prevCameraHndl;
  eastl::optional<dafg::VirtualResourceHandle<const CameraParams, false, false>> cameraHndl;
  eastl::optional<dafg::VirtualResourceHandle<const CameraParams, false, true>> lensCameraHndl;
  eastl::optional<dafg::VirtualResourceHandle<const CameraParams, false, true>> prevLensCameraHndl;
  eastl::optional<camera_in_camera::ApplyPostfxState> viewState;
  unsigned lensFrame = 0; // the frame of the last lens trace, see set_trace_view_state
};

static bool rt_frame_ready() { return bool(bvhRenderingContextId); }

// The trace shaders load the gbuffer through the bvh bindless indices and the view through its vectors
static void declare_rt_trace_inputs(dafg::Registry registry)
{
  registry.readBlob<OrderingToken>("bvh_ready_token");
  bvh_read_all_gbuffer_bindlessly(registry);
  CameraViewShvars{registry.readBlob<CameraParams>("current_camera")}.bindViewVecs();
  registry.bindTexCs("close_depth", "downsampled_close_depth_tex");
}

static void set_view_vecs(const CameraParams &camera)
{
  const ViewVecs &v = camera.viewVecs;
  set_viewvecs_to_shader(v.viewVecLT, v.viewVecRT, v.viewVecLB, v.viewVecRB);
}

static void set_trace_view_state(RtTraceState &state, bool lens_view)
{
  state.viewState.reset();
  const CameraParams *lens = lens_view ? state.lensCameraHndl->get() : nullptr;
  if (!lens)
  {
    state.viewState.emplace(true, state.cameraHndl->ref());
    return;
  }
  // the provider writes the lens camera while the lens is active, after a gap its history is of another aiming
  const unsigned frame = dagor_frame_no();
  const CameraParams *prevLens = state.lensFrame + 1 == frame ? state.prevLensCameraHndl->get() : nullptr;
  state.lensFrame = frame;
  state.viewState.emplace(false, *lens, prevLens ? *prevLens : *lens);
  set_view_vecs(*lens);
}

static void reset_trace_view_state(RtTraceState &state)
{
  state.viewState.reset();
  set_view_vecs(state.cameraHndl->ref()); // the binding of the node, daFG checks it after the node
}

static denoiser::ViewStateHooks make_view_state_hooks(const eastl::shared_ptr<RtTraceState> &state)
{
  denoiser::ViewStateHooks hooks;
  hooks.set = [state](bool lens_view) { set_trace_view_state(*state, lens_view); };
  hooks.reset = [state]() { reset_trace_view_state(*state); };
  return hooks;
}

// The scope lens camera while the lens is rendered, the lens pixels of the gbuffer are traced with it
static eastl::optional<denoiser::TraceView> lens_trace_view(
  const dafg::VirtualResourceHandle<const CameraParams, false, true> &lens_camera)
{
  const CameraParams *lens = lens_camera.get();
  if (!lens || !camera_in_camera::is_lens_render_active())
    return eastl::nullopt;
  return denoiser::TraceView{lens->jitterProjTm, Point3::xyz(lens->cameraWorldPos)};
}

// No camera in camera multiplexing in the RT nodes, the traces take the lens camera from their frame params
static dafg::NodeHandle make_denoiser_params_node()
{
  return dafg::register_node("denoiser_params", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto paramsHndl = denoiser::create_prepare_params(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
    return [paramsHndl, cameraHndl, prevCameraHndl]() {
      denoiser::PrepareParams &params = paramsHndl.ref();
      params = denoiser::PrepareParams();
      params.render = rt_frame_ready();
      if (!params.render)
        return;
      const CameraParams &camera = cameraHndl.ref();
      const CameraParams &prevCamera = prevCameraHndl.ref();
      params.viewPos = camera.viewItm.getcol(3);
      params.prevViewPos = prevCamera.viewItm.getcol(3);
      params.viewDir = camera.viewItm.getcol(2);
      params.prevViewDir = prevCamera.viewItm.getcol(2);
      params.viewItm = camera.viewItm;
      params.prevViewItm = prevCamera.viewItm;
      params.projTm = camera.noJitterProjTm;
      params.prevProjTm = prevCamera.noJitterProjTm;
      params.jitter = Point2(camera.jitterPersp.ox, camera.jitterPersp.oy);
      params.prevJitter = Point2(prevCamera.jitterPersp.ox, prevCamera.jitterPersp.oy);
      params.motionMultiplier = Point3::ONE;
      params.denoiserCameraAttachedRadius = calculate_denoiser_airplane_radius();
    };
  });
}

static void make_denoiser_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.clear();
  if (!is_denoiser_enabled())
    return;
  nodes.push_back(make_denoiser_params_node());

  const RTRMode rtrMode = get_resolved_rt_settings().rtrMode;
  denoiser::PrepareNodeParams params;
  params.halfResCloseSet = (is_rtao_enabled() || is_ptgi_enabled()) && !is_rr_enabled();
  params.checkerboardSet = rtrMode == RTRMode::halfCheckerboardDepth;
  params.smartDepthSet = rtrMode == RTRMode::halfSmartPattern;
  params.shadowTrace = is_rtsm_enabled();
  params.declareInputs = [halfResCloseSet = params.halfResCloseSet, checkerboardSet = params.checkerboardSet,
                           smartDepthSet = params.smartDepthSet](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");
    read_gbuffer(registry, dafg::Stage::PS_OR_CS);
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
    if (halfResCloseSet || smartDepthSet)
      registry.bindTexCs("close_depth", "downsampled_close_depth_tex");
    if (checkerboardSet || smartDepthSet)
      read_gbuffer_motion(registry, dafg::Stage::PS_OR_CS); // the rtr half set takes its motion vectors from the gbuffer
    if (checkerboardSet)
      registry.readTexture("checkerboard_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_checkerboard_depth_tex");
    if (smartDepthSet)
      registry.readTexture("far_downsampled_depth").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("downsampled_far_depth_tex");
  };
  nodes.push_back(denoiser::make_prepare_node(params));
}

static dafg::NodeHandle make_rtsm_params_node()
{
  return dafg::register_node("rtsm_params", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto paramsHndl = rtsm::create_frame_params(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    return [paramsHndl, cameraHndl, lensCameraHndl]() {
      rtsm::FrameParams &params = paramsHndl.ref();
      params = rtsm::FrameParams();
      params.render = rt_frame_ready();
      if (!params.render)
        return;
      const CameraParams &camera = cameraHndl.ref();
      const Color4 sunDir = ShaderGlobal::get_float4(from_sun_directionVarId);
      params.ctxId = bvhRenderingContextId;
      params.viewPos = Point3::xyz(camera.cameraWorldPos);
      params.lightDir = Point3(sunDir.r, sunDir.g, sunDir.b);
      params.projTm = camera.jitterProjTm;
      params.quality = is_rr_enabled() ? 0 : rtsm_quality.get();
      params.lensView = lens_trace_view(lensCameraHndl);
      ShaderGlobal::set_float4(var::sun_dir_for_shadows, params.lightDir); // For WT compatibility
    };
  });
}

static void make_rtsm_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.clear();
  if (!is_rtsm_enabled())
    return;
  nodes.push_back(make_rtsm_params_node());

  auto state = eastl::make_shared<RtTraceState>();
  rtsm::NodeParams params;
  params.motionVectorsName = "motion_vecs";
  params.declareInputs = [state](dafg::Registry registry) {
    // the trace sets world_view_pos itself from its frame params, a binding would be changed during the node
    declare_rt_trace_inputs(registry);
    state->cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    state->lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    state->prevLensCameraHndl = registry.readBlobHistory<CameraParams>("lens_area_camera").optional().handle();
  };
  params.viewState = make_view_state_hooks(state);
  rtsm::register_nodes(params, nodes);
}

dafg::NodeHandle make_rtsm_dynamic_node()
{
  return dafg::register_node("rtsm_dynamic", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.orderMeAfter("combine_shadows_node");
    registry.readBlob<OrderingToken>("bvh_ready_token");
    denoiser::read_ao_output(registry); // the lights take the AO, the optional read orders after the denoiser
    registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
    registry.read("upscale_sampling_tex").texture().atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("upscale_sampling_tex").optional();

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    auto viewPosHndl = registry.readBlob<Point4>("world_view_pos").handle();

    bvh_read_all_gbuffer_bindlessly(registry);

    auto dynamicLightTexHndl =
      registry.modifyTexture("dynamic_lighting_texture").atStage(dafg::Stage::COMPUTE).useAs(dafg::Usage::SHADER_RESOURCE).handle();
    auto hasAnyDynamicLightsHndl = registry.readBlob<bool>("has_any_dynamic_lights").handle();

    return [cameraHndl, viewPosHndl, dynamicLightTexHndl, hasAnyDynamicLightsHndl,
             rtsm_dynamic_minimum_output_valueVarId = get_shader_variable_id("rtsm_dynamic_minimum_output_value"),
             precomputed_dynamic_lightsVarId = get_shader_variable_id("precomputed_dynamic_lights")](
             const dafg::multiplexing::Index &multiplexing_index) {
      if (!hasAnyDynamicLightsHndl.ref())
        return;

      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

      auto &lights = WRDispatcher::getClusteredLights();
      lights.setInsideOfFrustumLightsToShader();

      ShaderGlobal::set_float(rtsm_dynamic_minimum_output_valueVarId, 6.1E-5f); // Smallest value on R11G11B10F
      rtsm::render_dynamic_light_shadows(bvhRenderingContextId, Point3::xyz(viewPosHndl.ref()), dynamicLightTexHndl.get(),
        dynamic_light_radius, is_rr_enabled());
      // Workaround for rtsm dynamic leaving an FG resource bound to shadervar
      ShaderGlobal::set_texture(precomputed_dynamic_lightsVarId, nullptr);
    };
  });
}

template <typename Callable>
static void bvh_check_is_binocular_ecs_query(ecs::EntityManager &manager, Callable c);

static dafg::NodeHandle make_rtr_params_node()
{
  return dafg::register_node("rtr_params", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto paramsHndl = rtr::create_frame_params(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    return [paramsHndl, cameraHndl, lensCameraHndl]() {
      rtr::FrameParams &params = paramsHndl.ref();
      params = rtr::FrameParams();
      params.render = rt_frame_ready();
      if (!params.render)
        return;
      params.ctxId = bvhRenderingContextId;
      params.projTm = cameraHndl.ref().jitterProjTm;
      params.rtShadow = rtr_shadow;
      params.randomizeRays = true;
      params.lensView = lens_trace_view(lensCameraHndl);
    };
  });
}

static void make_rtr_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.clear();
  const ResolvedRTSettings &rtSettings = get_resolved_rt_settings();
  prev_rtr_validation_enabled = rtr::is_validation_layer_enabled();
  if (!rtSettings.isRTREnabled())
    return;
  nodes.push_back(make_rtr_params_node());

  auto state = eastl::make_shared<RtTraceState>();
  rtr::NodeParams params;
  params.checkerboardDepth = rtSettings.rtrMode == RTRMode::halfCheckerboardDepth;
  params.smartDepth = rtSettings.rtrMode == RTRMode::halfSmartPattern;
  params.motionVectorsName = "motion_vecs";
  params.halfMotionVectorsName = "downsampled_motion_vectors_tex";
  params.declareInputs = [state](dafg::Registry registry) {
    declare_rt_trace_inputs(registry);
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
    use_bindless_fom_shadows(registry, dafg::Stage::CS);
    registry.readTextureHistory("prev_frame_tex").atStage(dafg::Stage::CS).bindlessShaderVar("bvh_prev_frame_tex_index");
    registry.historyFor("far_downsampled_depth")
      .texture()
      .atStage(dafg::Stage::CS)
      .bindlessShaderVar("bvh_prev_downsampled_far_depth_tex_index");
    registry.readTexture("checkerboard_depth")
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("downsampled_checkerboard_depth_tex")
      .optional();
    registry.readTexture("ssao_tex").atStage(dafg::Stage::PS_OR_CS).bindToShaderVar("ssao_tex").optional();
    registry.read("ssao_sampler").blob<d3d::SamplerHandle>().bindToShaderVar("ssao_tex_samplerstate").optional();
    denoiser::read_ao_output(registry); // the hit points take the AO
    state->cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    state->lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    state->prevLensCameraHndl = registry.readBlobHistory<CameraParams>("lens_area_camera").optional().handle();
    state->prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
  };
  params.declareProbeInputs = [state](dafg::Registry registry) {
    declare_rt_trace_inputs(registry);
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");
    read_gbuffer_depth(registry, dafg::Stage::PS_OR_CS);
    use_bindless_fom_shadows(registry, dafg::Stage::CS);
    state->prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
  };
  params.viewState = make_view_state_hooks(state);
  params.setShaderState = [state]() {
    const CameraParams &prevCamera = state->prevCameraHndl->ref();
    ShaderGlobal::set_float4(var::prev_zn_zfar, prevCamera.znear, prevCamera.zfar);
    ShaderGlobal::set_int(var::rtr_use_prev_frame_sample, rtr_use_prev_frame_sample ? 1 : 0);
    ShaderGlobal::set_int(var::rtr_rr_stochastic_ray, rtr_rr_stochastic_ray ? 1 : 0);

    bool isBinocular = false;
    bvh_check_is_binocular_ecs_query(*g_entity_mgr,
      [&isBinocular](ECS_REQUIRE(ecs::Tag cockpitEntity) ECS_REQUIRE(ecs::EntityId watchedByPlr)
          ecs::EntityId human_binocular__cockpitEid) { isBinocular |= human_binocular__cockpitEid != ecs::INVALID_ENTITY_ID; });
    ShaderGlobal::set_int(var::rtr_is_binocular, isBinocular ? 1 : 0);

    set_bvh_lights_to_shader();
  };
  params.resetShaderState = []() { WRDispatcher::getClusteredLights().setInsideOfFrustumLightsToShader(); };
  rtr::register_nodes(params, nodes);
}

ECS_TAG(render, dev)
ECS_NO_ORDER
ECS_REQUIRE(dag::Vector<dafg::NodeHandle> rtr_nodes)
static void rtr_debug_es(const UpdateStageInfoRenderDebug &)
{
  if (!is_rtr_enabled())
    return;
  rtr::render_probes();
}

static dafg::NodeHandle makeRTTRNode()
{
  if (!is_rttr_enabled())
    return {};
  auto nodeNs = dafg::root() / "transparent" / "close";
  return nodeNs.registerNode("rttr", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");

    use_bindless_fom_shadows(registry, dafg::Stage::CS);

    auto glassGbufferHndl =
      registry.modify("glass_target").texture().atStage(dafg::Stage::CS).bindToShaderVar("translucent_gbuffer").handle();

    registry.bindTexCs("glass_depth", "translucent_gbuffer_depth");
    registry.bindTexCs("far_downsampled_depth", "downsampled_far_depth_tex");

    return [cameraHndl, glassGbufferHndl, glass_rt = Ptr(new_compute_shader("rt_glass_reflection")),
             rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true),
             rtr_resolutionIVarId = get_shader_variable_id("rtr_resolutionI")](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref()};

      auto &lights = WRDispatcher::getClusteredLights();
      set_bvh_lights_to_shader();
      TextureInfo glassGbufferTi;
      glassGbufferHndl.view().getTex2D()->getinfo(glassGbufferTi);
      ShaderGlobal::set_int4(rtr_resolutionIVarId, glassGbufferTi.w, glassGbufferTi.h, 0, 0);
      rtr::set_rtr_hit_distance_params();
      ShaderGlobal::set_int(rtr_shadowVarId, rtr_shadow ? 1 : 0);
      bvh::bind_resources(bvhRenderingContextId, glassGbufferTi.w);

      glass_rt->dispatchThreads(glassGbufferTi.w, glassGbufferTi.h, 1);
      bvh::unbind_resources();
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

static dafg::NodeHandle make_rtao_params_node()
{
  return dafg::register_node("rtao_params", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto paramsHndl = rtao::create_frame_params(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    auto lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    auto renderResolution = registry.getResolution<2>("main_view");
    auto displayResolution = registry.getResolution<2>("display");
    return [paramsHndl, cameraHndl, lensCameraHndl, renderResolution, displayResolution]() {
      rtao::FrameParams &params = paramsHndl.ref();
      params = rtao::FrameParams();
      params.render = rt_frame_ready();
      if (!params.render)
        return;
      params.ctxId = bvhRenderingContextId;
      params.projTm = cameraHndl.ref().jitterProjTm;
      params.performanceMode = false;
      // an upscaler has few pixels to begin with, they all get a ray
      params.checkerboard = renderResolution.get().y >= displayResolution.get().y;
      params.lensView = lens_trace_view(lensCameraHndl);
    };
  });
}

static void make_rtao_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.clear();
  if (!is_rtao_enabled())
    return;
  nodes.push_back(make_rtao_params_node());

  auto state = eastl::make_shared<RtTraceState>();
  rtao::NodeParams params;
  params.motionVectorsName = "motion_vecs";
  params.halfMotionVectorsName = "downsampled_motion_vectors_tex";
  params.declareInputs = [state](dafg::Registry registry) {
    declare_rt_trace_inputs(registry);
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");
    state->cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    state->lensCameraHndl = registry.readBlob<CameraParams>("lens_area_camera").optional().handle();
    state->prevLensCameraHndl = registry.readBlobHistory<CameraParams>("lens_area_camera").optional().handle();
  };
  params.viewState = make_view_state_hooks(state);
  rtao::register_nodes(params, nodes);
}

static dafg::NodeHandle make_ptgi_params_node()
{
  return dafg::register_node("ptgi_params", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto paramsHndl = ptgi::create_frame_params(registry);
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();
    return [paramsHndl, cameraHndl]() {
      ptgi::FrameParams &params = paramsHndl.ref();
      params = ptgi::FrameParams();
      params.render = rt_frame_ready();
      if (!params.render)
        return;
      params.ctxId = bvhRenderingContextId;
      params.projTm = cameraHndl.ref().jitterProjTm;
      params.quality = static_cast<ptgi::Quality>(ptgi_quality.get());
      params.inCockpit = false;
      params.halfRate = ptgi_half_rate.get();
    };
  });
}

static void make_ptgi_nodes(dag::Vector<dafg::NodeHandle> &nodes)
{
  nodes.clear();
  prev_ptgi_validation_enabled = ptgi::is_validation_layer_enabled();
  prev_ptgi_second_chance_enabled = ptgi::is_second_chance_history_enabled();
  if (!is_ptgi_enabled())
    return;
  nodes.push_back(make_ptgi_params_node());

  auto state = eastl::make_shared<RtTraceState>();
  ptgi::NodeParams params;
  params.motionVectorsName = "motion_vecs";
  params.halfMotionVectorsName = "downsampled_motion_vectors_tex";
  params.declareInputs = [state](dafg::Registry registry) {
    declare_rt_trace_inputs(registry);
    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");
    registry.readTextureHistory("prev_frame_tex").atStage(dafg::Stage::CS).bindlessShaderVar("bvh_prev_frame_tex_index");
    registry.historyFor("far_downsampled_depth")
      .texture()
      .atStage(dafg::Stage::CS)
      .bindlessShaderVar("bvh_prev_downsampled_far_depth_tex_index");
    state->prevCameraHndl = registry.readBlobHistory<CameraParams>("current_camera").handle();
  };
  params.declareDebugInputs = [](dafg::Registry registry) { read_gbuffer(registry, dafg::Stage::PS_OR_CS); };
  params.setShaderState = [state]() {
    const CameraParams &prevCamera = state->prevCameraHndl->ref();
    ShaderGlobal::set_float4(var::prev_zn_zfar, prevCamera.znear, prevCamera.zfar);
    ShaderGlobal::set_int(var::ptgi_use_prev_frame_sample, ptgi_use_prev_frame_sample ? 1 : 0);
    set_bvh_lights_to_shader();
  };
  params.resetShaderState = []() { WRDispatcher::getClusteredLights().setInsideOfFrustumLightsToShader(); };
  ptgi::register_nodes(params, nodes);
}

enum class WaterRTOutputMode
{
  REGULAR = 0,
  RR = 1
};

dafg::NodeHandle makeWaterRTNode()
{
  if (!is_rt_water_enabled())
    return {};
  if (!get_world_renderer())
    return {};
  WaterRenderMode mode = WRDispatcher::getWaterRenderMode();
  const uint32_t modeIdx = eastl::to_underlying(mode);
  return dafg::register_node("water_rt_node", DAFG_PP_NODE_SRC, [modeIdx](dafg::Registry registry) {
    registry.readBlob<OrderingToken>("bvh_ready_token");

    registry.multiplex(dafg::multiplexing::Mode::FullMultiplex);
    auto camera = read_camera_in_camera(registry);
    auto cameraHndl = CameraViewShvars{camera}.bindViewVecs().toHandle();
    auto prevCameraHndl = read_history_camera_in_camera(registry).handle();

    registry.bindBlob<Point4>("world_view_pos", "world_view_pos");

    registry.bindTexCs("water_normal_dir", "water_normal_dir");
    auto colorTexHndl =
      registry.modify("water_ssr_color").texture().atStage(dafg::Stage::CS).bindToShaderVar("water_reflection_tex_uav").handle();
    auto strengthTexHndl = registry.modify("water_ssr_strength")
                             .texture()
                             .atStage(dafg::Stage::CS)
                             .bindToShaderVar("water_reflection_strength_tex_uav")
                             .handle();

    if (!is_rr_enabled())
    {
      registry.historyFor("water_ssr_color").texture().atStage(dafg::Stage::CS).bindToShaderVar("water_reflection_tex");
      registry.historyFor("water_ssr_strength").texture().atStage(dafg::Stage::CS).bindToShaderVar("water_reflection_strength_tex");
    }

    registry.read(is_rr_enabled() ? "water_rt_depth" : WATER_SSR_DEPTH_TEX[modeIdx + 1])
      .texture()
      .atStage(dafg::Stage::CS)
      .bindToShaderVar("downsampled_depth");

    use_bindless_fom_shadows(registry, dafg::Stage::CS);

    registry.bindTexCs("water_planar_reflection_clouds", "water_planar_reflection_clouds").optional();
    registry.read("water_planar_reflection_clouds_sampler")
      .blob<d3d::SamplerHandle>()
      .bindToShaderVar("water_planar_reflection_clouds_samplerstate")
      .optional();

    auto resHndl = registry.getResolution<2>("main_view", is_rr_enabled() ? 1.0f : 0.5f);

    return [prevCameraHndl, cameraHndl, resHndl, colorTexHndl, strengthTexHndl,
             water_rt = Ptr(new_compute_shader("raytraced_water_reflections")),
             rtr_shadowVarId = get_shader_variable_id("rtr_shadow", true)](const dafg::multiplexing::Index &multiplexing_index) {
      camera_in_camera::ApplyPostfxState camcam{multiplexing_index, cameraHndl.ref(), prevCameraHndl.ref()};

      auto &lights = WRDispatcher::getClusteredLights();
      set_bvh_lights_to_shader();
      static int frameIdx = 0;
      ShaderGlobal::set_int(var::water_rt_frame_index, (frameIdx++) % 32);
      ShaderGlobal::set_float4(var::sun_dir_for_shadows, ShaderGlobal::get_float4(from_sun_directionVarId)); // For WT compatibility
      ShaderGlobal::set_int(rtr_shadowVarId, rtr_shadow ? 1 : 0);
      ShaderGlobal::set_int(var::water_rt_output_mode,
        is_rr_enabled() ? eastl::to_underlying(WaterRTOutputMode::RR) : eastl::to_underlying(WaterRTOutputMode::REGULAR));
      IPoint2 res = resHndl.get();
      bvh::bind_resources(bvhRenderingContextId, res.x);
      rtr::set_water_params();

      if (camera_in_camera::is_main_view(multiplexing_index))
      {
        d3d::clear_rt({colorTexHndl.get()}, {});
        d3d::clear_rt({strengthTexHndl.get()}, make_clear_value(1.0f, 0.0f, 0.0f, 0.0f));
      }

      water_rt->dispatchThreads(res.x, res.y, 1);
      bvh::unbind_resources();
      lights.setInsideOfFrustumLightsToShader();
    };
  });
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, ChangeRenderFeaturesEarly)
ECS_TRACK(render_settings__enableRTTRShadows)
static void set_glass_rtr_shadow_es(const ecs::Event &, bool render_settings__enableRTTRShadows)
{
  ShaderGlobal::set_int(var::glass_rtr_shadow, render_settings__enableRTTRShadows);
}

template <typename Callable>
static void recreate_bvh_nodes_ecs_query(ecs::EntityManager &manager, Callable c);

static void recreateBVHNodes()
{
  recreate_bvh_nodes_ecs_query(*g_entity_mgr,
    [](dafg::NodeHandle &bvh__update_node, dag::Vector<dafg::NodeHandle> &denoiser_nodes, dag::Vector<dafg::NodeHandle> &rtsm_nodes,
      dag::Vector<dafg::NodeHandle> &rtr_nodes, dafg::NodeHandle &rttr_node, dag::Vector<dafg::NodeHandle> &rtao_nodes,
      dag::Vector<dafg::NodeHandle> &ptgi_nodes, dafg::NodeHandle &water_rt_node, dafg::NodeHandle &bvh_register_fom_shadows) {
      bvh__update_node = makeBVHUpdateNode();
      bvh_register_fom_shadows = makeBvhRegisterFomShadowsNode();
      pufdFallbackNode = makePUFDFallbackNode();
      make_denoiser_nodes(denoiser_nodes);
      make_rtsm_nodes(rtsm_nodes);
      make_rtr_nodes(rtr_nodes);
      make_rtao_nodes(rtao_nodes);
      make_ptgi_nodes(ptgi_nodes);
      toggle_rtsm_dynamic(is_rtsm_dynamic_enabled() && is_bvh_usable());
      rttr_node = makeRTTRNode();
      water_rt_node = makeWaterRTNode();
    });
}

void recreate_water_rt_node() { recreateBVHNodes(); }

ECS_TAG(dev, render)
static void rtr_ptgi_debug_view_recreate_es(const UpdateStageInfoBeforeRender &, const ResolvedRTSettings &resolved_rt_settings)
{
  const bool rtrChanged =
    resolved_rt_settings.rtrMode != RTRMode::off && prev_rtr_validation_enabled != rtr::is_validation_layer_enabled();
  const bool ptgiChanged =
    resolved_rt_settings.isPTGIEnabled && (prev_ptgi_validation_enabled != ptgi::is_validation_layer_enabled() ||
                                            prev_ptgi_second_chance_enabled != ptgi::is_second_chance_history_enabled());
  if (rtrChanged || ptgiChanged)
    recreateBVHNodes();
}

ECS_TAG(render)
static void rt_set_resolution_es(const SetResolutionEvent &resEvt)
{
  if (resEvt.type == SetResolutionEvent::Type::DYNAMIC_RESOLUTION)
  {
    rtr::set_dynamic_resolution(resEvt.renderingResolution);
    return;
  }

  rtr::set_resolution(resEvt.maxPossibleRenderResolution);
  recreateBVHNodes();
};

template <typename Callable>
static void bvh_destroy_ri_visibility_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
static void bvh_create_ri_visibility_ecs_query(ecs::EntityManager &manager, Callable c);


static void destroyBVH()
{
  ScopedSwapchainPresentBlock presentBlock;
  closeBVHScene();
  bvh_destroy_ri_visibility_ecs_query(*g_entity_mgr,
    [](bool &bvh__initialized, RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility) {
      bvh__initialized = false;
      wait_bvh_worker_threads();
      bvh__rendinst_visibility = {};
      bvh__rendinst_oof_visibility = {};
    });
  closeBVH();
  apply_united_vdata_allocation_limits(is_bvh_enabled());
}

static void createBVH()
{
  initBVH();
  bvh_create_ri_visibility_ecs_query(*g_entity_mgr,
    [](RiGenVisibilityECS &bvh__rendinst_visibility, RiGenVisibilityECS &bvh__rendinst_oof_visibility) {
      bvh__rendinst_visibility = RiGenVisibilityECS::create();
      bvh__rendinst_oof_visibility = RiGenVisibilityECS::create();
    });
  apply_united_vdata_allocation_limits(is_bvh_enabled());
}

static bool is_rr_supported()
{
  nv::Streamline *streamline = nullptr;
  d3d::driver_command(Drv3dCommand::GET_STREAMLINE, &streamline);
  return streamline && streamline->isDlssRRSupported() == nv::SupportState::Supported;
}

ECS_TAG(render)
ECS_ON_EVENT(OnRenderSettingsReady, ChangeRenderFeaturesEarly)
ECS_TRACK(render_settings__enableBVH,
  render_settings__enableRTSM,
  render_settings__enableRTR,
  render_settings__enableRTTR,
  render_settings__enableRTAO,
  render_settings__enablePTGI,
  render_settings__RTRWater,
  render_settings__enableRTGI,
  render_settings__bvhDagdp,
  render_settings__bare_minimum,
  render_settings__antialiasing_mode,
  render_settings__rayReconstruction,
  render_settings__bvhDynModels,
  render_settings__RTpreset)
static void bvh_render_settings_changed_es(const ecs::Event &,
  ecs::EntityManager &manager,
  bool render_settings__enableBVH,
  const ecs::string &render_settings__enableRTSM,
  bool render_settings__enableRTR,
  bool render_settings__enableRTTR,
  bool render_settings__enableRTAO,
  bool render_settings__enablePTGI,
  bool render_settings__RTRWater,
  bool render_settings__enableRTGI,
  bool render_settings__bvhDagdp,
  bool render_settings__bare_minimum,
  const ecs::string &render_settings__antialiasing_mode,
  bool render_settings__rayReconstruction,
  bool render_settings__bvhDynModels,
  const ecs::string &render_settings__RTpreset)
{
  static bool enableAllRTFeatures = dgs_get_settings()->getBlockByNameEx("graphics")->getBool("enableAllRTFeatures", false);

  ResolvedRTSettings oldSettings = get_resolved_rt_settings();

  ResolvedRTSettings settings;
  const bool isBVHAvailable =
    is_rt_supported() && !render_settings__bare_minimum && get_current_render_features().test(FeatureRenderFlags::FULL_DEFERRED);
  settings.isRTSMEnabled =
    isBVHAvailable && (render_settings__enableRTSM == "sun_and_dynamic" || render_settings__enableRTSM == "sun");
  settings.isRTSMDynamicEnabled = isBVHAvailable && render_settings__enableRTSM == "sun_and_dynamic";
  settings.isRTTREnabled = isBVHAvailable && render_settings__enableRTTR;
  settings.isRTWaterEnabled = isBVHAvailable && render_settings__RTRWater;
  settings.isRayReconstructionEnabled =
    isBVHAvailable && render_settings__antialiasing_mode == "dlss" && render_settings__rayReconstruction && is_rr_supported();
  settings.isPTGIEnabled =
    isBVHAvailable && render_settings__enablePTGI && (enableAllRTFeatures || settings.isRayReconstructionEnabled);
  settings.isRTAOEnabled = isBVHAvailable && render_settings__enableRTAO && !settings.isPTGIEnabled;
  settings.isRTGIEnabled = isBVHAvailable && render_settings__enableRTGI && enableAllRTFeatures && !settings.isPTGIEnabled;

  if (settings.isRayReconstructionEnabled)
    settings.rtrMode = RTRMode::full;
  else if (isBVHAvailable && render_settings__enableRTR)
    settings.rtrMode = rtr_checkerboard_depth.get() ? RTRMode::halfCheckerboardDepth : RTRMode::halfSmartPattern;

  settings.isDenoiserEnabled = settings.isRTSMEnabled || settings.isRTREnabled() || settings.isRTAOEnabled || settings.isPTGIEnabled;
  // Allow for debugging in the ImGUI window, even with no render features
  constexpr bool isDebug = DAGOR_DBGLEVEL > 0;
  settings.isBVHEnabled = (isDebug && isBVHAvailable && render_settings__enableBVH) || settings.isRTSMEnabled ||
                          settings.isRTTREnabled || settings.isRTREnabled() || settings.isRTAOEnabled || settings.isPTGIEnabled ||
                          settings.isRTWaterEnabled || settings.isRTGIEnabled;
  settings.isDagdpEnabled = settings.isBVHEnabled && render_settings__bvhDagdp;

  settings.isBvhDynModelsEnabled = settings.isBVHEnabled && render_settings__bvhDynModels;
  settings.ultraPerformanceOverwrite = settings.isBVHEnabled && render_settings__RTpreset == "ultraPerformance";

  set_resolved_rt_settings_ecs_query(manager, [&settings](ResolvedRTSettings &resolved_rt_settings, bool &needs_water_heightmap) {
    resolved_rt_settings = settings;
    needs_water_heightmap = settings.isBVHEnabled;
  });

  const bool wasBVHEnabled = oldSettings.isBVHEnabled;
  const bool isBVHEnabled = settings.isBVHEnabled;

  const bool bvhTurnedOn = !wasBVHEnabled && isBVHEnabled;
  const bool bvhTurnedOff = wasBVHEnabled && !isBVHEnabled;
  const bool bvhRemainsEnabled = wasBVHEnabled && isBVHEnabled;
  const bool bvhRecreate = bvhRemainsEnabled && oldSettings.ultraPerformanceOverwrite != settings.ultraPerformanceOverwrite;

  if (bvhRemainsEnabled && oldSettings.isBvhDynModelsEnabled != settings.isBvhDynModelsEnabled && !bvhRecreate)
  {
    if (settings.isBvhDynModelsEnabled)
      bvh::enable_dyn_models(bvhRenderingContextId);
    else
      bvh::disable_dyn_models(bvhRenderingContextId);
    ResolvedRTSettings dynModelsOnlyChange = oldSettings;
    dynModelsOnlyChange.isBvhDynModelsEnabled = settings.isBvhDynModelsEnabled;
    if (dynModelsOnlyChange == settings)
      return;
  }

  closeRTFeatures();
  if (bvhTurnedOff || bvhRecreate)
    destroyBVH();
  if (bvhTurnedOn || bvhRecreate)
    createBVH();

  if (oldSettings.isDagdpEnabled != settings.isDagdpEnabled)
    manager.broadcastEventImmediate(BVHDagdpChanged{});
  if (get_world_renderer()) // Can't query resolution on login screen
  {
    initRTFeatures();
    recreateBVHNodes();

    // Shadow subscribes to render_settings__enableRTSM
    const bool rayReconstructionChanged = oldSettings.isRayReconstructionEnabled != settings.isRayReconstructionEnabled;
    uint32_t featuresToReset = 0;
    if (oldSettings.isRTWaterEnabled != settings.isRTWaterEnabled)
      featuresToReset |= WRDispatcher::WATER;
    if (oldSettings.isRTAOEnabled != settings.isRTAOEnabled || rayReconstructionChanged)
      featuresToReset |= WRDispatcher::SSAO;
    if (oldSettings.isPTGIEnabled != settings.isPTGIEnabled || rayReconstructionChanged)
      featuresToReset |= WRDispatcher::GI | WRDispatcher::SSAO;
    if (oldSettings.isRTGIEnabled != settings.isRTGIEnabled)
      featuresToReset |= WRDispatcher::GI;
    if (oldSettings.rtrMode != settings.rtrMode || rayReconstructionChanged)
      featuresToReset |= WRDispatcher::SSR;
    if (rayReconstructionChanged)
      featuresToReset |= WRDispatcher::REINIT_TARGET;
    WRDispatcher::recreateRayTracingDependentNodes(featuresToReset);
  }
}

template <typename Callable>
static void set_resolved_rtr_settings_ecs_query(ecs::EntityManager &manager, Callable c);

ECS_TAG(render, dev)
static void rtr_checkerboard_depth_convar_es(const UpdateStageInfoBeforeRender &, ecs::EntityManager &manager)
{
  if (!rtr_checkerboard_depth.pullValueChange())
    return;

  ResolvedRTSettings settings = get_resolved_rt_settings();
  if (settings.rtrMode == RTRMode::halfSmartPattern && rtr_checkerboard_depth.get())
    settings.rtrMode = RTRMode::halfCheckerboardDepth;
  else if (settings.rtrMode == RTRMode::halfCheckerboardDepth && !rtr_checkerboard_depth.get())
    settings.rtrMode = RTRMode::halfSmartPattern;
  else
    return;

  set_resolved_rtr_settings_ecs_query(manager,
    [&settings](ResolvedRTSettings &resolved_rt_settings) { resolved_rt_settings = settings; });

  rtr::initialize(settings.isRTRHalfRes(), settings.isRTRRaySkipping(), rtr_path_traced.get()); // the ray skipping follows the pattern
  recreateBVHNodes();
}

ECS_TAG(render, dev)
static void rtr_path_traced_convar_es(const UpdateStageInfoBeforeRender &)
{
  if (!rtr_path_traced.pullValueChange())
    return;

  const ResolvedRTSettings &settings = get_resolved_rt_settings();
  if (!settings.isBVHEnabled || !settings.isRTREnabled())
    return;

  rtr::initialize(settings.isRTRHalfRes(), settings.isRTRRaySkipping(), rtr_path_traced.get());
  set_rtr_ray_limit_params(settings.isRayReconstructionEnabled);
  recreateBVHNodes(); // the probe nodes come and go with the mode
}

bool bvh_do_early_occlusion_culling() { return is_bvh_enabled() && (bvh_animchar_lod_dist_mul > 1 || bvh_max_skinned_memory_mb >= 0); }
float get_bvh_animchar_lod_dist_mul()
{
  // Zoom boosts the quality of lods, so to save memory this reverses that for anything not in main view
  return bvh_animchar_lod_dist_mul *
         safediv(WRDispatcher::getLodDistanceScaleBase(), DynamicRenderableSceneInstance::lodDistanceScale);
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
ECS_REQUIRE(dafg::NodeHandle bvh__update_node)
static void bvh_update_animchar_es(const UpdateStageInfoBeforeRender &stg)
{
  if (!is_bvh_enabled())
    return;
  if (!bvh::has_features(bvhRenderingContextId, bvh::Features::AnyDynrend))
    return;
  TIME_PROFILE(bvh_update_animchar)
  TMatrix4 cullingMatrix = get_bvh_culling_matrix(stg.camPos);
  Frustum frustum = cullingMatrix;
  preprocess_visible_animchars_in_frustum(stg, frustum, v_ldu_p3(&stg.camPos.x), VISFLG_BVH, get_bvh_animchar_lod_dist_mul());
}

static void bvh_set_dynmodel_instance_data(int instance_offset)
{
  if (!bvhDynmodelDataReady)
    return;
  static_cast<const bvh::SkinnedVertexProcessorBatched &>(bvh::ProcessorInstances::getSkinnedVertexProcessorBatched())
    .lastInstanceOffset = (uint32_t)(instance_offset);
}

template <typename Callable>
static void bvh_iterate_over_animchars_ecs_query(ecs::EntityManager &manager, Callable c);
template <typename Callable>
static void bvh_query_camo_params_ecs_query(ecs::EntityManager &manager, ecs::EntityId eid, Callable c);

struct BVHAnimcharCallbackData
{
  const DynamicRenderableSceneInstance *inst = nullptr;
  const DynamicRenderableSceneResource *res = nullptr;
  eastl::vector<int, framemem_allocator> offsets;
  bool animate = false;
  dynrend::BVHCamoData bvhCamoData;

  float skinPriority = 0; // higher priority gets added first up to a max memory limit
};
static dag::Vector<BVHAnimcharCallbackData> animcharCallbackDatas;

static void record_active_dyn_resource(const DynamicRenderableSceneInstance *inst)
{
  if (!bvhUnloadDynModels)
    return;
  if (const DynamicRenderableSceneLodsResource *lodsRes = inst->getLodsResource())
    bvhActiveDynResources.insert(lodsRes->getFirstOriginal());
}

static void bvh_iterate_over_animchars(
  dynrend::BVHIterateOneInstanceCallback iterate_one_instance, const Point3 &view_position, void *user_data)
{
  constexpr animchar_visbits_t FLAGS_TO_TEST =
    VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE | VISFLG_CSM_SHADOW_RENDERED | VISFLG_COCKPIT_VISIBLE;

  vec3f viewPosition = v_ldu(&view_position.x);
  auto calculate_priority = [&](animchar_visbits_t animchar_visbits, const vec4f &position) {
    constexpr float VISIBLE_BOOST = 1; // prioritize rasterized objects above all
    float distSq = v_extract_x(v_length3_sq_x(v_sub(position, viewPosition)));
    float weight = 1.f / (1.f + distSq); // [0;1]
    if (animchar_visbits & VISFLG_BVH_MAIN_VISIBLE)
      weight += VISIBLE_BOOST;
    return weight;
  };

  TEXTUREID burntTankCamo = var::burnt_tank_camo.get_texture();
  auto getCamoData = [&](ecs::EntityId eid) {
    dynrend::BVHCamoData bvhCamoData;
    bvh_query_camo_params_ecs_query(*g_entity_mgr, eid,
      [&](float vehicle_camo_condition, float vehicle_camo_scale, float vehicle_camo_rotation) {
        bvhCamoData.condition = vehicle_camo_condition;
        bvhCamoData.scale = vehicle_camo_scale;
        bvhCamoData.rotation = vehicle_camo_rotation;
      });
    bvhCamoData.burntCamo = burntTankCamo;
    return bvhCamoData;
  };

  {
    TIME_PROFILE(iterate_entities)
    bvh_iterate_over_animchars_ecs_query(*g_entity_mgr,
      [&](ECS_REQUIRE_NOT(ecs::Tag excludeFromAnimcharRender, ecs::Tag invisibleUpdatableAnimchar, ecs::Tag excludeFromBvh)
            ecs::EntityId eid,
        const animchar_visbits_t &animchar_visbits_copy_for_bvh, const AnimV20::AnimcharRendComponent &animchar_render,
        const ecs::Point4List *additional_data, const ecs::UInt8List *animchar_render__nodeVisibleStgFilters,
        const vec4f &animchar_bsph) {
        if (!(animchar_visbits_copy_for_bvh & VISFLG_BVH))
          return;

        const DynamicRenderableSceneInstance *inst = animchar_render.getSceneInstance();
        G_ASSERT_RETURN(inst != nullptr, );
        const auto res = inst->getCurSceneResource();
        if (!res)
          return;

        record_active_dyn_resource(inst);

        auto filter = dynrend::PathFilterView(animchar_render__nodeVisibleStgFilters);
        G_ASSERT(!animchar_render__nodeVisibleStgFilters || filter.size() == inst->getNodeCount());

        animchar_additional_data::AnimcharAdditionalDataView additionalDataView =
          animchar_additional_data::get_optional_data(additional_data);

        // Only checking for shadow, because vehicles might be hidden in main rendering in cockpit mode!
        constexpr uint8_t filterMask = UpdateStageInfoRender::RENDER_SHADOW;

        auto &data = animcharCallbackDatas.push_back();

        dynrend::add_animchar(bvhDynmodelCtx, ShaderMesh::STG_opaque, ShaderMesh::STG_atest, inst, res, additionalDataView,
          draw_debug_dynmodels ? dynrend::NeedPreviousMatrices::Yes : dynrend::NeedPreviousMatrices::No, {}, filter, filterMask,
          dynrend::RenderPriority::DEFAULT, get_bvh_debug_gvars_state(), TexStreamingContext(0), &data.offsets);

        dynrend::BVHCamoData bvhCamoData = getCamoData(eid);
        bool animate = [viewPosition, animchar_visbits_copy_for_bvh, position = animchar_bsph]() {
          if (static_cast<bool>(animchar_visbits_copy_for_bvh & FLAGS_TO_TEST))
            return true;
          if (v_extract_x(v_length3_sq_x(v_sub(position, viewPosition))) < animate_all_animchars_range * animate_all_animchars_range)
            return true;
          return false;
        }();

        data.inst = inst;
        data.res = res;
        data.animate = animate;
        data.bvhCamoData = bvhCamoData;
        data.skinPriority = calculate_priority(animchar_visbits_copy_for_bvh, animchar_bsph);
      });
    BVHAdditionalAnimcharIterateCallback additonalAnimcharCallback =
      [&](ecs::EntityId eid, DynamicRenderableSceneInstance *inst, DynamicRenderableSceneResource *res,
        animchar_additional_data::AnimcharAdditionalDataView additional_data_view,
        const animchar_visbits_t &animchar_visbits_copy_for_bvh) {
        if (!(animchar_visbits_copy_for_bvh & VISFLG_BVH))
          return;
        if (!res)
          return;

        record_active_dyn_resource(inst);

        auto &data = animcharCallbackDatas.push_back();

        dynrend::add_animchar(bvhDynmodelCtx, ShaderMesh::STG_opaque, ShaderMesh::STG_atest, inst, res, additional_data_view,
          draw_debug_dynmodels ? dynrend::NeedPreviousMatrices::Yes : dynrend::NeedPreviousMatrices::No, {},
          dynrend::PathFilterView::NULL_FILTER, 0, dynrend::RenderPriority::DEFAULT, get_bvh_debug_gvars_state(),
          TexStreamingContext(0), &data.offsets);

        dynrend::BVHCamoData bvhCamoData = getCamoData(eid);
        bool animate = static_cast<bool>(animchar_visbits_copy_for_bvh & FLAGS_TO_TEST);

        data.inst = inst;
        data.res = res;
        data.animate = animate;
        data.bvhCamoData = bvhCamoData;
        data.skinPriority = calculate_priority(animchar_visbits_copy_for_bvh, v_ldu_p3_safe(&inst->getNodeWtm(0).getcol(3).x));
      };
    g_entity_mgr->broadcastEventImmediate(BVHAdditionalAnimcharIterate(additonalAnimcharCallback));
  }

  {
    TIME_PROFILE(sort)
    stlsort::sort(animcharCallbackDatas.begin(), animcharCallbackDatas.end(),
      [](auto &a, auto &b) { return a.skinPriority > b.skinPriority; });
  }

  {
    TIME_PROFILE(add_entities_to_bvh)
    dynrend::BVHSkinnedMemoryUsage skinMem = {.max_bytes = int64_t(bvh_max_skinned_memory_mb) * (1ll << 20), .current_bytes = 0ll};
    for (auto &data : animcharCallbackDatas)
    {
      iterate_one_instance(*data.inst, *data.res, data.offsets, bvh_set_dynmodel_instance_data, data.animate, data.bvhCamoData,
        skinMem, user_data);
    }
#if DA_PROFILER_ENABLED
    if (bvh_max_skinned_memory_mb >= 0)
    {
      auto mb = [](int64_t v) { return double(v) / (1024.0 * 1024.0); };
      float currentMemMB = mb(skinMem.current_bytes);
      DA_PROFILE_TAG(add_entities_to_bvh, "Skinned Memory: %fMB of %dMB", currentMemMB, bvh_max_skinned_memory_mb.get())
    }
#endif
  }

  // Clear all offsets stored in framemem
  // Keeping the vector allocated to avoid potential perf spikes
  animcharCallbackDatas.clear();
}

static dafg::NodeHandle makeBvhDrawDebugNode()
{
  return dafg::register_node("bvh_debug_dynmodels_node", DAFG_PP_NODE_SRC, [](dafg::Registry registry) {
    auto debugNs = registry.root() / "debug";
    auto colorTarget = debugNs.modifyTexture("target_for_debug");
    auto displayResolution = registry.getResolution<2>("display");
    auto dynmodelDebugDepthRT =
      registry.create("bvh_dynmodel_debug_depth").texture({TEXCF_RTARGET | TEXFMT_DEPTH32, displayResolution});
    registry.requestRenderPass().color({colorTarget}).depth(dynmodelDebugDepthRT);
    registry.requestState().setFrameBlock("global_frame");
    registry.readBlob<OrderingToken>("bvh_ready_token");
    auto cameraHndl = registry.readBlob<CameraParams>("current_camera").handle();

    return [cameraHndl, dyn_model_render_passVarId = ::get_shader_variable_id("dyn_model_render_pass"),
             dynamicSceneBlockId = ShaderGlobal::getBlockId("dynamic_scene"), displayResolution]() {
      if (!dynrend::is_valid_context(bvhDynmodelCtx) || !bvhDynmodelDataReady)
        return;
      ShaderGlobal::set_int(dyn_model_render_passVarId, eastl::to_underlying(dynmodel::RenderPass::Color));
      SCOPE_VIEW_PROJ_MATRIX;
      SCOPE_VIEWPORT;
      d3d::setview(0, 0, displayResolution.get().x / 2, displayResolution.get().y / 2, 0, 1);
      d3d::clearview(CLEAR_TARGET | CLEAR_ZBUFFER, e3dcolor(Color4{}), 0, 0);
      d3d::settm(TM_PROJ, &cameraHndl.ref().noJitterProjTm);
      TMatrix vtm = cameraHndl.ref().viewTm;
      vtm.setcol(3, 0, 0, 0);
      d3d::settm(TM_VIEW, vtm);
      SCENE_LAYER_GUARD(dynamicSceneBlockId);
      dynrend::render_all_stages(bvhDynmodelCtx);
    };
  });
}

ECS_TAG(render, dev)
static void recreate_draw_dynmodel_debug_node_es(const UpdateStageInfoBeforeRender &, dafg::NodeHandle &bvh_debug_dynmodels_node)
{
  bool needNode = is_bvh_enabled() && draw_debug_dynmodels;
  if (needNode && !bvh_debug_dynmodels_node)
    bvh_debug_dynmodels_node = makeBvhDrawDebugNode();
  else if (!needNode && bvh_debug_dynmodels_node)
    bvh_debug_dynmodels_node = {};
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
  static int texcoord_scale_offsetVarId = get_shader_variable_id("texcoord_scale_offset", true);
  static int ri_landclass_indexVarId = get_shader_variable_id("ri_landclass_index", true);
  static int colorVarId = get_shader_variable_id("color", true);
  static int metalnessVarId = get_shader_variable_id("metalness", true);
  static int smoothnessVarId = get_shader_variable_id("smoothness", true);
  static int reflectanceVarId = get_shader_variable_id("reflectance", true);
  static int tank_decals_smoothnessVarId = get_shader_variable_id("tank_decals_smoothness", true);
  static int mask_gamma_startVarId = get_shader_variable_id("mask_gamma_start");
  static int mask_gamma_endVarId = get_shader_variable_id("mask_gamma_end");
  static int mask_tile_uVarId = get_shader_variable_id("mask_tile_u");
  static int mask_tile_vVarId = get_shader_variable_id("mask_tile_v");
  static int detail1_tile_uVarId = get_shader_variable_id("detail1_tile_u");
  static int detail1_tile_vVarId = get_shader_variable_id("detail1_tile_v");
  static int detail2_tile_uVarId = get_shader_variable_id("detail2_tile_u");
  static int detail2_tile_vVarId = get_shader_variable_id("detail2_tile_v");

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
                strcmp(elem.mat->getShaderClassName(), "dynamic_masked_ship") == 0 ||
                strcmp(elem.mat->getShaderClassName(), "dynamic_tank_net_atest") == 0;
  bool isEye = strncmp(elem.mat->getShaderClassName(), "dynamic_eye", 12) == 0;
  bool isSkin =
    strcmp(elem.mat->getShaderClassName(), "dynamic_skin") == 0 || strcmp(elem.mat->getShaderClassName(), "dynamic_skin_morph") == 0;
  bool isSkinFaceMorph = strcmp(elem.mat->getShaderClassName(), "dynamic_skin_morph") == 0;
  bool isHairFaceMorph = strcmp(elem.mat->getShaderClassName(), "dynamic_hair_morph") == 0;
  bool isDynamicSheenCamo = strcmp(elem.mat->getShaderClassName(), "dynamic_sheen_camo") == 0;
  bool isPaintedByMask = strcmp(elem.mat->getShaderClassName(), "dynamic_painted_by_mask") == 0;
  bool isRiLandclass = strncmp(elem.mat->getShaderClassName(), "rendinst_landclass", 18) == 0;
  bool isMonochrome = strcmp(elem.mat->getShaderClassName(), "rendinst_monochrome") == 0 ||
                      strcmp(elem.mat->getShaderClassName(), "dynamic_monochrome") == 0;
  bool isVcolor = strncmp(elem.mat->getShaderClassName(), "rendinst_cliff", 15) == 0 ||
                  strncmp(elem.mat->getShaderClassName(), "rendinst_vcolor_layered", 24) == 0;

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
  elem.mat->getColor4Variable(colorVarId, colorOverride);

  float mask_gamma_startValue = 0.5;
  float mask_gamma_endValue = 2;
  float mask_tile_uValue = 1;
  float mask_tile_vValue = 1;
  float detail1_tile_uValue = 1;
  float detail1_tile_vValue = 1;
  float detail2_tile_uValue = 1;
  float detail2_tile_vValue = 1;
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

    elem.mat->getRealVariable(mask_gamma_startVarId, mask_gamma_startValue);
    elem.mat->getRealVariable(mask_gamma_endVarId, mask_gamma_endValue);
    elem.mat->getRealVariable(mask_tile_uVarId, mask_tile_uValue);
    elem.mat->getRealVariable(mask_tile_vVarId, mask_tile_vValue);
    elem.mat->getRealVariable(detail1_tile_uVarId, detail1_tile_uValue);
    elem.mat->getRealVariable(detail1_tile_vVarId, detail1_tile_vValue);
    elem.mat->getRealVariable(detail2_tile_uVarId, detail2_tile_uValue);
    elem.mat->getRealVariable(detail2_tile_vVarId, detail2_tile_vValue);
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
  if (isRiLandclass)
    mesh_info.isRiLandclass = true;

  // Although vcolored shaders use painting related values, they use it quite differently then regular painted shaders,
  // whilst also missing white_point, and black_point values which cause ugly bugs and side effects on console
  if (!isVcolor && use_paintingValue > 0)
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

    mesh_info.maskGammaStart = mask_gamma_startValue;
    mesh_info.maskGammaEnd = mask_gamma_endValue;
    mesh_info.maskTileU = mask_tile_uValue;
    mesh_info.maskTileV = mask_tile_vValue;
    mesh_info.detail1TileU = detail1_tile_uValue;
    mesh_info.detail1TileV = detail1_tile_vValue;
    mesh_info.detail2TileU = detail2_tile_uValue;
    mesh_info.detail2TileV = detail2_tile_vValue;

    mesh_info.extraTextureId = elem.mat->get_texture(1);
    mesh_info.alphaTextureId = elem.mat->get_texture(3);
    mesh_info.normalTextureId = elem.mat->get_texture(5);

    mesh_info.texcoordOffset = parser.secTexcoordFormat != -1 ? parser.secTexcoordOffset : MeshInfo::invalidOffset;
    mesh_info.texcoordFormat = parser.secTexcoordFormat;
    mesh_info.secTexcoordOffset = parser.thirdTexcoordFormat != -1 ? parser.thirdTexcoordOffset : MeshInfo::invalidOffset;
    // Since we only have 1 texcoord format field in the meta
    G_ASSERT(parser.secTexcoordFormat == parser.thirdTexcoordFormat);
  }

  if (isMaskLayered)
  {
    mesh_info.isLayered = true;
    mesh_info.useAtlas = false;
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

  if (isRiLandclass)
  {
    mesh_info.texcoordFormat = parser.fourthTexcoordFormat;
    mesh_info.texcoordOffset = parser.fourthTexcoordOffset;

    Color4 mappingVal;
    elem.mat->getColor4Variable(texcoord_scale_offsetVarId, mappingVal);
    mesh_info.landclassMapping = Point4::rgba(mappingVal);
    elem.mat->getIntVariable(ri_landclass_indexVarId, mesh_info.riLandclassIndex);
  }

  if (isMonochrome)
  {
    mesh_info.isMonochrome = true;
    elem.mat->getRealVariable(metalnessVarId, mesh_info.monochromeData.x);
    elem.mat->getRealVariable(smoothnessVarId, mesh_info.monochromeData.y);
    elem.mat->getRealVariable(reflectanceVarId, mesh_info.monochromeData.z);
  }

  mesh_info.albedoTextureId = impostor_textures ? impostor_textures->albedo_alpha : elem.mat->get_texture(0);
  if (!isLayered)
    mesh_info.normalTextureId = elem.mat->get_texture(2);
  mesh_info.alphaTest = hasAlphaTest;
  mesh_info.isCamo = isCamo;
  mesh_info.forceNonMetal = isSkin || isTree;
  if (isSkinFaceMorph || isHairFaceMorph)
  {
    const int morphAtlasMatSlotSharedWithAoTex = 3;
    const uint32_t morphUvFormat = isHairFaceMorph ? parser.secTexcoordFormat : parser.texcoordFormat;
    unsigned morphUvSize = 0;
    channel_size(morphUvFormat == BufferProcessor::bvhAttributeShort2TC ? VSDT_SHORT2 : morphUvFormat, morphUvSize);
    mesh_info.faceMorphAtlasTextureId = elem.mat->get_texture(morphAtlasMatSlotSharedWithAoTex);
    mesh_info.faceMorphUvOffset = isHairFaceMorph ? mesh_info.secTexcoordOffset : mesh_info.texcoordOffset;
    mesh_info.faceMorphUvSize = morphUvSize;

    const bool isSkinnedMesh = mesh_info.weightsOffset != MeshInfo::invalidOffset;
    if (!isSkinnedMesh || mesh_info.faceMorphAtlasTextureId == BAD_TEXTUREID || mesh_info.faceMorphUvSize == 0)
    {
      LOGERR_ONCE("BVH: %s has no skin weights, morph atlas or morph UV, so its BVH copy keeps the neutral face",
        elem.mat->getShaderClassName());
      mesh_info.faceMorphAtlasTextureId = BAD_TEXTUREID;
    }
  }
  mesh_info.hasColorMod = isDynamicSheenCamo;
  mesh_info.isPaintedByMask = isPaintedByMask;
  if (isPaintedByMask)
  {
    mesh_info.extraTextureId = elem.mat->get_texture(4);
    mesh_info.secondaryMaskTextureId = elem.mat->get_texture(5);
  }
  real tank_decals_smoothness; // Values are hard coded in shader to default, only checking if variable exists
  mesh_info.hasAnimcharDecals = elem.mat->getRealVariable(tank_decals_smoothnessVarId, tank_decals_smoothness);
}
} // namespace bvh

namespace bind_dascript
{
bool is_bvh_enabled() { return ::is_bvh_enabled(); }
bool is_rr_enabled() { return ::is_rr_enabled(); }
} // namespace bind_dascript
