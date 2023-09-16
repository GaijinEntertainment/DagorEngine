// clang-format off  // generated text, do not modify!
#include <generic/dag_tab.h>
#include <scriptHelpers/tunedParams.h>

#include <fx/effectClassTools.h>

#include <staticVisSphere_decl.h>
#include <stdEmitter_decl.h>
#include <stdFxShaderParams_decl.h>
#include <animPlanes2Fx_decl.h>


ScriptHelpers::TunedElement *AnimPlane2CurveTime::createTunedElement(const char *name)
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


ScriptHelpers::TunedElement *AnimPlane2Scalar::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(3);

  elems.push_back(ScriptHelpers::create_tuned_real_param("scale", 1));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("func", E3DCOLOR(255, 255, 0)));
  elems.push_back(AnimPlane2CurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlane2NormScalar::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(2);

  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("func", E3DCOLOR(255, 255, 0)));
  elems.push_back(AnimPlane2CurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlane2Rgb::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("rFunc", E3DCOLOR(255, 0, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("gFunc", E3DCOLOR(0, 255, 0)));
  elems.push_back(ScriptHelpers::create_tuned_cubic_curve("bFunc", E3DCOLOR(0, 0, 255)));
  elems.push_back(AnimPlane2CurveTime::createTunedElement("time"));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlane2Color::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(5);

  elems.push_back(ScriptHelpers::create_tuned_E3DCOLOR_param("color", E3DCOLOR(255, 255, 255)));
  elems.push_back(ScriptHelpers::create_tuned_real_param("fakeHdrBrightness", 1));
  elems.push_back(AnimPlane2NormScalar::createTunedElement("alpha"));
  elems.push_back(AnimPlane2NormScalar::createTunedElement("brightness"));
  elems.push_back(AnimPlane2Rgb::createTunedElement("rgb"));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *AnimPlane2Tc::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(4);

  elems.push_back(ScriptHelpers::create_tuned_real_param("uOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vOffset", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("uSize", 1024));
  elems.push_back(ScriptHelpers::create_tuned_real_param("vSize", 1024));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


ScriptHelpers::TunedElement *AnimPlane2Life::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(7);

  elems.push_back(ScriptHelpers::create_tuned_real_param("life", 1));
  elems.push_back(ScriptHelpers::create_tuned_real_param("lifeVar", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delayBefore", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("delayBeforeVar", 0));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("repeated", true));
  elems.push_back(ScriptHelpers::create_tuned_real_param("repeatDelay", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("repeatDelayVar", 0));

  return ScriptHelpers::create_tuned_struct(name, 2, elems);
}


ScriptHelpers::TunedElement *AnimPlane2Params::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(24);

  elems.push_back(AnimPlane2Life::createTunedElement("life"));
  elems.push_back(AnimPlane2Color::createTunedElement("color"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("size"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("scaleX"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("scaleY"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("rotationX"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("rotationY"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("roll"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("posX"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("posY"));
  elems.push_back(AnimPlane2Scalar::createTunedElement("posZ"));
  elems.push_back(AnimPlane2Tc::createTunedElement("tc"));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("posFromEmitter", false));
  elems.push_back(ScriptHelpers::create_tuned_bool_param("dirFromEmitter", false));
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
  elems.push_back(ScriptHelpers::create_tuned_int_param("framesX", 0));
  elems.push_back(ScriptHelpers::create_tuned_int_param("framesY", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("animSpeed", 1));

  return ScriptHelpers::create_tuned_struct(name, 4, elems);
}


ScriptHelpers::TunedElement *AnimPlanes2FxParams::createTunedElement(const char *name)
{
  Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
  elems.reserve(6);

  elems.push_back(ScriptHelpers::create_tuned_int_param("seed", 0));
  elems.push_back(StdFxShaderParams::createTunedElement("shader"));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ltPower", 0));
  elems.push_back(ScriptHelpers::create_tuned_real_param("sunLtPower", 0.5));
  elems.push_back(ScriptHelpers::create_tuned_real_param("ambLtPower", 0.25));
  elems.push_back(create_tuned_array("array", AnimPlane2Params::createTunedElement("AnimPlane2Params")));

  return ScriptHelpers::create_tuned_struct(name, 1, elems);
}


class AnimPlanes2FxEffectTools : public IEffectClassTools
{
public:
  virtual const char *getClassName() { return "AnimPlanes2Fx"; }

  virtual ScriptHelpers::TunedElement *createTunedElement()
  {
    Tab<ScriptHelpers::TunedElement *> elems(tmpmem);
    elems.reserve(3);

    elems.push_back(AnimPlanes2FxParams::createTunedElement("AnimPlanes2FxParams_data"));
    elems.push_back(StaticVisSphereParams::createTunedElement("StaticVisSphereParams_data"));
    elems.push_back(StdEmitterParams::createTunedElement("StdEmitterParams_data"));

    return ScriptHelpers::create_tuned_group("params", 2, elems);
  }
};

static AnimPlanes2FxEffectTools tools;


void register_anim_planes_2_fx_tools() { ::register_effect_class_tools(&tools); }
