// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvComboPopup : public darg::Behavior
{
public:
  BhvComboPopup();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/);
  virtual bool willHandleClick(Element *) override { return true; }
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;
};


extern BhvComboPopup bhv_combo_popup;


} // namespace darg
