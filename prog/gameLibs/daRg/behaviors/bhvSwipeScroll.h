// Copyright (C) Gaijin Games KFT.  All rights reserved.
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

  virtual int onDeactivateInput(Element *elem, InputDevice device, int pointer_id) override;
  virtual int onDeactivateAllInput(Element *elem) override;

  virtual int pointingEvent(ElementTree *etree, Element *elem, InputDevice device, InputEvent event, int pointer_id, int button_id,
    Point2 pos, int accum_res);
};

extern BhvSwipeScroll bhv_swipe_scroll;

} // namespace darg
