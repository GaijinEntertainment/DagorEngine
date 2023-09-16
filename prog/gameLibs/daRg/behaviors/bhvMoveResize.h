#pragma once

#include <daRg/dag_behavior.h>
#include <math/dag_math2d.h>


namespace darg
{


struct BhvMoveResizeData;


class BhvMoveResize : public darg::Behavior
{
public:
  enum HandlePos
  {
    MR_NONE = 0,
    MR_T = 0x01,
    MR_R = 0x02,
    MR_B = 0x04,
    MR_L = 0x08,
    MR_LT = 0x10,
    MR_RT = 0x20,
    MR_LB = 0x40,
    MR_RB = 0x80,
    MR_AREA = 0x100
  };

public:
  BhvMoveResize();

  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent event, int pointer_id, int data, short mx,
    short my, int buttons, int /*accum_res*/) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;

  HandlePos findHandle(Element *elem, const Point2 &pt);
  void setHandleCursor(Element *elem, HandlePos handle);

  static const float handle_eps;

private:
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice /*device*/, InputEvent event, int pointer_id, int button_id,
    const Point2 &pos, int accum_res);
};


extern BhvMoveResize bhv_move_resize;


} // namespace darg
