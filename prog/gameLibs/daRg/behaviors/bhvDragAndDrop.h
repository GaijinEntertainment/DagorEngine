// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <sqrat.h>

namespace darg
{


struct IGuiScene;
struct DragAndDropState;

class BhvDragAndDrop : public darg::Behavior
{
public:
  BhvDragAndDrop();

  virtual int mouseEvent(ElementTree *, Element *, InputDevice /*device*/, InputEvent /*event*/, int /*pointer_id*/, int /*btn_idx*/,
    short /*mx*/, short /*my*/, int /*buttons*/, int /*accum_res*/) override;
  virtual bool willHandleClick(Element *) override;
  virtual int touchEvent(ElementTree *, Element *, InputEvent event, HumanInput::IGenPointing *pnt, int touch_idx,
    const HumanInput::PointingRawState::Touch &touch, int /*accum_res*/) override;
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) override;

private:
  Element *findDropTarget(ElementTree *etree, const Point2 &pos, const Element *own_elem, Sqrat::Object &handler);

  Element *findElemByHandler(ElementTree *etree, const Sqrat::Object &handler);
  void activateTarget(ElementTree *etree, Element *elem, DragAndDropState *dd_state, Element *new_target_elem,
    const Sqrat::Object &new_target_handler, int state_flags);
  void callDragModeHandler(IGuiScene *scene, Element *elem, bool mode_on);
  int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    const Point2 &pointer_pos, int accum_res);
  void applyPointerMove(const Point2 &pointer_pos, darg::DragAndDropState *ddState, darg::Element *elem, darg::ElementTree *etree,
    int activeStateFlag);
  void startVisualDrag(ElementTree *etree, darg::Element *elem);

  DragAndDropState *activeDrag;
};


extern BhvDragAndDrop bhv_drag_and_drop;


} // namespace darg
