// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <daECS/core/coreEvents.h>
#include <ecs/input/message.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <util/dag_flyMode.h>
#include <EASTL/unique_ptr.h>
#include <stddef.h>
#include <math/dag_mathUtils.h>
#include <ecs/render/updateStageRender.h>

#define CAM_ATTR(x) ECS_HASH("free_cam__" #x)

struct FreeCam
{
  FreeCam(const ecs::EntityManager &mgr, ecs::EntityId eid);

  eastl::unique_ptr<FlyModeBlackBox> flyMode;

  void update(float dt, TMatrix &transform, const Point2 &mouse)
  {
    TMatrix tempItm;
    flyMode->getInvViewMatrix(tempItm);
    if (tempItm != transform)
      flyMode->setViewMatrix(transform);
    flyMode->integrate(dt, mouse.x, mouse.y);
    flyMode->getInvViewMatrix(transform);
  }
};


FreeCam::FreeCam(const ecs::EntityManager &mgr, ecs::EntityId eid) : flyMode(create_flymode_bb())
{
  // all these components are needed only in ES
  // bad implementation
#define GET_ATTR(x, def) float x##_attr = mgr.getOr(eid, CAM_ATTR(x), def)
  GET_ATTR(move_inertia, 0.95f);
  GET_ATTR(stop_inertia, 0.1f);
  GET_ATTR(ang_stop_inertia, 0.15f);
  GET_ATTR(ang_inertia, 0.5f);
#undef GET_ATTR

  flyMode.reset(create_flymode_bb());

  flyMode->moveInertia = move_inertia_attr;
  flyMode->stopInertia = stop_inertia_attr;
  flyMode->angStopInertiaH = flyMode->angStopInertiaV = ang_stop_inertia_attr;
  flyMode->angInertiaH = flyMode->angInertiaV = ang_inertia_attr;

  flyMode->setViewMatrix(mgr.getOr(eid, ECS_HASH("transform"), TMatrix::IDENT));
}


ECS_DECLARE_RELOCATABLE_TYPE(FreeCam);
ECS_REGISTER_RELOCATABLE_TYPE(FreeCam, nullptr);
ECS_AUTO_REGISTER_COMPONENT(FreeCam, "free_cam", nullptr, 0);
ECS_DEF_PULL_VAR(free_cam);

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
ECS_REQUIRE(eastl::true_type camera__active)
static inline void free_cam_es(const UpdateStageInfoBeforeRender &update_info, FreeCam &free_cam, TMatrix &transform,
  Point2 &free_cam__mouse, Point2 &free_cam__move, Point2 &free_cam__rotate, float free_cam__shiftY, bool free_cam__turbo,
  float free_cam__bank = 0.f)
{
  free_cam.flyMode->keys.left = free_cam__move.x < 0;
  free_cam.flyMode->keys.right = free_cam__move.x > 0;
  free_cam.flyMode->keys.fwd = free_cam__move.y > 0;
  free_cam.flyMode->keys.back = free_cam__move.y < 0;
  free_cam.flyMode->keys.worldUp = free_cam__shiftY > 0;
  free_cam.flyMode->keys.worldDown = free_cam__shiftY < 0;
  free_cam.flyMode->keys.turbo = free_cam__turbo;
  free_cam.flyMode->keys.bankLeft = free_cam__bank < 0;
  free_cam.flyMode->keys.bankRight = free_cam__bank > 0;
  free_cam__mouse.x += free_cam__rotate.x;
  free_cam__mouse.y -= free_cam__rotate.y;
  free_cam.update(update_info.actDt, transform, free_cam__mouse);
  free_cam__mouse.zero();
}

ECS_TAG(render)
ECS_TRACK(free_cam__move_speed, free_cam__angSpdScale)
ECS_ON_EVENT(on_appear)
ECS_BEFORE(free_cam_es)
static void free_cam_update_params_es_event_handler(const ecs::Event &, FreeCam &free_cam, Point2 free_cam__angSpdScale,
  float free_cam__move_speed = 5.f, float free_cam__turbo_scale = 20.f)
{
  free_cam.flyMode->moveSpd = free_cam__move_speed;
  free_cam.flyMode->turboSpd = free_cam.flyMode->moveSpd * free_cam__turbo_scale;
  free_cam.flyMode->angSpdScaleH = free_cam__angSpdScale.x;
  free_cam.flyMode->angSpdScaleV = free_cam__angSpdScale.y;
}

template <typename Callable>
void get_free_cam_speeds_ecs_query(Callable c);

Point3 get_free_cam_speeds()
{
  float defaultSpeed = 5, currentSpeed = 5, turboScale = 20;
  get_free_cam_speeds_ecs_query([&](FreeCam &free_cam, float free_cam__move_speed = 5.f, float free_cam__turbo_scale = 20.f) {
    defaultSpeed = free_cam__move_speed;
    currentSpeed = free_cam.flyMode->moveSpd;
    turboScale = free_cam__turbo_scale;
  });
  return Point3(defaultSpeed, currentSpeed, turboScale);
}
