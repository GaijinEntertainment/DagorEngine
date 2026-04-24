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

  virtual int pointingEvent(ElementTree *, Element *, InputDevice, InputEvent event, int touch_idx, int btn_idx, Point2 pos,
    int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;
};


extern BhvPannable2touch bhv_pannable_2touch;


} // namespace darg
