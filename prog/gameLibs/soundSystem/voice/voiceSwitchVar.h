// Copyright (C) Gaijin Games KFT.  All rights reserved.
#pragma once

#include <osApiWrappers/dag_atomic.h>
#include <util/dag_convar.h>

// the console writes the variable on the main thread; the voice threads read only its atomic copy
class VoiceSwitchVar final : public ConVarT<bool, false>
{
public:
  VoiceSwitchVar(const char *name, bool def_val, const char *help_tip = nullptr) : ConVarT(name, def_val, help_tip) { mirror(); }
  void set(bool val) override
  {
    ConVarT::set(val);
    mirror();
  }
  void revert() override
  {
    ConVarT::revert();
    mirror();
  }
  bool imguiWidget(const char *label_override) override
  {
    const bool changed = ConVarT::imguiWidget(label_override);
    mirror();
    return changed;
  }
  bool load() const { return interlocked_acquire_load(copy) != 0; }

private:
  void mirror() { interlocked_release_store(copy, value ? 1 : 0); }
  volatile int copy = 0;
};
