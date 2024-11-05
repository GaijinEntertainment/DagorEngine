// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include "bhvReplayFreeCameraControl.h"

#include <daRg/dag_element.h>
#include <ecs/core/entityManager.h>
#include <ecs/core/attributeEx.h>

#include <drv/hid/dag_hiXInputMappings.h>
#include <startup/dag_inpDevClsDrv.h>
#include <drv/hid/dag_hiComposite.h>
#include <drv/hid/dag_hiPointingData.h>
#include <drv/hid/dag_hiGlobals.h>

static HumanInput::IGenJoystick *get_joystick()
{
  if (::global_cls_composite_drv_joy)
    return ::global_cls_composite_drv_joy->getDefaultJoystick();
  else if (::global_cls_drv_joy)
    return ::global_cls_drv_joy->getDefaultJoystick();
  return nullptr;
}


template <typename Callable>
static void move_replay_camera_ecs_query(Callable c);

static void move_replay_camera(const Point2 &delta)
{
  move_replay_camera_ecs_query([&](Point2 &free_cam_input__moveUI) { free_cam_input__moveUI = delta; });
}

SQ_PRECACHED_STRINGS_REGISTER_WITH_BHV(BhvReplayFreeCameraControl, bhv_replay_free_camera_control, cstr);

BhvReplayFreeCameraControl::BhvReplayFreeCameraControl() :
  BhvMenuCameraControl(F_HANDLE_MOUSE | F_HANDLE_TOUCH | F_INTERNALLY_HANDLE_GAMEPAD_R_STICK | F_INTERNALLY_HANDLE_GAMEPAD_L_STICK)
{}

void BhvReplayFreeCameraControl::onAttach(darg::Element *elem)
{
  BhvMenuCameraControl::onAttach(elem);
  HumanInput::raw_state_pnt.resetDelta();
}

void BhvReplayFreeCameraControl::onDetach(darg::Element *elem, DetachMode mode)
{
  BhvMenuCameraControl::onDetach(elem, mode);
  HumanInput::raw_state_pnt.resetDelta();
}

int BhvReplayFreeCameraControl::update(UpdateStage stage, darg::Element *elem, float dt)
{
  int ret = BhvMenuCameraControl::update(stage, elem, dt);
  if (ret != 0)
    return ret;

  if (!(elem->getStateFlags() & darg::Element::S_HOVER))
    return 0;

  using namespace HumanInput;

  IGenJoystick *joy = get_joystick();
  if (!joy)
    return 0;
  Point2 delta(joy->getAxisPos(JOY_XINPUT_REAL_AXIS_L_THUMB_H) / JOY_XINPUT_MAX_AXIS_VAL,
    joy->getAxisPos(JOY_XINPUT_REAL_AXIS_L_THUMB_V) / JOY_XINPUT_MAX_AXIS_VAL);
  move_replay_camera(delta);
  return 0;
}
