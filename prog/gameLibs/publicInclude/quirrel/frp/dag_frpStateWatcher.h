//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>

namespace sqfrp
{

struct NodeId
{
  uint32_t index = UINT32_MAX;
  uint32_t generation = 0;
  bool isValid() const { return index != UINT32_MAX; }
  bool operator==(const NodeId &o) const { return index == o.index && generation == o.generation; }
  bool operator!=(const NodeId &o) const { return !(*this == o); }
  bool operator<(const NodeId &o) const { return index < o.index || (index == o.index && generation < o.generation); }
};

class IStateWatcher
{
public:
  virtual bool onSourceObservableChanged() = 0;
  virtual void onObservableRelease(NodeId id) = 0;
  virtual Sqrat::Object dbgGetWatcherScriptInstance() { return Sqrat::Object(); }
};

} // namespace sqfrp
