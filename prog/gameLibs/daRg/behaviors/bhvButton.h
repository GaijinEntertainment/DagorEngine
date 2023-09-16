#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint2.h>

namespace darg
{

struct BhvButtonData;


class BhvButton : public darg::Behavior
{
public:
  BhvButton();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int kbdEvent(ElementTree *, Element *, InputEvent event, int key_idx, bool repeat, wchar_t wc, int accum_res) override;
  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;
  virtual int touchEvent(ElementTree *, Element *, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int /*accum_res*/) override;
  virtual bool willHandleClick(Element *) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int joystickBtnEvent(ElementTree *, Element *, const HumanInput::IGenJoystick *, InputEvent /*event*/, int /*key_idx*/,
    int /*device_number*/, const HumanInput::ButtonBits & /*buttons*/, int /*accum_res*/) override;

  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) override;

private:
  int pointerEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pointer_pos, int accum_res);
  int simulateClick(ElementTree *etree, Element *elem, InputEvent event, InputDevice dev_id, int btn_idx, int accum_res);
};


extern BhvButton bhv_button;


} // namespace darg
