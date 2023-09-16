// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <dafxEmitter_decl.h>


ScriptHelpers::TunedElement *DafxEmitterParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "fixed";
    enumEntries[0].value = 0;
    enumEntries[1].name = "burst";
    enumEntries[1].value = 1;
    enumEntries[2].name = "linear";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("type", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_int_param("count", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("cycles", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("period", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delay", 0));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}
