// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>


namespace darg
{


class BhvRtPropUpdate : public darg::Behavior
{
public:
  BhvRtPropUpdate();

private:
  virtual int update(UpdateStage stage, Element *elem, float /*dt*/) override;
  virtual void onAttach(Element *) override;
  virtual void onDetach(Element *, DetachMode) override;

  void runForElem(Element *elem);
};


extern BhvRtPropUpdate bhv_rt_prop_update;


} // namespace darg
