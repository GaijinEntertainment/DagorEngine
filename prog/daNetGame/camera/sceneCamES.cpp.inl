// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "sceneCam.h"
#include <math/dag_TMatrix.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/utility/ecsRecreate.h>
#include <math/dag_mathUtils.h> // check_nan
#include "game/team.h"
#include "game/dasEvents.h"
#include "game/gameEvents.h"
#include "main/ecsUtils.h"
#include "net/net.h"

#include <quirrel/bindQuirrelEx/autoBind.h>
#include <ecs/scripts/sqEntity.h>
#include <daECS/core/coreEvents.h>
#include <daInput/input_api.h>

#include <startup/dag_globalSettings.h>
#include <ioSys/dag_dataBlock.h>

static ecs::EntityId sphCam = ecs::INVALID_ENTITY_ID, freeCam = ecs::INVALID_ENTITY_ID;
static ecs::EntityId curCam = ecs::INVALID_ENTITY_ID, lastCam = ecs::INVALID_ENTITY_ID;
static ecs::EntityId spectateCam = ecs::INVALID_ENTITY_ID;

bool create_free_camera()
{
  const DataBlock *freeCamSettings = ::dgs_get_game_params()->getBlockByNameEx("free_cam_settings");
  if (!freeCamSettings->getBool("hasFreeCamera", true))
    return false;
  G_ASSERT(freeCam == ecs::INVALID_ENTITY_ID);
  ecs::ComponentsInitializer amap;
  ECS_INIT(amap, transform, TMatrix::IDENT);
  ECS_INIT(amap, camera__active, false);
  ECS_INIT(amap, fov, 90.f);
  freeCam = create_simple_entity(freeCamSettings->getStr("template_name", "free_cam"), eastl::move(amap));
  G_ASSERT_RETURN(freeCam != ecs::INVALID_ENTITY_ID, false);
  return true;
}

template <typename Callable>
inline void vehicle_ecs_query(ecs::EntityId, Callable c);
template <typename Callable>
inline void player_view_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
inline void player_camera_ecs_query(ecs::EntityId, Callable c);

template <typename Callable>
inline void actor_ecs_query(ecs::EntityId, Callable c);

const TMatrix &get_cam_itm()
{
  const TMatrix &tm = ECS_GET_OR(curCam, transform, TMatrix::IDENT);
  G_ASSERTF(tm.getcol(3).lengthSq() < 1e10f, "Bad camera %d(%s) pos (%f,%f,%f)", (ecs::entity_id_t)curCam,
    g_entity_mgr->getEntityTemplateName(curCam), P3D(tm.getcol(3)));
  return tm;
}

template <typename Callable>
inline void set_camera_active_flag_ecs_query(ecs::EntityId, Callable c);

static void set_camera_flag(ecs::EntityId cam_eid, bool flag)
{
  set_camera_active_flag_ecs_query(cam_eid, [&](bool &camera__active, bool *camera__input_enabled) {
    camera__active = flag;
    if (camera__input_enabled)
      *camera__input_enabled = flag;
  });
}

static void switch_cam(ecs::EntityId active_cam, bool take_transform)
{
  if (freeCam == ecs::INVALID_ENTITY_ID)
    if (!create_free_camera())
      return;

  TMatrix itm = TMatrix::IDENT;

  if (active_cam != ecs::INVALID_ENTITY_ID)
  {
    set_camera_flag(active_cam, false);
    itm = get_cam_itm();
    G_ASSERT(!check_nan(itm));
  }

  ecs::EntityId nextCam = ecs::INVALID_ENTITY_ID;

  if (active_cam == freeCam) // turning off freecam, toggle last active camera
  {
    nextCam = lastCam;
    dainput::activate_action_set(dainput::get_action_set_handle("Camera"), false);
  }
  else
  {
    lastCam = active_cam;
    nextCam = freeCam;
    dainput::activate_action_set(dainput::get_action_set_handle("Camera"));
  }

  if (nextCam != ecs::INVALID_ENTITY_ID)
  {
    if (nextCam == freeCam && take_transform)
      g_entity_mgr->set(nextCam, ECS_HASH("transform"), itm);
    set_camera_flag(nextCam, true);
  }
  curCam = nextCam;
  debug("scene camera switched to entity %d<%s>", (ecs::entity_id_t)curCam, g_entity_mgr->getEntityTemplateName(curCam));
}

template <typename Callable>
static inline void switch_active_camera_ecs_query(Callable c);

static ecs::EntityId get_active_cam()
{
  ecs::EntityId activeCam = ecs::INVALID_ENTITY_ID;
  switch_active_camera_ecs_query([&](ecs::EntityId eid ECS_REQUIRE(eastl::true_type camera__active)) {
    activeCam = eid;
    debug("active camera is %d<%s>", (ecs::entity_id_t)activeCam, g_entity_mgr->getEntityTemplateName(activeCam));
  });
  return activeCam;
}

void toggle_free_camera() { switch_cam(get_active_cam(), true); }

void enable_free_camera()
{
  if (curCam != freeCam || freeCam == ecs::INVALID_ENTITY_ID)
    toggle_free_camera();
}

void disable_free_camera()
{
  if (freeCam == ecs::INVALID_ENTITY_ID)
    return;
  if (curCam == freeCam)
    toggle_free_camera();
}

void force_free_camera()
{
  if (curCam != freeCam || freeCam == ecs::INVALID_ENTITY_ID)
    switch_cam(get_active_cam(), false);
}

static void enable_spectate_cam(const TMatrix &itm, int team, ecs::EntityId target)
{
  if (curCam)
    g_entity_mgr->set(curCam, ECS_HASH("camera__active"), false);
  ecs::ComponentsInitializer cinit;
  ECS_INIT(cinit, camera__active, true);
  ECS_INIT(cinit, transform, itm);
  ECS_INIT(cinit, team, team);
  ECS_INIT(cinit, spectator__target, target);
  // Note: sync entity creation currently isn't properly destroyed when it called from within EntityManager::clear
  // (hence we use async here)
  curCam = spectateCam = g_entity_mgr->createEntityAsync("spectator_camera", eastl::move(cinit));
  G_ASSERT(curCam);
}

ecs::EntityId enable_spectator_camera(const TMatrix &itm, int team, ecs::EntityId target)
{
  if (!spectateCam || !g_entity_mgr->doesEntityExist(spectateCam))
    enable_spectate_cam(itm, team, target);
  return spectateCam;
}

bool is_free_camera_enabled() { return freeCam != ecs::INVALID_ENTITY_ID && curCam == freeCam; }

void set_cam_itm(const TMatrix &itm) { g_entity_mgr->set(curCam, ECS_HASH("transform"), itm); }

ecs::EntityId set_scene_camera_entity(ecs::EntityId cam_eid)
{
  ecs::EntityId oldCam = curCam;

  G_ASSERT(cam_eid != ecs::INVALID_ENTITY_ID);

  if (g_entity_mgr->doesEntityExist(curCam))
    g_entity_mgr->set(curCam, ECS_HASH("camera__active"), false);

  curCam = sphCam = cam_eid;

  if (g_entity_mgr->doesEntityExist(curCam))
    g_entity_mgr->set(curCam, ECS_HASH("camera__active"), true);

  return oldCam;
}

ecs::EntityId get_cur_cam_entity() { return curCam; }

void reset_all_cameras()
{
  freeCam = ecs::INVALID_ENTITY_ID;
  curCam = ecs::INVALID_ENTITY_ID;
  sphCam = ecs::INVALID_ENTITY_ID;
  spectateCam = ecs::INVALID_ENTITY_ID;
}


SQ_DEF_AUTO_BINDING_MODULE(bind_scene_camera, "camera")
{
  Sqrat::Table tbl(vm);
  tbl //
    .Func("get_cur_cam_entity", get_cur_cam_entity)
    .Func("set_scene_camera_entity", set_scene_camera_entity)
    /**/;
  return tbl;
}

static inline void setup_camera_accurate_pos(ecs::EntityId eid, const TMatrix *transform, DPoint3 *camera__accuratePos)
{
  const Point3 &campos = (transform ? transform : &TMatrix::IDENT)->getcol(3);
  if (camera__accuratePos)
    *camera__accuratePos = dpoint3(campos);
  debug("set camera %d<%s> accurate pos to (%f,%f,%f)", (ecs::entity_id_t)eid, g_entity_mgr->getEntityTemplateName(eid), P3D(campos));
}

ECS_ON_EVENT(on_appear)
static void scene_cam_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, bool camera__active, const TMatrix *transform, DPoint3 *camera__accuratePos)
{
  setup_camera_accurate_pos(eid, transform, camera__accuratePos);
  if (camera__active)
    curCam = eid;
}

ECS_TRACK(camera__active)
static void scene_cam_activate_es_event_handler(
  const ecs::Event &, ecs::EntityId eid, const TMatrix *transform, bool camera__active, DPoint3 *camera__accuratePos)
{
  if (camera__active)
  {
    setup_camera_accurate_pos(eid, transform, camera__accuratePos);
    curCam = eid;
  }
}
