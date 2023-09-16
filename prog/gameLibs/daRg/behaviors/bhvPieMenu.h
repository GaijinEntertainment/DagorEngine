#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{

struct BhvPieMenuData;

class BhvPieMenu : public darg::Behavior
{
public:
  BhvPieMenu();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;
  virtual int update(UpdateStage /*stage*/, darg::Element *, float /*dt*/) override;
  virtual void collectConsumableButtons(Element *elem, Tab<HotkeyButton> &input) override;
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;

private:
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pos, int accum_res);
};


extern BhvPieMenu bhv_pie_menu;


} // namespace darg
