// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <compoundPs_decl.h>


ScriptHelpers::TunedElement *CompoundPsParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(60);

  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx1", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx1_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx1_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx1_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx1_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx1_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx1_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx2", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx2_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx2_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx2_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx2_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx2_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx2_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx3", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx3_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx3_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx3_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx3_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx3_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx3_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx4", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx4_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx4_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx4_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx4_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx4_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx4_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx5", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx5_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx5_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx5_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx5_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx5_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx5_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx6", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx6_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx6_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx6_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx6_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx6_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx6_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx7", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx7_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx7_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx7_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx7_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx7_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx7_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("fx8", "Effects"));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx8_offset", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx8_scale", Point3(1, 1, 1)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx8_rot", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("fx8_rot_speed", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx8_delay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fx8_emitter_lifetime", -1));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("useCommonEmitter", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));

  return ScriptHelpers::create_tuned_struct(name, 8, elems);
}


class CompoundPsEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "CompoundPs"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(CompoundPsParams::createTunedElement("CompoundPsParams_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static CompoundPsEffectTools tools;


void register_compound_ps_fx_tools() { ::register_effect_class_tools(&tools); }
