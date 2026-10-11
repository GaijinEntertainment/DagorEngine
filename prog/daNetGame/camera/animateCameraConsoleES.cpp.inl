// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <drv/3d/dag_decl.h>
#include <3d/dag_render.h>
#include <ioSys/dag_dataBlock.h>
#include <math/dag_check_nan.h>
#include <math/dag_TMatrix.h>
#include <osApiWrappers/dag_clipboard.h>
#include <osApiWrappers/dag_direct.h>
#include <perfMon/dag_cpuFreq.h>
#include <render/cameraPositions.h>
#include <util/dag_console.h>
#include <util/dag_delayedAction.h>
#include <util/dag_string.h>
#include <daECS/core/coreEvents.h>
#include <daECS/core/entityManager.h>
#include <daECS/core/entitySystem.h>
#include <daECS/core/componentTypes.h>
#include "camera/sceneCam.h"
#include "game/player.h"

using namespace console;

ECS_ON_EVENT(on_appear)
static void camera_animator_es_event_handler(const ecs::Event &, TMatrix &camera_animator__prev_transform)
{
  camera_animator__prev_transform = get_cam_itm();
}

ECS_NO_ORDER
static void camera_animator_update_es(const ecs::UpdateStageInfoAct &info,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const Point3 &camera_animator__origo,
  float camera_animator__speed,
  float &camera_animator__angle,
  TMatrix &camera_animator__prev_transform)
{
  const TMatrix camTm = get_cam_itm();
  if (camTm != camera_animator__prev_transform || camera_animator__angle <= 0)
  {
    manager.destroyEntity(eid);
    return;
  }
  float angle = camera_animator__speed * info.dt;
  camera_animator__angle -= angle;
  TMatrix T;
  T.identity();
  T.setcol(3, -camera_animator__origo);
  TMatrix invT;
  invT.identity();
  invT.setcol(3, camera_animator__origo);
  TMatrix rot = rotyTM(angle);
  TMatrix mat = invT * rot * T;
  camera_animator__prev_transform = mat * camTm;
  set_cam_itm(camera_animator__prev_transform);
}

ECS_NO_ORDER
static void camera_path_animator_update_es(const ecs::UpdateStageInfoAct &info,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const ecs::Array &camera_path_animator__transforms,
  float camera_animator__speed,
  float camera_path_animator__distance,
  float &camera_path_animator__progress,
  TMatrix &camera_animator__prev_transform)
{
  const TMatrix camTm = get_cam_itm();
  if (camTm != camera_animator__prev_transform)
  {
    manager.destroyEntity(eid);
    return;
  }

  camera_path_animator__progress += info.dt * camera_animator__speed;
  while (camera_path_animator__progress >= camera_path_animator__distance)
    camera_path_animator__progress -= camera_path_animator__distance;
  while (camera_path_animator__progress < 0)
    camera_path_animator__progress += camera_path_animator__distance;

  float lerpVal = 0;
  float distance = 0;
  Point3 p0 = camera_path_animator__transforms[0].get<TMatrix>().getcol(3);
  for (uint32_t index = 1; index < camera_path_animator__transforms.size(); ++index)
  {
    Point3 p1 = camera_path_animator__transforms[index].get<TMatrix>().getcol(3);
    float len = (p1 - p0).length();
    if (distance + len > camera_path_animator__progress)
    {
      lerpVal = (camera_path_animator__progress - distance) / len;

      Quat q0 = Quat(camera_path_animator__transforms[index - 1].get<TMatrix>());
      Quat q1 = Quat(camera_path_animator__transforms[index].get<TMatrix>());

      TMatrix tm;
      tm.makeTM(qinterp(q0, q1, lerpVal));
      tm.setcol(0, normalize(Point3(tm.getcol(0).x, 0, tm.getcol(0).z)));
      tm.setcol(1, normalize(cross(tm.getcol(2), tm.getcol(0))));
      tm.setcol(2, cross(tm.getcol(0), tm.getcol(1)));
      tm.setcol(3, lerp(p0, p1, lerpVal));

      camera_animator__prev_transform = tm;
      set_cam_itm(camera_animator__prev_transform);
      break;
    }
    distance += len;
    p0 = p1;
  }
}

static void set_cam_itm_with_roll(const TMatrix &itm)
{
  set_cam_itm(itm);
  if (Point3 *upVector = g_entity_mgr->getNullableRW<Point3>(get_cur_cam_entity(), ECS_HASH("free_camera__upVector")))
    *upVector = itm.getcol(1);
}

static bool camera_moved_by_user(const TMatrix &expected)
{
  const TMatrix &camTm = get_cam_itm();
  return lengthSq(camTm.getcol(3) - expected.getcol(3)) > sqr(0.0001f) ||
         lengthSq(camTm.getcol(2) - expected.getcol(2)) > sqr(0.00001f);
}

static TMatrix lerp_camera_tm(const TMatrix &from, const TMatrix &to, float t)
{
  TMatrix tm;
  tm.makeTM(normalize(qinterp(Quat(from), Quat(to), t)));
  tm.setcol(3, lerp(from.getcol(3), to.getcol(3), t));
  return tm;
}

ECS_NO_ORDER
static void camera_lerp_animator_update_es(const ecs::UpdateStageInfoAct &,
  ecs::EntityManager &manager,
  ecs::EntityId eid,
  const TMatrix &camera_lerp_animator__from,
  const TMatrix &camera_lerp_animator__to,
  float camera_lerp_animator__duration,
  int camera_lerp_animator__startTime,
  TMatrix &camera_animator__prev_transform)
{
  if (camera_moved_by_user(camera_animator__prev_transform))
  {
    manager.destroyEntity(eid);
    return;
  }

  const float progress = (get_time_msec() - camera_lerp_animator__startTime) * 0.001f / camera_lerp_animator__duration;
  const bool finished = progress >= 1.f;
  camera_animator__prev_transform =
    finished ? camera_lerp_animator__to : lerp_camera_tm(camera_lerp_animator__from, camera_lerp_animator__to, progress);
  set_cam_itm_with_roll(camera_animator__prev_transform);
  if (finished)
    manager.destroyEntity(eid);
}

static void start_camera_move_between(const TMatrix &from, const TMatrix &to, float duration)
{
  enable_free_camera();
  if (duration <= 0)
  {
    set_cam_itm_with_roll(to);
    return;
  }
  set_cam_itm_with_roll(from);
  ecs::ComponentsInitializer attrs;
  ECS_INIT(attrs, camera_lerp_animator__from, from);
  ECS_INIT(attrs, camera_lerp_animator__to, to);
  ECS_INIT(attrs, camera_lerp_animator__duration, duration);
  ECS_INIT(attrs, camera_lerp_animator__startTime, get_time_msec());
  g_entity_mgr->createEntitySync("camera_lerp_animator", eastl::move(attrs));
}

static bool is_finite_pos(const Point3 &p) { return check_finite(p.x) && check_finite(p.y) && check_finite(p.z); }

static bool animate_camera_console_handler(const char *argv[], int argc)
{
  if (argc < 1)
    return false;
  int found = 0;
  CONSOLE_CHECK_NAME("camera", "animate_circle", 1, 4)
  {
    if (argc == 1)
      console::print("camera.animate_circle [distance] [speed] [angleRange]");

    const TMatrix &camTm = get_cam_itm();
    float distance = 10;
    float angle = TWOPI * 1000;
    float speed = TWOPI * 0.1f;

    if (argc > 1)
      distance = console::to_real(argv[1]);
    if (argc > 2)
      speed = console::to_real(argv[2]);
    if (argc > 3)
      angle = console::to_real(argv[3]);


    Point3 origo = camTm.getcol(3);
    TMatrix transform;
    transform.identity();
    transform.setcol(3, -distance * camTm.getcol(2));
    set_cam_itm(transform * camTm);

    ecs::ComponentsInitializer attrs;
    ECS_INIT(attrs, camera_animator__origo, origo);
    ECS_INIT(attrs, camera_animator__angle, angle);
    ECS_INIT(attrs, camera_animator__speed, speed);
    g_entity_mgr->createEntitySync("camera_animator", eastl::move(attrs));
  }
  CONSOLE_CHECK_NAME("camera", "animate_path", 1, 11)
  {
    if (argc <= 2)
      console::print("camera.animate_path <speed> <path...>  (path is a list of saved camera positions)");

    float speed = 1;
    if (argc > 1)
      speed = console::to_real(argv[1]);

    DataBlock cameraPositions;
    if (dd_file_exist("cameraPositions.blk"))
      cameraPositions.load("cameraPositions.blk");

    bool success = true;
    ecs::Array transforms;
    float distance = 0;
    transforms.push_back(get_cam_itm());
    Point3 lastPos = get_cam_itm().getcol(3);
    for (uint32_t i = 2; i < argc && success; ++i)
    {
      const DataBlock *slotBlk = cameraPositions.getBlockByNameEx(String(100, "slot%d", console::to_int(argv[i])));
      if (slotBlk)
      {
        TMatrix camTm;
        camTm.setcol(0, slotBlk->getPoint3("tm0", Point3(1.f, 0.f, 0.f)));
        camTm.setcol(1, slotBlk->getPoint3("tm1", Point3(0.f, 1.f, 0.f)));
        camTm.setcol(2, slotBlk->getPoint3("tm2", Point3(0.f, 0.f, 1.f)));
        camTm.setcol(3, slotBlk->getPoint3("tm3", ZERO<Point3>()));
        transforms.push_back(camTm);
        distance += (camTm.getcol(3) - lastPos).length();
        lastPos = camTm.getcol(3);
      }
      else
      {
        console::print_d("Camera slot not found: %s", argv[i]);
        success = false;
      }
    }
    distance += (get_cam_itm().getcol(3) - lastPos).length();
    transforms.push_back(get_cam_itm());
    if (success && transforms.size() > 0 && distance > 0)
    {
      ecs::ComponentsInitializer attrs;
      ECS_INIT(attrs, camera_animator__speed, speed);
      ECS_INIT(attrs, camera_path_animator__transforms, eastl::move(transforms));
      ECS_INIT(attrs, camera_path_animator__distance, distance);
      g_entity_mgr->createEntitySync("camera_path_animator", eastl::move(attrs));
    }
  }
  CONSOLE_CHECK_NAME("camera", "move_between", 1, 8)
  {
    const float duration = argc > 1 ? console::to_real(argv[1]) : 0.f;
    if (argc > 1 && !check_finite(duration))
      console::print_d("Number of seconds must be finite: %s", argv[1]);
    else if (argc == 4)
    {
      TMatrix from, to;
      if (!load_camera_position("cameraPositions.blk", argv[2], from))
        console::print_d("Can not read camera slot from cameraPositions.blk: %s", argv[2]);
      else if (!load_camera_position("cameraPositions.blk", argv[3], to))
        console::print_d("Can not read camera slot from cameraPositions.blk: %s", argv[3]);
      else
        delayed_call([=] { start_camera_move_between(from, to, duration); });
    }
    else if (argc == 8)
    {
      const Point3 fromPos(console::to_real(argv[2]), console::to_real(argv[3]), console::to_real(argv[4]));
      const Point3 toPos(console::to_real(argv[5]), console::to_real(argv[6]), console::to_real(argv[7]));
      if (!is_finite_pos(fromPos) || !is_finite_pos(toPos))
        console::print("camera.move_between: coordinates must be finite");
      else
        delayed_call([=] {
          TMatrix from = get_cam_itm(), to = from;
          from.setcol(3, fromPos);
          to.setcol(3, toPos);
          start_camera_move_between(from, to, duration);
        });
    }
    else
    {
      console::print("camera.move_between <seconds> <from_slot> <to_slot>  (slots saved by camera.save)");
      console::print("camera.move_between <seconds> <x0> <y0> <z0> <x1> <y1> <z1>");
    }
  }
  return found;
}

REGISTER_CONSOLE_HANDLER(animate_camera_console_handler);
