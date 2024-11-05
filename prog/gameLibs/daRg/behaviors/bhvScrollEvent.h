// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvScrollEvent : public darg::Behavior
{
public:
  BhvScrollEvent();

  virtual int update(UpdateStage stage, Element *elem, float dt);
};


extern BhvScrollEvent bhv_scroll_event;


} // namespace darg
