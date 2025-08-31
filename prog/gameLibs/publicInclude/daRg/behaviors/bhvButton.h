//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_bounds2.h>
#include <math/integer/dag_IPoint2.h>

namespace darg
{

struct BhvButtonData
{
  InputDevice pressedByDevice = DEVID_NONE;
  int pressedByPointerId = -1;
  int pressedByBtnId = -1;
  Point2 pointerStartPos = Point2(-9999, -9999);
  BBox2 clickStartClippedBBox;
  BBox2 clickStartFullBBox;

  float touchHoldCallTime = 0;

  // double click handling
  bool wasClicked = false;
  int lastClickTime = 0;
  Point2 lastClickPos = Point2(0, 0);

  void press(InputDevice device, int pointer_id, int btn_id, const Point2 &pos, const BBox2 &clipped_elem_box,
    const BBox2 &full_elem_box);

  void release();

  bool isPressed();

  bool isPressedBy(InputDevice device, int pointer_id);

  bool isPressedBy(InputDevice device, int pointer_id, int btn_id);

  void saveClick(int cur_time, const Point2 &pos);

  bool isDoubleClick(int cur_time, const Point2 &pos, float pos_thresh);
};

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

  static BhvButtonData *getData(Element *elem);

private:
  int pointerEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pointer_pos, int accum_res);
  int simulateClick(ElementTree *etree, Element *elem, InputEvent event, InputDevice dev_id, int btn_idx, int accum_res);
};


extern BhvButton bhv_button;


} // namespace darg
