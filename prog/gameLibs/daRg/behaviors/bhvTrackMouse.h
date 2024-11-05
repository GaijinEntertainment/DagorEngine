// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>

namespace darg
{


class BhvTrackMouse : public darg::Behavior
{
public:
  BhvTrackMouse();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;

  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) override;
};


extern BhvTrackMouse bhv_track_mouse;


} // namespace darg
