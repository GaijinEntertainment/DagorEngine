#pragma once

#include <daRg/dag_behavior.h>

namespace darg
{

struct BhvSwipeScrollData;

class BhvSwipeScroll : public darg::Behavior
{
public:
  BhvSwipeScroll();
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int mouseEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int accum_res) override;
  virtual int touchEvent(ElementTree *etree, Element *elem, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int accum_res) override;
  virtual int onDeactivateInput(Element *elem, InputDevice device, int pointer_id) override;

private:
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pos, int accum_res);
};

extern BhvSwipeScroll bhv_swipe_scroll;

} // namespace darg
