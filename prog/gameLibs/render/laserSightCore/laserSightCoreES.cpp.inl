// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/coreEvents.h>

#include <ecs/render/updateStageRender.h>
#include <ecs/render/decalsES.h>
#include <daECS/delayedAct/actInThread.h>
#include <ecs/render/resPtr.h>
#include <daECS/core/entityId.h>

#include <math/dag_vecMathCompatibility.h>
#include <util/dag_convar.h>

#include <3d/dag_render.h>
#include <3d/dag_lockSbuffer.h>
#include <render/beamTracer/beamTracerClient.h>
#include <shaders/dag_dynSceneRes.h>
#include <shaders/dag_DynamicShaderHelper.h>

#include "../../prog/gameLibs/render/shaders/laser_screen_spot_inc.hlsli"
#include "render/laserSight/laserSightManager.h"

ECS_REGISTER_RELOCATABLE_TYPE(LaserDecalManager, nullptr)

CONSOLE_BOOL_VAL("render", player_laser_always_visible, true);

namespace var
{
static ShaderVariableInfo laser_decal_reproj_max_correction("laser_decal_reproj_max_correction", true);
}

template <typename Callable>
static void get_laser_manager_ecs_query(ecs::EntityManager &manager, Callable c);

template <typename Callable>
inline void gather_laser_spots_ecs_query(ecs::EntityManager &manager, Callable c);

LaserDecalData::LaserDecalData(uint32_t decal_id, float rad, bool is_from_fps_view, const Point3 &normal, const Point3 &right,
  const Point3 &pos, const Point4 &color, const Point3 &origin_pos) :
  DecalDataBase(Point4::xyzV(pos, rad), normalize(normal), decal_id),
  right(normalize(right)),
  is_from_fps_view(is_from_fps_view),
  color(color),
  pad2(0),
  origin_pos(origin_pos)
{}

LaserDecalData::LaserDecalData() :
  DecalDataBase(), right(Point3::ZERO), is_from_fps_view(0), color(Point4::ZERO), pad2(0), origin_pos(Point3::ZERO)
{}

LaserDecalData::LaserDecalData(uint32_t id) :
  DecalDataBase(Point4::ZERO, Point3::ZERO, id),
  right(Point3::ZERO),
  is_from_fps_view(0),
  color(Point4::ZERO),
  pad2(0),
  origin_pos(Point3::ZERO)
{}

static int create_decal(ecs::EntityManager &manager)
{
  int decalId = -1;
  get_laser_manager_ecs_query(manager, [&](LaserDecalManager &laser_decals_manager) {
    LaserDecalData decalData(0, 1.f, false, Point3::ZERO, Point3::ZERO, Point3::ZERO, Point4::ZERO, Point3::ZERO);
    decalId = laser_decals_manager.addDecal(decalData);
  });
  return decalId;
}


static void update_decal(ecs::EntityManager &manager, uint32_t id, const Point3 &decal_pos, const Point3 &decal_dir,
  const Point4 &color, const Point3 &origin_pos, bool is_from_fps_view, float decal_size, float decal_bbox_depth)
{
  get_laser_manager_ecs_query(manager, [&](LaserDecalManager &laser_decals_manager) {
    Point3 pointToEye = decal_dir;
    Point3 norm = -normalize(pointToEye);
    Point3 right = normalize(Point3(-norm.z, 0, norm.x));
    TMatrix transform;
    transform.setcol(1, norm);
    transform.setcol(2, right);
    transform.setcol(3, decal_pos);

    laser_decals_manager.updateDecal(LaserDecalData{id, decal_size, is_from_fps_view, norm, right, decal_pos, color, origin_pos});
  });
}

static void destroy_decal(ecs::EntityManager &manager, uint32_t id)
{
  get_laser_manager_ecs_query(manager,
    [id](LaserDecalManager &laser_decals_manager) { laser_decals_manager.removeDecal(LaserDecalData{id}); });
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void create_laser_screen_spot_buffer_es(const ecs::Event &, UniqueBufHolder &laser_screen_spot_buffer,
  bool laser_decal_manager__is_compatible)
{
  if (!laser_decal_manager__is_compatible)
    return;

  laser_screen_spot_buffer = dag::buffers::create_one_frame_cb(
    dag::buffers::cb_array_reg_count<LaserScreenSpot>(MAX_LASER_SCREEN_SPOTS_COUNT), "laser_screen_spots");
}

ECS_TAG(render)
ECS_AFTER(animchar_before_render_es)
static void update_laser_spots_buffer_es(const UpdateStageInfoBeforeRender &evt, ecs::EntityManager &manager,
  UniqueBufHolder &laser_screen_spot_buffer, int &laser_screen_spot_count, bool laser_decal_manager__is_compatible)
{
  if (!laser_decal_manager__is_compatible)
    return;

  G_ASSERT_RETURN(laser_screen_spot_buffer, );
  dag::Vector<LaserScreenSpot, framemem_allocator> lasersData;
  lasersData.reserve(MAX_LASER_SCREEN_SPOTS_COUNT);
  gather_laser_spots_ecs_query(manager,
    [&](bool laserActive, bool laserAvailable, bool laserVisible, const Point3 &laser_data__fxPos, const Point3 &laser_data__fxDir,
      const float laser_data__intesity, const float &laser_data__crownSize, const Point3 &laserBeamColor) {
      if (!laserActive || !laserAvailable || !laserVisible)
        return;
      Point3 cameraDir = evt.viewItm.getcol(2);
      if (dot(laser_data__fxDir, cameraDir) > 0)
        return;
      if (!evt.mainCullingFrustum.testSphereB(laser_data__fxPos, 1.0))
        return;
      if ((laser_data__intesity < 0.0001) || (laser_data__crownSize < 0.01))
        return;

      lasersData.push_back();
      lasersData.back().position = laser_data__fxPos;
      lasersData.back().color = laserBeamColor;
      lasersData.back().intensity = laser_data__intesity;
      lasersData.back().crownSize = laser_data__crownSize;
    });

  laser_screen_spot_count = lasersData.size();
  if (laser_screen_spot_count == 0)
    return;
  laser_screen_spot_buffer.getBuf()->updateData(0, laser_screen_spot_count * sizeof(LaserScreenSpot), lasersData.data(),
    VBLOCK_WRITEONLY | VBLOCK_DISCARD);
}

ECS_TAG(render)
ECS_ON_EVENT(on_appear)
static void laser_decals_created_es_event_handler(const ecs::Event &, LaserDecalManager &laser_decals_manager,
  int laser_decals_initial_count, const ecs::string &laser_decals_generator_shader, const ecs::string &laser_decals_render_shader,
  const ecs::string &laser_point_decal_culler_shader, const ecs::string &laser_decal_init_shader,
  bool laser_decal_manager__is_compatible)
{
  if (!laser_decal_manager__is_compatible)
    return;
  // todo add framegraph node.
  laser_decals_manager.init(laser_decals_generator_shader.c_str(), laser_decals_render_shader.c_str(),
    laser_point_decal_culler_shader.c_str(), laser_decal_init_shader.c_str());
  laser_decals_manager.init_buffer(laser_decals_initial_count);
}

ECS_TAG(render)
static void laser_decals_prepare_render_es(const UpdateStageInfoBeforeRender &stg, LaserDecalManager &laser_decals_manager,
  const bool laser_decal_manager__is_compatible)
{
  if (!laser_decal_manager__is_compatible)
    return;

  laser_decals_manager.prepareRender(stg.mainCullingFrustum);
}


void disable_laser(ecs::EntityManager &manager, int &beam_tracer_id, int &decal_id)
{
  if (beam_tracer_id >= 0)
  {
    render::beam_tracer::remove_beam_tracer(beam_tracer_id);
    beam_tracer_id = -1;
  }
  if (decal_id >= 0)
  {
    destroy_decal(manager, decal_id);
    decal_id = -1;
  }
}

ECS_TAG(render)
ECS_ON_EVENT(on_disappear)
static void disable_laser_on_death_es(const ecs::Event &, ecs::EntityManager &manager, int &laserBeamTracerId, int &laserDecalId)
{
  disable_laser(manager, laserBeamTracerId, laserDecalId);
}

ECS_TAG(render)
ECS_TRACK(laserActive)
ECS_TRACK(laserAvailable)
ECS_TRACK(laserVisible)
static void disable_laser_es(const ecs::Event &, ecs::EntityManager &manager, bool laserActive, bool laserAvailable, bool laserVisible,
  int &laserBeamTracerId, int &laserDecalId)
{
  if (!laserActive || !laserAvailable || !laserVisible)
    disable_laser(manager, laserBeamTracerId, laserDecalId);
}


ECS_TAG(render)
ECS_NO_ORDER
static void update_lasers_es(const ParallelUpdateFrameDelayed &, ecs::EntityManager &manager, int &laserBeamTracerId,
  int &laserDecalId, const Point3 &laserBeamColor, float laserBeamMaxLength, float laserStartSize, float laserMaxSize,
  float laserMaxIntensity, float laserScrollingSpeed, bool laserActive, bool laserAvailable, bool laserVisible,
  const Point3 &laser_data__rayHit, const Point3 &laser_data__fxPos, const Point3 &laser_data__fxDir,
  const float &laser_data__laserLen, const ecs::EntityId &laser_data__gunOwner, const ecs::EntityId &laser_data__playerId,
  bool laser_sight__is_compatible, const float laser_data__dotIntensity, const Point3 &laserBeamDotColor)
{
  if (!laser_sight__is_compatible)
    return;

  if (!laserActive || !laserAvailable || !laserVisible)
    return;
  TIME_D3D_PROFILE(laser_sight_update)

  const float laserPointSize = laserStartSize + (laser_data__laserLen / laserBeamMaxLength) * (laserMaxSize - laserStartSize);

  if (laserBeamTracerId < 0)
  {
    Color4 laserColor = Color4(laserBeamColor.x, laserBeamColor.y, laserBeamColor.z, 1);
    float isHeroLaser = manager.has(laser_data__playerId, ECS_HASH("hero")) ? 1.f : 0.f;
    laserBeamTracerId = render::beam_tracer::create_beam_tracer(laser_data__fxPos, laser_data__fxDir, laserMaxSize, laserColor,
      reinterpret_cast<const Color3 &>(laserBeamColor), laserMaxIntensity, 1, 1e9, 0.5, 0.9, 1, laserScrollingSpeed, true,
      isHeroLaser);
  }
  if (laserBeamTracerId >= 0)
  {
    render::beam_tracer::update_beam_tracer_pos(laserBeamTracerId, laser_data__rayHit, laser_data__fxPos);
  }
  if (laserDecalId == -1)
  {
    laserDecalId = create_decal(manager);
  }
  if (laserDecalId != -1)
  {
    bool isFPSView = laser_data__gunOwner == laser_data__playerId && player_laser_always_visible;
    const Point4 decalColor = Point4::xyzV(laserBeamDotColor, laser_data__dotIntensity);

    update_decal(manager, laserDecalId, laser_data__rayHit, laser_data__fxDir, decalColor, laser_data__fxPos, isFPSView,
      laserPointSize, laserPointSize);
  }
}