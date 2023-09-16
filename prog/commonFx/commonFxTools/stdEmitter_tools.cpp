// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <stdEmitter_decl.h>


ScriptHelpers::TunedElement *StdEmitterParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("height", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("width", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("depth", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("isVolumetric", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("isTrail", false));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "sphere";
    enumEntries[0].value = 0;
    enumEntries[1].name = "cylinder";
    enumEntries[1].value = 1;
    enumEntries[2].name = "box";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("geometry", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}
