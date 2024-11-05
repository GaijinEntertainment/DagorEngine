// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <ecs/input/hidEventRouter.h>
#include <daInput/input_api.h>

namespace darg
{
struct HotkeyButton;
}

namespace uiinput
{
enum class DaInputMaskLayers : int
{
  Console = 0,
  UserUiButtons = 1,
  UserUiPointers = 2,
  OverlayUiButtons = 3,
  OverlayUiPointers = 4
};

class MouseKbdHandler : public ecs::IGenHidEventHandler
{
public:
  MouseKbdHandler();
  ~MouseKbdHandler();

  virtual bool ehIsActive() const { return true; }

  virtual bool gmehMove(const Context &ctx, float dx, float dy);
  virtual bool gmehButtonDown(const Context &ctx, int btn_idx);
  virtual bool gmehButtonUp(const Context &ctx, int btn_idx);
  virtual bool gmehWheel(const Context &ctx, int dz);
  virtual bool gkehButtonDown(const Context &ctx, int btn_idx, bool repeat, wchar_t wc);
  virtual bool gkehButtonUp(const Context &ctx, int btn_idx);
  virtual bool gjehStateChanged(const Context &ctx, HumanInput::IGenJoystick *joy, int joy_ord_id);

  virtual bool gkcLocksChanged(const Context &, unsigned locks) override;
  virtual bool gkcLayoutChanged(const Context &, const char *layout) override;

  virtual bool gtehTouchBegan(const Context & /* ctx */,
    HumanInput::IGenPointing *,
    int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */) override;
  virtual bool gtehTouchMoved(const Context & /* ctx */,
    HumanInput::IGenPointing *,
    int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */) override;
  virtual bool gtehTouchEnded(const Context & /* ctx */,
    HumanInput::IGenPointing *,
    int /* touch_idx */,
    const HumanInput::PointingRawState::Touch & /* touch */) override;
};

dag::ConstSpan<dainput::SingleButtonId> get_keys_consumed_by_console();
dag::ConstSpan<dainput::SingleButtonId> get_keys_consumed_by_editbox();
void mask_dainput_buttons(dag::ConstSpan<darg::HotkeyButton> buttons, bool consume_text_input, int layer);
void mask_dainput_pointers(int darg_interactive_flags, int layer);
void update_joystick_input();
} // namespace uiinput
