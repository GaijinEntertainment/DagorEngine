//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <drv/hid/dag_hiPointingData.h>


namespace ecs
{
class IGenHidEventHandler;
}
namespace HumanInput
{
class IGenJoystick;
class IGenPointing;
class IGenKeyboardClient;
class IGenPointingClient;
class IGenJoystickClient;
} // namespace HumanInput

class ecs::IGenHidEventHandler
{
public:
  struct Context
  {
    unsigned keyModif;
    int msX, msY;
    unsigned msButtons;
    bool msOverWnd;
  };

  virtual bool ehIsActive() const = 0;

  virtual bool gmehMove(const Context &ctx, float dx, float dy) = 0;
  virtual bool gmehButtonDown(const Context &ctx, int btn_idx) = 0;
  virtual bool gmehButtonUp(const Context &ctx, int btn_idx) = 0;
  virtual bool gmehWheel(const Context &ctx, int dz) = 0;
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wchar) = 0;
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx) = 0;

  virtual bool gkcLocksChanged(const Context & /*ctx*/, unsigned /*locks*/) { return false; }

  virtual bool gkcLayoutChanged(const Context & /*ctx*/, const char * /*layout*/) { return false; }

  virtual bool gjehStateChanged(const Context & /*ctx*/, HumanInput::IGenJoystick * /*joy*/, int /*joy_ord_id*/) { return false; }

  virtual bool gtehTouchBegan(const Context & /* ctx */, HumanInput::IGenPointing *, int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */)
  {
    return false;
  }
  virtual bool gtehTouchMoved(const Context & /* ctx */, HumanInput::IGenPointing *, int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */)
  {
    return false;
  }
  virtual bool gtehTouchEnded(const Context & /* ctx */, HumanInput::IGenPointing *, int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */)
  {
    return false;
  }

  virtual bool gpehGesture(const Context & /*ctx*/, HumanInput::IGenPointing *, HumanInput::Gesture::State /*state*/,
    const HumanInput::Gesture & /*gesture*/)
  {
    return false;
  }
};

void register_hid_event_handler(ecs::IGenHidEventHandler *eh, unsigned priority = 0);
void unregister_hid_event_handler(ecs::IGenHidEventHandler *eh);

void set_secondary_hid_client_kbd(HumanInput::IGenKeyboardClient *cli);
void set_secondary_hid_client_pnt(HumanInput::IGenPointingClient *cli);
void set_secondary_hid_client_joy(HumanInput::IGenJoystickClient *cli);
