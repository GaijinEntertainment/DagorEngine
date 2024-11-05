//
// Dagor Engine 6.5 - Game Libraries
// Copyright (C) Gaijin Games KFT.  All rights reserved.
//
#pragma once

#include <sqrat.h>

namespace sqfrp
{
class BaseObservable;
class ComputedValue; // -V1062

class IStateWatcher
{
public:
  virtual bool onSourceObservableChanged() = 0;
  virtual void onObservableRelease(BaseObservable *observable) = 0;
  virtual Sqrat::Object dbgGetWatcherScriptInstance() { return Sqrat::Object(); }

  virtual ComputedValue *getComputed() { return nullptr; }
};

} // namespace sqfrp
