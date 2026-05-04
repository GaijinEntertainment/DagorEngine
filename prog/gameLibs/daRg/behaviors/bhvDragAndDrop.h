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

  virtual bool willHandleClick(Element *) override;
  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    Point2 pos, int accum_res) override;
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;
  virtual void onElemSetup(Element *, SetupMode) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;
  virtual int update(UpdateStage /*stage*/, Element * /*elem*/, float /*dt*/) override;

private:
  Element *findDropTarget(ElementTree *etree, const Point2 &pos, const Element *own_elem, Sqrat::Object &handler);

  Element *findElemByHandler(ElementTree *etree, const Sqrat::Object &handler);
  void activateTarget(ElementTree *etree, Element *elem, DragAndDropState *dd_state, Element *new_target_elem,
    const Sqrat::Object &new_target_handler, int state_flags);
  void callDragModeHandler(IGuiScene *scene, Element *elem, bool mode_on);
  int applyPointerMove(const Point2 &pointer_pos, darg::DragAndDropState *ddState, darg::Element *elem, darg::ElementTree *etree,
    int activeStateFlag);
  int startVisualDrag(ElementTree *etree, darg::Element *elem);
  int startDragAfterDelay(darg::DragAndDropState *ddState, darg::Element *elem);
  int deactivateInputImpl(Element *elem, DragAndDropState *ddState);

  DragAndDropState *activeDrag;
};


extern BhvDragAndDrop bhv_drag_and_drop;


} // namespace darg
