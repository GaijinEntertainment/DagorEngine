// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <stdFxShaderParams_decl.h>
#include <animPlanesFx_decl.h>


ScriptHelpers::TunedElement *AnimPlaneCurveTime::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_real_param("timeScale", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("minTimeOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("maxTimeOffset", 0));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(4);

    enumEntries[0].name = "repeat";
    enumEntries[0].value = 0;
    enumEntries[1].name = "constant";
    enumEntries[1].value = 1;
    enumEntries[2].name = "offset";
    enumEntries[2].value = 2;
    enumEntries[3].name = "extrapolate";
    enumEntries[3].value = 3;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("outType", enumEntries));
  }

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlaneScalar::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("func", E3DCOLOR(255, 255, 0)));
  elems.push_back(AnimPlaneCurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlaneNormScalar::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("func", E3DCOLOR(255, 255, 0)));
  elems.push_back(AnimPlaneCurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlaneRgb::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("rFunc", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("gFunc", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("bFunc", E3DCOLOR(0, 0, 255)));
  elems.push_back(AnimPlaneCurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlaneColor::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fakeHdrBrightness", 1));
  elems.push_back(AnimPlaneNormScalar::createTunedElement("alpha"));
  elems.push_back(AnimPlaneNormScalar::createTunedElement("brightness"));
  elems.push_back(AnimPlaneRgb::createTunedElement("rgb"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *AnimPlaneTc::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_real_param("uOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("uSize", 1024));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vSize", 1024));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlaneLife::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(8);

  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("lifeVar", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delayBefore", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delayBeforeVar", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("repeated", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("canRepeatWithoutEmitter", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("repeatDelay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("repeatDelayVar", 0));

  return ScriptHelpers::create_tuned_struct(name, 3, elems);
}


ScriptHelpers::TunedElement *AnimPlaneParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(22);

  elems.push_back(AnimPlaneLife::createTunedElement("life"));
  elems.push_back(AnimPlaneColor::createTunedElement("color"));
  elems.push_back(AnimPlaneScalar::createTunedElement("size"));
  elems.push_back(AnimPlaneScalar::createTunedElement("scaleX"));
  elems.push_back(AnimPlaneScalar::createTunedElement("scaleY"));
  elems.push_back(AnimPlaneScalar::createTunedElement("rotationX"));
  elems.push_back(AnimPlaneScalar::createTunedElement("rotationY"));
  elems.push_back(AnimPlaneScalar::createTunedElement("roll"));
  elems.push_back(AnimPlaneScalar::createTunedElement("posX"));
  elems.push_back(AnimPlaneScalar::createTunedElement("posY"));
  elems.push_back(AnimPlaneScalar::createTunedElement("posZ"));
  elems.push_back(AnimPlaneTc::createTunedElement("tc"));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("posFromEmitter", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("dirFromEmitter", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("dirFromEmitterRandom", true));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("moveWithEmitter", false));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rollOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("rollOffsetVar", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("randomRollSign", false));
  elems.push_back(ScriptHelpers::create_tuned_Point3_param("posOffset", Point3(0, 0, 0)));
  {
    Tab<ScriptHelpers::EnumEntry> enumEntries(tmpmem);
    enumEntries.resize(3);

    enumEntries[0].name = "Facing";
    enumEntries[0].value = 0;
    enumEntries[1].name = "AxisFacing";
    enumEntries[1].value = 1;
    enumEntries[2].name = "Free";
    enumEntries[2].value = 2;

    elems.push_back(ScriptHelpers::create_tuned_enum_param("facingType", enumEntries));
  }
  elems.push_back(ScriptHelpers::create_tuned_real_param("viewZOffset", 0));

  return ScriptHelpers::create_tuned_struct(name, 5, elems);
}


ScriptHelpers::TunedElement *AnimPlanesFxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(StdFxShaderParams::createTunedElement("shader"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));
  elems.push_back(create_tuned_array("array", AnimPlaneParams::createTunedElement("AnimPlaneParams")));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


class AnimPlanesFxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "AnimPlanesFx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(AnimPlanesFxParams::createTunedElement("AnimPlanesFxParams_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static AnimPlanesFxEffectTools tools;


void register_anim_planes_fx_tools() { ::register_effect_class_tools(&tools); }
