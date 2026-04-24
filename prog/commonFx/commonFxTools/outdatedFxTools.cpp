// Copyright (C) Gaijin Games KFT.  All rights reserved.

#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>
#include <fx/effectClassTools.h>

namespace
{
class OutdatedFxTools : public IEffectClassTools
{
  const char *name;

public:
  OutdatedFxTools(const char *_name) : name(_name) {}
  const char *getClassName() override { return name; }

  virtual ScriptHelpers::TunedElement *createTunedElement() override
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    return ScriptHelpers::create_tuned_group("params", 1, elems);
  }
};
} // namespace

static OutdatedFxTools compoundPsTools{"CompoundPs"};
static OutdatedFxTools flowPs2tools{"FlowPs2"};

void register_outdatedFx_tools(const DataBlock &appblk)
{
  if (!appblk.getBool("outdatedFx", false))
    return;

  ::register_effect_class_tools(&compoundPsTools);
  ::register_effect_class_tools(&flowPs2tools);
}
