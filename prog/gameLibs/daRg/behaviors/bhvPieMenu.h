// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    Point2 pos, int accum_res) override;
  virtual int update(UpdateStage /*stage*/, darg::Element *, float /*dt*/) override;
  virtual void collectConsumableButtons(Element *elem, Tab<HotkeyButton> &input) override;
  virtual int onDeactivateInput(Element *, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;
};


extern BhvPieMenu bhv_pie_menu;


} // namespace darg
