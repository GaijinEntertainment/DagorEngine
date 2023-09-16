// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <flowPs2_decl.h>


ScriptHelpers::TunedElement *FlowPsColor2::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(9);

  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color2", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fakeHdrBrightness", 1));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("rFunc", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("gFunc", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("bFunc", E3DCOLOR(0, 0, 255)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("aFunc", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("specFunc", E3DCOLOR(255, 255, 255)));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *FlowPsSize2::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_real_param("radius", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("lengthDt", 0));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("sizeFunc", E3DCOLOR(255, 255, 0)));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *FlowPsParams2::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(41);

  elems.push_back(ScriptHelpers::create_tuned_int_param("framesX", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("framesY", 0));
  elems.push_back(FlowPsColor2::createTunedElement("color"));
  elems.push_back(FlowPsSize2::createTunedElement("size"));
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
  elems.push_back(ScriptHelpers::create_tuned_real_param("animSpeed", 1));
  elems.push_back(ScriptHelpers::create_tuned_int_param("numParts", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("texture", "Texture"));
  elems.push_back(ScriptHelpers::create_tuned_ref_slot("normal", "Normal"));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(8);

    enumEntries[0].name = "AlphaBlend";
    enumEntries[0].value = 0;
    enumEntries[1].name = "Additive";
    enumEntries[1].value = 1;
    enumEntries[2].name = "AddSmooth";
    enumEntries[2].value = 2;
    enumEntries[3].name = "AlphaTest";
    enumEntries[3].value = 3;
    enumEntries[4].name = "PreMultAlpha";
    enumEntries[4].value = 4;
    enumEntries[5].name = "GbufferAlphaTest";
    enumEntries[5].value = 5;
    enumEntries[6].name = "AlphaTestRefraction";
    enumEntries[6].value = 6;
    enumEntries[7].name = "GbufferPatch";
    enumEntries[7].value = 7;

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
  elems.push_back(ScriptHelpers::create_tuned_bool_param("useColorMult", true));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(2);

    enumEntries[0].name = "for_system";
    enumEntries[0].value = 0;
    enumEntries[1].name = "for_emitter";
    enumEntries[1].value = 1;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("colorMultMode", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_bool_param("burstMode", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("amountScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceTimeScale", 0.1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("turbulenceMultiplier", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("windScale", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("distortion", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("softnessDistance", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("waterProjection", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("waterProjectionOnly", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("airFriction", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("simulateInLocalSpace", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("animatedFlipbook", false));

  return ScriptHelpers::create_tuned_struct(name, 14, elems);
}


class FlowPs2EffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "FlowPs2"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(FlowPsParams2::createTunedElement("FlowPsParams2_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static FlowPs2EffectTools tools;


void register_flow_ps_2_fx_tools() { ::register_effect_class_tools(&tools); }
