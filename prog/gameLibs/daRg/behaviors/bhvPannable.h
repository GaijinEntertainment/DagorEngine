// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_Point2.h>
#include "kineticTouch.h"


namespace darg
{

struct BhvPannableData;


class BhvPannable : public darg::Behavior
{
public:
  BhvPannable();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    Point2 pos, int accum_res) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;
};


extern BhvPannable bhv_pannable;


} // namespace darg
