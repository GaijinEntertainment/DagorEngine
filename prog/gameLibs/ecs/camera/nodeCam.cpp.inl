#include <ecs/core/entityManager.h>
#include <daECS/core/coreEvents.h>
#include <ecs/core/attributeEx.h>
#include <daECS/core/updateStage.h>
#include <ecs/input/message.h>
#include <math/dag_Point3.h>
#include <math/dag_Point2.h>
#include <math/dag_TMatrix.h>
#include <math/dag_mathUtils.h>
#include <math/dag_mathAng.h>
#include <ecs/anim/anim.h>
#include <math/dag_geomTree.h>


struct NodeCamera
{
  enum ECtrlBits
  {
    ECB_ORI_WORLD = 0,
    ECB_ORI_LOCKED,
    ECB_OFFS_LOCKED,

    ECB_MOVE_FWD,
    ECB_MOVE_BACK,
    ECB_MOVE_LEFT,
    ECB_MOVE_RIGHT,
    ECB_MOVE_UP,
    ECB_MOVE_DOWN,

    ECB_ZOOM_IN,
    ECB_ZOOM_OUT,

    ECB_NUM
  };
  dag::Index16 nodeIndex;
  eastl::bitset<ECB_NUM> ctrlBits;

  float velTau = 0;
  float velFactor = 0;
  float tau = 0;

  float angTau = 0;

  float moveSpeed = 1.f;
  float fovSpeed = 1.f;

  Point3 vel = Point3(0, 0, 0);
  Point3 lastPos = Point3(0, 0, 0);

  Quat lastOri = Quat(0, 0, 0, 1);

  void act(const AnimV20::AnimcharBaseComponent *animChar, float dt, float &fov, TMatrix &itm, Point3 &offset, const Point3 &ang)
  {
    if (!animChar || !nodeIndex)
      return;
    Point3 wpos = animChar->getNodeTree().getNodeWposScalar(nodeIndex);

    ANIMCHAR_VERIFY_NODE_POS_S(wpos, nodeIndex, *animChar);
    TMatrix nodeTm;
    if (ctrlBits[ECB_ORI_WORLD])
    {
      Point3 lookDir = sph_ang_to_dir(Point2::xy(DEG_TO_RAD * ang));
      nodeTm.setcol(0, lookDir);
      nodeTm.setcol(2, normalize(Point3(0.f, 1.f, 0.f) % lookDir));
      nodeTm.setcol(1, normalize(lookDir % nodeTm.getcol(2)));
      nodeTm.setcol(3, wpos);
    }
    else
    {
      Quat oriQuat;
      euler_to_quat(-DEG_TO_RAD * ang.x, DEG_TO_RAD * ang.y, DEG_TO_RAD * ang.z, oriQuat);
      TMatrix nodeSpaceTm;
      animChar->getNodeTree().getNodeWtmScalar(nodeIndex, nodeSpaceTm);
      nodeTm.setcol(0, nodeSpaceTm.getcol(1));
      nodeTm.setcol(1, nodeSpaceTm.getcol(0));
      nodeTm.setcol(2, -nodeSpaceTm.getcol(2));
      Quat nodeOri(nodeTm);
      nodeOri = approach(lastOri, nodeOri, dt, angTau);
      lastOri = nodeOri;
      nodeTm = makeTM(lastOri * oriQuat);
      nodeTm.setcol(2, -nodeTm.getcol(2));
      nodeTm.setcol(3, nodeSpaceTm.getcol(3));
    }

    if (!ctrlBits[ECB_OFFS_LOCKED])
    {
      Point3 move = ZERO<Point3>();
      move.x += ctrlBits[ECB_MOVE_FWD] ? +1.f : 0.f;
      move.x += ctrlBits[ECB_MOVE_BACK] ? -1.f : 0.f;
      move.y += ctrlBits[ECB_MOVE_UP] ? +1.f : 0.f;
      move.y += ctrlBits[ECB_MOVE_DOWN] ? -1.f : 0.f;
      move.z += ctrlBits[ECB_MOVE_RIGHT] ? +1.f : 0.f;
      move.z += ctrlBits[ECB_MOVE_LEFT] ? -1.f : 0.f;
      offset = offset + move * moveSpeed * dt;
    }
    if (ctrlBits[ECB_ZOOM_IN])
      fov = clamp(fov - fov * dt * fovSpeed, 2.f, 120.f);
    if (ctrlBits[ECB_ZOOM_OUT])
      fov = clamp(fov + fov * dt * fovSpeed, 2.f, 120.f);

    TMatrix offsTm = TMatrix::IDENT;
    offsTm.setcol(3, offset);
    TMatrix posTm = nodeTm * offsTm;

    for (int i = 0; i < 3; ++i)
      itm.setcol(2 - i, posTm.getcol(i));

    Point3 realPos = posTm.getcol(3);
    Point3 wishPos = approach_vel(lastPos, realPos, dt, tau, vel, velTau, velFactor);

    lastPos = wishPos;
    itm.setcol(3, wishPos);
  }
  NodeCamera(const ecs::EntityManager &mgr, ecs::EntityId eid)
  {
    initActions();
    ecs::EntityId attachedEid = mgr.getOr(eid, ECS_HASH("camera__target"), ecs::INVALID_ENTITY_ID);
    G_ASSERT_RETURN(attachedEid != ecs::INVALID_ENTITY_ID, );
    ctrlBits.reset();

#define GET_ATTR(x, def_val) x = mgr.getOr(eid, ECS_HASH("node_cam__" #x), def_val)
    GET_ATTR(velTau, 0.f);
    GET_ATTR(velFactor, 0.f);
    GET_ATTR(tau, 0.f);
    GET_ATTR(angTau, 0.f);

    GET_ATTR(moveSpeed, 1.f);
    GET_ATTR(fovSpeed, 1.f);
#undef GET_ATTR
    const AnimV20::AnimcharBaseComponent *animChar =
      mgr.getNullable<AnimV20::AnimcharBaseComponent>(attachedEid, ECS_HASH("animchar"));
    nodeIndex = animChar->getNodeTree().findNodeIndex(mgr.get<ecs::string>(eid, ECS_HASH("node_cam__node")).c_str());
    G_ASSERT(nodeIndex);
  }
  ~NodeCamera() { resetActions(); }

  void initActions()
  {
    SETUP_ACTION_STICK(Move, "Camera.");
    SETUP_ACTION_STICK(Rotate, "Camera.");
    SETUP_ACTION_AXIS(ShiftY, "Camera.");
    SETUP_ACTION_DIGITAL(ZoomIn, "Camera.");
    SETUP_ACTION_DIGITAL(ZoomOut, "Camera.");
    SETUP_ACTION_DIGITAL(LockOri, "Camera.");
    SETUP_ACTION_DIGITAL(LockOfs, "Camera.");
    SETUP_ACTION_DIGITAL(OriWorld, "Camera.");
  }
  void resetActions()
  {
    CLEAR_DAINPUT_ACTION(Move);
    CLEAR_DAINPUT_ACTION(Rotate);
    CLEAR_DAINPUT_ACTION(ShiftY);
    CLEAR_DAINPUT_ACTION(ZoomIn);
    CLEAR_DAINPUT_ACTION(ZoomOut);
    CLEAR_DAINPUT_ACTION(LockOri);
    CLEAR_DAINPUT_ACTION(LockOfs);
    CLEAR_DAINPUT_ACTION(OriWorld);
  }
  dainput::action_handle_t aMove, aRotate, aShiftY, aZoomIn, aZoomOut, aLockOri, aLockOfs, aOriWorld;
};

ECS_DECLARE_RELOCATABLE_TYPE(NodeCamera);
ECS_REGISTER_RELOCATABLE_TYPE(NodeCamera, nullptr);

ECS_AUTO_REGISTER_COMPONENT_DEPS(NodeCamera, "node_cam", nullptr, 0, "?input");
ECS_DEF_PULL_VAR(node_cam);

ECS_TAG(render)
ECS_AFTER(camera_set_sync)
ECS_BEFORE(before_camera_sync)
static void node_cam_es(const ecs::UpdateStageInfoAct &update_info, NodeCamera &node_cam, float &fov, TMatrix &transform,
  Point3 &node_cam__offset, Point3 &node_cam__angles, ecs::EntityId camera__target)
{
  auto animChar = ECS_GET_NULLABLE(AnimV20::AnimcharBaseComponent, camera__target, animchar);
  if (!animChar)
    return;

  const dainput::AnalogStickAction &move = dainput::get_analog_stick_action_state(node_cam.aMove);
  const dainput::AnalogStickAction &rotate = dainput::get_analog_stick_action_state(node_cam.aRotate);
  const dainput::AnalogAxisAction &shiftY = dainput::get_analog_axis_action_state(node_cam.aShiftY);

  if (move.bActive)
  {
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_LEFT] = move.x < 0;
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_RIGHT] = move.x > 0;
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_FWD] = move.y > 0;
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_BACK] = move.y < 0;
  }
  if (shiftY.bActive)
  {
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_UP] = shiftY.x > 0;
    node_cam.ctrlBits[NodeCamera::ECB_MOVE_DOWN] = shiftY.x < 0;
  }
  if (dainput::is_action_active(node_cam.aZoomIn))
    node_cam.ctrlBits[NodeCamera::ECB_ZOOM_IN] = dainput::get_digital_action_state(node_cam.aZoomIn).bState;
  if (dainput::is_action_active(node_cam.aZoomOut))
    node_cam.ctrlBits[NodeCamera::ECB_ZOOM_OUT] = dainput::get_digital_action_state(node_cam.aZoomOut).bState;
  if (rotate.bActive)
  {
    if (node_cam.ctrlBits[NodeCamera::ECB_ORI_WORLD] || !node_cam.ctrlBits[NodeCamera::ECB_ORI_LOCKED])
    {
      node_cam__angles.x += rotate.x / 1000.f;
      node_cam__angles.y += rotate.y / 1000.f;
    }
  }

  node_cam.act(animChar, update_info.dt, fov, transform, node_cam__offset, node_cam__angles);
}

ECS_TAG(render)
ECS_TRACK(camera__target)
static void node_cam_target_changed_es_event_handler(const ecs::Event &, NodeCamera &node_cam, const ecs::string &node_cam__node,
  ecs::EntityId camera__target)
{
  auto animChar = ECS_GET_NULLABLE(AnimV20::AnimcharBaseComponent, camera__target, animchar);
  if (animChar)
    node_cam.nodeIndex = animChar->getNodeTree().findNodeIndex(node_cam__node.c_str());
}

ECS_TAG(input)
ECS_REQUIRE(eastl::false_type node_cam__locked)
static void node_cam_input_es_event_handler(const EventDaInputActionTriggered &evt, NodeCamera &node_cam, const TMatrix &transform,
  Point3 &node_cam__angles)
{
  if (eastl::get<0>(evt) == node_cam.aLockOfs)
    node_cam.ctrlBits[NodeCamera::ECB_OFFS_LOCKED] = !node_cam.ctrlBits[NodeCamera::ECB_OFFS_LOCKED];
  else if (eastl::get<0>(evt) == node_cam.aLockOri)
    node_cam.ctrlBits[NodeCamera::ECB_ORI_LOCKED] = !node_cam.ctrlBits[NodeCamera::ECB_ORI_LOCKED];
  else if (eastl::get<0>(evt) == node_cam.aOriWorld)
  {
    node_cam.ctrlBits[NodeCamera::ECB_ORI_WORLD] = !node_cam.ctrlBits[NodeCamera::ECB_ORI_WORLD];
    if (node_cam.ctrlBits[NodeCamera::ECB_ORI_WORLD])
      node_cam__angles = Point3::xyV(dir_to_sph_ang(transform.getcol(2)), node_cam__angles.z);
  }
}
