// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <stdFxShaderParams_decl.h>
#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <trailFlowFx_decl.h>


ScriptHelpers::TunedElement *TrailFlowFxColor::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fakeHdrBrightness", 1));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("rFunc", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("gFunc", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("bFunc", E3DCOLOR(0, 0, 255)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("aFunc", E3DCOLOR(255, 255, 255)));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *TrailFlowFxSize::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 0.1));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("sizeFunc", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *TrailFlowFxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(24);

  elems.push_back(ScriptHelpers::create_tuned_int_param("numTrails", 10));
  elems.push_back(ScriptHelpers::create_tuned_int_param("numSegs", 50));
  elems.push_back(ScriptHelpers::create_tuned_real_param("timeLength", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("numFrames", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("tile", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("scrollSpeed", 0));
  elems.push_back(TrailFlowFxColor::createTunedElement("color"));
  elems.push_back(TrailFlowFxSize::createTunedElement("size"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("randomLife", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("emitterVelK", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbVel", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbFreq", 10));
  elems.push_back(ScriptHelpers::create_tuned_real_param("normalVel", 0));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("startVel", Point3(0, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("gravity", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("viscosity", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(StdFxShaderParams::createTunedElement("shader"));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("burstMode", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("amountScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


class TrailFlowFxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "TrailFlowFx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(TrailFlowFxParams::createTunedElement("TrailFlowFxParams_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static TrailFlowFxEffectTools tools;


void register_trail_flow_fx_tools() { ::register_effect_class_tools(&tools); }
