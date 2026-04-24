// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>

namespace darg
{


class BhvEatInput : public Behavior
{
public:
  BhvEatInput();

  virtual int pointingEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/,
    int /*btn_idx*/, Point2 pos, int /*accum_res*/) override;

private:
};

extern BhvEatInput bhv_eat_input;


} // namespace darg
