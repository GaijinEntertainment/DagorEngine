// Copyright (C) Gaijin Games KFT.  All rights reserved.

// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <dafxEmitter_decl.h>


ScriptHelpers::TunedElement *DafxEmitterDistanceBased::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_int_param("elem_limit", 10));
  elems.push_back(ScriptHelpers::create_tuned_real_param("distance", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("idle_period", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *DafxEmitterParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(4);

    enumEntries[0].name = "fixed";
    enumEntries[0].value = 0;
    enumEntries[1].name = "burst";
    enumEntries[1].value = 1;
    enumEntries[2].name = "linear";
    enumEntries[2].value = 2;
    enumEntries[3].name = "distance_based";
    enumEntries[3].value = 3;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_int_param("count", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("cycles", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("period", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delay", 0));
  elems.push_back(DafxEmitterDistanceBased::createTunedElement("distance_based"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}
