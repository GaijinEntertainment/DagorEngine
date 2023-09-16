#include <daECS/core/updateStage.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>
#include <ecs/input/message.h>
#include <daECS/core/coreEvents.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <stddef.h>

#define CAM_ATTR(x) ECS_HASH("sphere_cam__" #x)

static Point3 default_offset(0.f, 9.f, -38.f);

struct SphereCamera
{
  Point2 ang = Point2(0, 0), lookOffs = Point2(0, 0); // azimuth & elevation
  bool lookAround = false;
  Point3 offs = default_offset;
  dainput::action_handle_t aRotate, aLookAround, aSpeedUp, aSpeedDown;

  void updateItm(const ecs::EntityManager &mgr, ecs::EntityId target_eid, Point3 const &static_look_at, TMatrix &itm)
  {
    DPoint3 lookAt;
    Point3 lookDir = Point3(1.f, 0.f, 0.f);
    bool lookDirInited = false;
    if (mgr.doesEntityExist(target_eid))
    {
      if (mgr.has(target_eid, ECS_HASH("camera__look_at")))
        lookAt = mgr.get<DPoint3>(target_eid, ECS_HASH("camera__look_at"));
      else
        lookAt = mgr.getOr(target_eid, ECS_HASH("transform"), TMatrix::IDENT).getcol(3);
      if (mgr.has(target_eid, ECS_HASH("camera__lookDir")))
      {
        lookDir = ECS_GET(Point3, target_eid, camera__lookDir);
        lookDirInited = true;
      }
    }
    else
    {
      lookAt = static_look_at;
    }

    float xSine, xCos;
    float ySine, yCos;
    sincos(ang.x + lookOffs.x, xSine, xCos);
    sincos(ang.y + lookOffs.y, ySine, yCos);
    Point3 inputDir = lookDirInited ? lookDir : Point3(xCos * yCos, ySine, xSine * yCos);
    itm.setcol(2, inputDir);
    itm.setcol(0, normalize(Point3(0.f, 1.f, 0.f) % inputDir));
    itm.setcol(1, normalize(inputDir % itm.getcol(0)));

    itm.setcol(3, (itm % offs) + lookAt);
  }


  SphereCamera(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    initActions();
    offs = mgr.getOr(eid, CAM_ATTR(offs), default_offset);
  }
  ~SphereCamera() { resetActions(); }

  void initActions()
  {
    SETUP_ACTION_STICK(Rotate, "Camera.");
    SETUP_ACTION_AXIS(LookAround, "Camera.");
    SETUP_ACTION_DIGITAL(SpeedUp, "Camera.");
    SETUP_ACTION_DIGITAL(SpeedDown, "Camera.");
  }
  void resetActions()
  {
    CLEAR_DAINPUT_ACTION(Rotate);
    CLEAR_DAINPUT_ACTION(LookAround);
    CLEAR_DAINPUT_ACTION(SpeedUp);
    CLEAR_DAINPUT_ACTION(SpeedDown);
  }
};

ECS_DECLARE_RELOCATABLE_TYPE(SphereCamera);
ECS_REGISTER_RELOCATABLE_TYPE(SphereCamera, nullptr);
ECS_AUTO_REGISTER_COMPONENT_DEPS(SphereCamera, "sphere_cam", nullptr, 0, "?input");
ECS_DEF_PULL_VAR(sphere_cam);

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
static void sphere_cam_es(const ecs::UpdateStageInfoAct & /*update_info*/, const ecs::EntityManager &manager, SphereCamera &sphere_cam,
  ecs::EntityId camera__target, TMatrix &transform, const Point3 &sphere_cam__look_at = Point3(0, 0, 0))
{
  const dainput::AnalogStickAction &rotate = dainput::get_analog_stick_action_state(sphere_cam.aRotate);
  const dainput::AnalogAxisAction &lookAround = dainput::get_analog_axis_action_state(sphere_cam.aLookAround);

  if (lookAround.bActive)
  {
    sphere_cam.lookAround = lookAround.x > 0;
    if (!sphere_cam.lookAround)
      sphere_cam.lookOffs.zero();
  }
  if (rotate.bActive)
  {
    Point2 &ang = sphere_cam.lookAround ? sphere_cam.lookOffs : sphere_cam.ang;
    ang.x -= rotate.x / 1000.f;
    ang.y += rotate.y / 1000.f;
    if (sphere_cam.lookOffs.y + sphere_cam.ang.y >= HALFPI - 1e-5f)
      sphere_cam.lookOffs.y += HALFPI - 1e-5f - (sphere_cam.lookOffs.y + sphere_cam.ang.y);
    if (sphere_cam.lookOffs.y + sphere_cam.ang.y <= -HALFPI + 1e-5f)
      sphere_cam.lookOffs.y += -HALFPI + 1e-5f - (sphere_cam.lookOffs.y + sphere_cam.ang.y);
  }

  sphere_cam.updateItm(manager, camera__target, sphere_cam__look_at, transform);
}

ECS_TAG(render)
static void sphere_cam_input_es_event_handler(const EventDaInputActionTriggered &evt, SphereCamera &sphere_cam)
{
  if (eastl::get<0>(evt) == sphere_cam.aSpeedUp)
    sphere_cam.offs = sphere_cam.offs * 10;
  if (eastl::get<0>(evt) == sphere_cam.aSpeedDown)
    sphere_cam.offs = sphere_cam.offs / 10;
}
