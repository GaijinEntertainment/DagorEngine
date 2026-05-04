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
  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    Point2 pointer_pos, int accum_res) override;
  virtual bool willHandleClick(Element *) override { return true; }
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;

private:
  bool onMouseWheel(Element *elem, int delta);

  void updateSliderHover(darg::Element *elem, darg::BhvSliderData *bhvData, const Point2 &pointer_pos);

  float alignValue(Element *elem, float progress);
  void setVal(Element *elem, float req_val);
  bool getValueFromMousePos(BhvSliderData *bhvData, Element *elem, int mx, int my, bool knob_drag, float &out) const;
  void setValueFromMousePos(BhvSliderData *, Element *elem, int mx, int my, bool knob_drag);
};


extern BhvSlider bhv_slider;


} // namespace darg
