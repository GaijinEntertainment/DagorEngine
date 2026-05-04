// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/behaviors/bhvButton.h>
#include <daRg/dag_behavior.h>
#include <math/integer/dag_IPoint2.h>
#include <daRg/dag_element.h>
#include <daRg/dag_guiScene.h>

namespace darg
{

class BhvJoystickScroll : public darg::Behavior
{
public:
  BhvJoystickScroll();

  virtual int update(UpdateStage /*stage*/, darg::Element * /*elem*/, float /*dt*/) override;
};

extern BhvJoystickScroll bhv_joystick_scroll;

} // namespace darg