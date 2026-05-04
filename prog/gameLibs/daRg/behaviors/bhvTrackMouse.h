// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>

namespace darg
{


class BhvTrackMouse : public darg::Behavior
{
public:
  BhvTrackMouse();

  virtual int pointingEvent(ElementTree *, Element *, InputDevice device, InputEvent event, int pointer_id, int btn_id, Point2 pos,
    int accum_res) override;

  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) override;
};


extern BhvTrackMouse bhv_track_mouse;


} // namespace darg
