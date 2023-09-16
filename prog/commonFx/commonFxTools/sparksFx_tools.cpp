// generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <sparksFx_decl.h>


ScriptHelpers::TunedElement *SparksFxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_int_param("numParts", 50));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 1));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


class SparksFxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "SparksFx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(1);

    elems.push_back(SparksFxParams::createTunedElement("SparksFxParams_data"));

    return ScriptHelpers::create_tuned_group("params", 1, elems);
  }
};

static SparksFxEffectTools tools;


void register_sparks_fx_tools() { ::register_effect_class_tools(&tools); }
