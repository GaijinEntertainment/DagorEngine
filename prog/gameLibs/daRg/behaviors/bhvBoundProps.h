// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <daRg/dag_behavior.h>
#include <quirrel/frp/dag_frpStateWatcher.h>
#include <dag/dag_vector.h>


namespace darg
{


// Per-element state for BoundProps: FRP subscription targets and the dirty
// flag. Holds no Sqrat::Object: the bindProps table is re-read from the desc
// at every use, so nothing script-side is cached here.
struct BhvBoundPropsData final : public sqfrp::IStateWatcher
{
  dag::Vector<sqfrp::NodeId> nodes; // currently subscribed
  bool dirty = false;
  bool layoutAffecting = false; // derived from bound key names at setup

  virtual bool onSourceObservableChanged() override
  {
    dirty = true;
    return true;
  }

  virtual void onObservableRelease(sqfrp::NodeId id) override
  {
    for (int i = (int)nodes.size() - 1; i >= 0; --i)
      if (nodes[i] == id)
        nodes.erase(nodes.begin() + i);
  }
};


class BhvBoundProps : public darg::Behavior
{
public:
  BhvBoundProps();

  virtual int update(UpdateStage stage, Element *elem, float dt) override;
  virtual void onAttach(Element *elem) override;
  virtual void onDetach(Element *elem, DetachMode) override;
  virtual void onElemSetup(Element *elem, SetupMode setup_mode) override;

private:
  int applyForElem(Element *elem, BhvBoundPropsData *data);
};


extern BhvBoundProps bhv_bound_props;


} // namespace darg
