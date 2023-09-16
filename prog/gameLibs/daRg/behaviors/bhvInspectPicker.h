#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvInspectPicker : public darg::Behavior
{
public:
  BhvInspectPicker();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;

  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;

private:
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int /*pointer_id*/, int /*button_id*/,
    const Point2 &p, int /*accum_res*/);
};


extern BhvInspectPicker bhv_inspect_picker;


} // namespace darg
