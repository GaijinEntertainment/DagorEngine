// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <stdFxShaderParams_decl.h>


ScriptHelpers::TunedElement *StdFxShaderParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(9);

  elems.push_back(ScriptHelpers::create_tuned_ref_slot("texture", "Texture"));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "AlphaBlend";
    enumEntries[0].value = 0;
    enumEntries[1].name = "Additive";
    enumEntries[1].value = 1;
    enumEntries[2].name = "AddSmooth";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("shader", enumEntries));
  }
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "SimpleShading";
    enumEntries[0].value = 0;
    enumEntries[1].name = "TwoSidedShading";
    enumEntries[1].value = 1;
    enumEntries[2].name = "TranslucencyShading";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("shaderType", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_bool_param("sorted", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("useColorMult", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("distortion", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("softnessDistance", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("waterProjection", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("waterProjectionOnly", false));

  return ScriptHelpers::create_tuned_struct(name, 6, elems);
}
