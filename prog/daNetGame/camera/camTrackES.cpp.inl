// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "camera/camTrack.h"
#include "camera/sceneCam.h"
#include "net/time.h"

#include <camTrack/camTrack.h>
#include <generic/dag_tabUtils.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

struct CamTrack
{
  camtrack::handle_t trackHandle;

  CamTrack(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    trackHandle = camtrack::load_track(mgr.getOr(eid, ECS_HASH("camtrack__filename"), ecs::nullstr));
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(CamTrack);
ECS_REGISTER_RELOCATABLE_TYPE(CamTrack, nullptr);
ECS_AUTO_REGISTER_COMPONENT(CamTrack, "camtrack", nullptr, 0);

ECS_AFTER(camtrack_executor_es)
static void camtrack_updater_es(const ecs::UpdateStageInfoAct &info, const CamTrack &camtrack, TMatrix &transform, float &fov)
{
  if (camtrack.trackHandle == camtrack::INVALID_HANDLE)
    return;

  float fovTo = fov;
  camtrack::get_track(camtrack.trackHandle, info.curTime, transform, fovTo);
  fov = fovTo;
}

static ecs::EntityId camtrack_eid = ecs::INVALID_ENTITY_ID, camtrack_prev_cam = ecs::INVALID_ENTITY_ID;

static void clear_cur_camtrack()
{
  if (camtrack_prev_cam != ecs::INVALID_ENTITY_ID)
  {
    set_scene_camera_entity(camtrack_prev_cam); // restore prev cam
    camtrack_prev_cam = ecs::INVALID_ENTITY_ID;
  }
  if (camtrack_eid != ecs::INVALID_ENTITY_ID)
    g_entity_mgr->destroyEntity(camtrack_eid);
}

void camtrack::stop() { clear_cur_camtrack(); }

void camtrack::play(const char *filename)
{
  clear_cur_camtrack();
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, camtrack__filename, ecs::string(filename));
  camtrack_eid = g_entity_mgr->createEntitySync("camtrack", eastl::move(attrs));
  camtrack_prev_cam = set_scene_camera_entity(camtrack_eid);
}
