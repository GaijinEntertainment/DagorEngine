// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>

/*
This is a thin interface to internal daRg pointing device input handling.
It allows to implement arbitrary input processing in script.

One should be careful when using this behavior:

* Handler functions (onPointerPress/onPointerRelease/onPointerMove)
  must be lightweight, because they are called during internal input
  processing, not in a separate script-specific step

* Consistent state should be maintained in script - that means
  properly checking 'hit' and 'accumRes' flags, paying attention
  to devId/pointerId/btnId in different events and returning
  correct code (R_PROCESSED or 0/null)

See dagor4\tools\dargbox\samples\_basic\behaviors\process_pointing_input.ui.nut
for demo
*/

namespace darg
{


class BhvProcessKeyInput : public darg::Behavior
{
public:
  BhvProcessKeyInput();

  virtual int joystickBtnEvent(ElementTree * /*etree*/, Element *elem, const HumanInput::IGenJoystick *, InputEvent event, int btn_idx,
    int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int accum_res) override;

  virtual int kbdEvent(ElementTree * /*etree*/, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t /*wc*/,
    int accum_res) override;

private:
  int sendKeyEvent(Element *elem, InputEvent event, InputDevice dev_id, int btn_idx, int accum_res);
};


extern BhvProcessKeyInput bhv_process_key_input;


} // namespace darg
