// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvMenuCameraControl.h"

#include <daRg/dag_element.h>
#include <daRg/dag_transform.h>
#include <daRg/dag_inputIds.h>
#include <daRg/dag_stringKeys.h>
#include <daRg/dag_guiScene.h>
#include <daRg/dag_properties.h>

#include <gui/dag_stdGuiRender.h>
#include <math/dag_mathUtils.h>
#include <math/dag_mathBase.h>
#include <math/dag_mathAng.h>
#include <perfMon/dag_cpuFreq.h>

#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

#include <sqrat.h>

#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiPointingData.h>


template <typename Callable>
static void rotate_menu_cam_ecs_query(Callable c);

template <typename Callable>
static void rotate_menu_cam_target_ecs_query(Callable c);

template <typename Callable>
static void rotate_spectator_cam_ecs_query(Callable c);

template <typename Callable>
static void rotate_spectator_free_cam_ecs_query(Callable c);

static void rotate_camera(const Point2 &delta, darg::InputDevice device)
{
  rotate_menu_cam_ecs_query([&](const Point2 &menu_cam__limitYaw, const Point2 &menu_cam__limitPitch,
                              Point2 &menu_cam__angles ECS_REQUIRE(eastl::false_type menu_cam__shouldRotateTarget)) {
    menu_cam__angles += delta / 200.f; // 200?????? (copied from prog\gameLibs\ecs\camera\menuCam.cpp.inl)

    menu_cam__angles.x = norm_s_ang(menu_cam__angles.x);
    menu_cam__angles.y = norm_s_ang(menu_cam__angles.y);

    Point2 limitYaw = DEG_TO_RAD * menu_cam__limitYaw;
    Point2 limitPitch = DEG_TO_RAD * menu_cam__limitPitch;
    menu_cam__angles.x = clamp(menu_cam__angles.x, limitYaw[0], limitYaw[1]);
    menu_cam__angles.y = clamp(menu_cam__angles.y, -limitPitch[1], limitPitch[0]);
  });


  rotate_menu_cam_target_ecs_query([&](const Point2 &menu_cam__limitYaw, const Point2 &menu_cam__limitPitch,
                                     Point2 &menu_cam__angles ECS_REQUIRE(eastl::true_type menu_cam__shouldRotateTarget)
                                       ECS_REQUIRE(eastl::true_type menu_cam__dirInited)) {
    Point2 limitYaw = DEG_TO_RAD * menu_cam__limitYaw;
    Point2 limitPitch = DEG_TO_RAD * menu_cam__limitPitch;
    menu_cam__angles.x = clamp(norm_s_ang(menu_cam__angles.x + delta.x / 200.f), limitYaw[0], limitYaw[1]);
    menu_cam__angles.y = clamp(norm_s_ang(menu_cam__angles.y + delta.y / 200.f), limitPitch[0], limitPitch[1]);
  });

  rotate_spectator_cam_ecs_query(
    [&](Point2 &spectator__ang, Point3 &spectator__dir, bool camera__active, float specator_cam__smoothDiv = 400.f) {
      if (!camera__active)
        return;

      spectator__ang += Point2(-delta.x / specator_cam__smoothDiv * PI, delta.y / specator_cam__smoothDiv * PI);
      spectator__ang.y = clamp<float>(spectator__ang.y, -PI / 2. + 1e-3f, PI / 2. - 1e-3f);
      spectator__dir = angles_to_dir(spectator__ang);
    });

  rotate_spectator_free_cam_ecs_query([&](ECS_REQUIRE(ecs::Tag replay_camera__tpsFree) bool camera__active,
                                        Point2 &replay_camera__tpsInputAngle, float specator_cam__smoothDiv = 400.f) {
    if (!camera__active)
      return;
    // Invers input for gamepad
    int dir = device == darg::DEVID_JOYSTICK ? -1 : 1;
    replay_camera__tpsInputAngle = replay_camera__tpsInputAngle + (delta * dir) / specator_cam__smoothDiv * PI;
    replay_camera__tpsInputAngle.y = clamp<float>(replay_camera__tpsInputAngle.y, -PI / 2. + 1e-3f, PI / 2. - 1e-3f);
  });
}

template <typename Callable>
static void change_spectator_camera_tps_speed_ecs_query(Callable c);

static bool change_camera_speed(float wheelStep)
{
  bool processed = false;
  change_spectator_camera_tps_speed_ecs_query(
    [&](ECS_REQUIRE(ecs::Tag replay_camera__tpsFree) bool camera__active, float &free_cam__move_speed) {
      if (camera__active)
      {
        free_cam__move_speed *= powf(1.03f, 1 * wheelStep);
        processed = true;
      }
    });
  return processed;
}

using namespace darg;

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvMenuCameraControl, bhv_menu_camera_control, cstr);


static HumanInput::IGenJoystick *get_joystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  else if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


struct MenuCameraControlData
{
  InputDevice activeDeviceId = DEVID_NONE;
  int pointerId = -1;
  int buttonId = -1;
  Point2 lastPos = Point2(0, 0);

  int lastUpdT = 0;

  bool isPressed() { return activeDeviceId != DEVID_NONE; }
  bool isPressedBy(InputDevice device, int pointer) { return activeDeviceId == device && pointerId == pointer; }
  bool isPressedBy(InputDevice device, int pointer, int button)
  {
    return activeDeviceId == device && pointerId == pointer && buttonId == button;
  }
  void release()
  {
    activeDeviceId = DEVID_NONE;
    pointerId = -1;
    buttonId = -1;
  }
};


const char *bhv_data_slot_key = "bhv:MenuCameraControlData";


BhvMenuCameraControl::BhvMenuCameraControl(int flags) : Behavior(STAGE_ACT, flags) {}


void BhvMenuCameraControl::onAttach(Element *elem)
{
  MenuCameraControlData *bhvData = new MenuCameraControlData();
  elem->props.storage.SetValue(bhv_data_slot_key, bhvData);
}


void BhvMenuCameraControl::onDetach(Element *elem, DetachMode)
{
  MenuCameraControlData *bhvData = elem->props.storage.RawGetSlotValue<MenuCameraControlData *>(bhv_data_slot_key, nullptr);
  if (bhvData)
  {
    elem->props.storage.DeleteSlot(bhv_data_slot_key);
    delete bhvData;
  }
}


static int get_mouse_pan_button(const darg::Element *elem, const BhvMenuCameraControl::CachedStringsMap &cstr)
{
  static constexpr int def_btn = 1;
  auto strings = cstr.resolveVm(elem->getVM());
  G_ASSERT_RETURN(strings, def_btn);

  return elem->props.scriptDesc.RawGetSlotValue<int>(strings->panMouseButton, def_btn);
}


int BhvMenuCameraControl::mouseEvent(ElementTree *etree,
  Element *elem,
  InputDevice device,
  InputEvent event,
  int pointer_id,
  int data,
  short mx,
  short my,
  int /*buttons*/,
  int accum_res)
{
  return pointingEvent(etree, elem, device, event, pointer_id, data, Point2(mx, my), accum_res);
}


int BhvMenuCameraControl::touchEvent(ElementTree *etree,
  Element *elem,
  InputEvent event,
  HumanInput::IGenPointing * /*pnt*/,
  int touch_idx,
  const HumanInput::PointingRawState::Touch &touch,
  int accum_res)
{
  return pointingEvent(etree, elem, DEVID_TOUCH, event, touch_idx, 0, Point2(touch.x, touch.y), accum_res);
}


int BhvMenuCameraControl::pointingEvent(
  ElementTree *, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id, const Point2 &pos, int accum_res)
{
  MenuCameraControlData *bhvData = elem->props.storage.RawGetSlotValue<MenuCameraControlData *>(bhv_data_slot_key, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  if (event == INP_EV_PRESS)
  {
    if (!bhvData->isPressed() && elem->hitTest(pos) && !(accum_res & R_PROCESSED) &&
        (device == DEVID_TOUCH || button_id == get_mouse_pan_button(elem, cstr)))
    {
      bhvData->activeDeviceId = device;
      bhvData->pointerId = pointer_id;
      bhvData->buttonId = button_id;
      bhvData->lastPos = pos;
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_POINTER_MOVE)
  {
    if (bhvData->isPressedBy(device, pointer_id))
    {
      Point2 delta = pos - bhvData->lastPos;

      rotate_camera(-delta, device);

      bhvData->lastPos = pos;

      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_RELEASE)
  {
    if (bhvData->isPressedBy(device, pointer_id, button_id))
    {
      bhvData->release();
      return R_PROCESSED;
    }
  }
  else if (event == INP_EV_MOUSE_WHEEL)
  {
    if (!(accum_res & R_PROCESSED))
    {
      if (change_camera_speed(button_id /*direction*/ < 0 ? -1 : 1))
        return R_PROCESSED;
    }
  }

  return 0;
}


int BhvMenuCameraControl::update(UpdateStage /*stage*/, Element *elem, float /*dt*/)
{
  MenuCameraControlData *bhvData = elem->props.storage.RawGetSlotValue<MenuCameraControlData *>(bhv_data_slot_key, nullptr);
  G_ASSERT_RETURN(bhvData, 0);

  using namespace HumanInput;

  IGenJoystick *joy = get_joystick();
  if (!joy)
    return 0;

  if (!(elem->getStateFlags() & Element::S_HOVER))
    return 0;

  Point2 delta(joy->getAxisPos(JOY_XINPUT_REAL_AXIS_R_THUMB_H) / JOY_XINPUT_MAX_AXIS_VAL,
    -joy->getAxisPos(JOY_XINPUT_REAL_AXIS_R_THUMB_V) / JOY_XINPUT_MAX_AXIS_VAL);
  const float speedMul = 4.0f;
  if (lengthSq(delta) > 0.01f)
    rotate_camera(delta * speedMul, DEVID_JOYSTICK);

  return 0;
}
