// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <rendInst/gpuObjects.h>
#include <camera/sceneCam.h>
#include <gamePhys/collision/collisionLib.h>
#include <util/dag_console.h>
#include <ecs/core/entityManager.h>
#include <daECS/core/componentTypes.h>
#include <daECS/core/coreEvents.h>
#include <scene/dag_tiledScene.h>
#include <render/omniLight.h>
#include <gameRes/dag_gameResources.h>
#include <ecs/anim/anim.h>
#include <ecs/anim/animchar_visbits.h>
#include <ecs/phys/collRes.h>
#include <ecs/render/updateStageRender.h>
#include <gameRes/dag_gameResSystem.h>
#include <gameRes/dag_stdGameResId.h>
#include <ioSys/dag_dataBlock.h>
#include <osApiWrappers/dag_cpuJobs.h>
#include <ecs/gameres/commonLoadingJobMgr.h>
#include <nodeBasedShaderManager/nodeBasedShaderManager.h>
#include <billboardDecals/billboardDecals.h>
#include "main/gameObjects.h"
#include "main/level.h"
#include "game/player.h"
#include <util/dag_convar.h>
#include <landMesh/virtualtexture.h>
#include "render/boundsUtils.h"

#include "render/fx/effectEntity.h"
#include "render/renderEvent.h"
#define INSIDE_RENDERER 1
#include "render/world/private_worldRenderer.h"
#include "render/world/parallelOcclusionRasterizer.h"
#include "render/world/worldRendererQueries.h"
#include <ecs/render/updateStageRender.h>
#include <render/indoorProbeManager.h>
#include <render/indoorProbeScenes.h>
#include <shaders/indoor_probes_const.hlsli>

CONSOLE_BOOL_VAL("render", spam_animchars_to_raster, false);
CONSOLE_BOOL_VAL("clipmap", invalidate_under_camera_once, false);
CONSOLE_BOOL_VAL("clipmap", invalidate_under_camera_at_every_frame, false);

ECS_AUTO_REGISTER_COMPONENT(mat44f, "close_geometry_prev_to_curr_frame_transform", nullptr, 0);
ECS_AUTO_REGISTER_COMPONENT(mat44f, "close_geometry_prev_transform", nullptr, 0);

extern const char *const EMPTY_LEVEL_NAME;

static __forceinline WorldRenderer *get_renderer() { return ((WorldRenderer *)get_world_renderer()); }

template <typename Callable>
static void get_fpv_shadow_dist_ecs_query(ecs::EntityId eid, Callable c);

float get_fpv_shadow_dist()
{
  float shadowDist = 0.f;
  get_fpv_shadow_dist_ecs_query(get_cur_cam_entity(),
    [&shadowDist](float fov, bool isTpsView = false, float shadow_cascade0_distance = -1.0f) {
      if (shadow_cascade0_distance > 0.0f)
      {
        shadowDist = shadow_cascade0_distance;
        return;
      }
      // TODO: dynamically calculate isTpsView. Right now isTpsView is more like isTpsCamera, so tps camera will have this component
      // set as true even in aiming mode.
      shadowDist = isTpsView || fov < 36.f ? 0.f : 1.6f;
    });
  return shadowDist;
}

template <typename Callable>
static void get_camera_fov_ecs_query(ecs::EntityId eid, Callable c);

float get_camera_fov()
{
  float FoV = 0.f;
  get_camera_fov_ecs_query(get_cur_cam_entity(), [&FoV](float fov) { FoV = fov; });
  return FoV;
}

template <typename Callable>
static void volfog_optional_graphs_ecs_query(Callable c);

void add_volfog_optional_graphs()
{
  if (!get_renderer())
    return;
  volfog_optional_graphs_ecs_query(
    [](const ecs::string &volfog) { get_renderer()->enableVolumeFogOptionalShader(String(volfog.c_str()), true); });
}

ECS_ON_EVENT(on_appear, on_disappear)
static void add_volfog_optional_graph_es_event_handler(const ecs::Event &evt, const ecs::string &volfog)
{
  if (!get_renderer())
    return;
  bool dest = evt.is<ecs::EventEntityDestroyed>() || evt.is<ecs::EventComponentsDisappear>();
  get_renderer()->enableVolumeFogOptionalShader(String(volfog.c_str()), !dest);
}

struct VolfogShaderPreload
{
  static void requestResources(const char *, const ecs::resource_request_cb_t &res_cb)
  {
    // Note: currently we can't just add resources to level component since it will change level entity
    // creation order in regard of other entities that implicitly depend on level being loaded (e.g. phys actors)
    // hence add manual pre-load job that executes before level load job
    const ecs::string &levelBlkPath = res_cb.get<ecs::string>(ECS_HASH("level__blk"));
    DataBlock levelBlk;
    if (levelBlkPath == EMPTY_LEVEL_NAME || !dblk::load(levelBlk, levelBlkPath.c_str(), dblk::ReadFlag::ROBUST_IN_REL))
      return;
    if (levelBlk.getBool("disableVolfogPreload", false))
      return;
    if (const char *shader_graph = levelBlk.getBlockByNameEx("volFog")->getStr("rootFogGraph", nullptr))
    {
      eastl::string fullName = node_based_shader_get_resource_name(shader_graph);
      if (get_resource_type_id(fullName.c_str()) == LShaderGameResClassId)
      {
        struct LoadNodeBasedShaderJob final : public cpujobs::IJob
        {
          eastl::string resName;
          DataBlock levelBlk;
          LoadNodeBasedShaderJob(eastl::string &&name, DataBlock &&lblk) : resName(eastl::move(name)), levelBlk(eastl::move(lblk)) {}
          void doJob() override
          {
            if (auto res = get_one_game_resource_ex(GAMERES_HANDLE_FROM_STRING(resName.c_str()), LShaderGameResClassId))
              release_game_resource(res);
          }
          void releaseJob() override
          {
            if (auto wr = static_cast<WorldRenderer *>(get_world_renderer()))
              wr->loadFogNodes(levelBlk);
            delete this;
          }
        };
        auto job = new LoadNodeBasedShaderJob(eastl::move(fullName), eastl::move(levelBlk));
        G_VERIFY(cpujobs::add_job(ecs::get_common_loading_job_mgr(), job));
        return;
      }
    }

    // Note: world level might be not yet created since level isn't created either
    static_cast<WorldRenderer *>(create_world_renderer())->loadFogNodes(levelBlk);
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(VolfogShaderPreload);
ECS_REGISTER_RELOCATABLE_TYPE(VolfogShaderPreload, nullptr);
ECS_AUTO_REGISTER_COMPONENT(VolfogShaderPreload, "level__node_based_fog_shader_preload", nullptr, 0);


ECS_TAG(render)
static void move_indoor_light_probes_to_render_es_event_handler(const EventGameObjectsOptimize &, GameObjects &game_objects)
{
  auto indoorProbeShapes = eastl::make_unique<IndoorProbeScenes>();

  auto gatherScenes = [&indoorProbeShapes, &game_objects](const char *sceneName, uint8_t sceneType) {
    game_objects.moveScene(sceneName, [&indoorProbeShapes, &sceneName, &sceneType](GOScenePtr &&s) {
      if (s->getNodesAliveCount() > 32)
        auto_rearrange_game_object_scene(*s, 8, 128);

      indoorProbeShapes->addScene(sceneName, eastl::move(s), sceneType);
    });
  };

  gatherScenes("envi_probe_box", ENVI_PROBE_CUBE_TYPE);
  gatherScenes("envi_probe_capsule", ENVI_PROBE_CAPSULE_TYPE);
  gatherScenes("envi_probe_cylinder", ENVI_PROBE_CYLINDER_TYPE);

  get_renderer()->initIndoorProbeShapes(eastl::move(indoorProbeShapes));
}

template <typename Callable>
ECS_REQUIRE(ecs::auto_type is_occluder)
static void gather_closest_occluders_ecs_query(Callable c);

void make_rasterization_tasks_by_collision(const CollisionResource *coll_res,
  mat44f_cref worldviewproj,
  eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks,
  bool allow_convex);

void gather_animchars_for_occlusion_rasterization(
  mat44f_cref globtm, const vec3f &origin, eastl::vector<ParallelOcclusionRasterizer::RasterizationTaskData> &tasks)
{
  const uint32_t MAX_OBJECTS_TO_RASTERIZE = 2;
  eastl::array<const CollisionResource *, MAX_OBJECTS_TO_RASTERIZE> collisions;
  eastl::array<const TMatrix *, MAX_OBJECTS_TO_RASTERIZE> tms;
  eastl::array<float, MAX_OBJECTS_TO_RASTERIZE> tanAngleQuarts;
  uint32_t objectsToRasterize = 0;
  gather_closest_occluders_ecs_query(
    [&](const CollisionResource &collres, const TMatrix &transform, uint8_t animchar_visbits, const vec4f &animchar_bsph) {
      // We reuse visibility from the previous frame for simplicity. If an animchar was visible on the previous frame,
      // it could be visible on the current frame too.
      if (!(animchar_visbits & (VISFLG_MAIN_VISIBLE | VISFLG_MAIN_AND_SHADOW_VISIBLE)))
        return;
      const float TAN_THRESHOLD = 120.f;
      const float distSq = v_extract_x(v_length3_sq_x(v_sub(origin, animchar_bsph)));
      const float squaredRadius = v_extract_w(v_mul(animchar_bsph, animchar_bsph));
      float tanAngleQuart = squaredRadius * squaredRadius / distSq;
      if (tanAngleQuart < TAN_THRESHOLD)
        return;
      const CollisionResource *collPtr = &collres;
      const TMatrix *tmPtr = &transform;
      for (uint32_t i = 0; i < objectsToRasterize; ++i)
      {
        if (tanAngleQuart > tanAngleQuarts[i])
        {
          eastl::swap(tanAngleQuarts[i], tanAngleQuart);
          eastl::swap(tms[i], tmPtr);
          eastl::swap(collisions[i], collPtr);
        }
      }
      if (objectsToRasterize < MAX_OBJECTS_TO_RASTERIZE)
      {
        tanAngleQuarts[objectsToRasterize] = tanAngleQuart;
        tms[objectsToRasterize] = tmPtr;
        collisions[objectsToRasterize] = collPtr;
        objectsToRasterize++;
      }
    });
  for (uint32_t i = 0; i < objectsToRasterize; ++i)
  {
    TMatrix4_vec4 tmpTrans(*tms[i]);
    mat44f trans = (mat44f &)tmpTrans;
    mat44f worldviewproj;
    v_mat44_mul43(worldviewproj, globtm, trans);
    make_rasterization_tasks_by_collision(collisions[i], worldviewproj, tasks, false);
  }
  if (spam_animchars_to_raster.get())
    debug("%d animchars prepared for SW rasterization.", objectsToRasterize);
}


ECS_TAG(render)
ECS_BEFORE(animchar_before_render_es) // Before to start exec jobs right after p-for jobs of `animchar_before_render_es`
static void start_occlusion_and_sw_raster_es(const UpdateStageInfoBeforeRender &)
{
  if (auto wr = get_renderer())
    wr->startOcclusionAndSwRaster();
}

void invalidate_after_heightmap_change(const BBox3 &box)
{
  if (WorldRenderer *renderer = get_renderer())
  {
    renderer->delayedInvalidateAfterHeightmapChange(box);
    g_entity_mgr->broadcastEventImmediate(InvalidateBoxAfterHeightmapChange(box));
  }
}

void WorldRenderer::delayedInvalidateAfterHeightmapChange(const BBox3 &box)
{
  invalidationBoxesAfterHeightmapChange.push_back(box);
  // Keep 1 spare for v_ldu(&bbox[1].x) to work
  if (invalidationBoxesAfterHeightmapChange.size() == invalidationBoxesAfterHeightmapChange.capacity())
  {
    invalidationBoxesAfterHeightmapChange.push_back_uninitialized();
    invalidationBoxesAfterHeightmapChange.pop_back();
  }
}

void WorldRenderer::invalidateAfterHeightmapChange()
{
  g_entity_mgr->broadcastEventImmediate(AfterHeightmapChange());
  for (const BBox3 &box : invalidationBoxesAfterHeightmapChange)
  {
    shadowsInvalidate(box);
    rendinst::gpuobjects::invalidate_inside_bbox(BBox2::xz(box));
  }
  clear_and_shrink(invalidationBoxesAfterHeightmapChange);
}


bool can_change_altitude_unexpectedly()
{
  QueryUnexpectedAltitudeChange event;
  g_entity_mgr->broadcastEventImmediate(event);
  return event.enabled;
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag customSkyRenderer)
static ecs::QueryCbResult find_custom_sky_renderer_ecs_query(Callable c);

bool has_custom_sky_render(CustomSkyRequest request)
{
  return find_custom_sky_renderer_ecs_query([request](ecs::EntityId eid) {
    if (request == CustomSkyRequest::RENDER_IF_EXISTS)
      g_entity_mgr->sendEventImmediate(eid, CustomSkyRender());
    return ecs::QueryCbResult::Stop;
  }) == ecs::QueryCbResult::Stop;
}

template <typename Callable>
static void get_envi_probe_render_flags_ecs_query(Callable c);

uint32_t WorldRenderer::getEnviProbeRenderFlags() const
{
  uint32_t flags = 0;
  get_envi_probe_render_flags_ecs_query([&flags](bool envi_probe_use_geometry, bool envi_probe_sun_enabled) {
    flags = (envi_probe_use_geometry ? ENVI_PROBE_USE_GEOMETRY : 0) | (envi_probe_sun_enabled ? ENVI_PROBE_SUN_ENABLED : 0);
  });
  return flags;
}

template <typename Callable>
ECS_REQUIRE(ecs::Tag customEnviProbeTag)
static ecs::QueryCbResult find_custom_envi_probe_renderer_ecs_query(Callable c);

bool render_custom_envi_probe(const ManagedTex *cubeTarget, int face_num)
{
  return find_custom_envi_probe_renderer_ecs_query([&](ecs::EntityId eid) {
    g_entity_mgr->sendEventImmediate(eid, CustomEnviProbeRender(cubeTarget, face_num));
    return ecs::QueryCbResult::Stop;
  }) == ecs::QueryCbResult::Stop;
}

eastl::vector<Color4> get_custom_envi_probe_spherical_hamornics()
{
  CustomEnviProbeGetSphericalHarmonics evt({});
  g_entity_mgr->broadcastEventImmediate(evt);
  return evt.get<0>();
}

void log_custom_envi_probe_spherical_harmonics(const Color4 *result)
{
  g_entity_mgr->broadcastEventImmediate(CustomEnviProbeLogSphericalHarmonics(result));
}

ECS_BROADCAST_EVENT_TYPE(InvalidateClipmap);
ECS_REGISTER_EVENT(InvalidateClipmap);

static void invalidate_clipmap_es(const InvalidateClipmap &)
{
  if (const WorldRenderer *renderer = get_renderer())
  {
    if (Clipmap *clipmap = renderer->getClipmap())
      clipmap->invalidate(true /*force redraw*/);
  }
}

ECS_REGISTER_EVENT(InvalidateClipmapBox);
static void invalidate_clipmap_box_es(const InvalidateClipmapBox &evt)
{
  if (const WorldRenderer *renderer = get_renderer())
  {
    if (Clipmap *clipmap = renderer->getClipmap())
      clipmap->invalidateBox(evt.get<0>());
  }
}


ECS_NO_ORDER
ECS_TAG(dev, render)
static void invalidate_clipmap_under_camera_es(const ecs::UpdateStageInfoAct &)
{
  if (!invalidate_under_camera_at_every_frame && !invalidate_under_camera_once)
    return;

  invalidate_under_camera_once.set(false);

  if (const WorldRenderer *renderer = get_renderer())
  {
    if (Clipmap *clipmap = renderer->getClipmap())
    {
      const Point3 &camPos = get_cam_itm().getcol(3);
      clipmap->invalidatePoint(Point2(camPos.x, camPos.z));
    }
  }
}

ECS_TAG(render)
ECS_NO_ORDER
static void reset_occlusion_data_es(const UpdateStageInfoBeforeRender &,
  float &close_geometry_bounding_radius,
  mat44f &close_geometry_prev_to_curr_frame_transform,
  bool &occlusion_available)
{
  close_geometry_bounding_radius = 0.0f;
  v_mat44_ident(close_geometry_prev_to_curr_frame_transform);
  occlusion_available = false;
}

template <typename T>
static inline void gather_occlusion_data_ecs_query(T cb);
OcclusionData gather_occlusion_data()
{
  OcclusionData occlusionData{};
  gather_occlusion_data_ecs_query([&](const mat44f &close_geometry_prev_to_curr_frame_transform,
                                    const float &close_geometry_bounding_radius, const bool &occlusion_available) {
    occlusionData.closeGeometryPrevToCurrFrameTransform = close_geometry_prev_to_curr_frame_transform;
    occlusionData.closeGeometryBoundingRadius = close_geometry_bounding_radius;
    occlusionData.occlusionAvailable = occlusion_available;
  });
  return occlusionData;
}

template <typename T>
static inline void gather_underground_zones_ecs_query(T cb);

void get_underground_zones_data(Tab<Point3_vec4> &bboxes)
{
  gather_underground_zones_ecs_query([&](ECS_REQUIRE(ecs::Tag underground_zone) const TMatrix &transform) {
    BBox3 aabb = get_containing_box(transform);

    bboxes.emplace_back(aabb.boxMin());
    bboxes.emplace_back(aabb.boxMax());
  });
}

int get_underground_zones_count()
{
  int result = 0;
  gather_underground_zones_ecs_query([&](ECS_REQUIRE(ecs::Tag underground_zone) const TMatrix &transform) {
    G_UNUSED(transform);
    result++;
  });
  return result;
}

template <typename Callable>
static void needs_water_heightmap_ecs_query(Callable c);
bool needs_water_heightmap(Color4 underwaterFade)
{
  bool needsWaterHeightmap = (underwaterFade.r != 0) || (underwaterFade.g != 0) || (underwaterFade.b != 0);
  needs_water_heightmap_ecs_query(
    [&needsWaterHeightmap](bool needs_water_heightmap) { needsWaterHeightmap = needsWaterHeightmap || needs_water_heightmap; });
  return needsWaterHeightmap;
}

template <typename Callable>
static void use_foam_tex_ecs_query(Callable c);
bool use_foam_tex()
{
  bool useFoamTex = false;
  use_foam_tex_ecs_query([&useFoamTex](bool use_foam_tex) { useFoamTex = useFoamTex || use_foam_tex; });
  return useFoamTex;
}


template <typename Callable>
static void use_wfx_textures_ecs_query(Callable c);
bool use_wfx_textures()
{
  bool useWfxTextures = false;
  use_wfx_textures_ecs_query(
    [&useWfxTextures](bool should_use_wfx_textures) { useWfxTextures = useWfxTextures || should_use_wfx_textures; });
  return useWfxTextures;
}

template <typename T>
static inline void gather_is_camera_inside_custom_gi_zones_ecs_query(T cb);
template <typename T>
static inline void gather_custom_gi_zones_bbox_ecs_query(T cb);

bool is_camera_inside_custom_gi_zone()
{
  bool isCameraInside = false;
  gather_is_camera_inside_custom_gi_zones_ecs_query([&](ECS_REQUIRE(ecs::Tag custom_gi_zone) const bool is_camera_inside_box) {
    isCameraInside = isCameraInside || is_camera_inside_box;
  });
  return isCameraInside;
}

BBox3 get_custom_gi_zone_bbox()
{
  BBox3 zoneBBox;
  gather_custom_gi_zones_bbox_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag custom_gi_zone) const TMatrix &transform, const bool is_camera_inside_box) {
      if (is_camera_inside_box)
        zoneBBox = transform * BBox3(Point3(0, 0, 0), 1.0f);
    });
  return zoneBBox;
}