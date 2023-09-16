//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) 2023  Gaijin Games KFT.  All rights reserved
// (for conditions of use see prog/license.txt)
//
#pragma once

#include <daRg/dag_guiConstants.h>
#include <daRg/dag_inputIds.h>
#include <debug/dag_assert.h>
#include <debug/dag_debug.h>
#include <generic/dag_tab.h>
#include <humanInput/dag_hiPointingData.h>


namespace HumanInput
{
struct ButtonBits;
class IGenJoystick;
class IGenPointing;
} // namespace HumanInput


namespace darg
{

class Element;
class ElementTree;
struct HotkeyButton;


class Behavior
{
public:
  enum UpdateStage
  {
    STAGE_ACT = 0x01,
    STAGE_BEFORE_RENDER = 0x02,
  };

  enum Flags
  {
    F_HANDLE_KEYBOARD = 0x0001,
    F_HANDLE_KEYBOARD_GLOBAL = F_HANDLE_KEYBOARD | 0x10000000,
    F_HANDLE_MOUSE = 0x0002,
    F_HANDLE_TOUCH = 0x0004,
    F_HANDLE_JOYSTICK = 0x0008,
    F_CAN_HANDLE_CLICKS = 0x0010,
    F_INTERNALLY_HANDLE_GAMEPAD_L_STICK = 0x0020,
    F_INTERNALLY_HANDLE_GAMEPAD_R_STICK = 0x0040,
    F_INTERNALLY_HANDLE_GAMEPAD_STICKS = 0x0060,
    F_DISPLAY_IME = 0x0080,
    F_OVERRIDE_GAMEPAD_STICK = 0x0100,
    F_FOCUS_ON_CLICK = 0x0200,
    F_USES_TOUCH_MARGIN = 0x400,
    F_ALLOW_AUTO_SCROLL = 0x800,
  };

  enum DetachMode
  {
    DETACH_FINAL,
    DETACH_REBUILD
  };

public:
  Behavior(int update_stages, int flags_) : updateStages(update_stages), flags(flags_) {}

  virtual ~Behavior() {}

  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) { return 0; }

  virtual int kbdEvent(ElementTree *, Element *, InputEvent /*event*/, int /*key_idx*/, bool /*repeat*/, wchar_t /*wc*/,
    int /*accum_res*/)
  {
    return 0;
  }
  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/, int /*btn_idx*/,
    short /*mx*/, short /*my*/, int /*buttons*/, int /*accum_res*/)
  {
    return 0;
  }
  virtual int joystickBtnEvent(ElementTree *, Element *, const HumanInput::IGenJoystick *, InputEvent /*event*/, int /*key_idx*/,
    int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int /*accum_res*/)
  {
    return 0;
  }
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/)
  {
    return 0;
  }

  virtual void onAttach(Element *) {}
  virtual void onDetach(Element *, DetachMode) {}
  virtual void onDelete(Element *) {}
  virtual void onRecalcLayout(Element *) {}
  virtual void onElemSetup(Element *, SetupMode /*setup_mode*/) {}
  virtual void collectConsumableButtons(Element *, Tab<HotkeyButton> &) {}
  virtual bool willHandleClick(Element *) { return false; }
  virtual int onDeactivateInput(Element *, InputDevice, int /*pointer_id*/) { return 0; }

public:
  const int updateStages;
  const int flags;
};


} // namespace darg
