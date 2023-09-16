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

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;
  virtual int touchEvent(ElementTree *, Element *, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;

private:
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pos, int accum_res);
};


extern BhvPannable bhv_pannable;


} // namespace darg
