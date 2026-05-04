// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvComboPopup : public darg::Behavior
{
public:
  BhvComboPopup();

  virtual int pointingEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int btn_id, Point2 pos,
    int /*accum_res*/) override;
  virtual bool willHandleClick(Element *) override { return true; }
};


extern BhvComboPopup bhv_combo_popup;


} // namespace darg
