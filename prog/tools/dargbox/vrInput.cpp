#include "vrInput.h"
#include "vr.h"
#include "main.h"
#include <humanInput/dag_hiVrInput.h>
#include <3d/dag_drv3d.h>
#include <3d/dag_stereoIndex.h>
#include <vr/vrGuiSurface.h>
#include <debug/dag_debug3d.h>
#include <startup/dag_inpDevClsDrv.h>
#include <humanInput/dag_hiPointing.h>
#include <daRg/dag_guiScene.h>
#include <gui/dag_visualLog.h>


using VrInput = HumanInput::VrInput;
using ActionSetId = VrInput::ActionSetId;
using ActionId = VrInput::ActionId;
using ActionSetIndex = VrInput::ActionSetIndex;
using ActionIndex = VrInput::ActionIndex;
using DigitalAction = VrInput::DigitalAction;
using AnalogAction = VrInput::AnalogAction;
using StickAction = VrInput::StickAction;


namespace vr
{

static const int NUM_HANDS = 2;

struct HandPose
{
  bool isActive = false;
  TMatrix grip, aim;
};


static HandPose hand_poses[NUM_HANDS];

static const ActionSetId UI_ACTION_SET_ID = 1;
static const ActionId LH_CLICK_ACTION_ID = 1;
static const ActionId RH_CLICK_ACTION_ID = 2;
static const ActionId LH_THUMBSTICK_POS_ID = 3;
static const ActionId RH_THUMBSTICK_POS_ID = 4;
static const char *UI_ACTION_SET_NAME = "ui";

static bool controllers_initialized = false;


static void load_bindings()
{
  auto input = VRDevice::getInput();
  if (!input)
    return;

  auto asIdx = input->addActionSet(UI_ACTION_SET_ID, UI_ACTION_SET_NAME);
  G_ASSERTF_RETURN(asIdx != VrInput::INVALID_ACTION_SET, , "[VRIP] Failed to create action set '%d:%s'", UI_ACTION_SET_ID,
    UI_ACTION_SET_NAME);

  input->setupHands(UI_ACTION_SET_ID, "controls/grip_pose", "controls/aim_pose", "controls/haptics");

  VrInput::ActionIndex lhClickAction =
    input->addAction(UI_ACTION_SET_ID, LH_CLICK_ACTION_ID, VrInput::ActionType::DIGITAL, "vr_ui_lh_click", nullptr);

  VrInput::ActionIndex rhClickAction =
    input->addAction(UI_ACTION_SET_ID, RH_CLICK_ACTION_ID, VrInput::ActionType::DIGITAL, "vr_ui_rh_click", nullptr);

  G_ASSERT(lhClickAction != VrInput::INVALID_ACTION);
  G_ASSERT(rhClickAction != VrInput::INVALID_ACTION);

  if (lhClickAction != VrInput::INVALID_ACTION && rhClickAction != VrInput::INVALID_ACTION)
  {
    input->suggestBinding(LH_CLICK_ACTION_ID, "microsoft_motion", "/user/hand/left/input/trigger/value");
    input->suggestBinding(RH_CLICK_ACTION_ID, "microsoft_motion", "/user/hand/right/input/trigger/value");
    input->suggestBinding(LH_CLICK_ACTION_ID, "oculus_touch", "/user/hand/left/input/trigger/value");
    input->suggestBinding(RH_CLICK_ACTION_ID, "oculus_touch", "/user/hand/right/input/trigger/value");
    input->suggestBinding(LH_CLICK_ACTION_ID, "valve_index", "/user/hand/left/input/trigger/click");
    input->suggestBinding(RH_CLICK_ACTION_ID, "valve_index", "/user/hand/right/input/trigger/click");
    input->suggestBinding(LH_CLICK_ACTION_ID, "htc_vive", "/user/hand/left/input/trigger/click");
    input->suggestBinding(RH_CLICK_ACTION_ID, "htc_vive", "/user/hand/right/input/trigger/click");
  }

  ActionIndex lhThumbstickAction =
    input->addAction(UI_ACTION_SET_ID, LH_THUMBSTICK_POS_ID, VrInput::ActionType::STICK, "vr_ui_lh_thumbstick", nullptr);
  ActionIndex rhThumbstickAction =
    input->addAction(UI_ACTION_SET_ID, RH_THUMBSTICK_POS_ID, VrInput::ActionType::STICK, "vr_ui_rh_thumbstick", nullptr);

  G_ASSERT(lhThumbstickAction != VrInput::INVALID_ACTION);
  G_ASSERT(rhThumbstickAction != VrInput::INVALID_ACTION);
  if (lhThumbstickAction != VrInput::INVALID_ACTION && rhThumbstickAction != VrInput::INVALID_ACTION)
  {
    input->suggestBinding(LH_THUMBSTICK_POS_ID, "microsoft_motion", "/user/hand/left/input/thumbstick");
    input->suggestBinding(RH_THUMBSTICK_POS_ID, "microsoft_motion", "/user/hand/right/input/thumbstick");
    input->suggestBinding(LH_THUMBSTICK_POS_ID, "oculus_touch", "/user/hand/left/input/thumbstick");
    input->suggestBinding(RH_THUMBSTICK_POS_ID, "oculus_touch", "/user/hand/right/input/thumbstick");
    input->suggestBinding(LH_THUMBSTICK_POS_ID, "valve_index", "/user/hand/left/input/thumbstick");
    input->suggestBinding(RH_THUMBSTICK_POS_ID, "valve_index", "/user/hand/right/input/thumbstick");
  }

  if (!input->completeActionsInit() || !input->activateActionSet(UI_ACTION_SET_ID))
    logerr("Initializing VR input failed!");
}


static TMatrix get_pose(const VrInput::Pose &p)
{
  TMatrix m = makeTM(p.rot);
  m.setcol(3, p.pos);
  return m;
}


static void init_controllers()
{
  if (controllers_initialized)
    return;

  if (auto input = VRDevice::getInput())
    input->setAndCallInitializationCallback(load_bindings);

  controllers_initialized = true;
}


void handle_controller_input()
{
  init_controllers();

  // if (!is_enabled_for_this_frame())
  //   return;

  const VrInput *input = VRDevice::getInputIfUsable();
  if (!input)
    return;

  darg::IGuiScene *darg_scene = get_ui_scene();
  if (darg_scene)
    darg_scene->ignoreDeviceCursorPos(true);

  for (int side = 0; side < NUM_HANDS; ++side)
  {
    VrInput::Controller state = input->getControllerState(VrInput::Hands(side));

    hand_poses[side].isActive = state.isActive;
    if (state.isActive && state.hasChanged)
    {
      hand_poses[side].grip = get_pose(state.grip);
      hand_poses[side].aim = get_pose(state.aim);

      if (darg_scene)
        darg_scene->onVrInputEvent(darg::INP_EV_POINTER_MOVE, side);
    }
  }


  DigitalAction clickActions[NUM_HANDS] = {
    input->getDigitalActionState(LH_CLICK_ACTION_ID), input->getDigitalActionState(RH_CLICK_ACTION_ID)};

  StickAction stickActions[NUM_HANDS] = {
    input->getStickActionState(LH_THUMBSTICK_POS_ID), input->getStickActionState(RH_THUMBSTICK_POS_ID)};

  if (darg_scene)
  {
    for (int side = 0; side < NUM_HANDS; ++side)
    {
      DigitalAction &click = clickActions[side];
      if (click.isActive && click.hasChanged)
      {
        darg::InputEvent evt = click.state ? darg::INP_EV_PRESS : darg::INP_EV_RELEASE;
        darg_scene->onVrInputEvent(evt, side);
      }

      StickAction &stick = stickActions[side];
      if (stick.isActive)
      {
        if (stick.hasChanged)
          darg_scene->setVrStickScroll(side, Point2(stick.state.x, -stick.state.y));
      }
      else
        darg_scene->setVrStickScroll(side, Point2(0, 0));
    }
  }
}


void render_controller_poses()
{
  if (!is_enabled_for_this_frame())
    return;

  SCOPE_RENDER_TARGET;

  for (auto index : {StereoIndex::Left, StereoIndex::Right})
  {
    auto &view = get_view(index);

    d3d::set_render_target(get_frame_target(index), 0);

    TMatrix localCamera;
    localCamera.makeTM(view.orientation);
    localCamera.setcol(3, view.position.x, view.position.y, view.position.z);

    d3d::settm(TM_VIEW, ::orthonormalized_inverse(localCamera));
    d3d::setpersp(view.projection);

    ::begin_draw_cached_debug_lines(false, false);

    for (int side = 0; side < NUM_HANDS; ++side)
    {
      HandPose &hand = hand_poses[side];
      if (!hand.isActive)
        continue;

      TMatrix pose = hand.aim;

      const float mainAxisLen = 0.33f;
      const float sideAxisLen = 0.05f;

      ::draw_cached_debug_line(pose.getcol(3), pose.getcol(3) + pose.getcol(0) * sideAxisLen, E3DCOLOR(255, 0, 0));
      ::draw_cached_debug_line(pose.getcol(3), pose.getcol(3) + pose.getcol(1) * sideAxisLen, E3DCOLOR(0, 255, 0));
      ::draw_cached_debug_line(pose.getcol(3), pose.getcol(3) + pose.getcol(2) * mainAxisLen, E3DCOLOR(0, 0, 255));
    }

    ::end_draw_cached_debug_lines();
  }
}


TMatrix get_aim_pose(int index) { return hand_poses[index].aim; }


TMatrix get_grip_pose(int index) { return hand_poses[index].grip; }


} // namespace vr
