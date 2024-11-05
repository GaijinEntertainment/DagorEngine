// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_Point2.h>


namespace darg
{

struct BhvPannable2touchData
{
  int activeTouchIdx[2] = {-1, -1};
  Point2 lastTouchPos[2] = {Point2(0, 0), Point2(0, 0)};
};


class BhvPannable2touch : public darg::Behavior
{
public:
  BhvPannable2touch();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int touchEvent(ElementTree *, Element *, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
};


extern BhvPannable2touch bhv_pannable_2touch;


} // namespace darg
