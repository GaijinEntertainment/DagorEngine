// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <sqrat.h>


namespace darg
{

struct BhvSliderData;


class BhvSlider : public darg::Behavior
{
public:
  BhvSlider();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int kbdEvent(ElementTree *, Element *elem, InputEvent event, int key_idx, bool repeat, wchar_t wc, int /*accum_res*/);
  virtual int mouseEvent(ElementTree *, Element *elem, InputDevice /*device*/, InputEvent event, int pointer_id, int btn_idx, short mx,
    short my, int buttons, int /*accum_res*/);
  virtual bool willHandleClick(Element *) override { return true; }
  virtual int touchEvent(ElementTree *, Element *, InputEvent /*event*/, HumanInput::IGenPointing * /*pnt*/, int /*touch_idx*/,
    const HumanInput::PointingRawState::Touch & /*touch*/, int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;

private:
  float progressToValue(Element *elem, float progress);
  bool getValueFromMousePos(BhvSliderData *bhvData, Element *elem, int mx, int my, bool knob_drag, float &out) const;
  int pointerEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pointer_pos, int accum_res);
  bool onMouseWheel(Element *elem, int delta);
  void setVal(Element *elem, float req_val);
  void recalcKnob(Element *elem);

  Element *findChildElemByScriptCtor(Element *elem, Sqrat::Object &ctor);
  void setValueFromMousePos(BhvSliderData *, Element *elem, int mx, int my, bool knob_drag);
};


extern BhvSlider bhv_slider;


} // namespace darg
