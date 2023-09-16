// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <emitterFlowPs_decl.h>


ScriptHelpers::TunedElement *EmitterFlowPsParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(23);

  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx1", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("randomLife", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("randomVel", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("normalVel", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("startVel", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("gravity", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("viscosity", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("randomRot", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rotSpeed", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rotViscosity", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("randomPhase", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("numParts", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("amountScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceTimeScale", 0.1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceMultiplier", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("windScale", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("burstMode", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


class EmitterFlowPsEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "EmitterFlowPs"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(EmitterFlowPsParams::createTunedElement("EmitterFlowPsParams_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static EmitterFlowPsEffectTools tools;


void register_emitterflow_ps_fx_tools() { ::register_effect_class_tools(&tools); }
